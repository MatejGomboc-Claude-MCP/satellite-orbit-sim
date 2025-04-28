#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class VulkanInstance;
class VulkanSwapchain;

/**
 * Manages the Vulkan rendering pipeline and resources.
 * 
 * This class is responsible for initializing Vulkan, creating the graphics
 * pipeline, and rendering the simulation elements (Earth and satellite).
 */
class Renderer {
public:
    /**
     * Constructor initializes Vulkan and creates required resources.
     * 
     * @param window GLFW window handle
     */
    Renderer(GLFWwindow* window);
    
    /**
     * Destructor cleans up Vulkan resources.
     */
    ~Renderer();
    
    /**
     * Begins a new frame, acquiring a swapchain image.
     * 
     * @return True if frame was successfully started, false if rendering should be skipped
     */
    bool beginFrame();
    
    /**
     * Ends the current frame, submitting commands and presenting the result.
     */
    void endFrame();
    
    /**
     * Updates camera view and projection matrices.
     * 
     * @param position Camera position
     * @param target Camera target (look-at point)
     */
    void updateCamera(const glm::vec3& position, const glm::vec3& target);
    
    /**
     * Draws the Earth as a simple sphere.
     */
    void drawEarth();
    
    /**
     * Draws the satellite as a bright point at the specified position.
     * 
     * @param position Satellite's 3D position
     */
    void drawSatellite(const glm::vec3& position);
    
    /**
     * Creates a command buffer for one-time use commands.
     * 
     * @return Command buffer ready for recording
     */
    VkCommandBuffer beginSingleTimeCommands();
    
    /**
     * Submits and frees a one-time use command buffer.
     * 
     * @param commandBuffer Command buffer to submit
     */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    // Accessor methods for other components (like ImGui)
    VkInstance getInstance() const;
    VkPhysicalDevice getPhysicalDevice() const;
    VkDevice getDevice() const;
    uint32_t getGraphicsQueueFamily() const;
    VkQueue getGraphicsQueue() const;
    VkRenderPass getRenderPass() const;
    VkCommandBuffer getCurrentCommandBuffer() const;
    
private:
    GLFWwindow* m_window;
    
    // Core Vulkan components
    std::unique_ptr<VulkanInstance> m_instance;
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    
    // Rendering resources
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_earthPipeline;
    VkPipeline m_satellitePipeline;
    
    // Descriptor sets for uniform data
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    
    // Uniform buffer for MVP matrices
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    
    struct BufferResource {
        VkBuffer buffer;
        VkDeviceMemory memory;
    };
    
    std::vector<BufferResource> m_uniformBuffers;
    
    // Earth mesh data
    BufferResource m_earthVertexBuffer;
    BufferResource m_earthIndexBuffer;
    uint32_t m_earthIndexCount;
    
    // Satellite rendering data (just position)
    BufferResource m_satelliteVertexBuffer;
    
    // Frame resources
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    uint32_t m_currentFrame;
    uint32_t m_currentImageIndex;
    
    // Camera state
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    
    /**
     * Recreates the swapchain when the window is resized.
     */
    void recreateSwapchain();

    /**
     * Creates the render pass.
     */
    void createRenderPass();
    
    /**
     * Creates the graphics pipeline.
     */
    void createGraphicsPipelines();
    
    /**
     * Creates descriptor set layout and pool.
     */
    void createDescriptorResources();
    
    /**
     * Creates uniform buffers for MVP matrices.
     */
    void createUniformBuffers();
    
    /**
     * Creates command pool and buffers.
     */
    void createCommandResources();
    
    /**
     * Creates synchronization objects (semaphores and fences).
     */
    void createSyncObjects();
    
    /**
     * Creates a sphere mesh for Earth.
     */
    void createEarthGeometry();
    
    /**
     * Updates uniform buffer with current matrices.
     */
    void updateUniformBuffer();
    
    /**
     * Creates a generic buffer.
     * 
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags
     * @param properties Memory property flags
     * @param buffer Output buffer handle
     * @param bufferMemory Output buffer memory handle
     */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                     VkMemoryPropertyFlags properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
    /**
     * Finds a suitable memory type for allocation.
     * 
     * @param typeFilter Type filter from memory requirements
     * @param properties Required memory properties
     * @return Index of a suitable memory type
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    /**
     * Loads a shader module from a file.
     * 
     * @param filename Shader file path
     * @return Shader module handle
     */
    VkShaderModule createShaderModule(const std::string& filename);

    /**
     * Finds a suitable depth format supported by the device.
     * 
     * @return A supported depth format
     */
    VkFormat findDepthFormat();

    /**
     * Finds a supported format from a list of candidates.
     * 
     * @param candidates List of candidate formats to check
     * @param tiling Tiling mode (linear or optimal)
     * @param features Required format features
     * @return The first supported format from the candidates
     */
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                                VkImageTiling tiling, 
                                VkFormatFeatureFlags features);
};
