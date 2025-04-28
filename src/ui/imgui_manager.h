#pragma once

#include "vulkan/renderer.h"
#include <GLFW/glfw3.h>

/**
 * Manages ImGui integration with GLFW and Vulkan.
 * 
 * This class handles setup, rendering, and cleanup of the ImGui interface
 * which provides interactive controls for the simulation.
 */
class ImGuiManager {
public:
    /**
     * Constructor initializes ImGui with GLFW and Vulkan.
     * 
     * @param window GLFW window handle
     * @param renderer Pointer to the Vulkan renderer
     */
    ImGuiManager(GLFWwindow* window, Renderer* renderer);
    
    /**
     * Destructor cleans up ImGui resources.
     */
    ~ImGuiManager();
    
    /**
     * Begins a new ImGui frame.
     * Must be called before any ImGui commands.
     */
    void beginFrame();
    
    /**
     * Ends the current ImGui frame and renders UI.
     * Must be called after all ImGui commands.
     */
    void endFrame();
    
private:
    GLFWwindow* m_window;
    Renderer* m_renderer;
    
    // ImGui Vulkan resources
    VkDescriptorPool m_descriptorPool;
};
