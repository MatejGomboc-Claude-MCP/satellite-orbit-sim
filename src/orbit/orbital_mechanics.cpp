#include "orbit/orbital_mechanics.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

OrbitalMechanics::OrbitalMechanics()
    : m_semimajorAxis(12.0f),  // Default semi-major axis
      m_eccentricity(0.3f),    // Default eccentricity
      m_inclination(30.0f),    // Default inclination in degrees
      m_raan(0.0f),            // Default RAAN in degrees
      m_argPeriapsis(0.0f),    // Default argument of periapsis in degrees
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
    
    // Calculate position in orbital plane
    glm::vec2 positionInOrbitalPlane = calculatePositionInOrbitalPlane(eccentricAnomaly);
    
    // Transform to reference frame with inclination, RAAN, and arg of periapsis
    return transformToReferenceFrame(positionInOrbitalPlane);
}

void OrbitalMechanics::setSemimajorAxis(float value) {
    if (value <= 0.0f) return;  // Validate input
    
    m_semimajorAxis = value;
    m_period = calculatePeriod(); // Recalculate period when changing semi-major axis
}

void OrbitalMechanics::setEccentricity(float value) {
    if (value < 0.0f || value >= 1.0f) return;  // Validate input
    
    m_eccentricity = value;
}

void OrbitalMechanics::setInclination(float value) {
    // Clamp to valid range (0 to 180 degrees)
    m_inclination = std::max(0.0f, std::min(value, 180.0f));
}

void OrbitalMechanics::setRaan(float value) {
    // Normalize to 0-360 degrees
    m_raan = fmodf(value, 360.0f);
    if (m_raan < 0.0f) m_raan += 360.0f;
}

void OrbitalMechanics::setArgPeriapsis(float value) {
    // Normalize to 0-360 degrees
    m_argPeriapsis = fmodf(value, 360.0f);
    if (m_argPeriapsis < 0.0f) m_argPeriapsis += 360.0f;
}

float OrbitalMechanics::calculateEccentricAnomaly(float meanAnomaly) const {
    // Solve Kepler's equation: M = E - e * sin(E)
    // Using Newton-Raphson method for numerical approximation
    
    const float e = m_eccentricity;
    float E = meanAnomaly;  // Initial guess
    
    // Iterate until convergence (typically 5-10 iterations are needed)
    constexpr int MAX_ITERATIONS = 15;  // Increased for better convergence with high eccentricity
    constexpr float CONVERGENCE_THRESHOLD = 1e-8f;  // Tighter threshold
    
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

glm::vec2 OrbitalMechanics::calculatePositionInOrbitalPlane(float eccentricAnomaly) const {
    // Calculate position in orbital plane (perifocal coordinates)
    float cosE = std::cos(eccentricAnomaly);
    float sinE = std::sin(eccentricAnomaly);
    
    // Distance from focus (Earth) to satellite
    float distance = m_semimajorAxis * (1.0f - m_eccentricity * cosE);
    
    // Position in orbital plane (2D)
    // X-axis points toward periapsis
    float x = m_semimajorAxis * (cosE - m_eccentricity);
    // Y-axis is perpendicular to X in orbital plane
    float y = m_semimajorAxis * std::sqrt(1.0f - m_eccentricity * m_eccentricity) * sinE;
    
    return glm::vec2(x, y);
}

glm::vec3 OrbitalMechanics::transformToReferenceFrame(const glm::vec2& positionInOrbitalPlane) const {
    // Convert degrees to radians
    float inclinationRad = glm::radians(m_inclination);
    float raanRad = glm::radians(m_raan);
    float argPeriapsisRad = glm::radians(m_argPeriapsis);
    
    // Extract x, y from orbital plane
    float x = positionInOrbitalPlane.x;
    float y = positionInOrbitalPlane.y;
    
    // First, rotate by argument of periapsis in the orbital plane
    float xRotated = x * std::cos(argPeriapsisRad) - y * std::sin(argPeriapsisRad);
    float yRotated = x * std::sin(argPeriapsisRad) + y * std::cos(argPeriapsisRad);
    
    // Then, apply inclination and RAAN rotations to get 3D coordinates
    float xFinal = xRotated * std::cos(raanRad) - yRotated * std::cos(inclinationRad) * std::sin(raanRad);
    float yFinal = xRotated * std::sin(raanRad) + yRotated * std::cos(inclinationRad) * std::cos(raanRad);
    float zFinal = yRotated * std::sin(inclinationRad);
    
    return glm::vec3(xFinal, zFinal, yFinal);  // Adjust for our coordinate system
}
