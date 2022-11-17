#ifndef STARGEN_H
#define STARGEN_H
#include <string>

#include "structs.h"
#include "accrete.h"

using namespace std;

#ifdef macintosh
#define SUBDIR "html"
#define DIRSEP ":"
#else
#ifdef WIN32
#define SUBDIR "html"
#define DIRSEP "\\"
#else
#define SUBDIR "html"
#define DIRSEP "/"
#endif
#endif

#define fUseSolarsystem 0x0001
#define fReuseSolarsystem 0x0002
#define fUseKnownPlanets 0x0004
#define fNoGenerate 0x0008

#define fDoGases 0x0010
#define fDoMoons 0x0020
#define fOnlyAsteroids 0x0040  // seb
#define fDoMigration 0x0080

#define fOnlyHabitable 0x0100
#define fOnlyMultiHabitable 0x0200
#define fOnlyJovianHabitable 0x0400
#define fOnlyEarthlike 0x0800
#define fOnlyThreeHabitable 0x1000
#define fOnlySuperTerans 0x2000
#define fOnlyPotentialHabitable 0x4000

#define fIsCircubinaryStar 0x8000

// Values of out_format
#define ffHTML 'HTML'
#define ffTEXT 'TEXT'
#define ffCELESTIA '.SSC'
#define ffMOONGEN '.BSH'
#define ffCSV '.CSV'
#define ffJSON '.JSO'
#define ffCSVdl '+CSV'
#define ffSVG '.SVG'

// Values of graphic_format
#define gfGIF '.GIF'
#define gfSVG '.SVG'

using actions = enum actions {  // Callable StarGen can:
  aGenerate,                    //	- Generate randon system(s)
  aListGases,                   //	- List the gas table
  aListCatalog,                 //	- List the stars in a catalog
  aListCatalogAsHTML,           //  - For creating a <FORM>
  aSizeCheck,                   //  - List sizes of various types
  aListVerbosity,               //  - List values of the -v option
};

extern int flags_arg_clone;
extern sun the_sun_clone;

extern int flag_verbose;
extern bool allow_planet_migration;
extern bool is_circumbinary;
extern long double compainion_mass_arg;
extern long double compainion_eccentricity_arg;
extern long double compainion_distant_arg;
extern long double compainion_lum_arg;
extern long double compainion_eff_arg;
extern string companion_spec_arg;
extern long double min_age;
extern long double max_age;
extern long double max_age_backup;
extern long double temp_arg;
extern string type_arg;
extern int decimals_arg;
extern long double max_distance_arg;

// Various statistics that are kept:
extern int total_earthlike;
extern int total_habitable;
extern int total_habitable_earthlike;
extern int total_habitable_conservative;
extern int total_habitable_optimistic;
extern int total_potentially_habitable;
extern int total_potentially_habitable_earthlike;
extern int total_potentially_habitable_conservative;
extern int total_potentially_habitable_optimistic;
extern int total_worlds;

extern long double min_breathable_terrestrial_g;
extern long double min_breathable_g;
extern long double max_breathable_terrestrial_g;
extern long double max_breathable_g;
extern long double min_breathable_terrestrial_l;
extern long double min_breathable_l;
extern long double max_breathable_terrestrial_l;
extern long double max_breathable_l;
extern long double min_breathable_temp;
extern long double max_breathable_temp;
extern long double min_breathable_p;
extern long double max_breathable_p;
extern long double min_breathable_mass;
extern long double max_breathable_mass;

extern long double min_potential_terrestrial_g;
extern long double min_potential_g;
extern long double max_potential_terrestrial_g;
extern long double max_potential_g;
extern long double min_potential_terrestrial_l;
extern long double min_potential_l;
extern long double max_potential_terrestrial_l;
extern long double max_potential_l;
extern long double min_potential_temp;
extern long double max_potential_temp;
extern long double min_potential_p;
extern long double max_potential_p;
extern long double min_potential_mass;
extern long double max_potential_mass;

extern string stargen_revision;

extern long flag_seed;

void init();
void generate_planet(planet* /*the_planet*/, int /*planet_no*/,
                     sun& /*the_sun*/, bool /*random_tilt*/,
                     const string& /*planet_id*/, bool /*do_gases*/,
                     bool /*do_moons*/, bool /*is_moon*/,
                     long double /*parent_mass*/);
void generate_planets(sun& /*the_sun*/, bool /*random_tilt*/,
                      const string& /*flag_char*/, int /*sys_no*/,
                      const string& /*system_name*/, bool /*do_gases*/,
                      bool /*do_moons*/);
void generate_stellar_system(sun &the_sun, bool use_seed_system,
                             planet *seed_system, const string& flag_char, int sys_no,
                             const string& system_name, long double inner_dust_limit,
                             long double outer_planet_limit,
                             long double ecc_coef,
                             long double inner_planet_factor, bool do_gases,
                             bool do_moons, accrete &myAccreteObject);
auto stargen(actions /*action*/, const string& /*flag_char*/, string /*path*/,
             const string& /*url_path_arg*/, const string& /*filename_arg*/,
             const string& /*sys_name_arg*/, string /*prognam*/,
             long double /*mass_arg*/, long double /*luminosity_arg*/,
             long /*seed_arg*/, int /*count_arg*/, int /*incr_arg*/,
             catalog& /*cat_arg*/, int /*sys_no_arg*/,
             long double /*ratio_arg*/, long double /*ecc_coef_arg*/,
             long double /*inner_planet_factor_arg*/, int /*flags_arg*/,
             int /*out_format*/, int /*graphic_format*/) -> int;
void check_planet(planet* /*the_planet*/, const string& /*planet_id*/,
                  bool /*is_moon*/);
void assign_type(sun& /*the_sun*/, planet* /*the_planet*/,
                 const string& /*planet_id*/, bool /*is_moon*/,
                 bool /*do_gases*/, bool /*second_time*/);
#endif
