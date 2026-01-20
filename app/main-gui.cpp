#ifdef _WIN32
#ifdef BUILD_GUI

#include "../ui/window.h"
#include "../core/audio/audio-engine.h"
#include "../common/logger.h"
#include "../common/config.h"
#include <windows.h>

using namespace openmeters;

/**
 * GUI callback for audio data.
 * Updates the window with meter values.
 */
class GuiCallback : public core::audio::IAudioDataCallback {
public:
    explicit GuiCallback(ui::Window* window) : m_window(window) {}
    
    void onAudioData(
        const float* buffer,
        std::size_t frameCount,
        const common::AudioFormat& format
    ) override {
        // Silently consume audio data
        (void)buffer;
        (void)frameCount;
        (void)format;
    }
    
    void onMeterData(const common::MeterSnapshot& snapshot) override {
        if (m_window) {
            m_window->updateMeters(snapshot);
        }
    }
    
private:
    ui::Window* m_window;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    try {
        // Initialize logger
        std::string logPath = "logs/openmeters.log";
        if (!common::Logger::initialize(logPath, common::LogLevel::Info, true)) {
            MessageBoxA(nullptr, "Failed to initialize logger", "OpenMeters Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        
        LOG_INFO("OpenMeters starting...");
        
        // Load configuration
        common::ConfigManager::load();
        
        // Create window
        ui::Window window;
        if (!window.initialize(hInstance, nCmdShow)) {
            LOG_ERROR("Failed to initialize window");
            MessageBoxA(nullptr, "Failed to initialize window", "OpenMeters Error", MB_OK | MB_ICONERROR);
            common::Logger::shutdown();
            return 1;
        }
        
        // Create audio engine
        core::audio::AudioEngine engine;
        bool audioAvailable = engine.initialize();
        if (!audioAvailable) {
            LOG_WARNING("Audio engine failed to initialize. Meters will show zero until audio is available.");
            MessageBoxA(nullptr, 
                "Audio capture is unavailable.\n\n"
                "This can happen if:\n"
                "- No audio is currently playing on your system\n"
                "- Your audio device is in use by another application\n\n"
                "The meter window will open, but meters will show zero.\n"
                "Try playing some audio and restarting the app.",
                "OpenMeters - Audio Warning", MB_OK | MB_ICONWARNING);
        }
        
        GuiCallback callback(&window);
        
        if (audioAvailable) {
            LOG_INFO("Audio format: " + std::to_string(engine.getFormat().sampleRate) + " Hz, " +
                     std::to_string(engine.getFormat().channelCount) + " channel(s)");
            
            // Register callback
            engine.registerCallback(&callback);
            
            // Start capture
            if (!engine.start()) {
                LOG_WARNING("Failed to start audio capture");
            } else {
                LOG_INFO("Audio capture started");
            }
        }
        
        // Run main loop (window always opens)
        window.run();
        
        // Cleanup
        LOG_INFO("Shutting down...");
        engine.stop();
        engine.unregisterCallback(&callback);
        engine.shutdown();
        window.shutdown();
        
        // Save configuration
        common::ConfigManager::save();
        
        common::Logger::shutdown();
        return 0;

    } catch (const std::exception& e) {
        std::string errorMsg = "Unhandled Exception: ";
        errorMsg += e.what();
        MessageBoxA(nullptr, errorMsg.c_str(), "OpenMeters Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBoxA(nullptr, "Unknown Unhandled Exception occurred.", "OpenMeters Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}

#endif // BUILD_GUI
#endif // _WIN32

