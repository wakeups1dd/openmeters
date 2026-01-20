#pragma once

#include "../common/config.h"
#include "../common/meter-values.h"
#include <windows.h>
#include <d3d11.h>
#include <memory>
#include <mutex>

// Forward declarations
struct ImGuiContext;
struct ImGuiIO;
struct ImVec2;

namespace openmeters::ui {

/**
 * Main application window using ImGui.
 * Creates an always-on-top overlay window with audio meters.
 */
class Window {
public:
    Window();
    ~Window();
    
    // Non-copyable, non-movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;
    
    /**
     * Initialize the window and graphics context.
     * 
     * @param hInstance Windows instance handle
     * @param nCmdShow Show command
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(HINSTANCE hInstance, int nCmdShow);
    
    /**
     * Main message loop.
     * Runs until the window is closed.
     */
    void run();
    
    /**
     * Shutdown and cleanup resources.
     */
    void shutdown();
    
    /**
     * Update meter values for display.
     * Called from audio callback thread.
     * 
     * @param snapshot Current meter snapshot
     */
    void updateMeters(const common::MeterSnapshot& snapshot);
    
    /**
     * Check if window should close.
     */
    [[nodiscard]] bool shouldClose() const;

private:
    /**
     * Create Win32 window.
     */
    bool createWindow(HINSTANCE hInstance, int nCmdShow);
    
    /**
     * Initialize DirectX 11.
     */
    bool initializeD3D11();
    
    /**
     * Initialize ImGui.
     */
    bool initializeImGui();
    
    /**
     * Render frame.
     */
    void renderFrame();
    
    /**
     * Render meter UI.
     */
    void renderMeters();
    
    /**
     * Render settings window.
     */
    void renderSettings();
    
    /**
     * Setup custom ImGui style.
     */
    void setupStyle();
    
    /**
     * Draw a segmented LED-style meter.
     */
    void drawMeter(const char* label, float value, const ImVec2& size);
    
    /**
     * Window procedure.
     */
    static LRESULT CALLBACK windowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Window handles
    HWND m_hWnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    
    // DirectX 11
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    
    // ImGui
    ImGuiContext* m_imguiContext = nullptr;
    
    // State
    bool m_shouldClose = false;
    bool m_showSettings = false;
    
    // Meter data (protected by mutex)
    std::mutex m_meterMutex;
    common::MeterSnapshot m_currentSnapshot;
    
    // Configuration
    common::AppConfig m_config;
};

} // namespace openmeters::ui

