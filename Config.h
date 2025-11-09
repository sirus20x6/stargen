#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
 * @brief Configuration class to hold all simulation configuration parameters
 *
 * This class replaces the global configuration variables previously scattered
 * throughout stargen.h and other files. It provides a centralized, thread-safe
 * way to manage simulation configuration.
 */
class Config {
public:
    // Verbosity and debugging
    int verbose_level = 0;
    int decimals = 0;

    // Star configuration
    long double stellar_mass = 0.0;           // Fraction of Sun's mass
    long double stellar_luminosity = 0.0;     // Fraction of Sun's luminosity
    long double stellar_temperature = 0.0;    // Temperature of star in Kelvin
    std::string stellar_type = "";            // Spectral type of star

    // Age constraints
    long double min_age = 0.0;                // Minimum stellar age (years)
    long double max_age = 6.0E9;              // Maximum stellar age (years)
    long double max_age_backup = 6.0E9;       // Backup value for max age

    // Companion star configuration (for binary systems)
    bool is_circumbinary = false;
    long double companion_mass = 0.0;
    long double companion_luminosity = 0.0;
    long double companion_temperature = 0.0;
    long double companion_eccentricity = 0.0;
    long double companion_distance = 0.0;
    std::string companion_spectral_type = "";

    // Accretion parameters
    long double dust_density_ratio = 0.0;     // Dust density coefficient
    long double eccentricity_coeff = 0.077;   // Dole's eccentricity coefficient
    long double inner_planet_factor = 0.3;    // Inner dust boundary factor

    // Simulation options
    bool do_gases = false;                    // Calculate atmospheric gases
    bool do_moons = false;                    // Generate moons
    bool do_migration = false;                // Allow planet migration
    bool use_known_planets = false;           // Use known planets as seeds
    bool only_generate_known = false;         // Only generate known planets
    bool use_solar_system = false;            // Use Solar System masses/orbits
    bool reuse_solar_system = false;          // Reuse Solar System varying Earth

    // Filtering options
    bool filter_habitable = false;            // Only save habitable systems
    bool filter_earthlike = false;            // Only save earthlike systems
    bool filter_multi_habitable = false;      // Only save multi-habitable systems
    bool filter_jovian_habitable = false;     // Only save Jovian-habitable systems
    bool filter_potentially_habitable = false;// Only save potentially habitable

    // Distance constraints
    long double max_distance = 0.0;           // Maximum distance constraint

    // Random seed
    long random_seed = 0;
    int seed_increment = 1;
    int system_count = 1;
    int num_threads = 1;  // Number of threads for parallel generation (1 = sequential)

    // Flags (bitfield for backward compatibility)
    int flags = 0;

    // Output configuration
    int output_format = 0;                    // ffHTML, ffTEXT, etc.
    int graphic_format = 0;                   // gfGIF, gfSVG
    std::string output_path = "html";
    std::string url_path = "";
    std::string filename = "";
    bool use_stdout = false;

    Config() = default;

    // Helper methods
    bool isVerbose(int level = 0x0001) const {
        return (verbose_level & level) != 0;
    }

    void setFlag(int flag, bool value) {
        if (value) {
            flags |= flag;
        } else {
            flags &= ~flag;
        }
    }

    bool hasFlag(int flag) const {
        return (flags & flag) != 0;
    }
};

#endif // CONFIG_H
