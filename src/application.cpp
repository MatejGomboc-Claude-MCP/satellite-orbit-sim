#include "application.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

// Callback for GLFW errors
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Mouse scroll callback for zooming
static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Retrieve the Application instance from user pointer
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    // Actual implementation is in the Application class
}

Application::Application() 
    : m_running(true), m_timeMultiplier(1.0f), m_lastFrameTime(0.0),
      m_cameraPosition(0.0f, 0.0f, 15.0f), m_cameraTarget(0.0f, 0.0f, 0.0f),
      m_cameraDistance(15.0f), m_cameraYaw(0.0f), m_cameraPitch(0.0f),
      m_mousePressed(false), m_lastMouseX(0.0), m_lastMouseY(0.0) {
    
    // Initialize GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    // GLFW window hints for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Create window
    m_window = glfwCreateWindow(1280, 720, "Satellite Orbit Simulator", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    
    // Store this pointer for callbacks
    glfwSetWindowUserPointer(m_window, this);
    
    // Initialize renderer first (sets up Vulkan)
    m_renderer = std::make_unique<Renderer>(m_window);
    
    // Initialize ImGui after renderer
    m_uiManager = std::make_unique<ImGuiManager>(m_window, m_renderer.get());
    
    // Initialize orbital mechanics
    m_orbitalMechanics = std::make_unique<OrbitalMechanics>();
    
    // Set initial time
    m_lastFrameTime = glfwGetTime();
}

Application::~Application() {
    // Cleanup in reverse order of initialization
    m_orbitalMechanics.reset();
    m_uiManager.reset();
    m_renderer.reset();
    
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

void Application::run() {
    while (m_running && !glfwWindowShouldClose(m_window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;
        
        // Limit delta time to avoid large jumps
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }
        
        // Process input events
        processInput();
        
        // Update simulation
        update(deltaTime * m_timeMultiplier);
        
        // Render frame
        render();
        
        // Poll for events
        glfwPollEvents();
    }
}

void Application::processInput() {
    // Handle keyboard input
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_running = false;
    }
    
    // Handle mouse input for camera control
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!m_mousePressed) {
            // First press
            m_mousePressed = true;
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        } else {
            // Mouse movement
            double deltaX = mouseX - m_lastMouseX;
            double deltaY = mouseY - m_lastMouseY;
            
            // Update camera yaw and pitch
            m_cameraYaw += static_cast<float>(deltaX) * 0.1f;
            m_cameraPitch -= static_cast<float>(deltaY) * 0.1f;
            
            // Clamp pitch to avoid flipping
            if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
            if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;
            
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
    } else {
        m_mousePressed = false;
    }
    
    // Handle mouse wheel for zoom (implemented via callback)
    
    // Update camera based on input
    updateCamera();
}

void Application::update(float deltaTime) {
    // Update orbital mechanics
    m_orbitalMechanics->update(deltaTime);
}

void Application::render() {
    // Begin frame
    if (!m_renderer->beginFrame()) {
        return; // Frame was skipped (e.g., window minimized)
    }
    
    // Get satellite position from orbital mechanics
    glm::vec3 satellitePosition = m_orbitalMechanics->getSatellitePosition();
    
    // Draw the Earth
    m_renderer->drawEarth();
    
    // Draw the satellite
    m_renderer->drawSatellite(satellitePosition);
    
    // Render ImGui UI
    m_uiManager->beginFrame();
    
    // Render UI elements
    ImGui::Begin("Simulation Controls");
    
    // Time controls
    ImGui::Text("Time Controls");
    ImGui::SliderFloat("Time Multiplier", &m_timeMultiplier, 0.1f, 100.0f, "%.1fx");
    if (ImGui::Button("Reset Time")) {
        m_timeMultiplier = 1.0f;
    }
    
    // Camera controls
    ImGui::Separator();
    ImGui::Text("Camera Controls");
    ImGui::SliderFloat("Distance", &m_cameraDistance, 5.0f, 50.0f);
    ImGui::SliderFloat("Yaw", &m_cameraYaw, -180.0f, 180.0f);
    ImGui::SliderFloat("Pitch", &m_cameraPitch, -89.0f, 89.0f);
    if (ImGui::Button("Reset Camera")) {
        m_cameraDistance = 15.0f;
        m_cameraYaw = 0.0f;
        m_cameraPitch = 0.0f;
        updateCamera();
    }
    
    // Orbit parameters
    ImGui::Separator();
    ImGui::Text("Orbit Parameters");
    float semimajorAxis = m_orbitalMechanics->getSemimajorAxis();
    float eccentricity = m_orbitalMechanics->getEccentricity();
    float inclination = m_orbitalMechanics->getInclination();
    
    if (ImGui::SliderFloat("Semi-major Axis", &semimajorAxis, 8.0f, 20.0f, "%.1f")) {
        m_orbitalMechanics->setSemimajorAxis(semimajorAxis);
    }
    
    if (ImGui::SliderFloat("Eccentricity", &eccentricity, 0.0f, 0.9f, "%.2f")) {
        m_orbitalMechanics->setEccentricity(eccentricity);
    }
    
    if (ImGui::SliderFloat("Inclination", &inclination, 0.0f, 90.0f, "%.1f deg")) {
        m_orbitalMechanics->setInclination(inclination);
    }
    
    // Orbital information
    ImGui::Separator();
    ImGui::Text("Orbital Information");
    ImGui::Text("Period: %.2f seconds", m_orbitalMechanics->getPeriod());
    ImGui::Text("Current Position: (%.2f, %.2f, %.2f)",
               satellitePosition.x, satellitePosition.y, satellitePosition.z);
    
    ImGui::End();
    
    m_uiManager->endFrame();
    
    // End frame
    m_renderer->endFrame();
}

void Application::updateCamera() {
    // Convert spherical coordinates to Cartesian for camera position
    float yawRad = glm::radians(m_cameraYaw);
    float pitchRad = glm::radians(m_cameraPitch);
    
    m_cameraPosition.x = m_cameraDistance * cos(pitchRad) * cos(yawRad);
    m_cameraPosition.y = m_cameraDistance * sin(pitchRad);
    m_cameraPosition.z = m_cameraDistance * cos(pitchRad) * sin(yawRad);
    
    // Update the renderer's view matrix
    m_renderer->updateCamera(m_cameraPosition, m_cameraTarget);
}
