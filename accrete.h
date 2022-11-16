#ifndef ACCRETE_H
#define ACCRETE_H
#include "structs.h"

class accrete {

    public:
        void free_generations();
        auto stellar_dust_limit(long double) -> long double;
        auto dist_planetary_masses(sun &, long double, long double, long double,
                            long double, long double, long double, planet *,
                            bool) -> planet *;

    private:

    /* Now for some variables global to the accretion process:	    */
    bool dust_left;
    long double r_inner;
    long double r_outer;
    long double reduced_mass;
    long double dust_density;
    long double cloud_eccentricity;
    dust *dust_head = nullptr;
    planet *planet_head = nullptr;
    gen *hist_head = nullptr;
    planet *seed_moons = nullptr;

    void set_initial_conditions(long double, long double);
    auto nearest_planet(long double, long double) -> long double;
    auto farthest_planet(long double) -> long double;
    auto inner_effect_limit(long double, long double, long double) -> long double;
    auto outer_effect_limit(long double, long double, long double) -> long double;
    auto dust_available(long double, long double) -> bool;
    void update_dust_lanes(long double, long double, long double, long double,
                        long double, long double);
    auto collect_dust(long double, long double &, long double &, long double,
                    long double, long double, dust *) -> long double;
    auto critical_limit(long double, long double, long double) -> long double;
    void accrete_dust(long double &, long double &, long double &, long double,
                    long double, long double, long double, long double);
    void coalesce_planetesimals(long double, long double, long double, long double,
                                long double, long double, long double, long double,
                                long double, bool);

    void free_dust(dust *);
    void free_planet(planet *);
    
    auto is_predefined_planet_helper(planet *the_planet, planet *predined_planet) -> bool;
    auto is_predefined_planet(planet *) -> bool;
    auto is_in_eriEps(planet *) -> bool;
    auto is_in_UMa47(planet *) -> bool;
    auto is_in_horIot(planet *) -> bool;
    auto is_in_xiumab(planet *) -> bool;
    auto is_in_51peg(planet *) -> bool;
    auto is_in_55can(planet *) -> bool;
    auto is_in_UPSAndA(planet *) -> bool;
    auto is_in_hd10180(planet *) -> bool;
    auto is_in_gliese581(planet *) -> bool;
    auto is_in_hd10647(planet *) -> bool;
    auto is_in_83leoB(planet *) -> bool;
    auto is_in_muari(planet *) -> bool;
    auto is_in_hd28185(planet *) -> bool;
    auto is_in_hd40307(planet *) -> bool;
    auto is_in_kepler22(planet *) -> bool;
    auto is_in_taucet(planet *) -> bool;
    auto is_in_alfcentb(planet *) -> bool;
    auto is_in_EPSEri(planet *) -> bool;
    auto is_cyteen(planet *) -> bool;
    auto is_in_GL849(planet *) -> bool;
    auto is_in_ILAqr(planet *) -> bool;
    auto is_in_HD20794(planet *) -> bool;
    auto is_in_BETHyi(planet *) -> bool;
    auto is_in_hd208527(planet *) -> bool;
    auto is_in_kepler11(planet *) -> bool;
    auto is_in_bajor(planet *) -> bool;
    auto is_in_gliese667C(planet *) -> bool;
    auto is_in_kepler283(planet *) -> bool;
    auto is_in_kepler62(planet *) -> bool;
    auto is_in_kepler296(planet *) -> bool;
    auto is_in_gliese180(planet *) -> bool;
    auto is_in_gliese163(planet *) -> bool;
    auto is_in_kepler61(planet *) -> bool;
    auto is_in_gliese422(planet *) -> bool;
    auto is_in_kepler298(planet *) -> bool;
    auto is_in_kepler174(planet *) -> bool;
    auto is_in_gliese682(planet *) -> bool;
    auto is_in_hd38529(planet *) -> bool;
    auto is_in_hd202206(planet *) -> bool;
    auto is_in_hd8673(planet *) -> bool;
    auto is_in_hd22781(planet *) -> bool;
    auto is_in_hd217786(planet *) -> bool;
    auto is_in_hd106270(planet *) -> bool;
    auto is_in_hd38801(planet *) -> bool;
    auto is_in_hd39091(planet *) -> bool;
    auto is_in_hd141937(planet *) -> bool;
    auto is_in_hd33564(planet *) -> bool;
    auto is_in_hd23596(planet *) -> bool;
    auto is_in_hd222582(planet *) -> bool;
    auto is_in_hd86264(planet *) -> bool;
    auto is_in_hd196067(planet *) -> bool;
    auto is_in_hd10697(planet *) -> bool;
    auto is_in_hd132406(planet *) -> bool;
    auto is_in_hd13908(planet *) -> bool;
    auto is_in_hd2039(planet *) -> bool;
    auto is_in_hd82943(planet *) -> bool;
    auto is_in_moa2011blg293l(planet *) -> bool;
    auto is_in_hd213240(planet *) -> bool;
    auto calcPerihelion(long double, long double) -> long double;

};
#endif
