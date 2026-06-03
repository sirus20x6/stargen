#ifndef ACCRETE_H
#define ACCRETE_H

#include <vector>

class dust;
class gen;
class planet;
class sun;
class RandomContext;

class accrete {

    public:
        accrete() = default;
        ~accrete(); // Destructor to clean up allocated memory

        void free_generations();
        void reset(); // Reset state for object pool reuse
        void setRandomContext(RandomContext* ctx) { random_ctx = ctx; }
        auto stellar_dust_limit(long double) -> long double;
        auto dist_planetary_masses(sun &, long double, long double, long double,
                            long double, long double, long double, planet *,
                            bool) -> planet *;

    private:
        RandomContext* random_ctx = nullptr;  // Random context for this accrete instance

    /* Now for some variables global to the accretion process:	    */
    bool dust_left;
    long double r_inner;
    long double r_outer;
    long double reduced_mass;
    long double dust_density;
    long double cloud_eccentricity;
    std::vector<dust> dust_bands;  // Changed from dust *dust_head to vector
    planet *planet_head = nullptr;
    gen *hist_head = nullptr;
    planet *seed_moons = nullptr;

    void set_initial_conditions(long double, long double);
    auto nearest_planet(long double, long double) -> long double;
    auto farthest_planet(long double) -> long double;
    auto inner_effect_limit(long double, long double, long double) -> long double;
    auto outer_effect_limit(long double, long double, long double) -> long double;
    auto dust_available(long double, long double) -> bool;
    void update_dust_lanes(long double, long double, long double, long double, long double, long double);
    auto collect_dust(long double, long double &, long double &, long double, long double, long double, size_t) -> long double;
    auto critical_limit(long double, long double, long double) -> long double;
    void accrete_dust(long double &, long double &, long double &, long double, long double, long double, long double, long double);
    void coalesce_planetesimals(long double, long double, long double, long double,
                                long double, long double, long double, long double,
                                long double, bool);

    // REMOVED: void free_dust(dust *); - No longer needed with std::vector
    void free_planet(planet *);
    
    auto is_predefined_planet_helper(planet *the_planet, planet *predined_planet) -> bool;
    auto is_predefined_planet(planet *) -> bool;
    auto calcPerihelion(long double, long double) -> long double;

};
#endif
