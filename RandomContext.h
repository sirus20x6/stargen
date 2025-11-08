#ifndef RANDOM_CONTEXT_H
#define RANDOM_CONTEXT_H

#include <random>

/**
 * @brief Modern thread-safe random number generator context
 *
 * This class replaces the old C-style rand()/srand() with C++11's <random> library.
 * Uses Mersenne Twister (std::mt19937) for high-quality random numbers.
 * Each instance maintains its own RNG state, enabling multiple independent
 * random streams and thread-safe simulation.
 */
class RandomContext {
public:
    // Legacy seed values kept for compatibility
    long seed = 0;
    long jseed = 0;  // Legacy - kept for compatibility
    long ifrst = 0;  // Legacy - kept for compatibility
    long nextn = 0;  // Legacy - kept for compatibility

    RandomContext() : rng(std::random_device{}()) {
        seed = rng();
    }

    explicit RandomContext(long initial_seed) : seed(initial_seed), rng(initial_seed) {}

    // Initialize with a specific seed
    void setSeed(long new_seed) {
        seed = new_seed;
        rng.seed(new_seed);
        jseed = 0;
        ifrst = 0;
        nextn = 0;
    }

    // Get current seed value
    long getSeed() const { return seed; }

    // Generate a random integer in range [0, max)
    int randInt(int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(rng);
    }

    // Generate a random long double in range [min, max)
    long double randDouble(long double min, long double max) {
        std::uniform_real_distribution<long double> dist(min, max);
        return dist(rng);
    }

    // Generate a random integer in range [min, max]
    int randIntInclusive(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }

    // Direct access to RNG for advanced use
    std::mt19937& getRng() { return rng; }

private:
    std::mt19937 rng;  // Mersenne Twister 19937 generator
};

#endif // RANDOM_CONTEXT_H
