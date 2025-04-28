#pragma once

#include "vulkan/instance.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

/**
 * Manages the Vulkan swapchain and associated resources.
 * 
 * This class handles the swapchain for presenting rendered images
 * to the window surface, including framebuffers and image views.
 */
class VulkanSwapchain {
public:
    /**
     * Constructor creates a new swapchain.
     * 
     * @param window GLFW window handle
     * @param instance VulkanInstance for device access
     * @param renderPass The render pass this swapchain will be used with
     */
    VulkanSwapchain(GLFWwindow* window, VulkanInstance* instance, VkRenderPass renderPass);
    
    /**
     * Destructor cleans up swapchain resources.
     */
    ~VulkanSwapchain();
    
    /**
     * Acquires the next available swapchain image.
     * 
     * @param imageAvailableSemaphore Semaphore to signal when image is available
     * @param imageIndex Output parameter for the acquired image index
     * @return Result of the acquisition (VK_SUCCESS if successful)
     */
    VkResult acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex);
    
    /**
     * Presents the rendered image to the display.
     * 
     * @param renderFinishedSemaphore Semaphore to wait on before presenting
     * @param imageIndex Index of the image to present
     * @return Result of the presentation (VK_SUCCESS if successful)
     */
    VkResult presentImage(VkSemaphore renderFinishedSemaphore, uint32_t imageIndex);
    
    // Getters
    VkExtent2D getExtent() const { return m_swapchainExtent; }
    VkFormat getImageFormat() const { return m_swapchainImageFormat; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_swapchainImages.size()); }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return m_swapchainFramebuffers; }
    
private:
    GLFWwindow* m_window;
    VulkanInstance* m_instance;
    VkRenderPass m_renderPass;
    
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    
    /**
     * Queries swapchain support details for a physical device.
     * 
     * @param device Physical device to query
     * @param surface Surface to check support for
     * @return Struct containing swapchain support details
     */
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    
    /**
     * Selects the best surface format from available options.
     * 
     * @param availableFormats List of available formats
     * @return The chosen format
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    
    /**
     * Selects the best presentation mode from available options.
     * 
     * @param availablePresentModes List of available presentation modes
     * @return The chosen presentation mode
     */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    
    /**
     * Determines the best swap extent (resolution) for the swapchain.
     * 
     * @param capabilities Surface capabilities
     * @return The chosen swap extent
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    /**
     * Creates the swapchain and its images.
     */
    void createSwapchain();
    
    /**
     * Creates image views for the swapchain images.
     */
    void createImageViews();
    
    /**
     * Creates framebuffers for rendering to the swapchain images.
     */
    void createFramebuffers();
};
