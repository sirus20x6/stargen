#ifndef STAR_GENERATOR_H
#define STAR_GENERATOR_H

#include "Config.h"
#include "SimulationContext.h"
#include "RandomContext.h"
#include "structs.h"

/**
 * @brief Main class for generating stellar systems
 *
 * This class encapsulates all state needed for system generation,
 * enabling multiple independent instances and thread-safe operation.
 * It replaces the old global variable architecture.
 *
 * Thread Safety:
 * - Each StarGenerator instance is NOT thread-safe internally
 * - However, multiple StarGenerator instances can run in parallel
 * - Each thread should create its own StarGenerator instance
 *
 * Usage:
 *   StarGenerator gen(config);
 *   gen.setRandomSeed(42);
 *   auto system = gen.generateSystem(star);
 */
class StarGenerator {
public:
    // Direct access to contexts (public for compatibility during migration)
    Config config;
    SimulationContext sim_context;
    RandomContext random_context;

    /**
     * @brief Default constructor with default configuration
     */
    StarGenerator();

    /**
     * @brief Constructor with custom configuration
     */
    explicit StarGenerator(const Config& cfg);

    /**
     * @brief Initialize the generator with a random seed
     */
    void setRandomSeed(long seed);

    /**
     * @brief Get the current random seed
     */
    long getRandomSeed() const;

    /**
     * @brief Reset all statistics (both global and per-system)
     */
    void resetAllStatistics();

    /**
     * @brief Reset only per-system statistics
     */
    void resetSystemStatistics();

    /**
     * @brief Reset generator state for reuse in object pool
     *
     * Clears system-specific state while preserving configuration.
     * Allows safe reuse from object pool.
     */
    void reset();

    /**
     * @brief Get reference to configuration (for legacy code)
     */
    Config& getConfig() { return config; }
    const Config& getConfig() const { return config; }

    /**
     * @brief Get reference to simulation context (for legacy code)
     */
    SimulationContext& getSimContext() { return sim_context; }
    const SimulationContext& getSimContext() const { return sim_context; }

    /**
     * @brief Get reference to random context (for legacy code)
     */
    RandomContext& getRandomContext() { return random_context; }
    const RandomContext& getRandomContext() const { return random_context; }

    // Convenience accessors for commonly used values
    int getVerboseLevel() const { return config.verbose_level; }
    void setVerboseLevel(int level) { config.verbose_level = level; }

    int getFlags() const { return config.flags; }
    void setFlags(int f) { config.flags = f; }

    sun& getCurrentSun() { return sim_context.current_sun; }
    const sun& getCurrentSun() const { return sim_context.current_sun; }

private:
    void initializeDefaults();
};

#endif // STAR_GENERATOR_H
