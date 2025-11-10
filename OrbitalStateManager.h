#ifndef ORBITAL_STATE_MANAGER_H
#define ORBITAL_STATE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include "OrbitalSimulator.h"
#include "structs.h"

/**
 * @brief Manages orbital simulations for multiple stellar systems
 *
 * Provides efficient runtime position tracking and updates across multiple
 * generated systems. Designed for real-time visualization applications.
 *
 * ## Architecture
 *
 * Each system gets its own OrbitalSimulator instance. The manager:
 * - Tracks simulation time globally (synchronized across all systems)
 * - Updates all systems efficiently (batch updates, dirty tracking)
 * - Provides query interface for positions/velocities
 * - Supports system hot-swapping (add/remove without pause)
 *
 * ## Usage
 *
 * ```cpp
 * OrbitalStateManager manager;
 *
 * // Add systems
 * manager.addSystem("Sol", sol_planets);
 * manager.addSystem("Alpha Centauri", alpha_planets);
 *
 * // Game loop
 * while (running) {
 *     manager.update(delta_time);  // Update all systems
 *
 *     Vector3 earth_pos = manager.getPosition("Sol", earth_planet);
 *     // ... render ...
 * }
 * ```
 */
class OrbitalStateManager {
public:
    /**
     * @brief System handle for efficient lookups
     */
    using SystemID = std::string;

    /**
     * @brief Construct manager with optional initial time
     */
    explicit OrbitalStateManager(double initial_time = 0.0);

    /**
     * @brief Add a stellar system to be managed
     *
     * @param system_id Unique identifier for this system
     * @param root_planet Innermost planet in the system's linked list
     * @return true if added successfully, false if system_id already exists
     */
    bool addSystem(const SystemID& system_id, planet* root_planet);

    /**
     * @brief Remove a system from management
     *
     * @param system_id System to remove
     * @return true if removed, false if not found
     */
    bool removeSystem(const SystemID& system_id);

    /**
     * @brief Check if a system is being managed
     */
    bool hasSystem(const SystemID& system_id) const;

    /**
     * @brief Get simulator for a specific system (for advanced use)
     */
    OrbitalSimulator* getSimulator(const SystemID& system_id);

    /**
     * @brief Update all systems by delta time
     *
     * Advances simulation time and recalculates positions for all managed systems.
     *
     * @param delta_time Time step in years (e.g., 0.001 = ~8.7 hours)
     */
    void update(double delta_time);

    /**
     * @brief Set absolute simulation time for all systems
     */
    void setTime(double time);

    /**
     * @brief Get current simulation time
     */
    double getCurrentTime() const { return current_time_; }

    /**
     * @brief Set time scale multiplier for all systems
     *
     * @param scale Speed multiplier (1.0 = real-time, 365.25 = 1 year/second)
     */
    void setTimeScale(double scale);

    /**
     * @brief Get current time scale
     */
    double getTimeScale() const { return time_scale_; }

    /**
     * @brief Pause/unpause all simulations
     */
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }

    /**
     * @brief Get 3D position of a planet in a system
     *
     * @param system_id System containing the planet
     * @param p Planet to query
     * @return Position in AU (sun at origin), or (0,0,0) if not found
     */
    Vector3 getPosition(const SystemID& system_id, planet* p) const;

    /**
     * @brief Get 3D velocity of a planet in a system
     *
     * @param system_id System containing the planet
     * @param p Planet to query
     * @return Velocity in AU/year, or (0,0,0) if not found
     */
    Vector3 getVelocity(const SystemID& system_id, planet* p) const;

    /**
     * @brief Get number of systems being managed
     */
    size_t getSystemCount() const { return simulators_.size(); }

    /**
     * @brief Clear all systems
     */
    void clear();

private:
    // Simulator for each system
    std::unordered_map<SystemID, std::unique_ptr<OrbitalSimulator>> simulators_;

    // Global time state
    double current_time_;
    double time_scale_;
    bool paused_;
};

#endif // ORBITAL_STATE_MANAGER_H
