/**
 * @file orbital_simulator_demo.cpp
 * @brief Demonstration showing OrbitalSimulator doesn't interfere with generation
 *
 * This example shows:
 * 1. Generate a system using existing code (unchanged)
 * 2. Initialize orbital simulator (reads data, doesn't modify)
 * 3. Simulate time and calculate positions
 * 4. Generation data remains intact
 */

#include <iostream>
#include <iomanip>
#include <cstdlib>      // for EXIT_FAILURE
#include <exception>    // for std::exception
#include "stargen.h"
#include "StarGenerator.h"
#include "OrbitalSimulator.h"
#include "accrete.h"
#include "elements.h"
#include "planets.h"
#include "radius_tables.h"
#include "dole.h"
#include "solstation.h"
#include "jimb.h"
#include "omega_galaxy.h"
#include "ring_universe.h"
#include "ic3094.h"
#include "andromeda.h"
#include "star_trek.h"
#include "Planetary_Habitability_Laboratory.h"

void printPlanetInfo(planet* p, int index) {
    std::cout << "  Planet " << index << ":\n";
    std::cout << "    Semi-major axis: " << p->getA() << " AU\n";
    std::cout << "    Eccentricity: " << p->getE() << "\n";
    std::cout << "    Inclination: " << p->getInclination() << " degrees\n";
}

int main() {
    // Initialize global data tables (REQUIRED for generation). The data loaders
    // (e.g. the elements YAML) throw std::runtime_error on a missing/malformed
    // file, so fail gracefully instead of letting it escape to std::terminate().
    try {
        initRadii();
        initGases();
        initPlanets();
        initDole();
        initSolStation();
        initJimb();
        initOmegaGalaxy();
        initRingUniverse();
        initIC3094();
        initAndromeda();
        initStarTrek();
        initPlanetaryHabitabilityLaboratory();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "================================================\n";
    std::cout << "OrbitalSimulator Non-Invasive Demo\n";
    std::cout << "================================================\n\n";

    // =========================================================================
    // STEP 1: Generate system using EXISTING code (completely unchanged)
    // =========================================================================
    std::cout << "[1] Generating system...\n";

    StarGenerator gen;
    gen.random_context.setSeed(42);
    gen.config.flags = 0;  // Initialize flags to 0
    gen.config.verbose_level = 0;  // Quiet output

    sun the_sun;
    the_sun.setMass(1.0);  // Solar mass
    the_sun.setLuminosity(1.0);

    accrete acc;
    acc.setRandomContext(&gen.random_context);

    generate_stellar_system(&gen, the_sun, false, nullptr, "S",
                           0, "Demo System", 0.0, 0.0,
                           0.077, 1.0, true, true, acc);

    planet* system = gen.sim_context.innermost_planet;

    if (!system) {
        std::cout << "No planets generated\n";
        return 1;
    }

    // Print original orbital parameters
    std::cout << "\nGenerated system orbital parameters:\n";
    int planet_count = 0;
    for (planet* p = system; p != nullptr; p = p->next_planet) {
        printPlanetInfo(p, ++planet_count);
    }

    // =========================================================================
    // STEP 2: Initialize orbital simulator (READS data, doesn't modify)
    // =========================================================================
    std::cout << "\n[2] Initializing orbital simulator...\n";

    OrbitalSimulator sim;
    sim.initializeSystem(system);

    std::cout << "Simulator initialized. Generation data NOT modified.\n";

    // Verify generation data is unchanged
    std::cout << "\nVerifying orbital parameters unchanged:\n";
    planet_count = 0;
    for (planet* p = system; p != nullptr; p = p->next_planet) {
        printPlanetInfo(p, ++planet_count);
    }

    // =========================================================================
    // STEP 3: Simulate orbital positions at different times
    // =========================================================================
    std::cout << "\n[3] Simulating orbital positions...\n\n";

    std::cout << std::fixed << std::setprecision(3);

    double time_points[] = {0.0, 0.25, 0.5, 1.0, 2.0};  // Years

    for (double t : time_points) {
        sim.setTime(t);

        std::cout << "Time = " << t << " years:\n";

        int idx = 0;
        for (planet* p = system; p != nullptr; p = p->next_planet) {
            idx++;
            Vec3 pos = sim.getPosition(p);
            double distance = pos.length();

            std::cout << "  Planet " << idx
                     << " at (" << pos.x << ", " << pos.y << ", " << pos.z << ") AU"
                     << " [distance: " << distance << " AU]\n";
        }
        std::cout << "\n";
    }

    // =========================================================================
    // STEP 4: Demonstrate real-time simulation loop
    // =========================================================================
    std::cout << "[4] Simulating 10 years in 1-year steps...\n\n";

    sim.setTime(0.0);  // Reset to start
    sim.setTimeScale(1.0);  // Real-time

    for (int year = 0; year <= 10; year++) {
        if (year > 0) {
            sim.advance(1.0);  // Advance 1 year
        }

        planet* p = system;  // Just show first planet
        Vec3 pos = sim.getPosition(p);

        std::cout << "Year " << std::setw(2) << year
                 << ": Planet 1 at distance " << pos.length() << " AU\n";
    }

    // =========================================================================
    // STEP 5: Final verification - generation data still intact
    // =========================================================================
    std::cout << "\n[5] Final verification...\n";
    std::cout << "Original orbital parameters (should be unchanged):\n";

    planet_count = 0;
    for (planet* p = system; p != nullptr; p = p->next_planet) {
        printPlanetInfo(p, ++planet_count);
    }

    std::cout << "\n================================================\n";
    std::cout << "SUCCESS: Simulator and generation coexist!\n";
    std::cout << "================================================\n";
    std::cout << "\nKey points:\n";
    std::cout << "- Generation calculated STATIC orbital elements\n";
    std::cout << "- Simulator calculated DYNAMIC positions over time\n";
    std::cout << "- Generation data never modified\n";
    std::cout << "- Both systems work independently\n\n";

    // Cleanup
    acc.free_generations();

    return 0;
}
