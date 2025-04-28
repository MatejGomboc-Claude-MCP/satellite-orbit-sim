#include "orbit/orbital_mechanics.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

OrbitalMechanics::OrbitalMechanics()
    : m_semimajorAxis(12.0f),  // Default semi-major axis
      m_eccentricity(0.3f),    // Default eccentricity
      m_inclination(30.0f),    // Default inclination in degrees
      m_meanAnomaly(0.0f) {    // Start at periapsis
    
    // Calculate orbital period based on initial parameters
    m_period = calculatePeriod();
}

OrbitalMechanics::~OrbitalMechanics() {
    // Nothing to clean up
}

void OrbitalMechanics::update(float deltaTime) {
    // Calculate mean motion (angular velocity)
    float meanMotion = 2.0f * glm::pi<float>() / m_period;
    
    // Update mean anomaly based on time increment
    m_meanAnomaly += meanMotion * deltaTime;
    
    // Keep mean anomaly in the range [0, 2π]
    while (m_meanAnomaly > 2.0f * glm::pi<float>()) {
        m_meanAnomaly -= 2.0f * glm::pi<float>();
    }
}

glm::vec3 OrbitalMechanics::getSatellitePosition() const {
    // Convert mean anomaly to eccentric anomaly using Kepler's equation
    float eccentricAnomaly = calculateEccentricAnomaly(m_meanAnomaly);
    
    // Calculate position in orbital plane and apply inclination
    return calculatePosition(eccentricAnomaly);
}

void OrbitalMechanics::setSemimajorAxis(float value) {
    m_semimajorAxis = value;
    m_period = calculatePeriod(); // Recalculate period when changing semi-major axis
}

void OrbitalMechanics::setEccentricity(float value) {
    m_eccentricity = value;
}

void OrbitalMechanics::setInclination(float value) {
    m_inclination = value;
}

float OrbitalMechanics::calculateEccentricAnomaly(float meanAnomaly) const {
    // Solve Kepler's equation: M = E - e * sin(E)
    // Using Newton-Raphson method for numerical approximation
    
    const float e = m_eccentricity;
    float E = meanAnomaly;  // Initial guess
    
    // Iterate until convergence (typically 5-10 iterations are needed)
    constexpr int MAX_ITERATIONS = 10;
    constexpr float CONVERGENCE_THRESHOLD = 1e-6f;
    
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        float funcValue = E - e * std::sin(E) - meanAnomaly;
        float funcDerivative = 1.0f - e * std::cos(E);
        
        float correction = funcValue / funcDerivative;
        E -= correction;
        
        // Check for convergence
        if (std::abs(correction) < CONVERGENCE_THRESHOLD) {
            break;
        }
    }
    
    return E;
}

float OrbitalMechanics::calculatePeriod() const {
    // Calculate orbital period using Kepler's third law: T² = (4π²/μ) * a³
    return 2.0f * glm::pi<float>() * std::sqrt(
        (m_semimajorAxis * m_semimajorAxis * m_semimajorAxis) / m_earthMu
    );
}

glm::vec3 OrbitalMechanics::calculatePosition(float eccentricAnomaly) const {
    // Calculate position in orbital plane (perifocal coordinates)
    float cosE = std::cos(eccentricAnomaly);
    float sinE = std::sin(eccentricAnomaly);
    
    // Distance from focus (Earth) to satellite
    float distance = m_semimajorAxis * (1.0f - m_eccentricity * cosE);
    
    // Position in orbital plane (2D)
    // X-axis points toward periapsis
    float x = distance * std::cos(eccentricAnomaly);
    // Y-axis is perpendicular to X in orbital plane
    float y = distance * std::sin(eccentricAnomaly) * std::sqrt(1.0f - m_eccentricity * m_eccentricity);
    
    // Apply inclination rotation to get 3D coordinates
    float inclinationRad = glm::radians(m_inclination);
    float z = y * std::sin(inclinationRad);
    y = y * std::cos(inclinationRad);
    
    return glm::vec3(x, y, z);
}
