#include "audio-engine.h"

#ifdef _WIN32

#include <algorithm>

namespace openmeters::core::audio {

AudioEngine::AudioEngine()
    : m_meteringCallback(this)
{
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    if (!m_capture.initialize()) {
        return false;
    }
    
    // Register internal metering callback
    m_capture.registerCallback(&m_meteringCallback);
    
    return true;
}

bool AudioEngine::start() {
    return m_capture.start();
}

void AudioEngine::stop() {
    m_capture.stop();
}

void AudioEngine::shutdown() {
    stop();
    
    // Unregister internal callback
    m_capture.unregisterCallback(&m_meteringCallback);
    
    // Clear external callbacks
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_callbacks.clear();
    }
    
    m_capture.shutdown();
}

void AudioEngine::registerCallback(IAudioDataCallback* callback) {
    if (!callback) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.push_back(callback);
}

void AudioEngine::unregisterCallback(IAudioDataCallback* callback) {
    if (!callback) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.erase(
        std::remove(m_callbacks.begin(), m_callbacks.end(), callback),
        m_callbacks.end()
    );
}

common::AudioFormat AudioEngine::getFormat() const {
    return m_capture.getFormat();
}

bool AudioEngine::isCapturing() const {
    return m_capture.isCapturing();
}

void AudioEngine::forwardMeterData(const common::MeterSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (IAudioDataCallback* callback : m_callbacks) {
        if (callback) {
            callback->onMeterData(snapshot);
        }
    }
}

// MeteringCallback implementation

AudioEngine::MeteringCallback::MeteringCallback(AudioEngine* engine)
    : m_engine(engine)
{
}

void AudioEngine::MeteringCallback::onAudioData(
    const float* buffer,
    std::size_t frameCount,
    const common::AudioFormat& format
) {
    if (!buffer || frameCount == 0) {
        return;
    }
    
    // Compute peak and RMS
    const auto peak = m_peakMeter.process(buffer, frameCount, format);
    const auto rms = m_rmsMeter.process(buffer, frameCount, format);
    
    // Create snapshot
    common::MeterSnapshot snapshot;
    snapshot.peak = peak;
    snapshot.rms = rms;
    // TODO: Implement proper timestamp
    snapshot.timestampMs = 0;
    
    // Forward to engine callbacks
    m_engine->forwardMeterData(snapshot);
}

void AudioEngine::MeteringCallback::onMeterData(const common::MeterSnapshot& snapshot) {
    // This callback is not used (we generate meter data ourselves)
    (void)snapshot;
}

} // namespace openmeters::core::audio

#else
// Non-Windows platforms: This file should not be compiled
#error "Audio engine implementation is Windows-only. This file should not be compiled on non-Windows systems."
#endif // _WIN32

