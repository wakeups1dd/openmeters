#include "wasapi-capture.h"

#ifdef _WIN32

#include "../../common/types.h"
#include <algorithm>
#include <cmath>

namespace openmeters::core::audio {

WasapiCapture::WasapiCapture() = default;

WasapiCapture::~WasapiCapture() {
    shutdown();
}

bool WasapiCapture::initialize() {
    if (m_comInitialized) {
        return true; // Already initialized
    }
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }
    m_comInitialized = true;
    
    // Create device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&m_deviceEnumerator)
    );
    if (FAILED(hr)) {
        releaseCom();
        return false;
    }
    
    // Get default audio render device (for loopback)
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &m_device
    );
    if (FAILED(hr)) {
        releaseCom();
        return false;
    }
    
    // Activate audio client
    hr = m_device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&m_audioClient)
    );
    if (FAILED(hr)) {
        releaseCom();
        return false;
    }
    
    // Get mix format
    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    if (FAILED(hr)) {
        releaseCom();
        return false;
    }
    
    // Validate format (must be PCM or float)
    if (m_waveFormat->wFormatTag != WAVE_FORMAT_PCM &&
        m_waveFormat->wFormatTag != WAVE_FORMAT_IEEE_FLOAT) {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
        releaseCom();
        return false;
    }
    
    // Store format
    m_format.sampleRate = m_waveFormat->nSamplesPerSec;
    m_format.channelCount = static_cast<common::ChannelCount>(m_waveFormat->nChannels);
    
    // Validate channel count (support mono and stereo only)
    if (m_format.channelCount < 1 || m_format.channelCount > 2) {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
        releaseCom();
        return false;
    }
    
    // Initialize audio client for loopback
    REFERENCE_TIME hnsRequestedDuration = 0; // Use default
    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        hnsRequestedDuration,
        0,
        m_waveFormat,
        nullptr
    );
    if (FAILED(hr)) {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
        releaseCom();
        return false;
    }
    
    // Get capture client
    hr = m_audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        reinterpret_cast<void**>(&m_captureClient)
    );
    if (FAILED(hr)) {
        releaseCom();
        return false;
    }
    
    // Create stop event
    m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent) {
        releaseCom();
        return false;
    }
    
    return true;
}

bool WasapiCapture::start() {
    if (m_capturing.load()) {
        return true; // Already capturing
    }
    
    if (!m_audioClient || !m_captureClient) {
        return false;
    }
    
    // Reset stop event
    ResetEvent(m_stopEvent);
    
    // Start audio client
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr)) {
        return false;
    }
    
    // Start capture thread
    m_capturing.store(true);
    m_captureThread = CreateThread(
        nullptr,
        0,
        captureThreadProc,
        this,
        0,
        nullptr
    );
    
    if (!m_captureThread) {
        m_audioClient->Stop();
        m_capturing.store(false);
        return false;
    }
    
    // Set thread priority to time-critical (real-time audio)
    SetThreadPriority(m_captureThread, THREAD_PRIORITY_TIME_CRITICAL);
    
    return true;
}

void WasapiCapture::stop() {
    if (!m_capturing.load()) {
        return;
    }
    
    m_capturing.store(false);
    
    // Signal stop event
    if (m_stopEvent) {
        SetEvent(m_stopEvent);
    }
    
    // Stop audio client
    if (m_audioClient) {
        m_audioClient->Stop();
    }
    
    // Wait for capture thread
    if (m_captureThread) {
        WaitForSingleObject(m_captureThread, INFINITE);
        CloseHandle(m_captureThread);
        m_captureThread = nullptr;
    }
}

void WasapiCapture::shutdown() {
    stop();
    
    releaseAudioClient();
    releaseCom();
    
    if (m_stopEvent) {
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }
}

common::AudioFormat WasapiCapture::getFormat() const {
    return m_format;
}

bool WasapiCapture::isCapturing() const {
    return m_capturing.load();
}

void WasapiCapture::registerCallback(IAudioDataCallback* callback) {
    if (!callback) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.push_back(callback);
}

void WasapiCapture::unregisterCallback(IAudioDataCallback* callback) {
    if (!callback) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.erase(
        std::remove(m_callbacks.begin(), m_callbacks.end(), callback),
        m_callbacks.end()
    );
}

DWORD WINAPI WasapiCapture::captureThreadProc(LPVOID lpParam) {
    auto* capture = static_cast<WasapiCapture*>(lpParam);
    if (capture) {
        capture->captureThread();
    }
    return 0;
}

