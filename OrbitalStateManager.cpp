#include "OrbitalStateManager.h"

/**
 * @brief Construct manager with optional initial time
 */
OrbitalStateManager::OrbitalStateManager(double initial_time)
    : current_time_(initial_time)
    , time_scale_(1.0)
    , paused_(false)
{
}

/**
 * @brief Add a stellar system to be managed
 *
 * Creates an OrbitalSimulator for the system and initializes it.
 *
 * @param system_id Unique identifier for this system
 * @param root_planet Innermost planet in the system's linked list
 * @return true if added successfully, false if system_id already exists
 */
bool OrbitalStateManager::addSystem(const SystemID& system_id, planet* root_planet) {
    // Check if system already exists
    if (simulators_.find(system_id) != simulators_.end()) {
        return false;
    }

    // Create new simulator
    auto simulator = std::make_unique<OrbitalSimulator>();

    // Initialize with the planet system
    simulator->initializeSystem(root_planet);

    // Set to current time and time scale
    simulator->setTime(current_time_);
    simulator->setTimeScale(time_scale_);

    // Add to map
    simulators_[system_id] = std::move(simulator);

    return true;
}

/**
 * @brief Remove a system from management
 *
 * @param system_id System to remove
 * @return true if removed, false if not found
 */
bool OrbitalStateManager::removeSystem(const SystemID& system_id) {
    auto it = simulators_.find(system_id);
    if (it == simulators_.end()) {
        return false;
    }

    simulators_.erase(it);
    return true;
}

/**
 * @brief Check if a system is being managed
 */
bool OrbitalStateManager::hasSystem(const SystemID& system_id) const {
    return simulators_.find(system_id) != simulators_.end();
}

/**
 * @brief Get simulator for a specific system (for advanced use)
 *
 * @return Pointer to simulator, or nullptr if not found
 */
OrbitalSimulator* OrbitalStateManager::getSimulator(const SystemID& system_id) {
    auto it = simulators_.find(system_id);
    if (it == simulators_.end()) {
        return nullptr;
    }
    return it->second.get();
}

/**
 * @brief Update all systems by delta time
 *
 * Advances simulation time and recalculates positions for all managed systems.
 * Respects paused state and time scale.
 *
 * @param delta_time Time step in years (e.g., 0.001 = ~8.7 hours)
 */
void OrbitalStateManager::update(double delta_time) {
    if (paused_) {
        return;
    }

    // Apply time scale
    double scaled_delta = delta_time * time_scale_;

    // Update global time
    current_time_ += scaled_delta;

    // Update all systems
    for (auto& [system_id, simulator] : simulators_) {
        simulator->advance(scaled_delta);
    }
}

/**
 * @brief Set absolute simulation time for all systems
 *
 * @param time New simulation time in years
 */
void OrbitalStateManager::setTime(double time) {
    current_time_ = time;

    // Propagate to all simulators
    for (auto& [system_id, simulator] : simulators_) {
        simulator->setTime(time);
    }
}

/**
 * @brief Set time scale multiplier for all systems
 *
 * @param scale Speed multiplier (1.0 = real-time, 365.25 = 1 year/second)
 */
void OrbitalStateManager::setTimeScale(double scale) {
    time_scale_ = scale;

    // Propagate to all simulators
    for (auto& [system_id, simulator] : simulators_) {
        simulator->setTimeScale(scale);
    }
}

/**
 * @brief Get 3D position of a planet in a system
 *
 * @param system_id System containing the planet
 * @param p Planet to query
 * @return Position in AU (sun at origin), or (0,0,0) if not found
 */
Vector3 OrbitalStateManager::getPosition(const SystemID& system_id, planet* p) const {
    auto it = simulators_.find(system_id);
    if (it == simulators_.end()) {
        return Vector3(0, 0, 0);
    }

    return it->second->getPosition(p);
}

/**
 * @brief Get 3D velocity of a planet in a system
 *
 * @param system_id System containing the planet
 * @param p Planet to query
 * @return Velocity in AU/year, or (0,0,0) if not found
 */
Vector3 OrbitalStateManager::getVelocity(const SystemID& system_id, planet* p) const {
    auto it = simulators_.find(system_id);
    if (it == simulators_.end()) {
        return Vector3(0, 0, 0);
    }

    return it->second->getVelocity(p);
}

/**
 * @brief Clear all systems
 *
 * Removes all systems from management. Does not free planet data - caller
 * is responsible for planet lifecycle management.
 */
void OrbitalStateManager::clear() {
    simulators_.clear();
}
