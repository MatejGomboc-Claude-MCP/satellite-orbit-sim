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
    if (app) {
        app->handleMouseScroll(yoffset);
    }
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
    
    // Set scroll callback
    glfwSetScrollCallback(m_window, scrollCallback);
    
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
    
    // Update camera based on input
    updateCamera();
}

void Application::handleMouseScroll(double yoffset) {
    // Handle mouse scroll for zoom
    m_cameraDistance -= static_cast<float>(yoffset) * 0.5f;
    
    // Clamp distance to reasonable values
    if (m_cameraDistance < 7.0f) m_cameraDistance = 7.0f;
    if (m_cameraDistance > 50.0f) m_cameraDistance = 50.0f;
    
    // Update camera position
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
    
    // Render UI elements with ImGui
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
    ImGui::SliderFloat("Distance", &m_cameraDistance, 7.0f, 50.0f);
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
    ImGui::Text("Orbital Elements");
    
    // Get current values
    float semimajorAxis = m_orbitalMechanics->getSemimajorAxis();
    float eccentricity = m_orbitalMechanics->getEccentricity();
    float inclination = m_orbitalMechanics->getInclination();
    float argOfPeriapsis = m_orbitalMechanics->getArgumentOfPeriapsis();
    float longAscNode = m_orbitalMechanics->getLongitudeOfAscendingNode();
    
    // Semi-major axis (orbit size)
    if (ImGui::SliderFloat("Semi-major Axis", &semimajorAxis, 8.0f, 20.0f, "%.1f")) {
        m_orbitalMechanics->setSemimajorAxis(semimajorAxis);
    }
    
    // Eccentricity (orbit shape)
    if (ImGui::SliderFloat("Eccentricity", &eccentricity, 0.0f, 0.9f, "%.2f")) {
        m_orbitalMechanics->setEccentricity(eccentricity);
    }
    
    // Inclination (orbit tilt)
    if (ImGui::SliderFloat("Inclination", &inclination, 0.0f, 90.0f, "%.1f deg")) {
        m_orbitalMechanics->setInclination(inclination);
    }
    
    // Argument of periapsis (orientation in orbit plane)
    if (ImGui::SliderFloat("Arg. of Periapsis", &argOfPeriapsis, 0.0f, 360.0f, "%.1f deg")) {
        m_orbitalMechanics->setArgumentOfPeriapsis(argOfPeriapsis);
    }
    
    // Longitude of ascending node (orbit plane orientation)
    if (ImGui::SliderFloat("Long. of Asc. Node", &longAscNode, 0.0f, 360.0f, "%.1f deg")) {
        m_orbitalMechanics->setLongitudeOfAscendingNode(longAscNode);
    }
    
    // Orbital information section
    ImGui::Separator();
    ImGui::Text("Orbital Information");
    ImGui::Text("Period: %.2f seconds", m_orbitalMechanics->getPeriod());
    ImGui::Text("Current Position: (%.2f, %.2f, %.2f)",
               satellitePosition.x, satellitePosition.y, satellitePosition.z);
    
    // Add help tooltips
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Satellite position in 3D space");
        ImGui::EndTooltip();
    }
    
    if (ImGui::Button("Reset Orbit")) {
        m_orbitalMechanics->setSemimajorAxis(12.0f);
        m_orbitalMechanics->setEccentricity(0.3f);
        m_orbitalMechanics->setInclination(30.0f);
        m_orbitalMechanics->setArgumentOfPeriapsis(0.0f);
        m_orbitalMechanics->setLongitudeOfAscendingNode(0.0f);
    }
    
    ImGui::End();
    
    // Render controls for help and about
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_FirstUseEver);
    ImGui::Begin("Help", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::Button("About")) {
        m_showAboutWindow = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Controls Help")) {
        m_showHelpWindow = true;
    }
    
    ImGui::End();
    
    // Show help window if needed
    if (m_showHelpWindow) {
        ImGui::Begin("Controls Help", &m_showHelpWindow);
        ImGui::Text("Mouse Controls:");
        ImGui::BulletText("Left click + drag: Rotate camera");
        ImGui::BulletText("Mouse wheel: Zoom in/out");
        ImGui::Text("\nKeyboard Controls:");
        ImGui::BulletText("ESC: Quit application");
        ImGui::End();
    }
    
    // Show about window if needed
    if (m_showAboutWindow) {
        ImGui::Begin("About", &m_showAboutWindow);
        ImGui::Text("Satellite Orbit Simulator");
        ImGui::Text("Version 1.0");
        ImGui::Separator();
        ImGui::Text("A Vulkan-based satellite orbit simulator using");
        ImGui::Text("Kepler's equations for realistic orbital mechanics.");
        ImGui::Separator();
        ImGui::Text("Built with:");
        ImGui::BulletText("C++20, Vulkan, GLFW, ImGui");
        ImGui::BulletText("HLSL shaders compiled to SPIR-V");
        ImGui::End();
    }
    
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
