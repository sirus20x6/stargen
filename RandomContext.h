#ifndef RANDOM_CONTEXT_H
#define RANDOM_CONTEXT_H

/**
 * @brief Random number generator context
 *
 * This class encapsulates the random number generator state that was
 * previously stored in global variables (seed, jseed, ifrst, nextn).
 * Each instance maintains its own RNG state, enabling multiple independent
 * random streams and thread-safe simulation.
 */
class RandomContext {
public:
    long seed = 0;
    long jseed = 0;
    long ifrst = 0;
    long nextn = 0;

    RandomContext() = default;

    explicit RandomContext(long initial_seed) : seed(initial_seed) {}

    // Initialize with a specific seed
    void setSeed(long new_seed) {
        seed = new_seed;
        jseed = 0;
        ifrst = 0;
        nextn = 0;
    }

    // Get current seed value
    long getSeed() const { return seed; }
};

#endif // RANDOM_CONTEXT_H