void WasapiCapture::captureThread() {
    const HANDLE waitArray[] = { m_stopEvent };
    const DWORD waitCount = 1;
    
    while (m_capturing.load()) {
        // Wait for data or stop signal (100ms timeout)
        DWORD waitResult = WaitForMultipleObjects(
            waitCount,
            waitArray,
            FALSE,
            100
        );
        
        if (waitResult == WAIT_OBJECT_0) {
            // Stop signaled
            break;
        }
        
        // Process available audio data
        BYTE* pData = nullptr;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;
        UINT64 devicePosition = 0;
        UINT64 qpcPosition = 0;
        
        HRESULT hr = m_captureClient->GetBuffer(
            &pData,
            &numFramesAvailable,
            &flags,
            &devicePosition,
            &qpcPosition
        );
        
        if (FAILED(hr)) {
            if (hr == AUDCLNT_E_BUFFER_ERROR) {
                // Buffer lost, try to recover by releasing any partial buffer
                // Note: GetBuffer failed, so we don't have a valid buffer to release
                // Just continue and try again on next iteration
            }
            continue;
        }
        
        if (numFramesAvailable == 0) {
            // No data available, release buffer
            m_captureClient->ReleaseBuffer(0);
            continue;
        }
        
        // Process audio data
        if (pData) {
            processAudioData(pData, numFramesAvailable, flags);
        }
        
        // Release buffer
        m_captureClient->ReleaseBuffer(numFramesAvailable);
    }
}

void WasapiCapture::processAudioData(BYTE* pData, UINT32 numFramesAvailable, DWORD flags) {
    if (!pData || numFramesAvailable == 0) {
        return;
    }
    
    // Check for silence
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        // Process silence (zero buffer)
        const std::size_t totalSamples = numFramesAvailable * m_format.samplesPerFrame();
        m_floatBuffer.resize(totalSamples);
        std::fill(m_floatBuffer.begin(), m_floatBuffer.end(), 0.0f);
    } else {
        // Convert to float32
        const std::size_t totalSamples = numFramesAvailable * m_format.samplesPerFrame();
        m_floatBuffer.resize(totalSamples);
        convertToFloat32(pData, m_floatBuffer.data(), numFramesAvailable);
    }
    
    // Call registered callbacks
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (IAudioDataCallback* callback : m_callbacks) {
        if (callback) {
            callback->onAudioData(m_floatBuffer.data(), numFramesAvailable, m_format);
        }
    }
}

void WasapiCapture::convertToFloat32(const BYTE* pSource, float* pDest, UINT32 numFrames) {
    if (!pSource || !pDest || !m_waveFormat || numFrames == 0) {
        return;
    }
    
    const UINT32 bytesPerFrame = m_waveFormat->nBlockAlign;
    const UINT16 bitsPerSample = m_waveFormat->wBitsPerSample;
    const UINT16 channels = m_waveFormat->nChannels;
    
    if (m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        // Already float32, just copy
        const float* pFloatSource = reinterpret_cast<const float*>(pSource);
        std::copy(pFloatSource, pFloatSource + (numFrames * channels), pDest);
    } else if (m_waveFormat->wFormatTag == WAVE_FORMAT_PCM) {
        // Convert from integer PCM to float32
        if (bitsPerSample == 16) {
            const std::int16_t* pInt16Source = reinterpret_cast<const std::int16_t*>(pSource);
            const float scale = 1.0f / 32768.0f;
            
            for (UINT32 frame = 0; frame < numFrames; ++frame) {
                for (UINT16 ch = 0; ch < channels; ++ch) {
                    const std::size_t srcIdx = frame * channels + ch;
                    const std::size_t destIdx = frame * channels + ch;
                    pDest[destIdx] = static_cast<float>(pInt16Source[srcIdx]) * scale;
                }
            }
        } else if (bitsPerSample == 32) {
            const std::int32_t* pInt32Source = reinterpret_cast<const std::int32_t*>(pSource);
            const float scale = 1.0f / 2147483648.0f;
            
            for (UINT32 frame = 0; frame < numFrames; ++frame) {
                for (UINT16 ch = 0; ch < channels; ++ch) {
                    const std::size_t srcIdx = frame * channels + ch;
                    const std::size_t destIdx = frame * channels + ch;
                    pDest[destIdx] = static_cast<float>(pInt32Source[srcIdx]) * scale;
                }
            }
        } else {
            // Unsupported bit depth - fill with zeros
            std::fill(pDest, pDest + (numFrames * channels), 0.0f);
        }
    } else {
        // Unsupported format - fill with zeros
        std::fill(pDest, pDest + (numFrames * channels), 0.0f);
    }
}

void WasapiCapture::releaseAudioClient() {
    if (m_captureClient) {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }
    
    if (m_audioClient) {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }
    
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    
    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }
    
    if (m_waveFormat) {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }
}

void WasapiCapture::releaseCom() {
    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
}

} // namespace openmeters::core::audio

#endif // _WIN32

