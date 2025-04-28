#include "vulkan/swapchain.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

VulkanSwapchain::VulkanSwapchain(GLFWwindow* window, VulkanInstance* instance, VkRenderPass renderPass)
    : m_window(window), m_instance(instance), m_renderPass(renderPass) {
    
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

VulkanSwapchain::~VulkanSwapchain() {
    // Clean up framebuffers
    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_instance->getLogicalDevice(), framebuffer, nullptr);
    }
    
    // Clean up image views
    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_instance->getLogicalDevice(), imageView, nullptr);
    }
    
    // Clean up swapchain
    vkDestroySwapchainKHR(m_instance->getLogicalDevice(), m_swapchain, nullptr);
}

VkResult VulkanSwapchain::acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex) {
    // Acquire the next image from the swapchain
    return vkAcquireNextImageKHR(
        m_instance->getLogicalDevice(),
        m_swapchain,
        UINT64_MAX,  // No timeout
        imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );
}

VkResult VulkanSwapchain::presentImage(VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) {
    // Set up the present info
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    // The semaphore to wait on
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    
    // The swapchain and image to present
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    // Submit the presentation request
    return vkQueuePresentKHR(m_instance->getPresentQueue(), &presentInfo);
}

void VulkanSwapchain::createSwapchain() {
    // Query swapchain support details
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(
        m_instance->getPhysicalDevice(), 
        m_instance->getInstance()
    );
    
    // Choose the best settings for our swapchain
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);
    
    // Determine how many images to use in the swapchain
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    
    // Don't exceed the maximum number of images (0 means no maximum)
    if (swapchainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }
    
    // Set up swapchain create info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_instance->getInstance();
    
    // Specify the number and format of images
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;  // 2 for stereo rendering
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    // Specify how images are handled across multiple queue families
    uint32_t queueFamilyIndices[] = {
        m_instance->getGraphicsQueueFamily(),
        m_instance->getPresentQueueFamily()
    };
    
    if (m_instance->getGraphicsQueueFamily() != m_instance->getPresentQueueFamily()) {
        // Images can be used across multiple queue families
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // Better performance when using the same queue family
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    // Specify transforms to apply to images
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    
    // Specify if the alpha channel should be used for blending with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    // Specify the presentation mode and whether to clip obscured pixels
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    // Reference to old swapchain in case of recreation
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    // Create the swapchain
    if (vkCreateSwapchainKHR(m_instance->getLogicalDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }
    
    // Get the swapchain images
    vkGetSwapchainImagesKHR(m_instance->getLogicalDevice(), m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_instance->getLogicalDevice(), m_swapchain, &imageCount, m_swapchainImages.data());
    
    // Store the format and extent for later use
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
}

void VulkanSwapchain::createImageViews() {
    // Resize the image views vector to match the number of swapchain images
    m_swapchainImageViews.resize(m_swapchainImages.size());
    
    // Create an image view for each swapchain image
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        
        // Specify how the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        
        // Default color channel mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // Specify the image's purpose and which part to access
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        // Create the image view
        if (vkCreateImageView(m_instance->getLogicalDevice(), &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void VulkanSwapchain::createFramebuffers() {
    // Resize the framebuffers vector to match the number of swapchain images
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());
    
    // Create a framebuffer for each image view
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapchainImageViews[i]
        };
        
        // Set up framebuffer create info
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;
        
        // Create the framebuffer
        if (vkCreateFramebuffer(m_instance->getLogicalDevice(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

VulkanSwapchain::SwapchainSupportDetails VulkanSwapchain::querySwapchainSupport(
    VkPhysicalDevice device, VkSurfaceKHR surface) {
    
    SwapchainSupportDetails details;
    
    // Query surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    // Query supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    // Query supported presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Preferred format: SRGB color space with 8-bit RGBA
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    // If the preferred format isn't available, just use the first one
    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Prefer mailbox mode for lower latency without tearing
    // (Triple buffering)
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    
    // FIFO mode is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If the surface size is defined, use it
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        // Otherwise, use the window size clamped to the allowed range
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        // Clamp to the allowed range
        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );
        
        return actualExtent;
    }
}
