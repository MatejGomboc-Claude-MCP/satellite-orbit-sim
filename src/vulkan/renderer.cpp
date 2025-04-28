#include "vulkan/renderer.h"
#include "vulkan/instance.h"
#include "vulkan/swapchain.h"
#include <stdexcept>
#include <array>
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Renderer::Renderer(GLFWwindow* window) 
    : m_window(window), m_currentFrame(0), m_currentImageIndex(0) {
    
    // Create Vulkan instance and select device
    m_instance = std::make_unique<VulkanInstance>(window);
    
    // Create render pass
    createRenderPass();
    
    // Create swapchain
    m_swapchain = std::make_unique<VulkanSwapchain>(window, m_instance.get(), m_renderPass);
    
    // Create descriptor resources for uniform buffers
    createDescriptorResources();
    
    // Create pipelines for Earth and satellite rendering
    createGraphicsPipelines();
    
    // Create uniform buffers for transformation matrices
    createUniformBuffers();
    
    // Create command resources for rendering
    createCommandResources();
    
    // Create synchronization objects
    createSyncObjects();
    
    // Create Earth geometry
    createEarthGeometry();
    
    // Initialize view and projection matrices
    m_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 15.0f),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
    );
    
    // Perspective projection with 45-degree FOV
    m_projectionMatrix = glm::perspective(
        glm::radians(45.0f),                                  // FOV
        static_cast<float>(m_swapchain->getExtent().width) /  // Aspect ratio
            static_cast<float>(m_swapchain->getExtent().height),
        0.1f,                                                 // Near plane
        100.0f                                                // Far plane
    );
    
    // Adjust for Vulkan's coordinate system 
    m_projectionMatrix[1][1] *= -1;
    
    std::cout << "Renderer initialized successfully" << std::endl;
}

