#include "ui/imgui_manager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

ImGuiManager::ImGuiManager(GLFWwindow* window, Renderer* renderer)
    : m_window(window), m_renderer(renderer) {
    
    // Create descriptor pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    
    if (vkCreateDescriptorPool(m_renderer->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool for ImGui");
    }
    
    // Initialize ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable keyboard navigation and docking features
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Set up ImGui style
    ImGui::StyleColorsDark();
    
    // Make the UI scale appropriately
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    
    // Create ImGui Vulkan implementation
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_renderer->getInstance();
    initInfo.PhysicalDevice = m_renderer->getPhysicalDevice();
    initInfo.Device = m_renderer->getDevice();
    initInfo.QueueFamily = m_renderer->getGraphicsQueueFamily();
    initInfo.Queue = m_renderer->getGraphicsQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    
    ImGui_ImplVulkan_Init(&initInfo, m_renderer->getRenderPass());
    
    // Upload fonts
    VkCommandBuffer commandBuffer = m_renderer->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    m_renderer->endSingleTimeCommands(commandBuffer);
    
    // Clear font textures from CPU memory
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImGuiManager::~ImGuiManager() {
    // Cleanup ImGui
    vkDeviceWaitIdle(m_renderer->getDevice());
    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Destroy descriptor pool
    vkDestroyDescriptorPool(m_renderer->getDevice(), m_descriptorPool, nullptr);
}

void ImGuiManager::beginFrame() {
    // Start a new ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::endFrame() {
    // Render ImGui
    ImGui::Render();
    
    // Record ImGui draw commands to the current command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_renderer->getCurrentCommandBuffer());
}
