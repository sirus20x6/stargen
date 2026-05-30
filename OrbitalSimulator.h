#ifndef ORBITAL_SIMULATOR_H
#define ORBITAL_SIMULATOR_H

#include <unordered_map>
#include <cmath>
#include "structs.h"
#include "Vector3.h"

/**
 * @brief Runtime state for a celestial body
 *
 * IMPORTANT: This is SEPARATE from the planet struct.
 * - planet struct = static orbital parameters (generated once)
 * - OrbitalState = dynamic runtime position (calculated each frame)
 */
struct OrbitalState {
    Vector3 position;           // Current 3D position (AU)
    Vector3 velocity;           // Current 3D velocity (AU/year)
    double mean_anomaly;        // Current mean anomaly (radians)
    double last_update_time;    // Last simulation time (years)

    OrbitalState()
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , mean_anomaly(0)
        , last_update_time(0) {}
};

/**
 * @brief Real-time orbital mechanics simulator
 *
 * ## NON-INVASIVE DESIGN
 *
 * This class does NOT modify planet generation:
 * - Reads orbital parameters from planet structs (const access)
 * - Stores runtime positions in separate OrbitalState structures
 * - Can be disabled/removed without affecting generation
 *
 * ## Usage
 *
 * After generating a system:
 *   planet* system = generate_stellar_system(...);  // ← Existing code
 *
 *   OrbitalSimulator sim;                           // ← New code
 *   sim.initializeSystem(system);                   // ← Read parameters
 *
 *   while (running) {
 *       sim.advance(delta_time);                    // ← Calculate positions
 *       Vector3 pos = sim.getPosition(planet);      // ← Get current position
 *       renderer.drawPlanet(planet, pos);           // ← Render
 *   }
 *
 * ## Physics
 *
 * Uses Kepler's laws to calculate positions from orbital elements:
 * - Semi-major axis (a) - from planet->getA()
 * - Eccentricity (e) - from planet->getE()
 * - Inclination (i) - from planet->getInclination()
 * - Longitude of ascending node (Ω) - from planet->getAscendingNode()
 * - Argument of periapsis (ω) - from planet->getArgPerihelion()
 * - Mean anomaly at epoch (M0) - calculated from planet position
 */
class OrbitalSimulator {
public:
    /**
     * @brief Initialize simulator with a planetary system
     *
     * Reads orbital parameters from planet structs and creates runtime state.
     * Does NOT modify the planet structs.
     *
     * @param system_root Innermost planet in the system
     */
    void initializeSystem(planet* system_root);

    /**
     * @brief Advance simulation time
     *
     * @param delta_time Time step in years (e.g., 0.01 = ~3.65 days)
     */
    void advance(double delta_time);

    /**
     * @brief Update all planetary positions for current time
     *
     * Recalculates positions using Kepler's equation.
     * Call this after advance() or when jumping to a new time.
     */
    void updatePositions();

    /**
     * @brief Get current 3D position of a planet
     *
     * @param p Planet to query (non-const as planet getters aren't const)
     * @return Position in AU (sun at origin)
     */
    Vector3 getPosition(planet* p) const;

    /**
     * @brief Get current 3D velocity of a planet
     *
     * @param p Planet to query (non-const as planet getters aren't const)
     * @return Velocity in AU/year
     */
    Vector3 getVelocity(planet* p) const;

    /**
     * @brief Get current simulation time
     *
     * @return Time in years since epoch
     */
    double getCurrentTime() const { return current_time_; }

    /**
     * @brief Set simulation time directly (jump to specific time)
     *
     * @param time Time in years
     */
    void setTime(double time);

    /**
     * @brief Set time scale multiplier
     *
     * @param scale Speed multiplier (1.0 = real-time, 365.25 = 1 year per second)
     */
    void setTimeScale(double scale) { time_scale_ = scale; }

    /**
     * @brief Get time scale multiplier
     */
    double getTimeScale() const { return time_scale_; }

    /**
     * @brief Check if simulator has state for a planet
     */
    bool hasState(planet* p) const;

private:
    // Runtime state for each planet (separate from planet structs)
    std::unordered_map<planet*, OrbitalState> states_;

    // Simulation time
    double current_time_ = 0.0;      // Years since epoch
    double time_scale_ = 1.0;        // Speed multiplier

    /**
     * @brief Calculate position from orbital elements
     *
     * Converts Kepler orbital elements to 3D Cartesian coordinates.
     *
     * @param a Semi-major axis (AU)
     * @param e Eccentricity
     * @param i Inclination (radians)
     * @param omega Longitude of ascending node (radians)
     * @param w Argument of periapsis (radians)
     * @param M Mean anomaly (radians)
     * @return Position vector in 3D space
     */
    Vector3 orbitalToCartesian(double a, double e, double i,
                               double omega, double w, double M) const;

    /**
     * @brief Solve Kepler's equation iteratively
     *
     * Finds eccentric anomaly (E) from mean anomaly (M) and eccentricity (e)
     * using Newton-Raphson iteration.
     *
     * M = E - e*sin(E)  (Kepler's equation)
     *
     * @param M Mean anomaly (radians)
     * @param e Eccentricity
     * @param tolerance Convergence threshold (default: 1e-8)
     * @param max_iterations Maximum iterations (default: 50)
     * @return Eccentric anomaly (radians)
     */
    double solveKeplersEquation(double M, double e,
                                double tolerance = 1e-8,
                                int max_iterations = 50) const;

    /**
     * @brief Calculate mean anomaly at given time
     *
     * M = M0 + n*t
     * where n = mean motion = sqrt(μ/a³) = 2π·sqrt(M_star)/a^(3/2)
     *
     * @param a Semi-major axis (AU)
     * @param t Time (years)
     * @param m_star Stellar mass (solar masses); scales μ. <=0 falls back to 1.
     * @param M0 Initial mean anomaly (radians)
     * @return Current mean anomaly (radians)
     */
    double calculateMeanAnomaly(double a, double t, double m_star, double M0 = 0.0) const;
};

#endif // ORBITAL_SIMULATOR_H
