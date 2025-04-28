#pragma once

#include <glm/glm.hpp>

/**
 * Implements Keplerian orbital mechanics for satellite motion.
 * 
 * This class calculates the position of a satellite in an elliptical orbit
 * around Earth using Kepler's equations.
 */
class OrbitalMechanics {
public:
    /**
     * Constructor initializes orbital parameters with default values.
     */
    OrbitalMechanics();
    
    /**
     * Destructor.
     */
    ~OrbitalMechanics();
    
    /**
     * Updates the orbital state based on elapsed time.
     * 
     * @param deltaTime Time increment to advance the simulation
     */
    void update(float deltaTime);
    
    /**
     * Gets the current 3D position of the satellite.
     * 
     * @return Position vector in 3D space
     */
    glm::vec3 getSatellitePosition() const;
    
    /**
     * Gets the current orbital period.
     * 
     * @return Orbital period in seconds
     */
    float getPeriod() const { return m_period; }
    
    // Getters for orbital parameters
    float getSemimajorAxis() const { return m_semimajorAxis; }
    float getEccentricity() const { return m_eccentricity; }
    float getInclination() const { return m_inclination; }
    
    // Setters for orbital parameters
    void setSemimajorAxis(float value);
    void setEccentricity(float value);
    void setInclination(float value);
    
private:
    // Earth parameters (in arbitrary units)
    const float m_earthRadius = 6.371f;  // Earth radius
    const float m_earthMu = 398600.0f;   // Earth gravitational parameter
    
    // Orbital parameters
    float m_semimajorAxis;  // Semi-major axis of the elliptical orbit
    float m_eccentricity;   // Eccentricity of the orbit
    float m_inclination;    // Inclination in degrees
    
    // Current state
    float m_meanAnomaly;    // Current mean anomaly (varies linearly with time)
    float m_period;         // Orbital period
    
    // Helper methods
    /**
     * Solves Kepler's equation to find eccentric anomaly.
     * 
     * @param meanAnomaly The mean anomaly
     * @return The eccentric anomaly
     */
    float calculateEccentricAnomaly(float meanAnomaly) const;
    
    /**
     * Calculates orbital period using Kepler's third law.
     * 
     * @return Orbital period in seconds
     */
    float calculatePeriod() const;
    
    /**
     * Converts eccentric anomaly to 3D position vector.
     * 
     * @param eccentricAnomaly The eccentric anomaly
     * @return 3D position vector
     */
    glm::vec3 calculatePosition(float eccentricAnomaly) const;
};
