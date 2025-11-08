#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include "structs.h"

/**
 * @brief Simulation context to hold runtime state and statistics
 *
 * This class replaces the global state variables previously in stargen.h.
 * It maintains statistics about generated systems and runtime state that
 * changes during simulation execution.
 */
class SimulationContext {
public:
    // Clone of the current sun (for context sharing)
    sun current_sun;

    // Planet generation state
    planet* innermost_planet = nullptr;  // DEPRECATED: Use planets vector instead
    std::vector<planet*> planets;  // Vector of planets (replacing linked list)
    std::vector<planet*> moons_cache; // Temporary cache for moon processing
    long double dust_density_coeff = 0.0;  // Dust density coefficient for accretion
    long current_system_seed = 0;  // Seed for current system being generated

    // Global/cross-system statistics counters
    int total_earthlike = 0;
    int total_habitable = 0;
    int total_habitable_earthlike = 0;
    int total_habitable_conservative = 0;
    int total_habitable_optimistic = 0;
    int total_potentially_habitable = 0;
    int total_potentially_habitable_earthlike = 0;
    int total_potentially_habitable_conservative = 0;
    int total_potentially_habitable_optimistic = 0;
    int total_worlds = 0;

    // Per-system statistics (reset for each system)
    int system_earthlike = 0;
    int system_habitable = 0;
    int system_habitable_jovians = 0;
    int system_habitable_superterrans = 0;
    int system_potentially_habitable = 0;
    long double system_max_moon_mass = 0.0;