Renderer::~Renderer() {
    // Wait for the device to finish operations
    vkDeviceWaitIdle(m_instance->getLogicalDevice());
    
    // Clean up synchronization objects
    for (size_t i = 0; i < m_inFlightFences.size(); i++) {
        vkDestroyFence(m_instance->getLogicalDevice(), m_inFlightFences[i], nullptr);
        vkDestroySemaphore(m_instance->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_instance->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
    }
    
    // Clean up command pool
    vkDestroyCommandPool(m_instance->getLogicalDevice(), m_commandPool, nullptr);
    
    // Clean up satellite vertex buffer
    vkDestroyBuffer(m_instance->getLogicalDevice(), m_satelliteVertexBuffer.buffer, nullptr);
    vkFreeMemory(m_instance->getLogicalDevice(), m_satelliteVertexBuffer.memory, nullptr);
    
    // Clean up Earth vertex and index buffers
    vkDestroyBuffer(m_instance->getLogicalDevice(), m_earthIndexBuffer.buffer, nullptr);
    vkFreeMemory(m_instance->getLogicalDevice(), m_earthIndexBuffer.memory, nullptr);
    
    vkDestroyBuffer(m_instance->getLogicalDevice(), m_earthVertexBuffer.buffer, nullptr);
    vkFreeMemory(m_instance->getLogicalDevice(), m_earthVertexBuffer.memory, nullptr);
    
    // Clean up uniform buffers
    for (auto& uniformBuffer : m_uniformBuffers) {
        vkDestroyBuffer(m_instance->getLogicalDevice(), uniformBuffer.buffer, nullptr);
        vkFreeMemory(m_instance->getLogicalDevice(), uniformBuffer.memory, nullptr);
    }
    
    // Clean up descriptor resources
    vkDestroyDescriptorPool(m_instance->getLogicalDevice(), m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_instance->getLogicalDevice(), m_descriptorSetLayout, nullptr);
    
    // Clean up pipelines
    vkDestroyPipeline(m_instance->getLogicalDevice(), m_satellitePipeline, nullptr);
    vkDestroyPipeline(m_instance->getLogicalDevice(), m_earthPipeline, nullptr);
    vkDestroyPipelineLayout(m_instance->getLogicalDevice(), m_pipelineLayout, nullptr);
    
    // Clean up swapchain
    m_swapchain.reset();
    
    // Clean up render pass
    vkDestroyRenderPass(m_instance->getLogicalDevice(), m_renderPass, nullptr);
    
    // Clean up Vulkan instance (handled by unique_ptr)
}

bool Renderer::beginFrame() {
    VkDevice device = m_instance->getLogicalDevice();
    
    // Wait for the previous frame to finish
    vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    
    // Acquire an image from the swapchain
    VkResult result = m_swapchain->acquireNextImage(
        m_imageAvailableSemaphores[m_currentFrame], m_currentImageIndex);
    
    // Check if we need to recreate the swapchain
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return false;  // Skip this frame
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }
    
    // Reset the fence only if we're submitting work
    vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);
    
    // Reset the command buffer
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    
    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
    
    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchain->getFramebuffers()[m_currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->getExtent();
    
    // Setup clear values (color and depth)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.05f, 1.0f};  // Dark blue space background
    clearValues[1].depthStencil = {1.0f, 0};           // Depth clear value (1.0 is "far")
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(m_commandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Update uniform buffer with current matrices
    updateUniformBuffer();
    
    return true;
}

void Renderer::endFrame() {
    // End the render pass
    vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);
    
    // End command buffer recording
    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
    
    // Submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    // Wait on the image available semaphore
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    // Command buffer to submit
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    
    // Semaphore to signal when the command buffer has finished execution
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Submit to the graphics queue
    if (vkQueueSubmit(m_instance->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    // Present the image
    VkResult result = m_swapchain->presentImage(m_renderFinishedSemaphores[m_currentFrame], m_currentImageIndex);
    
    // Check for swapchain recreation
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    
    // Advance to the next frame
    m_currentFrame = (m_currentFrame + 1) % m_imageAvailableSemaphores.size();
}

void Renderer::recreateSwapchain() {
    std::cout << "Recreating swapchain..." << std::endl;
    
    // Handle window minimization - wait until the window is visible again
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    
    // Wait for all GPU operations to complete
    vkDeviceWaitIdle(m_instance->getLogicalDevice());
    
    // Cleanup old swapchain and related resources
    
    // Clean up descriptor sets (will be recreated)
    vkDestroyDescriptorPool(m_instance->getLogicalDevice(), m_descriptorPool, nullptr);
    
    // We don't need to destroy the descriptor set layout
    
    // Clean up uniform buffers
    for (auto& uniformBuffer : m_uniformBuffers) {
        vkDestroyBuffer(m_instance->getLogicalDevice(), uniformBuffer.buffer, nullptr);
        vkFreeMemory(m_instance->getLogicalDevice(), uniformBuffer.memory, nullptr);
    }
    m_uniformBuffers.clear();
    
    // Keep pipelines and pipeline layout
    
    // Recreate swapchain
    m_swapchain = std::make_unique<VulkanSwapchain>(m_window, m_instance.get(), m_renderPass);
    
    // Recreate descriptor resources
    createDescriptorResources();
    
    // Recreate uniform buffers
    createUniformBuffers();
    
    // Update projection matrix for new swapchain dimensions
    m_projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(m_swapchain->getExtent().width) / static_cast<float>(m_swapchain->getExtent().height),
        0.1f,
        100.0f
    );
    m_projectionMatrix[1][1] *= -1;  // Adjust for Vulkan's coordinate system
    
    std::cout << "Swapchain recreated successfully" << std::endl;
}

void Renderer::updateCamera(const glm::vec3& position, const glm::vec3& target) {
    // Update the view matrix
    m_viewMatrix = glm::lookAt(
        position,                   // Camera position
        target,                     // Look at target
        glm::vec3(0.0f, 1.0f, 0.0f) // Up vector
    );
}

void Renderer::drawEarth() {
    VkCommandBuffer cmdBuffer = m_commandBuffers[m_currentFrame];
    
    // Bind the Earth pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_earthPipeline);
    
    // Bind descriptor set with MVP matrices
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &m_descriptorSets[m_currentImageIndex],
        0,
        nullptr
    );
    
    // Bind vertex and index buffers
    VkBuffer vertexBuffers[] = {m_earthVertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, m_earthIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    
    // Draw the Earth
    vkCmdDrawIndexed(cmdBuffer, m_earthIndexCount, 1, 0, 0, 0);
}

void Renderer::drawSatellite(const glm::vec3& position) {
    VkCommandBuffer cmdBuffer = m_commandBuffers[m_currentFrame];
    
    // Create a model matrix that places the satellite at the specified position
    glm::mat4 satelliteModel = glm::translate(glm::mat4(1.0f), position);
    
    // Update the uniform buffer with this model matrix
    UniformBufferObject ubo{};
    ubo.model = satelliteModel;
    ubo.view = m_viewMatrix;
    ubo.proj = m_projectionMatrix;
    
    // Map the uniform buffer memory
    void* data;
    vkMapMemory(
        m_instance->getLogicalDevice(),
        m_uniformBuffers[m_currentImageIndex].memory,
        0,
        sizeof(ubo),
        0,
        &data
    );
    
    // Copy the UBO data
    memcpy(data, &ubo, sizeof(ubo));
    
    // Unmap the memory
    vkUnmapMemory(m_instance->getLogicalDevice(), m_uniformBuffers[m_currentImageIndex].memory);
    
    // Bind the satellite pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_satellitePipeline);
    
    // Bind descriptor set with MVP matrices
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &m_descriptorSets[m_currentImageIndex],
        0,
        nullptr
    );
    
    // Create a simple vertex with just the position
    struct Vertex {
        glm::vec3 pos;
    };
    
    Vertex satelliteVertex = {position};
    
    // Update the satellite vertex buffer
    void* vertexData;
    vkMapMemory(
        m_instance->getLogicalDevice(),
        m_satelliteVertexBuffer.memory,
        0,
        sizeof(Vertex),
        0,
        &vertexData
    );
    
    // Copy the vertex data
    memcpy(vertexData, &satelliteVertex, sizeof(Vertex));
    
    // Unmap the memory
    vkUnmapMemory(m_instance->getLogicalDevice(), m_satelliteVertexBuffer.memory);
    
    // Bind the vertex buffer
    VkBuffer vertexBuffers[] = {m_satelliteVertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    
    // Draw the satellite as a point
    vkCmdDraw(cmdBuffer, 1, 1, 0, 0);
}

VkCommandBuffer Renderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_instance->getLogicalDevice(), &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(m_instance->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_instance->getGraphicsQueue());
    
    vkFreeCommandBuffers(m_instance->getLogicalDevice(), m_commandPool, 1, &commandBuffer);
}

