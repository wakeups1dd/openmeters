#pragma once

#include "audio-engine-interface.h"
#include "../../common/audio-format.h"

#ifdef _WIN32

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <mutex>
#include <atomic>

namespace openmeters::core::audio {

/**
 * WASAPI loopback capture implementation.
 * Captures system audio using Windows WASAPI loopback interface.
 * 
 * Thread safety: Thread-safe for start/stop operations.
 * Audio callbacks run on WASAPI capture thread (real-time priority).
 */
class WasapiCapture {
public:
    explicit WasapiCapture();
    ~WasapiCapture();
    
    // Non-copyable, non-movable
    WasapiCapture(const WasapiCapture&) = delete;
    WasapiCapture& operator=(const WasapiCapture&) = delete;
    WasapiCapture(WasapiCapture&&) = delete;
    WasapiCapture& operator=(WasapiCapture&&) = delete;
    
    /**
     * Initialize WASAPI capture.
     * Sets up COM, device enumeration, and audio client.
     * 
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize();
    
    /**
     * Start audio capture.
     * Begins streaming audio data.
     * 
     * @return true if start succeeded, false otherwise
     */
    bool start();
    
    /**
     * Stop audio capture.
     * Stops streaming and releases audio client.
     */
    void stop();
    
    /**
     * Shutdown and release all resources.
     */
    void shutdown();
    
    /**
     * Get the current audio format.
     * 
     * @return Audio format descriptor
     */
    [[nodiscard]] common::AudioFormat getFormat() const;
    
    /**
     * Check if currently capturing.
     * 
     * @return true if capturing, false otherwise
     */
    [[nodiscard]] bool isCapturing() const;
    
    /**
     * Register a callback for audio data.
     * 
     * @param callback Callback interface (must remain valid until unregistered)
     */
    void registerCallback(IAudioDataCallback* callback);
    
    /**
     * Unregister a callback.
     * 
     * @param callback Callback to remove
     */
    void unregisterCallback(IAudioDataCallback* callback);

private:
    /**
     * Audio capture thread function.
     * Runs on a separate thread to process WASAPI capture events.
     */
    static DWORD WINAPI captureThreadProc(LPVOID lpParam);
    
    /**
     * Internal capture thread function.
     */
    void captureThread();
    
    /**
     * Process captured audio data.
     * Converts format and calls registered callbacks.
     * 
     * @param pData Pointer to audio data
     * @param numFramesAvailable Number of frames available
     * @param pFlags Flags from WASAPI
     */
    void processAudioData(BYTE* pData, UINT32 numFramesAvailable, DWORD flags);
    
    /**
     * Convert audio samples to float32.
     * Handles various WASAPI formats (int16, int32, float32).
     * 
     * @param pSource Source buffer
     * @param pDest Destination float buffer
     * @param numFrames Number of frames to convert
     */
    void convertToFloat32(const BYTE* pSource, float* pDest, UINT32 numFrames);
    
    /**
     * Release audio client resources.
     */
    void releaseAudioClient();
    
    /**
     * Release COM resources.
     */
    void releaseCom();
    
    // COM interfaces
    IMMDeviceEnumerator* m_deviceEnumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    
    // Audio format
    WAVEFORMATEX* m_waveFormat = nullptr;
    common::AudioFormat m_format;
    
    // Capture state
    std::atomic<bool> m_capturing{false};
    HANDLE m_captureThread = nullptr;
    HANDLE m_stopEvent = nullptr;
    
    // Callbacks (protected by mutex)
    std::mutex m_callbackMutex;
    std::vector<IAudioDataCallback*> m_callbacks;
    
    // Conversion buffer (reused per capture)
    std::vector<float> m_floatBuffer;
    
    // COM initialization flag
    bool m_comInitialized = false;
};

} // namespace openmeters::core::audio

#else
#error "WASAPI capture is Windows-only. This file should not be compiled on non-Windows systems."
#endif // _WIN32

