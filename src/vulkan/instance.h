#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

/**
 * Manages the Vulkan instance, physical device selection, and logical device creation.
 * 
 * This class handles Vulkan initialization, device selection, and queue setup.
 */
class VulkanInstance {
public:
    /**
     * Constructor initializes the Vulkan instance and selects a suitable device.
     * 
     * @param window GLFW window handle
     */
    VulkanInstance(GLFWwindow* window);
    
    /**
     * Destructor cleans up resources.
     */
    ~VulkanInstance();
    
    // Getters for Vulkan resources
    VkInstance getInstance() const { return m_instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getLogicalDevice() const { return m_device; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
    uint32_t getPresentQueueFamily() const { return m_presentQueueFamily; }
    
private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    
    uint32_t m_graphicsQueueFamily;
    uint32_t m_presentQueueFamily;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    
    /**
     * Creates the Vulkan instance with validation layers if enabled.
     */
    void createInstance();
    
    /**
     * Sets up the debug messenger for validation layer callbacks.
     */
    void setupDebugMessenger();
    
    /**
     * Creates the window surface for presentation.
     * 
     * @param window GLFW window handle
     */
    void createSurface(GLFWwindow* window);
    
    /**
     * Selects a suitable physical device for rendering.
     */
    void pickPhysicalDevice();
    
    /**
     * Creates a logical device and retrieves queue handles.
     */
    void createLogicalDevice();
    
    /**
     * Checks if a physical device is suitable for our needs.
     * 
     * @param device Physical device to check
     * @return True if the device is suitable
     */
    bool isDeviceSuitable(VkPhysicalDevice device);
    
    /**
     * Finds queue families that support graphics and presentation.
     * 
     * @param device Physical device to check
     * @return Struct containing queue family indices
     */
    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    
    /**
     * Checks if a device supports the required extensions.
     * 
     * @param device Physical device to check
     * @return True if all required extensions are supported
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    
    /**
     * Queries which validation layers are available.
     * 
     * @return True if all requested layers are available
     */
    bool checkValidationLayerSupport();
    
    /**
     * Gets the required instance extensions.
     * 
     * @return Vector of required extension names
     */
    std::vector<const char*> getRequiredExtensions();
    
    // Validation layer settings
#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif
    
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};
