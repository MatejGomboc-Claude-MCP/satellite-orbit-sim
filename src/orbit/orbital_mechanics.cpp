#include "orbit/orbital_mechanics.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

OrbitalMechanics::OrbitalMechanics()
    : m_semimajorAxis(12.0f),         // Default semi-major axis
      m_eccentricity(0.3f),           // Default eccentricity
      m_inclination(30.0f),           // Default inclination in degrees
      m_argumentOfPeriapsis(0.0f),    // Default argument of periapsis
      m_longitudeOfAscendingNode(0.0f), // Default longitude of ascending node
      m_meanAnomaly(0.0f) {           // Start at periapsis
    
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
    
    // Calculate position in orbital plane
    glm::vec2 position2D = calculateOrbitalPlanePosition(eccentricAnomaly);
    
    // Transform to reference frame using orbital elements
    return transformToReferenceFrame(position2D);
}

void OrbitalMechanics::setSemimajorAxis(float value) {
    m_semimajorAxis = value;
    m_period = calculatePeriod(); // Recalculate period when changing semi-major axis
}

void OrbitalMechanics::setEccentricity(float value) {
    // Clamp eccentricity to valid range [0, 1)
    m_eccentricity = std::max(0.0f, std::min(0.99f, value));
}

void OrbitalMechanics::setInclination(float value) {
    m_inclination = value;
}

void OrbitalMechanics::setArgumentOfPeriapsis(float value) {
    m_argumentOfPeriapsis = value;
    
    // Normalize to [0, 360)
    while (m_argumentOfPeriapsis >= 360.0f) m_argumentOfPeriapsis -= 360.0f;
    while (m_argumentOfPeriapsis < 0.0f) m_argumentOfPeriapsis += 360.0f;
}

void OrbitalMechanics::setLongitudeOfAscendingNode(float value) {
    m_longitudeOfAscendingNode = value;
    
    // Normalize to [0, 360)
    while (m_longitudeOfAscendingNode >= 360.0f) m_longitudeOfAscendingNode -= 360.0f;
    while (m_longitudeOfAscendingNode < 0.0f) m_longitudeOfAscendingNode += 360.0f;
}

float OrbitalMechanics::calculateEccentricAnomaly(float meanAnomaly) const {
    // Solve Kepler's equation: M = E - e * sin(E)
    // Using Newton-Raphson method for numerical approximation
    
    const float e = m_eccentricity;
    float E = meanAnomaly;  // Initial guess
    
    // Increase iterations for higher accuracy, especially with higher eccentricity
    constexpr int MAX_ITERATIONS = 20;  // Increased from 10
    constexpr float CONVERGENCE_THRESHOLD = 1e-8f;  // Tighter tolerance
    
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

glm::vec2 OrbitalMechanics::calculateOrbitalPlanePosition(float eccentricAnomaly) const {
    float cosE = std::cos(eccentricAnomaly);
    float sinE = std::sin(eccentricAnomaly);
    
    // Calculate distance from focus (Earth) to satellite
    float distance = m_semimajorAxis * (1.0f - m_eccentricity * cosE);
    
    // Calculate true anomaly (angle from periapsis to current position)
    float trueAnomaly = 2.0f * std::atan2(
        std::sqrt(1.0f + m_eccentricity) * std::sin(eccentricAnomaly / 2.0f),
        std::sqrt(1.0f - m_eccentricity) * std::cos(eccentricAnomaly / 2.0f)
    );
    
    // Convert to position in orbital plane
    // X-axis is towards periapsis, Y-axis is 90 degrees counter-clockwise in orbital plane
    float cosTheta = std::cos(trueAnomaly);
    float sinTheta = std::sin(trueAnomaly);
    
    return glm::vec2(
        distance * cosTheta,
        distance * sinTheta
    );
}

glm::vec3 OrbitalMechanics::transformToReferenceFrame(const glm::vec2& position) const {
    // Convert degrees to radians for rotations
    float incRad = glm::radians(m_inclination);
    float argPeriRad = glm::radians(m_argumentOfPeriapsis);
    float lonAscNodeRad = glm::radians(m_longitudeOfAscendingNode);
    
    // First, create the position in the orbital plane (x, y, 0)
    glm::vec3 pos(position.x, position.y, 0.0f);
    
    // Apply 3D rotations in the correct order
    
    // 1. Rotate around Z-axis by argument of periapsis
    glm::mat3 rotArgPeri = glm::mat3(
        std::cos(argPeriRad), -std::sin(argPeriRad), 0.0f,
        std::sin(argPeriRad),  std::cos(argPeriRad), 0.0f,
        0.0f,                  0.0f,                 1.0f
    );
    
    // 2. Rotate around X-axis by inclination
    glm::mat3 rotInc = glm::mat3(
        1.0f,  0.0f,                0.0f,
        0.0f,  std::cos(incRad),   -std::sin(incRad),
        0.0f,  std::sin(incRad),    std::cos(incRad)
    );
    
    // 3. Rotate around Z-axis by longitude of ascending node
    glm::mat3 rotLAN = glm::mat3(
        std::cos(lonAscNodeRad), -std::sin(lonAscNodeRad), 0.0f,
        std::sin(lonAscNodeRad),  std::cos(lonAscNodeRad), 0.0f,
        0.0f,                     0.0f,                    1.0f
    );
    
    // Apply rotations in sequence
    // First around Z by arg of periapsis
    // Then around X by inclination
    // Finally around Z by longitude of ascending node
    return rotLAN * rotInc * rotArgPeri * pos;
}
