#pragma once

#include "vulkan/renderer.h"
#include "ui/imgui_manager.h"
#include "orbit/orbital_mechanics.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

/**
 * Main application class that manages the simulation.
 * 
 * This class initializes the window, Vulkan renderer, ImGui interface,
 * and orbital mechanics. It handles the main application loop, user input,
 * and updates the simulation state.
 */
class Application {
public:
    /**
     * Constructor initializes GLFW, creates window, and sets up all components.
     */
    Application();
    
    /**
     * Destructor cleans up resources in reverse order of initialization.
     */
    ~Application();

    /**
     * Main application loop that runs until the window is closed.
     */
    void run();

    /**
     * Handle mouse scroll events to adjust camera zoom.
     * 
     * @param yoffset Vertical scroll offset
     */
    void handleMouseScroll(double yoffset);

private:
    /**
     * Process keyboard and mouse input events.
     */
    void processInput();
    
    /**
     * Update simulation state based on elapsed time.
     * 
     * @param deltaTime Time elapsed since last frame, adjusted by time multiplier
     */
    void update(float deltaTime);
    
    /**
     * Render the current frame.
     */
    void render();
    
    /**
     * Update camera position and orientation based on user input.
     */
    void updateCamera();

    // Application state
    bool m_running;
    float m_timeMultiplier;
    double m_lastFrameTime;
    
    // GLFW window
    GLFWwindow* m_window;
    
    // Core components
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<ImGuiManager> m_uiManager;
    std::unique_ptr<OrbitalMechanics> m_orbitalMechanics;
    
    // Camera settings
    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    
    // Mouse input tracking
    bool m_mousePressed;
    double m_lastMouseX;
    double m_lastMouseY;
    
    // UI state
    bool m_showHelpWindow = false;
    bool m_showAboutWindow = false;
};
