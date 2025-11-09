#include "StarGenerator.h"
#include "const.h"

StarGenerator::StarGenerator() {
    initializeDefaults();
}

StarGenerator::StarGenerator(const Config& cfg) : config(cfg) {
    initializeDefaults();
}

void StarGenerator::initializeDefaults() {
    // Initialize Config defaults
    config.dust_density_ratio = DUST_DENSITY_COEFF;
    if (config.max_age == 0.0) {
        config.max_age = 10.0E9;
    }
    if (config.max_age_backup == 0.0) {
        config.max_age_backup = 10.0E9;
    }

    // Initialize SimulationContext defaults
    sim_context.dust_density_coeff = DUST_DENSITY_COEFF;
}

void StarGenerator::setRandomSeed(long seed) {
    config.random_seed = seed;
    random_context.setSeed(seed);
    sim_context.current_system_seed = seed;
}

long StarGenerator::getRandomSeed() const {
    return random_context.getSeed();
}

void StarGenerator::resetAllStatistics() {
    sim_context.resetGlobalStatistics();
    sim_context.resetSystemStatistics();
}

void StarGenerator::resetSystemStatistics() {
    sim_context.resetSystemStatistics();
}

void StarGenerator::reset() {
    // Reset system-specific state
    sim_context.resetSystemStatistics();
    sim_context.clearPlanets();
    sim_context.system_counter = 0;
    sim_context.current_system_seed = 0;

    // Config remains unchanged - set once per pooled object
    // Global statistics remain unchanged - they accumulate across systems
}