// Accessor methods
VkInstance Renderer::getInstance() const {
    return m_instance->getInstance();
}

VkPhysicalDevice Renderer::getPhysicalDevice() const {
    return m_instance->getPhysicalDevice();
}

VkDevice Renderer::getDevice() const {
    return m_instance->getLogicalDevice();
}

uint32_t Renderer::getGraphicsQueueFamily() const {
    return m_instance->getGraphicsQueueFamily();
}

VkQueue Renderer::getGraphicsQueue() const {
    return m_instance->getGraphicsQueue();
}

VkRenderPass Renderer::getRenderPass() const {
    return m_renderPass;
}

VkCommandBuffer Renderer::getCurrentCommandBuffer() const {
    return m_commandBuffers[m_currentFrame];
}

void Renderer::createRenderPass() {
    // Attachment descriptions
    std::array<VkAttachmentDescription, 2> attachments = {};
    
    // Color attachment
    attachments[0].format = m_swapchain->getImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    // Depth attachment
    attachments[1].format = findDepthFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Attachment references
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Subpass description
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    // Subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_instance->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

VkFormat Renderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_instance->getPhysicalDevice(), format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format!");
}

void Renderer::createGraphicsPipelines() {
    // Load shaders (HLSL compiled to SPIR-V using DXC)
    VkShaderModule earthVertShaderModule = createShaderModule("shaders/earth_vert.spv");
    VkShaderModule earthFragShaderModule = createShaderModule("shaders/earth_frag.spv");
    VkShaderModule satelliteVertShaderModule = createShaderModule("shaders/satellite_vert.spv");
    VkShaderModule satelliteFragShaderModule = createShaderModule("shaders/satellite_frag.spv");
    
    // Shader stage creation
    VkPipelineShaderStageCreateInfo earthVertShaderStageInfo{};
    earthVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    earthVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    earthVertShaderStageInfo.module = earthVertShaderModule;
    earthVertShaderStageInfo.pName = "VSMain";  // Changed to HLSL entry point name
    
    VkPipelineShaderStageCreateInfo earthFragShaderStageInfo{};
    earthFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    earthFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    earthFragShaderStageInfo.module = earthFragShaderModule;
    earthFragShaderStageInfo.pName = "PSMain";  // Changed to HLSL entry point name
    
    VkPipelineShaderStageCreateInfo satelliteVertShaderStageInfo{};
    satelliteVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    satelliteVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    satelliteVertShaderStageInfo.module = satelliteVertShaderModule;
    satelliteVertShaderStageInfo.pName = "VSMain";  // Changed to HLSL entry point name
    
    VkPipelineShaderStageCreateInfo satelliteFragShaderStageInfo{};
    satelliteFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    satelliteFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    satelliteFragShaderStageInfo.module = satelliteFragShaderModule;
    satelliteFragShaderStageInfo.pName = "PSMain";  // Changed to HLSL entry point name
    
    VkPipelineShaderStageCreateInfo earthShaderStages[] = {
        earthVertShaderStageInfo, earthFragShaderStageInfo
    };
    
    VkPipelineShaderStageCreateInfo satelliteShaderStages[] = {
        satelliteVertShaderStageInfo, satelliteFragShaderStageInfo
    };
    
    // Earth vertex input
    VkVertexInputBindingDescription earthBindingDescription{};
    earthBindingDescription.binding = 0;
    earthBindingDescription.stride = sizeof(float) * 6;  // position (3) + normal (3)
    earthBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 2> earthAttributeDescriptions{};
    
    // Position attribute
    earthAttributeDescriptions[0].binding = 0;
    earthAttributeDescriptions[0].location = 0;
    earthAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    earthAttributeDescriptions[0].offset = 0;
    
    // Normal attribute
    earthAttributeDescriptions[1].binding = 0;
    earthAttributeDescriptions[1].location = 1;
    earthAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    earthAttributeDescriptions[1].offset = sizeof(float) * 3;
    
    // Satellite vertex input (only position)
    VkVertexInputBindingDescription satelliteBindingDescription{};
    satelliteBindingDescription.binding = 0;
    satelliteBindingDescription.stride = sizeof(float) * 3;  // position (3)
    satelliteBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription satelliteAttributeDescription{};
    satelliteAttributeDescription.binding = 0;
    satelliteAttributeDescription.location = 0;
    satelliteAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    satelliteAttributeDescription.offset = 0;
    
    // Earth vertex input state
    VkPipelineVertexInputStateCreateInfo earthVertexInputInfo{};
    earthVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    earthVertexInputInfo.vertexBindingDescriptionCount = 1;
    earthVertexInputInfo.pVertexBindingDescriptions = &earthBindingDescription;
    earthVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(earthAttributeDescriptions.size());
    earthVertexInputInfo.pVertexAttributeDescriptions = earthAttributeDescriptions.data();
    
    // Satellite vertex input state
    VkPipelineVertexInputStateCreateInfo satelliteVertexInputInfo{};
    satelliteVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    satelliteVertexInputInfo.vertexBindingDescriptionCount = 1;
    satelliteVertexInputInfo.pVertexBindingDescriptions = &satelliteBindingDescription;
    satelliteVertexInputInfo.vertexAttributeDescriptionCount = 1;
    satelliteVertexInputInfo.pVertexAttributeDescriptions = &satelliteAttributeDescription;
    
    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo earthInputAssembly{};
    earthInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    earthInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    earthInputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Point list for satellite
    VkPipelineInputAssemblyStateCreateInfo satelliteInputAssembly{};
    satelliteInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    satelliteInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    satelliteInputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Viewport state
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->getExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->getExtent();
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Rasterization state for Earth (solid fill)
    VkPipelineRasterizationStateCreateInfo earthRasterizer{};
    earthRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    earthRasterizer.depthClampEnable = VK_FALSE;
    earthRasterizer.rasterizerDiscardEnable = VK_FALSE;
    earthRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    earthRasterizer.lineWidth = 1.0f;
    earthRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    earthRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    earthRasterizer.depthBiasEnable = VK_FALSE;
    
    // Rasterization state for satellite (point)
    VkPipelineRasterizationStateCreateInfo satelliteRasterizer{};
    satelliteRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    satelliteRasterizer.depthClampEnable = VK_FALSE;
    satelliteRasterizer.rasterizerDiscardEnable = VK_FALSE;
    satelliteRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    satelliteRasterizer.lineWidth = 1.0f;
    satelliteRasterizer.cullMode = VK_CULL_MODE_NONE;
    satelliteRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    satelliteRasterizer.depthBiasEnable = VK_FALSE;
    
    // Enable larger point sizes for the satellite
    satelliteRasterizer.depthBiasEnable = VK_FALSE;
    
    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Depth stencil state - add depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Color blending for Earth (opaque)
    VkPipelineColorBlendAttachmentState earthColorBlendAttachment{};
    earthColorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    earthColorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo earthColorBlending{};
    earthColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    earthColorBlending.logicOpEnable = VK_FALSE;
    earthColorBlending.attachmentCount = 1;
    earthColorBlending.pAttachments = &earthColorBlendAttachment;
    
    // Color blending for satellite (alpha blending)
    VkPipelineColorBlendAttachmentState satelliteColorBlendAttachment{};
    satelliteColorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    satelliteColorBlendAttachment.blendEnable = VK_TRUE;
    satelliteColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    satelliteColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    satelliteColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    satelliteColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    satelliteColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    satelliteColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo satelliteColorBlending{};
    satelliteColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    satelliteColorBlending.logicOpEnable = VK_FALSE;
    satelliteColorBlending.attachmentCount = 1;
    satelliteColorBlending.pAttachments = &satelliteColorBlendAttachment;
    
    // Dynamic states - make point size dynamic for satellite
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    
    if (vkCreatePipelineLayout(m_instance->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    // Earth pipeline creation
    VkGraphicsPipelineCreateInfo earthPipelineInfo{};
    earthPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    earthPipelineInfo.stageCount = 2;
    earthPipelineInfo.pStages = earthShaderStages;
    earthPipelineInfo.pVertexInputState = &earthVertexInputInfo;
    earthPipelineInfo.pInputAssemblyState = &earthInputAssembly;
    earthPipelineInfo.pViewportState = &viewportState;
    earthPipelineInfo.pRasterizationState = &earthRasterizer;
    earthPipelineInfo.pMultisampleState = &multisampling;
    earthPipelineInfo.pDepthStencilState = &depthStencil;
    earthPipelineInfo.pColorBlendState = &earthColorBlending;
    earthPipelineInfo.pDynamicState = &dynamicState;
    earthPipelineInfo.layout = m_pipelineLayout;
    earthPipelineInfo.renderPass = m_renderPass;
    earthPipelineInfo.subpass = 0;
    earthPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    // Satellite pipeline creation
    VkGraphicsPipelineCreateInfo satellitePipelineInfo{};
    satellitePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    satellitePipelineInfo.stageCount = 2;
    satellitePipelineInfo.pStages = satelliteShaderStages;
    satellitePipelineInfo.pVertexInputState = &satelliteVertexInputInfo;
    satellitePipelineInfo.pInputAssemblyState = &satelliteInputAssembly;
    satellitePipelineInfo.pViewportState = &viewportState;
    satellitePipelineInfo.pRasterizationState = &satelliteRasterizer;
    satellitePipelineInfo.pMultisampleState = &multisampling;
    satellitePipelineInfo.pDepthStencilState = &depthStencil;
    satellitePipelineInfo.pColorBlendState = &satelliteColorBlending;
    satellitePipelineInfo.pDynamicState = &dynamicState;
    satellitePipelineInfo.layout = m_pipelineLayout;
    satellitePipelineInfo.renderPass = m_renderPass;
    satellitePipelineInfo.subpass = 0;
    satellitePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    // Create the graphics pipelines
    if (vkCreateGraphicsPipelines(
            m_instance->getLogicalDevice(), 
            VK_NULL_HANDLE, 
            1, 
            &earthPipelineInfo, 
            nullptr, 
            &m_earthPipeline) != VK_SUCCESS ||
        vkCreateGraphicsPipelines(
            m_instance->getLogicalDevice(), 
            VK_NULL_HANDLE, 
            1, 
            &satellitePipelineInfo, 
            nullptr, 
            &m_satellitePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipelines!");
    }
    
    // Cleanup shader modules
    vkDestroyShaderModule(m_instance->getLogicalDevice(), earthVertShaderModule, nullptr);
    vkDestroyShaderModule(m_instance->getLogicalDevice(), earthFragShaderModule, nullptr);
    vkDestroyShaderModule(m_instance->getLogicalDevice(), satelliteVertShaderModule, nullptr);
    vkDestroyShaderModule(m_instance->getLogicalDevice(), satelliteFragShaderModule, nullptr);
}

void Renderer::createDescriptorResources() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(
            m_instance->getLogicalDevice(), 
            &layoutInfo, 
            nullptr, 
            &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
    
    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(m_swapchain->getImageCount());
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(m_swapchain->getImageCount());
    
    if (vkCreateDescriptorPool(
            m_instance->getLogicalDevice(), 
            &poolInfo, 
            nullptr, 
            &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
    
    // Prepare for descriptor set allocation
    std::vector<VkDescriptorSetLayout> layouts(m_swapchain->getImageCount(), m_descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchain->getImageCount());
    allocInfo.pSetLayouts = layouts.data();
    
    // Allocate descriptor sets
    m_descriptorSets.resize(m_swapchain->getImageCount());
    if (vkAllocateDescriptorSets(
            m_instance->getLogicalDevice(), 
            &allocInfo, 
            m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }
}

void Renderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    m_uniformBuffers.resize(m_swapchain->getImageCount());
    
    for (size_t i = 0; i < m_swapchain->getImageCount(); i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i].buffer,
            m_uniformBuffers[i].memory
        );
        
        // Update descriptor sets
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(m_instance->getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
    }
    
    // Create satellite vertex buffer (just a single point)
    createBuffer(
        sizeof(float) * 3,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_satelliteVertexBuffer.buffer,
        m_satelliteVertexBuffer.memory
    );
}

void Renderer::createCommandResources() {
    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_instance->getGraphicsQueueFamily();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if (vkCreateCommandPool(m_instance->getLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
    
    // Create command buffers (one per frame in flight)
    m_commandBuffers.resize(2);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_instance->getLogicalDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void Renderer::createSyncObjects() {
    // We'll use 2 frames in flight (double buffering)
    m_imageAvailableSemaphores.resize(2);
    m_renderFinishedSemaphores.resize(2);
    m_inFlightFences.resize(2);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled to avoid waiting in first frame
    
    for (size_t i = 0; i < 2; i++) {
        if (vkCreateSemaphore(m_instance->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_instance->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_instance->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
    }
}

void Renderer::createEarthGeometry() {
    // Generate a sphere mesh for Earth
    
    // Parameters for sphere generation
    const int stacks = 32;
    const int slices = 32;
    const float radius = 6.371f;  // Earth radius
    
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    // Generate vertices
    for (int stack = 0; stack <= stacks; ++stack) {
        float phi = static_cast<float>(stack) / stacks * glm::pi<float>();
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);
        
        for (int slice = 0; slice <= slices; ++slice) {
            float theta = static_cast<float>(slice) / slices * 2.0f * glm::pi<float>();
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);
            
            // Calculate position
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi;
            float z = radius * sinPhi * sinTheta;
            
            // Calculate normal
            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;
            
            // Add vertex data (position + normal)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }
    
    // Generate indices
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            int topLeft = stack * (slices + 1) + slice;
            int topRight = topLeft + 1;
            int bottomLeft = (stack + 1) * (slices + 1) + slice;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // Create vertex buffer
    VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );
    
    // Copy vertex data to staging buffer
    void* data;
    vkMapMemory(m_instance->getLogicalDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(m_instance->getLogicalDevice(), stagingBufferMemory);
    
    // Create vertex buffer
    createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_earthVertexBuffer.buffer,
        m_earthVertexBuffer.memory
    );
    
    // Copy from staging buffer to vertex buffer
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_earthVertexBuffer.buffer, 1, &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
    
    // Cleanup staging buffer
    vkDestroyBuffer(m_instance->getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_instance->getLogicalDevice(), stagingBufferMemory, nullptr);
    
    // Create index buffer
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    m_earthIndexCount = static_cast<uint32_t>(indices.size());
    
    // Create staging buffer for indices
    createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );
    
    // Copy index data to staging buffer
    vkMapMemory(m_instance->getLogicalDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(m_instance->getLogicalDevice(), stagingBufferMemory);
    
    // Create index buffer
    createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_earthIndexBuffer.buffer,
        m_earthIndexBuffer.memory
    );
    
    // Copy from staging buffer to index buffer
    commandBuffer = beginSingleTimeCommands();
    
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_earthIndexBuffer.buffer, 1, &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
    
    // Cleanup staging buffer
    vkDestroyBuffer(m_instance->getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_instance->getLogicalDevice(), stagingBufferMemory, nullptr);
}

void Renderer::updateUniformBuffer() {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    // Create Earth rotation model matrix
    glm::mat4 earthModel = glm::rotate(
        glm::mat4(1.0f),                  // Identity matrix
        time * glm::radians(10.0f),       // Rotation angle (10 degrees per second)
        glm::vec3(0.0f, 1.0f, 0.0f)       // Rotate around Y axis
    );
    
    // Update uniform buffer
    UniformBufferObject ubo{};
    ubo.model = earthModel;
    ubo.view = m_viewMatrix;
    ubo.proj = m_projectionMatrix;
    
    void* data;
    vkMapMemory(
        m_instance->getLogicalDevice(),
        m_uniformBuffers[m_currentImageIndex].memory,
        0,
        sizeof(ubo),
        0,
        &data
    );
    
    memcpy(data, &ubo, sizeof(ubo));
    
    vkUnmapMemory(m_instance->getLogicalDevice(), m_uniformBuffers[m_currentImageIndex].memory);
}

void Renderer::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) {
    
    VkDevice device = m_instance->getLogicalDevice();
    
    // Create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
    
    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    
    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }
    
    // Bind buffer memory
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_instance->getPhysicalDevice(), &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

VkShaderModule Renderer::createShaderModule(const std::string& filename) {
    // Read file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    // Create shader module
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_instance->getLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    
    return shaderModule;
}
