#pragma once

#include "audio-engine-interface.h"
#include "../../core/meters/peak-meter.h"
#include "../../core/meters/rms-meter.h"
#include <vector>
#include <mutex>

#ifdef _WIN32
#include "wasapi-capture.h"
#else
#error "Audio engine is Windows-only. This file should not be compiled on non-Windows systems."
#endif

namespace openmeters::core::audio {

/**
 * Audio engine implementation.
 * Integrates WASAPI capture with peak/RMS metering and exposes data via callbacks.
 * 
 * Thread safety: Thread-safe for public operations.
 * Audio callbacks run on WASAPI capture thread.
 */
class AudioEngine : public IAudioEngine {
public:
    AudioEngine();
    ~AudioEngine() override;
    
    // Non-copyable, non-movable
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;
    
    bool initialize() override;
    bool start() override;
    void stop() override;
    void shutdown() override;
    
    void registerCallback(IAudioDataCallback* callback) override;
    void unregisterCallback(IAudioDataCallback* callback) override;
    
    [[nodiscard]] common::AudioFormat getFormat() const override;
    [[nodiscard]] bool isCapturing() const override;

private:
    /**
     * Internal callback implementation.
     * Receives audio data from WASAPI capture and computes meters.
     */
    class MeteringCallback : public IAudioDataCallback {
    public:
        explicit MeteringCallback(AudioEngine* engine);
        
        void onAudioData(
            const float* buffer,
            std::size_t frameCount,
            const common::AudioFormat& format
        ) override;
        
        void onMeterData(const common::MeterSnapshot& snapshot) override;
        
    private:
        AudioEngine* m_engine;
        meters::PeakMeter m_peakMeter;
        meters::RmsMeter m_rmsMeter;
    };
    
    /**
     * Forward meter data to registered callbacks.
     */
    void forwardMeterData(const common::MeterSnapshot& snapshot);
    
    WasapiCapture m_capture;
    MeteringCallback m_meteringCallback;
    
    std::mutex m_callbackMutex;
    std::vector<IAudioDataCallback*> m_callbacks;
};

} // namespace openmeters::core::audio

