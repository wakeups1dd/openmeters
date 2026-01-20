#ifdef _WIN32

#include "../core/audio/audio-engine.h"
#include "../common/meter-values.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace openmeters;

/**
 * Simple console callback for testing audio capture.
 * Prints peak and RMS values to console.
 */
class ConsoleCallback : public core::audio::IAudioDataCallback {
public:
    void onAudioData(
        const float* buffer,
        std::size_t frameCount,
        const common::AudioFormat& format
    ) override {
        // Silently consume audio data (we only care about meters)
        (void)buffer;
        (void)frameCount;
        (void)format;
    }
    
    void onMeterData(const common::MeterSnapshot& snapshot) override {
        // Print meter values
        std::cout << "\rPeak L: " << std::fixed << std::setprecision(3) << snapshot.peak.left
                  << " R: " << snapshot.peak.right
                  << " | RMS L: " << snapshot.rms.left
                  << " R: " << snapshot.rms.right
                  << "    " << std::flush;
    }
};

int main() {
    std::cout << "OpenMeters - Audio Metering Test\n";
    std::cout << "================================\n\n";
    
    // Create audio engine
    core::audio::AudioEngine engine;
    
    // Initialize
    std::cout << "Initializing audio engine...\n";
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }
    
    // Print format info
    const auto format = engine.getFormat();
    std::cout << "Audio format: " << static_cast<int>(format.sampleRate) << " Hz, "
              << static_cast<int>(format.channelCount) << " channel(s)\n\n";
    
    // Register callback
    ConsoleCallback callback;
    engine.registerCallback(&callback);
    
    // Start capture
    std::cout << "Starting audio capture...\n";
    if (!engine.start()) {
        std::cerr << "Failed to start audio capture.\n";
        engine.shutdown();
        return 1;
    }
    
    std::cout << "Capturing audio. Press Enter to stop...\n\n";
    
    // Run for a bit (or until user presses Enter)
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Stop
    std::cout << "\n\nStopping audio capture...\n";
    engine.stop();
    
    // Unregister callback
    engine.unregisterCallback(&callback);
    
    // Shutdown
    engine.shutdown();
    
    std::cout << "Shutdown complete.\n";
    return 0;
}

#else
// Non-Windows platforms: This application is Windows-only
#error "OpenMeters is Windows-only. This application requires Windows and WASAPI."
#endif // _WIN32