    // Type diversity tracking (for experimental features)
    int type_counts[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int type_count = 0;
    int max_type_count = 0;

    // Breathable planet statistics (min/max values)
    // Min values initialized high so first real value will be less
    long double min_breathable_terrestrial_g = 1000.0;
    long double min_breathable_g = 1000.0;
    long double max_breathable_terrestrial_g = 0.0;
    long double max_breathable_g = 0.0;
    long double min_breathable_terrestrial_l = 1000.0;
    long double min_breathable_l = 1000.0;
    long double max_breathable_terrestrial_l = 0.0;
    long double max_breathable_l = 0.0;
    long double min_breathable_temp = 1000.0;
    long double max_breathable_temp = 0.0;
    long double min_breathable_p = 100000.0;
    long double max_breathable_p = 0.0;
    long double min_breathable_mass = 0.0;
    long double max_breathable_mass = 0.0;

    // Potentially habitable planet statistics (min/max values)
    // Min values initialized high so first real value will be less
    long double min_potential_terrestrial_g = 1000.0;
    long double min_potential_g = 1000.0;
    long double max_potential_terrestrial_g = 0.0;
    long double max_potential_g = 0.0;
    long double min_potential_terrestrial_l = 1000.0;
    long double min_potential_l = 1000.0;
    long double max_potential_terrestrial_l = 0.0;
    long double max_potential_l = 0.0;
    long double min_potential_temp = 1000.0;
    long double max_potential_temp = 0.0;
    long double min_potential_p = 100000.0;
    long double max_potential_p = 0.0;
    long double min_potential_mass = 0.0;
    long double max_potential_mass = 0.0;

    SimulationContext() = default;

    // Helper methods for statistics
    void recordEarthlike() {
        total_earthlike++;
        system_earthlike++;
    }

    void recordHabitable() {
        total_habitable++;
        system_habitable++;
    }

    void recordWorld() {
        total_worlds++;
    }

    void resetGlobalStatistics() {
        total_earthlike = 0;
        total_habitable = 0;
        total_habitable_earthlike = 0;
        total_habitable_conservative = 0;
        total_habitable_optimistic = 0;
        total_potentially_habitable = 0;
        total_potentially_habitable_earthlike = 0;
        total_potentially_habitable_conservative = 0;
        total_potentially_habitable_optimistic = 0;
        total_worlds = 0;
    }

    void resetSystemStatistics() {
        system_earthlike = 0;
        system_habitable = 0;
        system_habitable_jovians = 0;
        system_habitable_superterrans = 0;
        system_potentially_habitable = 0;
        system_max_moon_mass = 0.0;

        // Reset type diversity tracking
        for (int i = 0; i < 16; i++) {
            type_counts[i] = 0;
        }
        type_count = 0;
        max_type_count = 0;
    }

    // Backward compatibility alias
    void resetStatistics() {
        resetGlobalStatistics();
    }

    // Planet vector management
    void buildPlanetVector() {
        planets.clear();
        for (planet* p = innermost_planet; p != nullptr; p = p->next_planet) {
            planets.push_back(p);
        }
    }

    void clearPlanets() {
        planets.clear();
        innermost_planet = nullptr;
    }

    // Get planet count
    size_t getPlanetCount() const {
        return planets.size();
    }

    void updateBreathableStats(long double g, long double l, long double temp,
                               long double p, long double mass, bool is_terrestrial) {
        // Update min/max for all breathable planets
        if (min_breathable_g == 0.0 || g < min_breathable_g) min_breathable_g = g;
        if (g > max_breathable_g) max_breathable_g = g;
        if (min_breathable_l == 0.0 || l < min_breathable_l) min_breathable_l = l;
        if (l > max_breathable_l) max_breathable_l = l;
        if (min_breathable_temp == 0.0 || temp < min_breathable_temp) min_breathable_temp = temp;
        if (temp > max_breathable_temp) max_breathable_temp = temp;
        if (min_breathable_p == 0.0 || p < min_breathable_p) min_breathable_p = p;
        if (p > max_breathable_p) max_breathable_p = p;
        if (min_breathable_mass == 0.0 || mass < min_breathable_mass) min_breathable_mass = mass;
        if (mass > max_breathable_mass) max_breathable_mass = mass;

        // Update terrestrial-specific stats
        if (is_terrestrial) {
            if (min_breathable_terrestrial_g == 0.0 || g < min_breathable_terrestrial_g)
                min_breathable_terrestrial_g = g;
            if (g > max_breathable_terrestrial_g) max_breathable_terrestrial_g = g;
            if (min_breathable_terrestrial_l == 0.0 || l < min_breathable_terrestrial_l)
                min_breathable_terrestrial_l = l;
            if (l > max_breathable_terrestrial_l) max_breathable_terrestrial_l = l;
        }
    }

    void updatePotentialStats(long double g, long double l, long double temp,
                             long double p, long double mass, bool is_terrestrial) {
        // Update min/max for all potentially habitable planets
        if (min_potential_g == 0.0 || g < min_potential_g) min_potential_g = g;
        if (g > max_potential_g) max_potential_g = g;
        if (min_potential_l == 0.0 || l < min_potential_l) min_potential_l = l;
        if (l > max_potential_l) max_potential_l = l;
        if (min_potential_temp == 0.0 || temp < min_potential_temp) min_potential_temp = temp;
        if (temp > max_potential_temp) max_potential_temp = temp;
        if (min_potential_p == 0.0 || p < min_potential_p) min_potential_p = p;
        if (p > max_potential_p) max_potential_p = p;
        if (min_potential_mass == 0.0 || mass < min_potential_mass) min_potential_mass = mass;
        if (mass > max_potential_mass) max_potential_mass = mass;

        // Update terrestrial-specific stats
        if (is_terrestrial) {
            if (min_potential_terrestrial_g == 0.0 || g < min_potential_terrestrial_g)
                min_potential_terrestrial_g = g;
            if (g > max_potential_terrestrial_g) max_potential_terrestrial_g = g;
            if (min_potential_terrestrial_l == 0.0 || l < min_potential_terrestrial_l)
                min_potential_terrestrial_l = l;
            if (l > max_potential_terrestrial_l) max_potential_terrestrial_l = l;
        }
    }
};

#endif // SIMULATION_CONTEXT_H
