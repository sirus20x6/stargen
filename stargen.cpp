#include "stargen.h"
#include <cmath>       // for pow, NAN, exp, log
#include <cstdlib>     // for EXIT_SUCCESS, srand, rand
#include <ctime>       // for time, time_t
#include <iostream>    // for operator<<, basic_ostream, std::ostream, "\n", stri...
#include <iomanip>     // for std::setfill, std::setw
#include <sstream>     // for std::stringstream
#include <algorithm>   // for std::sort
#include <memory>      // for std::unique_ptr
#include "accrete.h"   // for accrete
#include "const.h"     // for SUN_MASS_IN_EARTH_MASSES, KM_PER_AU, EARTH_AVE...
#include "display.h"   // for type_string, close_html_file, create_svg_file
#include "elements.h"  // for gases
#include "enviro.h"    // for makeHabitable, est_temp, gravity, acceleration
#include "planets.h"   // for earth, mercury
#include "utils.h"     // for toString, random_number, replaceStrChar, about
#include "Config.h"    // for Config
#include "SimulationContext.h"  // for SimulationContext
#include "RandomContext.h"  // for RandomContext
#include "StarGenerator.h"  // for StarGenerator
#include "radius_tables.h"  // for initRadii (per-thread gas-table population)
#include "ThreadPool.h"  // for ThreadPool (parallel generation)
#include "ObjectPool.h"  // for ObjectPool (object reuse)
#include <mutex>  // for std::mutex (thread-safe output)

// Global StarGenerator instance - encapsulates config, simulation state, and RNG
// TODO: This will eventually be passed as a parameter instead of being global
StarGenerator g_generator;

// Legacy global references for backward compatibility
Config& g_config = g_generator.config;
SimulationContext& g_sim_context = g_generator.sim_context;
RandomContext& g_random_context = g_generator.random_context;

// Initialize Config defaults
void initConfig() {
    // Initialization now handled by StarGenerator constructor
    // Keep this function for backward compatibility
    g_generator.config.dust_density_ratio = DUST_DENSITY_COEFF;
    g_generator.config.max_age = 10.0E9;
    g_generator.config.max_age_backup = 10.0E9;
    g_generator.sim_context.dust_density_coeff = DUST_DENSITY_COEFF;
}

// Legacy global references - these now point to Config members for compatibility
int& flags_arg_clone = g_config.flags;
// the_sun_clone is the "active sun" of the system currently being generated. It
// is thread_local (NOT a reference into the shared g_generator) so each parallel
// worker owns its own copy: generate_stellar_system assigns this thread's fully
// initialised sun into it before generate_planets() runs. Previously this aliased
// g_generator.sim_context.current_sun, so all 8 ThreadPool workers read/wrote one
// shared sun unsynchronised -- the source of the gas-giant Density/Total Radius
// run-to-run jitter (the gas-radius age==0 fallback and the habitability
// getEffTemp() lazy-init both read it). TSan confirmed the race was on g_generator.
thread_local sun the_sun_clone;
int& flag_verbose = g_config.verbose_level;
long& flag_seed = g_config.random_seed;
long double& min_age = g_config.min_age;
long double& max_age = g_config.max_age;
long double& max_age_backup = g_config.max_age_backup;
bool& is_circumbinary = g_config.is_circumbinary;
long double& compainion_mass_arg = g_config.companion_mass;
long double& compainion_eccentricity_arg = g_config.companion_eccentricity;
long double& compainion_distant_arg = g_config.companion_distance;
long double& compainion_lum_arg = g_config.companion_luminosity;
long double& compainion_eff_arg = g_config.companion_temperature;
std::string& companion_spec_arg = g_config.companion_spectral_type;
int& decimals_arg = g_config.decimals;
long double& temp_arg = g_config.stellar_temperature;
std::string& type_arg = g_config.stellar_type;
long double& max_distance_arg = g_config.max_distance;
bool& allow_planet_migration = g_config.do_migration;

/*  Legacy global references - now point to SimulationContext members  */
planet*& innermost_planet = g_sim_context.innermost_planet;
long double& dust_density_coeff = g_sim_context.dust_density_coeff;
long& system_seed = g_sim_context.current_system_seed;

// Global statistics - now atomic types in SimulationContext
// NOTE: Can't create references to atomics - access g_sim_context.total_* directly
// Removed: int& g_sim_context.total_earthlike, g_sim_context.total_habitable, etc.
// Use: g_sim_context.g_sim_context.total_earthlike++, g_sim_context.g_sim_context.total_habitable.load(), etc.

// Per-system statistics - now point to SimulationContext members
int& earthlike = g_sim_context.system_earthlike;
int& habitable = g_sim_context.system_habitable;
int& habitable_jovians = g_sim_context.system_habitable_jovians;
int& habitable_superterrans = g_sim_context.system_habitable_superterrans;
int& potential_habitable = g_sim_context.system_potentially_habitable;
long double& max_moon_mass = g_sim_context.system_max_moon_mass;
int (&type_counts)[16] = g_sim_context.type_counts;
int& type_count = g_sim_context.type_count;
int& max_type_count = g_sim_context.max_type_count;

long double& min_breathable_terrestrial_g = g_sim_context.min_breathable_terrestrial_g;
long double& min_breathable_g = g_sim_context.min_breathable_g;
long double& max_breathable_terrestrial_g = g_sim_context.max_breathable_terrestrial_g;
long double& max_breathable_g = g_sim_context.max_breathable_g;
long double& min_breathable_temp = g_sim_context.min_breathable_temp;
long double& max_breathable_temp = g_sim_context.max_breathable_temp;
long double& min_breathable_p = g_sim_context.min_breathable_p;
long double& max_breathable_p = g_sim_context.max_breathable_p;
long double& min_breathable_terrestrial_l = g_sim_context.min_breathable_terrestrial_l;
long double& min_breathable_l = g_sim_context.min_breathable_l;
long double& max_breathable_terrestrial_l = g_sim_context.max_breathable_terrestrial_l;
long double& max_breathable_l = g_sim_context.max_breathable_l;
long double& min_breathable_mass = g_sim_context.min_breathable_mass;
long double& max_breathable_mass = g_sim_context.max_breathable_mass;

long double& min_potential_terrestrial_g = g_sim_context.min_potential_terrestrial_g;
long double& min_potential_g = g_sim_context.min_potential_g;
long double& max_potential_terrestrial_g = g_sim_context.max_potential_terrestrial_g;
long double& max_potential_g = g_sim_context.max_potential_g;
long double& min_potential_temp = g_sim_context.min_potential_temp;
long double& max_potential_temp = g_sim_context.max_potential_temp;
long double& min_potential_p = g_sim_context.min_potential_p;
long double& max_potential_p = g_sim_context.max_potential_p;
long double& min_potential_terrestrial_l = g_sim_context.min_potential_terrestrial_l;
long double& min_potential_l = g_sim_context.min_potential_l;
long double& max_potential_terrestrial_l = g_sim_context.max_potential_terrestrial_l;
long double& max_potential_l = g_sim_context.max_potential_l;
long double& min_potential_mass = g_sim_context.min_potential_mass;
long double& max_potential_mass = g_sim_context.max_potential_mass;

// Version identifier
std::string stargen_revision = "$Revision: 3.0 $";

/**
 * @brief Data structure to hold system information for deferred output in parallel mode
 *
 * ## Architecture Overview
 *
 * In sequential mode, systems are output immediately after generation:
 *   generate_system() -> output_system() -> free_planets()
 *
 * In parallel mode, output must be deferred until all generation completes:
 *   1. Generation phase: generate_system() -> store SystemOutputData
 *   2. Sort phase: sort systems by seed for consistent ordering
 *   3. Output phase: for each SystemOutputData -> output_system()
 *   4. Cleanup phase: SystemOutputData destructors free memory
 *
 * ## Memory Management
 *
 * This struct maintains ownership of generator and accrete objects, which in turn
 * own the planet memory. Uses RAII with unique_ptr for automatic cleanup and
 * exception safety.
 *
 * Memory lifecycle:
 *   - Thread acquires generator/accrete from pool
 *   - If system should be output, ownership transferred to SystemOutputData
 *   - Thread acquires new generator/accrete for next system
 *   - After output phase, SystemOutputData destructor:
 *       1. Calls accrete->free_generations() to free planet linked list
 *       2. unique_ptr automatically deletes generator and accrete objects
 *
 * ## Memory Usage Estimates
 *
 * Per system stored for output:
 *   - SystemOutputData struct: ~200 bytes
 *   - StarGenerator object: ~500 bytes
 *   - accrete object: ~500 bytes
 *   - Planet chain: ~1KB per planet (varies by planet count)
 *   - Typical total: ~2-10 KB per system
 *
 * Example: 10,000 systems = ~20-100 MB RAM
 *
 * For very large runs, consider:
 *   - Filters to reduce output count (--only-habitable)
 *   - Smaller batch sizes
 *   - Sequential mode for minimal memory usage
 *
 * ## Thread Safety
 *
 * - g_pending_outputs guarded by g_output_mutex
 * - emplace_back() performed inside lock
 * - Output phase is single-threaded (processes stored systems sequentially)
 */
struct SystemOutputData {
    // Ownership of generator and accrete (to keep planet data alive)
    // Using unique_ptr for automatic cleanup and exception safety
    std::unique_ptr<StarGenerator> generator;
    std::unique_ptr<accrete> accrete_obj;

    // Core system data
    planet* innermost_planet;
    sun the_sun;  // Copy of the sun for this system

    // System identification
    long flag_seed;
    std::string system_name;
    std::string file_name;
    int sys_no;

    // Habitability statistics
    int habitable;
    int earthlike;
    int potential_habitable;
    int habitable_jovians;
    int habitable_superterrans;

    // For Celestia output
    std::string designation;
    long double sys_inc;
    long double sys_an;

    // Constructor - takes ownership of raw pointers
    SystemOutputData(StarGenerator* gen, accrete* acc, planet* planets, const sun& sun_obj,
                     long seed, const std::string& name, const std::string& fname,
                     int sysno, int hab, int earth, int pot_hab, int hab_jov, int hab_super)
        : generator(gen), accrete_obj(acc), innermost_planet(planets), the_sun(sun_obj),
          flag_seed(seed), system_name(name), file_name(fname), sys_no(sysno),
          habitable(hab), earthlike(earth), potential_habitable(pot_hab),
          habitable_jovians(hab_jov), habitable_superterrans(hab_super),
          sys_inc(0.0), sys_an(0.0) {}

    // Destructor - cleanup happens automatically via unique_ptr
    ~SystemOutputData() {
        if (accrete_obj) {
            accrete_obj->free_generations();
        }
        // unique_ptr will automatically delete generator and accrete_obj
    }

    // Moveable but not copyable (unique ownership)
    SystemOutputData(SystemOutputData&&) = default;
    SystemOutputData& operator=(SystemOutputData&&) = default;
    SystemOutputData(const SystemOutputData&) = delete;
    SystemOutputData& operator=(const SystemOutputData&) = delete;
};

// Thread-safe storage for parallel output
std::vector<SystemOutputData> g_pending_outputs;
std::mutex g_output_mutex;

// (The former g_stats_mutex is gone: the breathable/potentially-habitable
// min/max summary stats are now accumulated per-worker and reduced after the
// threads join, so no worker touches a shared mutable global during generation.)

/**
 * @brief Handle aListGases action - list all gases with their properties
 */
static auto handle_list_gases_action() -> int {
  long double total = 0.0;
  int count = gases.count();
  for (int i = 0; i < count; i++) {
    if (gases[i].getWeight() >= AN_N &&
        gases[i].getMaxIpp() < INCREDIBLY_LARGE_NUMBER) {
      total += gases[i].getMaxIpp();
    }
  }
  std::cout << gases;
  std::cout << "Total Max ipp: " << toString(total) << "\n";
  std::cout << "Max pressure: " << toString(MAX_HABITABLE_PRESSURE) << " atm\n";
  return EXIT_SUCCESS;
}

/**
 * @brief Handle aListCatalog action - list star catalog
 */
static auto handle_list_catalog_action(catalog& cat_arg) -> int {
  std::cout << cat_arg;
  return EXIT_SUCCESS;
}

/**
 * @brief Handle aListCatalogAsHTML action - list catalog as HTML options
 */
static auto handle_list_catalog_html_action(catalog& cat_arg) -> int {
  int count = cat_arg.count();
  for (int i = 0; i < count; i++) {
    std::cout << "\t<option value=" << i << ">" << escapeXmlText(cat_arg[i].getName())
         << "</option>\n";
  }
  return EXIT_SUCCESS;
}

/**
 * @brief Handle aSizeCheck action - display type sizes and planet temperatures
 */
static auto handle_size_check_action() -> int {
  long double tempE = est_temp(1.0, 1.0, EARTH_ALBEDO);
  long double tempJ = est_temp(1.0, 5.2034, GAS_GIANT_ALBEDO);
  long double tempS = est_temp(1.0, 9.5371, GAS_GIANT_ALBEDO);
  long double tempU = est_temp(1.0, 19.1913, GAS_GIANT_ALBEDO);
  long double tempN = est_temp(1.0, 30.0690, GAS_GIANT_ALBEDO);

  std::cout << "Size of float: " << sizeof(float) << "\n";
  std::cout << "Size of doubles: " << sizeof(double) << "\n";
  std::cout << "Size of long doubles: " << sizeof(long double) << "\n";
  std::cout << "Earth Eff Temp: " << toString(tempE) << " K, "
       << toString(tempE - FREEZING_POINT_OF_WATER)
       << " C, Earth rel: " << toString(tempE - EARTH_AVERAGE_KELVIN)
       << " C\n";
  std::cout << "Jupiter Eff Temp: " << toString(tempJ) << " K\n";
  std::cout << "Saturn Eff Temp: " << toString(tempS) << " K\n";
  std::cout << "Uranus Eff Temp: " << toString(tempU) << " K\n";
  std::cout << "Neptune Eff Temp: " << toString(tempN) << " K\n";
  return EXIT_SUCCESS;
}

/**
 * @brief Handle aListVerbosity action - show verbosity flag documentation
 */
static auto handle_list_verbosity_action() -> int {
  std::cout << "Stargen " << stargen_revision << "\n"
  << "Verbosity flags are hexidecimal numbers:\n"
  << "\t0001\tEarthlike count\n"
  << "\t0002\tTrace Min/Max\n"
  << "\t0004\tList Habitable\n"
  << "\t0008\tList Earthlike & Sphinxlike\n\n"
  << "\t0010\tList Gases\n"
  << "\t0020\tTrace temp iterations\n"
  << "\t0040\tGas lifetimes\n"
  << "\t0080\tList loss of accreted gas mass\n\n"
  << "\t0100\tInjecting, collision\n"
  << "\t0200\tChecking..., Failed...\n"
  << "\t0400\tList binary info\n"
  << "\t0800\tList accreted atmospheres\n\n"
  << "\t1000\tMoons (experimental)\n"
  << "\t2000\tOxygen poisoned (experimental)\n"
  << "\t4000\tTrace gas percentages\n"
  << "\t8000\tList Jovians in the ecosphere\n\n"
  << "\t10000\tList type diversity\n"
  << "\t20000\tTrace Surface temp interations\n"
  << "\t40000\tDisplay Roche Limits and Hill Sphere distances\n"
  << "\t80000\tDisplay mass-radius maps\n";
  return EXIT_SUCCESS;
}

/**
 * @brief Normalize filter flags - handle mutually exclusive filters
 */
static void normalize_filter_flags(bool& only_habitable, bool& only_multi_habitable,
                                   bool& only_earthlike, bool& only_three_habitable,
                                   bool& only_superterrans) {
  if (only_habitable && only_multi_habitable) {
    only_habitable = false;
  }
  if (only_habitable && only_earthlike) {
    only_habitable = false;
  }
  if (only_three_habitable) {
    only_habitable = false;
    only_earthlike = false;
    only_multi_habitable = false;
  }
  if (only_superterrans) {
    only_habitable = false;
    only_earthlike = false;
    only_multi_habitable = false;
    only_three_habitable = false;
  }
}

/**
 * @brief Calculate weighted type count from type_counts array
 * @param type_counts Array of counts for each planet type
 * @param base_count Base count to add weights to
 * @return Weighted type count
 */
static auto calculate_weighted_type_count(const int type_counts[], int base_count) -> int {
  int wt_count = base_count;
  if (type_counts[0] > 0) wt_count += 1;   // Unknown
  if (type_counts[1] > 0) wt_count += 3;   // Rock
  if (type_counts[2] > 0) wt_count += 16;  // Venusian
  if (type_counts[3] > 0) wt_count += 20;  // Terrestrial
  if (type_counts[4] > 0) wt_count += 12;  // Gas Dwarf
  if (type_counts[5] > 0) wt_count += 11;  // Neptunian
  if (type_counts[6] > 0) wt_count += 2;   // Jovian
  if (type_counts[7] > 0) wt_count += 15;  // Martian
  if (type_counts[8] > 0) wt_count += 18;  // Water
  if (type_counts[9] > 0) wt_count += 14;  // Ice
  if (type_counts[10] > 0) wt_count += 13; // Asteroids
  if (type_counts[11] > 0) wt_count += 10; // 1-Face
  return wt_count;
}

/**
 * @brief Check if system passes filter criteria and should be output
 */
static auto should_output_system(bool only_habitable, bool only_multi_habitable,
                                bool only_jovian_habitable, bool only_earthlike,
                                bool only_three_habitable, bool only_superterrans,
                                bool only_potential_habitable, int habitable,
                                int habitable_jovians, int earthlike,
                                int habitable_superterrans, int potential_habitable) -> bool {
  // No filters active - output everything
  if (!(only_habitable || only_multi_habitable || only_jovian_habitable ||
        only_earthlike || only_three_habitable || only_superterrans ||
        only_potential_habitable)) {
    return true;
  }

  // Check each filter condition
  if (only_habitable && (habitable > 0) && !only_jovian_habitable) return true;
  if (only_habitable && only_jovian_habitable && habitable > 0 &&
      habitable_jovians > 0) return true;
  if (only_multi_habitable && (habitable > 1) && !only_jovian_habitable) return true;
  if (only_multi_habitable && only_jovian_habitable && habitable > 1 &&
      habitable_jovians > 0) return true;
  if (only_earthlike && (earthlike > 0) && !only_jovian_habitable) return true;
  if (only_earthlike && only_jovian_habitable && earthlike > 0 &&
      habitable_jovians > 0) return true;
  if (only_jovian_habitable && (habitable_jovians > 0) &&
      !(only_earthlike || only_multi_habitable || only_habitable ||
        only_three_habitable)) return true;
  if (only_three_habitable && only_jovian_habitable && habitable > 2 &&
      habitable_jovians > 0) return true;
  if (only_three_habitable && (habitable > 2)) return true;
  if (only_superterrans && habitable_superterrans > 0) return true;
  if (only_potential_habitable && potential_habitable > 0) return true;

  return false;
}

/**
 * @brief Output a single system based on format
 *
 * Encapsulates output logic to avoid duplication between parallel and sequential paths.
 *
 * @param planets Innermost planet in the system
 * @param out_format Output format (ffTEXT, ffHTML, etc.)
 * @param do_gases Whether to include gas data
 * @param do_moons Whether to include moon data
 * @param seed System seed
 * @param csv_file Optional file stream for CSV/JSON output (nullptr for other formats)
 * @param path Output path for HTML/SVG files
 * @param file_name Base filename for HTML/SVG files
 * @param prognam Program name for file headers
 * @param url_path URL path for HTML links
 */
static void output_system(planet* planets, int out_format,
                         bool do_gases, bool do_moons, long seed,
                         std::fstream* csv_file = nullptr,
                         const std::string& path = "",
                         const std::string& file_name = "",
                         const std::string& prognam = "",
                         const std::string& url_path = "") {
  switch (out_format) {
    case ffTEXT:
      text_describe_system(planets, do_gases, seed, do_moons);
      break;

    case ffHTML:
      if (!path.empty() && !file_name.empty()) {
        std::fstream html_file;
        open_html_file(planets->getTheSun().getName(), seed, path, url_path,
                      file_name, ".html", prognam, html_file);
        html_describe_system(planets, do_gases, do_moons, url_path, html_file);
        close_html_file(html_file);
      } else {
        std::cerr << "Warning: HTML output requires path and filename\n";
      }
      break;

    case ffSVG:
      if (!path.empty() && !file_name.empty()) {
        create_svg_file(planets, path, file_name, ".svg", prognam, do_moons);
      } else {
        std::cerr << "Warning: SVG output requires path and filename\n";
      }
      break;

    case ffCSV:
    case ffCSVdl:
      if (csv_file && csv_file->is_open()) {
        csv_describe_system(*csv_file, planets, do_gases, seed, do_moons);
      } else {
        std::cerr << "Warning: CSV output requires an open file stream\n";
      }
      break;

    case ffJSON:
      if (csv_file && csv_file->is_open()) {
        jsonDescribeSystem(*csv_file, planets, do_gases, seed, do_moons);
      } else {
        std::cerr << "Warning: JSON output requires an open file stream\n";
      }
      break;

    case ffCELESTIA:
      // TODO: Implement CELESTIA output for parallel mode
      std::cerr << "Warning: CELESTIA output not yet supported in parallel mode\n";
      break;

    default:
      break;
  }
}

/**
 * @brief Print summary statistics at the end of generation
 */
static void print_summary_statistics() {
  if (((flag_verbose & 0x0001) != 0) || ((flag_verbose & 0x0002) != 0)) {
    std::cerr << "Earthlike planets: " << g_sim_context.total_earthlike << "\n";
    std::cerr << "Breathable atmospheres: " << g_sim_context.total_habitable << "\n";
    std::cerr << "Breathable g range: " << toString(min_breathable_g) << " - "
         << toString(max_breathable_g) << "\n";
    std::cerr << "Terrestrial g range: " << toString(min_breathable_terrestrial_g)
         << " - " << toString(max_breathable_terrestrial_g) << "\n";
    std::cerr << "Breathable pressure range: " << toString(min_breathable_p) << " - "
         << toString(max_breathable_p) << "\n";
    std::cerr << "Breathable temp range: "
         << toString(min_breathable_temp - EARTH_AVERAGE_KELVIN) << " C - "
         << toString(max_breathable_temp - EARTH_AVERAGE_KELVIN) << " C"
         << "\n";
    std::cerr << "Breathable illumination range: " << toString(min_breathable_l)
         << " - " << toString(max_breathable_l) << "\n";
    std::cerr << "Terrestrial illumination range: "
         << toString(min_breathable_terrestrial_l) << " - "
         << toString(max_breathable_terrestrial_l) << "\n";
    std::cerr << "Max moon mass: "
         << toString(max_moon_mass * SUN_MASS_IN_EARTH_MASSES)
         << " Earth Masses\n";
  }
}

/**
 * @brief stargen
 * 
 * @param action 
 * @param flag_char 
 * @param path 
 * @param url_path_arg 
 * @param filename_arg 
 * @param sys_name_arg 
 * @param prognam 
 * @param mass_arg 
 * @param luminosity_arg 
 * @param seed_arg 
 * @param count_arg 
 * @param incr_arg 
 * @param cat_arg 
 * @param sys_no_arg 
 * @param ratio_arg 
 * @param ecc_coef_arg 
 * @param inner_planet_factor_arg 
 * @param flags_arg 
 * @param out_format 
 * @param graphic_format 
 * @return int 
 */
auto stargen(actions action, const std::string &flag_char, std::string path,
             const std::string &url_path_arg, const std::string &filename_arg,
             const std::string &sys_name_arg, std::string prognam, long double mass_arg,
             long double luminosity_arg, long seed_arg, int count_arg,
             int incr_arg, catalog &cat_arg, int sys_no_arg,
             long double ratio_arg, long double ecc_coef_arg,
             long double inner_planet_factor_arg, int flags_arg, int out_format,
             int graphic_format, int num_threads) -> int {
  sun the_sun;
  long double min_mass = 0.4;
  long double inc_mass = 0.05;
  long double max_mass = 2.35;
  long double sys_inc = 0.0;  // seb
  long double sys_an = 0.0;   // seb
  int system_count = 1;
  int seed_increment = 1;
  std::string default_path = SUBDIR; /* OS specific */
  std::string default_url_path = "../";
  std::string url_path = default_url_path;
  std::string thumbnail_file = "Thumbnails";
  std::string file_name = "StarGen";
  std::string subdir;
  std::string csv_file_name = "StarGen.csv";
  std::fstream html_file;
  std::fstream thumbnails;
  std::fstream csv_file;
  int index = 0;
  bool do_catalog = cat_arg.count() > 0 && sys_no_arg == 0;
  int catalog_count = 0;
  bool do_gases = (flags_arg & fDoGases) != 0;
  bool use_solar_system = (flags_arg & fUseSolarsystem) != 0;
  bool reuse_solar_system = (flags_arg & fReuseSolarsystem) != 0;
  bool use_known_planets = (flags_arg & fUseKnownPlanets) != 0;
  bool no_generate = (flags_arg & fNoGenerate) != 0;
  bool do_moons = (flags_arg & fDoMoons) != 0;
  bool only_habitable = (flags_arg & fOnlyHabitable) != 0;
  bool only_multi_habitable = (flags_arg & fOnlyMultiHabitable) != 0;
  bool only_three_habitable = (flags_arg & fOnlyThreeHabitable) != 0;
  bool only_jovian_habitable = (flags_arg & fOnlyJovianHabitable) != 0;
  bool only_earthlike = (flags_arg & fOnlyEarthlike) != 0;
  bool only_superterrans = (flags_arg & fOnlySuperTerans) != 0;
  bool only_potential_habitable = (flags_arg & fOnlyPotentialHabitable) != 0;
  allow_planet_migration = (flags_arg & fDoMigration) != 0;
  is_circumbinary = (flags_arg & fIsCircubinaryStar) != 0;
  std::stringstream ss;

  accrete myAccreteObject;

  if (do_catalog) {
    catalog_count = cat_arg.count();
  }

  // Normalize mutually exclusive filter flags
  normalize_filter_flags(only_habitable, only_multi_habitable, only_earthlike,
                        only_three_habitable, only_superterrans);

  if (prognam.empty()) {
    prognam = "StarGen";
  }

  if (path.empty()) {
    path = default_path;
  }

  if (graphic_format == 0) {
    graphic_format = gfGIF;
  }

  if (!url_path_arg.empty()) {
    url_path = url_path_arg;
  }

  // Find the last sub-dir in the path
  ss << path.substr(path.find_last_of('/') + 1) << DIRSEP;
  subdir = ss.str();
  ss.str("");

  if (path.find_last_of('/') != path.size()) {
    path.append(DIRSEP);
  }

  switch (action) {
    case aListGases:
      return handle_list_gases_action();
    case aListCatalog:
      return handle_list_catalog_action(cat_arg);
    case aListCatalogAsHTML:
      return handle_list_catalog_html_action(cat_arg);
    case aSizeCheck:
      return handle_size_check_action();
    case aListVerbosity:
      return handle_list_verbosity_action();
    case aGenerate:
      break;
  }

  flag_seed = seed_arg;
  the_sun.setMass(mass_arg);
  the_sun.setLuminosity(luminosity_arg);
  if (the_sun.getMass() == 0 && the_sun.getLuminosity() != 0) {
    the_sun.setMass(luminosity_to_mass(the_sun.getLuminosity()));
  }
  the_sun.setEffTemp(temp_arg);
  the_sun.setSpecType(type_arg);

  system_count = count_arg;
  seed_increment = incr_arg;

  if (ratio_arg > 0.0) {
    dust_density_coeff *= ratio_arg;
  }

  if (reuse_solar_system) {
    system_count = 1 + (int)((max_mass - min_mass) / inc_mass);

    the_sun.setLuminosity(1.0);
    the_sun.setMass(1.0);
    the_sun.setAge(5E9);

    use_solar_system = true;
  } else if (do_catalog) {
    system_count = catalog_count + ((system_count - 1) * (catalog_count - 1));
    use_solar_system = true;
  }

  if (system_count > 1 && out_format != ffCSVdl) {
    if (!filename_arg.empty()) {
      thumbnail_file = filename_arg;
    }

    open_html_file("Thumbnails", flag_seed, path, url_path, thumbnail_file,
                   ".html", prognam, thumbnails);
  }

  if (out_format == ffCSV || out_format == ffCSVdl || out_format == ffJSON) {
    std::string csv_url;
    std::string cleaned_arg = "StarGen";

    if (!filename_arg.empty()) {
      char *ptr = nullptr;

      cleaned_arg = filename_arg;
    }
    ss.str("");
    if (out_format == ffJSON) {
      ss << cleaned_arg << ".json";

    }
    else {
      ss << cleaned_arg << ".csv";
    }

    csv_file_name = ss.str();
    openCVSorJson(path, csv_file_name, csv_file);

  }

  // Determine if we can use parallel execution
  // Parallel mode is only safe for simple cases: fixed star, no catalogs, no special modes
  // Unified generation path: ALL eligible multi-system runs go through the pool,
  // including single-threaded ones (num_threads == 1 -> a pool of one). Routing
  // -T1 through the same path as -T8 makes output byte-identical regardless of
  // thread count and gives one canonical generator. Single-system and
  // catalog/solar/seed-system runs still use the serial fallback below.
  bool can_use_parallel = (num_threads >= 1) && (system_count > 1) && !do_catalog &&
                          (sys_no_arg == 0) && !use_solar_system && !reuse_solar_system &&
                          (out_format == ffHTML || out_format == ffTEXT || out_format == ffSVG ||
                           out_format == ffCSV || out_format == ffCSVdl || out_format == ffJSON);

  if (can_use_parallel && num_threads > system_count) {
    num_threads = system_count;  // Don't use more threads than systems
  }

  // Print threading information
  if (num_threads > 1 && system_count > 1) {
    if (can_use_parallel) {
      std::cerr << "Parallel generation enabled: " << num_threads << " threads processing "
                << system_count << " systems\n";
    } else {
      std::cerr << "Parallel generation disabled: sequential execution (catalogs, solar system, or complex modes not yet supported in parallel)\n";
    }
  }

  // Parallel execution path with work batching for performance
  if (can_use_parallel) {
    ThreadPool pool(num_threads);
    std::atomic<int> systems_generated{0};

    // Create object pools with extra capacity for output storage
    // Each thread needs 1 object, but may transfer ownership when storing output
    // Reserve 2x capacity to reduce dynamic allocation during generation
    int pool_capacity = num_threads * 2;

    ObjectPool<StarGenerator> generator_pool(pool_capacity, [&]() {
      return new StarGenerator(g_generator.config);
    });

    ObjectPool<accrete> accrete_pool(pool_capacity, []() {
      return new accrete();
    });

    // Divide work into batches - one batch per thread
    int systems_per_thread = system_count / num_threads;
    int remainder = system_count % num_threads;

    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);

    // Per-batch habitability min/max accumulators, one slot per worker batch.
    // Each worker folds its systems' stats into its OWN slot, so no shared
    // mutable global is touched during generation; the slots are reduced into
    // the run-wide totals after the threads join. unique_ptr because
    // SimulationContext holds a std::mutex (non-movable). Fresh contexts already
    // start at the correct min/max sentinels.
    std::vector<std::unique_ptr<SimulationContext>> batch_stats;
    batch_stats.reserve(num_threads);
    for (int b = 0; b < num_threads; b++) {
      batch_stats.emplace_back(std::make_unique<SimulationContext>());
    }

    for (int thread_id = 0; thread_id < num_threads; thread_id++) {
      // Calculate the range of systems this thread will process
      int start_index = thread_id * systems_per_thread + std::min(thread_id, remainder);
      int count = systems_per_thread + (thread_id < remainder ? 1 : 0);

      futures.push_back(pool.enqueue([=, &generator_pool, &accrete_pool, &systems_generated, &batch_stats]() {
        // Acquire objects from pools (one pair per thread)
        StarGenerator* gen = generator_pool.acquire();
        accrete* acc = accrete_pool.acquire();

        // Local statistics accumulators (avoid atomic contention)
        int local_habitable = 0;
        int local_potentially_habitable = 0;
        int local_earthlike = 0;
        int local_worlds = 0;

        // Populate THIS worker thread's thread_local gas-radius interpolation
        // tables. They are thread_local (the gas-radius helpers write per-planet
        // anchors into them), so the main thread's initRadii() in main.cpp does
        // not reach worker threads; each thread must initialise its own copy.
        // The guard inside initRadii() makes this a one-time cost per thread.
        initRadii();

        // Process all systems assigned to this thread
        for (int i = 0; i < count; i++) {
          int sys_index = start_index + i;

          // Reset objects for reuse
          gen->reset();
          acc->reset();
          // Accumulate each system's breathable/potential min/max in isolation;
          // folded into this worker's batch slot after generation (below).
          gen->sim_context.resetHabitabilityStats();

          gen->random_context.setSeed(seed_arg + (sys_index * seed_increment));
          acc->setRandomContext(&gen->random_context);

          // Route the free RNG functions (random_number/randf/...) used by
          // enviro/accrete generation to THIS worker's per-system context.
          // gen may have been reacquired from the pool, so set it each iteration.
          set_active_random_context(&gen->random_context);

          // Create sun object (lightweight)
          sun thread_sun;
          thread_sun.setMass(mass_arg);
          thread_sun.setLuminosity(luminosity_arg);
          if (thread_sun.getMass() == 0 && thread_sun.getLuminosity() != 0) {
            thread_sun.setMass(luminosity_to_mass(thread_sun.getLuminosity()));
          }
          thread_sun.setEffTemp(temp_arg);
          thread_sun.setSpecType(type_arg);

          // Generate system name (optimized - avoid stringstream allocation)
          std::string sys_name = prognam + " " + std::to_string(seed_arg + sys_index * seed_increment);
          thread_sun.setName(sys_name);

          // Generate the stellar system
          generate_stellar_system(gen, thread_sun, false, nullptr, flag_char,
                                  sys_index, sys_name, 0.0, 0.0,
                                  ecc_coef_arg, inner_planet_factor_arg, do_gases,
                                  do_moons, *acc);

          // Accumulate statistics locally
          local_habitable += gen->sim_context.system_habitable;
          local_potentially_habitable += gen->sim_context.system_potentially_habitable;
          local_earthlike += gen->sim_context.system_earthlike;
          local_worlds += gen->sim_context.total_worlds;

          // Fold this system's habitability min/max into this worker's batch
          // slot (its own slot -> no shared write during generation). Captures
          // every generated system, including ones filtered from output below.
          batch_stats[thread_id]->mergeHabitabilityStatsFrom(gen->sim_context);

          // Check if this system should be output
          bool should_output = should_output_system(
              only_habitable, only_multi_habitable, only_jovian_habitable,
              only_earthlike, only_three_habitable, only_superterrans,
              only_potential_habitable,
              gen->sim_context.system_habitable,
              gen->sim_context.system_habitable_jovians,
              gen->sim_context.system_earthlike,
              gen->sim_context.system_habitable_superterrans,
              gen->sim_context.system_potentially_habitable);

          if (should_output && gen->sim_context.innermost_planet != nullptr) {
            // Generate filename for this system
            std::stringstream fname_ss;
            fname_ss << prognam << std::setfill('0') << std::setw(4)
                     << (seed_arg + sys_index * seed_increment);
            std::string file_name = fname_ss.str();

            // Store system data for later output
            // Transfer ownership of generator and accrete to SystemOutputData
            try {
              std::lock_guard<std::mutex> lock(g_output_mutex);
              g_pending_outputs.emplace_back(
                  gen, acc,
                  gen->sim_context.innermost_planet,
                  thread_sun,  // Pass the sun object
                  seed_arg + (sys_index * seed_increment),
                  sys_name,
                  file_name,
                  sys_index,
                  gen->sim_context.system_habitable,
                  gen->sim_context.system_earthlike,
                  gen->sim_context.system_potentially_habitable,
                  gen->sim_context.system_habitable_jovians,
                  gen->sim_context.system_habitable_superterrans);

              // Get new objects for next iteration (ownership transferred)
              gen = generator_pool.acquire();
              acc = accrete_pool.acquire();
            } catch (const std::bad_alloc& e) {
              std::cerr << "\nWarning: Memory exhausted storing system " << sys_name
                        << ". System will not be output.\n";
              // Keep using current gen/acc for next iteration
              // Clean up manually since we couldn't store
              acc->free_generations();
            }
          }
        }

        // Update global statistics once per thread (not per system)
        g_sim_context.total_habitable += local_habitable;
        g_sim_context.total_potentially_habitable += local_potentially_habitable;
        g_sim_context.total_earthlike += local_earthlike;
        g_sim_context.total_worlds += local_worlds;

        systems_generated += count;

        // Clear this thread's active RNG pointer before the pooled objects are
        // released, so no dangling pointer survives for thread reuse.
        set_active_random_context(nullptr);

        // Return objects to pools (if not transferred to output)
        generator_pool.release(gen);
        accrete_pool.release(acc);
      }));
    }

    // Wait for all batches to complete
    for (auto& future : futures) {
      future.wait();
    }

    // Reduce the per-batch habitability min/max into the run-wide totals now
    // that all workers have joined (single-threaded). g_sim_context's min/max
    // summary is what print_summary_statistics and the HTML summary read. Order
    // does not matter: min/max are commutative, so this is byte-identical to the
    // old inline-under-g_stats_mutex accumulation.
    for (auto& bs : batch_stats) {
      g_sim_context.mergeHabitabilityStatsFrom(*bs);
    }

    std::cerr << "Parallel generation complete: " << systems_generated << " systems generated\n";

    // Report pool statistics
    size_t gen_created = generator_pool.getTotalCreated();
    size_t acc_created = accrete_pool.getTotalCreated();
    if (gen_created > pool_capacity || acc_created > pool_capacity) {
      std::cerr << "Object pool growth: "
                << "generators=" << gen_created << "/" << pool_capacity
                << ", accrete=" << acc_created << "/" << pool_capacity << "\n";
    }

    // Sort systems by seed for consistent output ordering
    std::sort(g_pending_outputs.begin(), g_pending_outputs.end(),
              [](const SystemOutputData& a, const SystemOutputData& b) {
                return a.flag_seed < b.flag_seed;
              });

    // Output phase - process all stored systems with exception safety
    std::cerr << "Outputting " << g_pending_outputs.size() << " systems...\n";
    size_t output_count = 0;
    size_t failed_count = 0;

    for (auto& sys_data : g_pending_outputs) {
      output_count++;
      if (g_pending_outputs.size() > 10) {
        std::cerr << "\rOutputting system " << output_count << "/" << g_pending_outputs.size()
                  << " (seed " << sys_data.flag_seed << ")..." << std::flush;
      }

      try {
        // Temporarily restore globals for output functions
        planet* saved_innermost = innermost_planet;
        long saved_seed = flag_seed;
        sun saved_sun = the_sun_clone;

        innermost_planet = sys_data.innermost_planet;
        flag_seed = sys_data.flag_seed;
        the_sun_clone = sys_data.the_sun;

        // The describe_system serializers (text/JSON/CSV/SVG/HTML in display.cpp)
        // iterate the GLOBAL g_sim_context.planets vector, not their
        // innermost_planet argument. In parallel mode each system was built into
        // its own per-thread context, so rebuild the global vector from THIS
        // system's planet list before output. The output phase is single-threaded,
        // so mutating the global here is safe.
        std::vector<planet*> saved_planets = g_sim_context.planets;
        planet* saved_global_innermost = g_sim_context.innermost_planet;
        g_sim_context.innermost_planet = sys_data.innermost_planet;
        g_sim_context.buildPlanetVector();

        // Output system using centralized helper with all necessary parameters
        // Only pass csv_file pointer if it's open (for CSV/JSON formats)
        std::fstream* csv_ptr = (csv_file.is_open()) ? &csv_file : nullptr;
        output_system(sys_data.innermost_planet, out_format, do_gases, do_moons, sys_data.flag_seed,
                     csv_ptr, path, sys_data.file_name, prognam, url_path);

        // Restore globals
        innermost_planet = saved_innermost;
        flag_seed = saved_seed;
        the_sun_clone = saved_sun;
        g_sim_context.planets = saved_planets;
        g_sim_context.innermost_planet = saved_global_innermost;

      } catch (const std::exception& e) {
        std::cerr << "\nError outputting system " << sys_data.system_name
                  << " (seed " << sys_data.flag_seed << "): " << e.what() << "\n";
        failed_count++;
      }
      // Cleanup happens automatically via SystemOutputData destructor
    }

    if (failed_count > 0) {
      std::cerr << "\nWarning: " << failed_count << " system(s) failed to output\n";
    }

    g_pending_outputs.clear();
    if (output_count > 10) {
      std::cerr << "\n";  // Clear progress line
    }
    print_summary_statistics();
    // A failed output (e.g. an unwritable directory throwing from the display.cpp
    // open functions) must surface as a non-zero exit, not a silent success.
    return failed_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  // Sequential execution path (fallback for complex cases)
  for (index = 0; index < system_count; index++) {
    std::string system_name;
    std::string designation;
    std::string cp;
    long double outer_limit = NAN;
    long double inner_dust_limit = NAN;
    int sys_no = 0;
    bool has_known_planets = false;
    planet *seed_planets = nullptr;
    bool use_seed_system = false;
    bool in_celestia = false;

    init();

    outer_limit = 0;

    if (do_catalog || (sys_no_arg != 0)) {
      if (sys_no_arg != 0) {
        sys_no = sys_no_arg;
      } else {
        if (index >= catalog_count) {
          sys_no = ((index - 1) % (catalog_count - 1)) + 1;
        } else {
          sys_no = index;
        }
      }

      sys_inc = cat_arg[sys_no].getInc();
      sys_an = cat_arg[sys_no].getAn();

      has_known_planets = cat_arg[sys_no].getKnownPlanets() != nullptr;

      if ((use_known_planets || no_generate) && has_known_planets) {
        seed_planets = cat_arg[sys_no].getKnownPlanets();
        use_seed_system = no_generate;
      } else {
        seed_planets = nullptr;
        use_seed_system = false;
      }

      // in_celestia = cat_arg[sys_no].getInCelestia();

      // std::cout << cat_arg[sys_no].getMass() << " " <<
      // cat_arg[sys_no].getLuminosity() << " " << cat_arg[sys_no].getEffTemp()
      // << " " << cat_arg[sys_no].getSpecType() << "\n";

      the_sun.setMass(cat_arg[sys_no].getMass());
      the_sun.setLuminosity(cat_arg[sys_no].getLuminosity());
      the_sun.setEffTemp(cat_arg[sys_no].getEffTemp());
      the_sun.setSpecType(cat_arg[sys_no].getSpecType());
      if (cat_arg[sys_no].getIsCircumbinary()) {
        the_sun.setIsCircumbinary(true);
        the_sun.setSecondaryMass(cat_arg[sys_no].getMass2());
        the_sun.setSecondaryLuminosity(cat_arg[sys_no].getLuminosity2());
        the_sun.setSecondaryEffTemp(cat_arg[sys_no].getEffTemp2());
        the_sun.setSecondarySpecType(cat_arg[sys_no].getSpecType2());
        the_sun.setSeperation(cat_arg[sys_no].getDistance());
        the_sun.setEccentricity(cat_arg[sys_no].getEccentricity());
      } else {
        the_sun.setIsCircumbinary(false);
      }

      if (do_catalog || sys_name_arg.empty()) {
        system_name = cat_arg[sys_no].getName();
        designation = cat_arg[sys_no].getDesig();
      } else {
        system_name = sys_name_arg;
        designation = sys_name_arg;
      }

      ss.str("");
      ss << designation << "-" << flag_seed;
      file_name = ss.str();

      if (cat_arg[sys_no].getMass2() == 0) {
        inner_dust_limit = cat_arg[sys_no].getDistance();
      } else {
        inner_dust_limit = 0;
      }

      if (cat_arg[sys_no].getMass2() > .001 &&
          !cat_arg[sys_no].getIsCircumbinary()) {
        long double m1 = the_sun.getMass();
        long double m2 = cat_arg[sys_no].getMass2();
        long double mu = m2 / (m1 + m2);
        long double e = cat_arg[sys_no].getEccentricity();
        long double a = cat_arg[sys_no].getDistance();

        outer_limit = (0.464 + (-0.380 * mu) + (-0.631 * e) + (0.586 * mu * e) +
                       (0.150 * pow2(e)) + (-0.198 * mu * pow2(e))) *
                      a;
      } else {
        outer_limit = 0;
      }
    } else if (reuse_solar_system) {
      /*system_name = "Earth-";
      system_name.append(float_to_string(earth->getMass() *
      SUN_MASS_IN_EARTH_MASSES));*/
      ss.str("");
      ss << "Earth-" << (earth->getMass() * SUN_MASS_IN_EARTH_MASSES);
      system_name = ss.str();
      file_name = designation = system_name;

      outer_limit = 0;
    } else {
      std::stringstream ss;
      if (!sys_name_arg.empty()) {
        system_name = sys_name_arg;
        designation = sys_name_arg;
      } else {
        ss.str("");
        ss << prognam << " " << flag_seed << "-" << the_sun.getMass();
        system_name = ss.str();
        designation = prognam;
      }
      ss.str("");
      ss << designation << "-" << flag_seed << "-" << the_sun.getMass();
      file_name = ss.str();
    }

    if (compainion_mass_arg > .001) {
      long double m1 = the_sun.getMass();
      long double m2 = compainion_mass_arg;
      long double mu = m2 / (m1 + m2);
      long double e = compainion_eccentricity_arg;
      long double a = compainion_distant_arg;

      if (is_circumbinary) {
        the_sun.setIsCircumbinary(true);
        the_sun.setSecondaryMass(m2);
        the_sun.setSecondaryLuminosity(compainion_lum_arg);
        the_sun.setSecondaryEffTemp(compainion_eff_arg);
        the_sun.setSecondarySpecType(companion_spec_arg);
        the_sun.setEccentricity(e);
        the_sun.setSeperation(a);
        outer_limit = 0;
      } else {
        outer_limit = (0.464 + (-0.380 * mu) + (-0.631 * e) + (0.586 * mu * e) +
                       (0.150 * pow2(e)) + (-0.198 * mu * pow2(e))) *
                      a;
      }
    }

    the_sun.setName(system_name);

    if (((flag_verbose & 0x0400) != 0) && outer_limit > 0.0) {
      std::cerr << system_name << ", Outer Limit: " << toString(outer_limit) << "\n";
    }
    if (system_count == 1 && !filename_arg.empty()) {
      file_name = filename_arg;
    }

    file_name = replaceStrChar(file_name, ' ', '-');
    file_name = replaceStrChar(file_name, '\'', '-');

    earthlike = habitable = habitable_jovians = habitable_superterrans =
        potential_habitable = 0;

    if (reuse_solar_system) {
      seed_planets = mercury;
      use_seed_system = true;
    } else if (use_solar_system) {
      if (index == 0) {
        seed_planets = mercury;
        use_seed_system = true;
      } else {
        use_seed_system = false;
        if (!use_known_planets) {
          seed_planets = nullptr;
        }
        if (has_known_planets && use_known_planets && no_generate) {
          use_seed_system = true;
        }
      }
    }

    for (int &type_count : type_counts) {
      type_count = 0;
    }
    type_count = 0;
    // std::cout << index << "\n";
    the_sun_clone = the_sun;

    // Reset generator and accrete for clean state (matches parallel path)
    g_generator.reset();
    myAccreteObject.reset();
    g_generator.random_context.setSeed(flag_seed + (index * seed_increment));
    myAccreteObject.setRandomContext(&g_generator.random_context);

    generate_stellar_system(&g_generator, the_sun, use_seed_system, seed_planets, flag_char,
                            sys_no, system_name, inner_dust_limit, outer_limit,
                            ecc_coef_arg, inner_planet_factor_arg, do_gases,
                            do_moons, myAccreteObject);

    // If planet generation failed (e.g., zero stellar mass), skip this system
    if (innermost_planet == nullptr) {
      continue;
    }

    planet *a_planet = nullptr;
    int counter = 0;
    int wt_type_count = calculate_weighted_type_count(type_counts, type_count);
    int norm_type_count = 0;

// why is there an empty for loop here? will investigate old versions to see
// what used to be here
    // for (a_planet = innermost_planet, counter = 0; a_planet != nullptr;
    //      a_planet = a_planet->next_planet, counter++) {
    //   ;
    // }

    norm_type_count = wt_type_count - (counter - type_count);

    if (max_type_count < norm_type_count) {
      max_type_count = norm_type_count;
    }

    if ((flag_verbose & 0x10000) != 0) {
      std::cerr << "System " << flag_seed << " - " << system_name << " (-s"
           << flag_seed << " -" << flag_char << sys_no << ") has " << type_count
           << " types out of " << counter << " planets. [" << norm_type_count
           << "]\n";
    }

    g_sim_context.total_habitable += habitable;
    g_sim_context.total_potentially_habitable += potential_habitable;
    g_sim_context.total_earthlike += earthlike;
    if (should_output_system(only_habitable, only_multi_habitable, only_jovian_habitable,
                            only_earthlike, only_three_habitable, only_superterrans,
                            only_potential_habitable, habitable, habitable_jovians,
                            earthlike, habitable_superterrans, potential_habitable)) {
      std::string system_url;
      std::string svg_url;

      ss.str("");
      ss << url_path << subdir << file_name << ".html";
      system_url = ss.str();

      ss.str("");
      ss << url_path << subdir << file_name << ".svg";
      svg_url = ss.str();
      switch (out_format) {
        case ffSVG:
          create_svg_file(innermost_planet, path, file_name, ".svg", prognam,
                          do_moons);
          break;
        case ffHTML:
          // refresh_file_stream(thumbnails, path, thumbnail_file, ".html");

          if (graphic_format == gfSVG) {
            create_svg_file(innermost_planet, path, file_name, ".svg", prognam,
                            do_moons);
          }
          html_thumbnails(innermost_planet, thumbnails, system_name, url_path,
                          system_url, svg_url, file_name, false, true, false,
                          do_moons, graphic_format, do_gases);
          open_html_file(system_name, flag_seed, path, url_path, file_name,
                         ".html", prognam, html_file);
          html_thumbnails(innermost_planet, html_file, system_name, url_path,
                          system_url, svg_url, file_name, true, false, true,
                          do_moons, graphic_format, do_gases);
          html_describe_system(innermost_planet, do_gases, do_moons, url_path,
                               html_file);
          close_html_file(html_file);
          break;
        case ffTEXT:
          text_describe_system(innermost_planet, do_gases, flag_seed, do_moons);
          break;
        case ffCSV:
        case ffCSVdl:
          csv_describe_system(csv_file, innermost_planet, do_gases, flag_seed,
                              do_moons);
          break;
        case ffJSON:
          jsonDescribeSystem(csv_file, innermost_planet, do_gases, flag_seed,
                              do_moons);
          break;
        case ffCELESTIA:
          celestia_describe_system(innermost_planet, designation, system_name,
                                   flag_seed, sys_inc, sys_an, do_moons);
          break;
      }
      if (habitable > 1 && ((flag_verbose & 0x0001) != 0)) {
        std::cerr << "System " << flag_seed << " - " << system_name << " (-s"
             << flag_seed << " -" << flag_char << sys_no << ") has "
             << habitable << " planets with breathable atmospheres.\n";
      }
    }

    if (!(use_solar_system && index == 0)) {
      flag_seed += seed_increment;
    }

    if (reuse_solar_system) {
      earth->setDustMass(earth->getDustMass() + EM(inc_mass));
    }

    if (!use_seed_system) {
      myAccreteObject.free_generations();
    } else {
      planet *node = nullptr;
      planet *next = nullptr;
      for (planet* ptr : g_sim_context.planets) {
        ptr->setImf(0);
        ptr->setRmf(0);
        ptr->setCmf(0);
        if (do_moons) {
          for (node = ptr->first_moon; node != nullptr; node = next) {
            next = node->next_planet;
            if (node->getDeletable()) {
              delete node;
            } else {
              node->setMoonA(0);
              node->setMoonE(0);
              node->setImf(0);
              node->setRmf(0);
              node->setCmf(0);
              node->next_planet = node->reconnect_to;
            }
            // std::cout << "Deleted World\n";
          }
          ptr->first_moon = ptr->first_moon_backup;
        }
      }
    }
  }

  print_summary_statistics();

  if (system_count > 1) {
    if (out_format == ffHTML) {
      if (do_gases) {
        html_thumbnail_totals(thumbnails);
      }
      close_html_file(thumbnails);
    }
  }
  ZoneScoped;
  return EXIT_SUCCESS;
}

/**
 * @brief init
 * 
 */
void init() {
  if (flag_seed == 0) {
    time_t temp_time = 0;
    auto seed = (unsigned)(time(&temp_time));
    flag_seed = seed;
  }
  g_random_context.setSeed(flag_seed);
  system_seed = flag_seed;
  ZoneScoped;
}

/**
 * @brief generate stellar system
 * 
 * @param the_sun 
 * @param use_seed_system 
 * @param seed_system 
 * @param flag_char 
 * @param sys_no 
 * @param system_name 
 * @param inner_dust_limit 
 * @param outer_planet_limit 
 * @param ecc_coef 
 * @param inner_planet_factor 
 * @param do_gases 
 * @param do_moons 
 * @param myAccreteObject 
 */
void generate_stellar_system(StarGenerator* gen, sun &the_sun, bool use_seed_system,
                             planet *seed_system, const std::string& flag_char, int sys_no,
                             const std::string& system_name, long double inner_dust_limit,
                             long double outer_planet_limit,
                             long double ecc_coef,
                             long double inner_planet_factor, bool do_gases,
                             bool do_moons, accrete &myAccreteObject) {
  do_gases = (gen->config.flags & fDoGases) != 0;
  do_moons = (gen->config.flags & fDoMoons) != 0;
  gen->sim_context.system_counter++;
  long double outer_dust_limit = NAN;

  if (!the_sun.getIsCircumbinary()) {
    outer_dust_limit = myAccreteObject.stellar_dust_limit(the_sun.getMass());
  } else {
    outer_dust_limit = myAccreteObject.stellar_dust_limit(the_sun.getCombinedMass());
  }

  if (the_sun.getLuminosity() == 0) {
    the_sun.setLuminosity(mass_to_luminosity(the_sun.getMass()));
  }
  if (the_sun.getMass() == 0) {
    the_sun.setMass(luminosity_to_mass(the_sun.getLuminosity()));
  }

  if (the_sun.getMass() == 0) {
    // std::cout << system_name << " " << sys_no << "\n";
  }

  if (the_sun.getEffTemp() == 0) {
    // No spectral type or effective temperature was supplied (e.g. a mass- or
    // luminosity-specified star). Derive the effective temperature from the
    // physical mass-luminosity-temperature relation rather than from a defaulted
    // spectral type, which previously floored it at ~273 K ("Y9V") and corrupted
    // the temperature-dependent Kopparapu habitable zone. setEffTemp() then
    // lazily derives the matching spectral type. See
    // research/modern/07-stellar-relations-binaries.md.
    the_sun.setEffTemp(
        mass_to_eff_temp(the_sun.getMass(), the_sun.getLuminosity()));
  }
  gen->config.max_age = gen->config.max_age_backup;
  if (use_seed_system) {
    gen->sim_context.innermost_planet = seed_system;

    long double max_age_of_star = the_sun.getLife();
    if (max_age_of_star > 10E9) {
      max_age_of_star = 10E9;
    }
    if (gen->config.max_age > max_age_of_star) {
      gen->config.max_age = max_age_of_star;
    }
    if (gen->config.min_age > max_age_of_star) {
      gen->config.min_age = max_age_of_star;
    }
    // std::cout << min_age << " " << max_age << "\n";
    the_sun.setAge(random_number(gen->config.min_age, gen->config.max_age));
  } else {
    long double max_age_of_star = the_sun.getLife();
    if (max_age_of_star > 10E9) {
      max_age_of_star = 10E9;
    }
    if (gen->config.max_age > max_age_of_star) {
      gen->config.max_age = max_age_of_star;
    }
    if (gen->config.min_age > max_age_of_star) {
      gen->config.min_age = max_age_of_star;
    }
    // std::cout << min_age << " " << max_age << "\n";
    gen->sim_context.innermost_planet =
        myAccreteObject.dist_planetary_masses(the_sun, inner_dust_limit, outer_dust_limit,
                              outer_planet_limit, gen->sim_context.dust_density_coeff, ecc_coef,
                              inner_planet_factor, seed_system, do_moons);
    if (gen->sim_context.innermost_planet == nullptr) {
      // Failed to generate planets (e.g., zero stellar mass)
      return;
    }
    the_sun.setAge(random_number(gen->config.min_age, gen->config.max_age));
  }
  // Publish THIS system's fully-initialised sun (mass, luminosity, effTemp, age
  // are all set by this point) into the per-thread active-sun clone, so the
  // gas-radius and habitability helpers in enviro.cpp/gas_radius_helpers.cpp read
  // this worker's sun rather than a sun shared across threads. This is what makes
  // parallel generation deterministic; it also pre-computes effTemp so the later
  // getEffTemp() on the clone is a pure read (no lazy-init write race).
  the_sun_clone = the_sun;

  // Build planet vector for efficient iteration (replacing linked list traversal)
  // MUST be called before generate_planets() which iterates over the vector
  gen->sim_context.buildPlanetVector();

  generate_planets(gen, the_sun, !use_seed_system, flag_char, sys_no,
                   system_name, do_gases, do_moons);

  ZoneScoped;
}

/**
 * @brief generate planets
 * 
 * @param the_sun 
 * @param random_tilt 
 * @param flag_char 
 * @param sys_no 
 * @param system_name 
 * @param do_gases 
 * @param do_moons 
 */
void generate_planets(StarGenerator* gen, sun &the_sun, bool random_tilt, const std::string &flag_char,
                      int sys_no, const std::string &system_name, bool do_gases,
                      bool do_moons) {
    do_gases = (gen->config.flags & fDoGases) != 0;
  do_moons = (gen->config.flags & fDoMoons) != 0;
  planet *the_planet = nullptr;
  int planet_no = 1;
  planet *moon = nullptr;
  int moons = 0;

  for (planet* the_planet : gen->sim_context.planets) {
    std::string planet_id;
    std::stringstream ss;

    ss.str("");
    ss << system_name << " (-s" << toString(gen->random_context.seed) << " -" << flag_char
       << toString(sys_no) << ") " << toString(planet_no);
    planet_id = ss.str();
    // std::cout << planet_id << "\n";
    if (!(the_planet->getKnownRadius() > 0)) {
      the_planet->setImf(0);
      the_planet->setRmf(0);
      the_planet->setCmf(0);
    }

    if (!the_sun.getIsCircumbinary()) {
      generate_planet(gen, the_planet, planet_no, the_sun, random_tilt, planet_id,
                      do_gases, do_moons, false, the_sun.getMass());
    } else {
      generate_planet(gen, the_planet, planet_no, the_sun, random_tilt, planet_id,
                      do_gases, do_moons, false, the_sun.getCombinedMass());
    }

    check_planet(gen, the_planet, planet_id, false);

    if (do_moons) {
      moons = 1;
      for (moon = the_planet->first_moon; moon != nullptr; moon = moon->next_planet) {
        std::string moon_id;

        ss.str("");
        ss << planet_id << "-" << toString(moons);
        moon_id = ss.str();

        check_planet(gen, moon, moon_id, true);
        moons++;
      }
    }
    planet_no++;
  }
  ZoneScoped;
}

/**
 * @brief Calculate atmospheric gas loss for rocky planets (H2 and He)
 * @param the_planet Planet to calculate gas loss for
 * @param the_sun Parent star
 * @param planet_id Planet identifier for logging
 */
static void calculate_gas_loss(planet *the_planet, sun &the_sun, const std::string &planet_id) {
  if ((the_planet->getGasMass() / the_planet->getMass()) <= 0.000001) {
    return;  // No significant gas to lose
  }

  long double h2_mass = the_planet->getGasMass() * 0.85;
  long double he_mass = (the_planet->getGasMass() - h2_mass) * 0.999;

  long double h2_loss = 0.0;
  long double he_loss = 0.0;

  long double h2_life = gas_life(MOL_HYDROGEN, the_planet);
  long double he_life = gas_life(HELIUM, the_planet);

  if (h2_life < the_sun.getAge()) {
    h2_loss = ((1.0 - (1.0 / exp(the_sun.getAge() / h2_life))) * h2_mass);
    the_planet->setGasMass(the_planet->getGasMass() - h2_loss);
    the_planet->setSurfAccel(acceleration(the_planet));
    the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));
  }

  if (he_life < the_sun.getAge()) {
    he_loss = ((1.0 - (1.0 / exp(the_sun.getAge() / he_life))) * he_mass);
    the_planet->setGasMass(the_planet->getGasMass() - he_loss);
    the_planet->setSurfAccel(acceleration(the_planet));
    the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));
  }

  if ((h2_loss + he_loss) > .000001 && ((flag_verbose & 0x0080) != 0)) {
    std::cerr << planet_id << "\tLosing gas: H2: "
         << toString(h2_loss * SUN_MASS_IN_EARTH_MASSES)
         << " EM, He: " << toString(he_loss * SUN_MASS_IN_EARTH_MASSES)
         << " EM\n";
  }
}

/**
 * @brief Finalize gas giant properties (temperature, albedo, atmosphere)
 * @param the_planet Planet to finalize
 * @param the_sun Parent star
 * @param planet_id Planet identifier for logging
 * @param do_gases Whether to calculate detailed atmospheric gases
 * @param force_gas_giant Whether this is a forced gas giant classification
 * @param parent_mass Mass of parent body (for moons)
 * @param is_moon Whether this is a moon
 */
static void finalize_gas_giant_properties(StarGenerator* gen, planet *the_planet, sun &the_sun,
                                         const std::string &planet_id, bool do_gases,
                                         bool force_gas_giant, long double parent_mass,
                                         bool is_moon) {
  the_planet->setGreenhouseEffect(false);
  the_planet->setVolatileGasInventory(INCREDIBLY_LARGE_NUMBER);
  the_planet->setSurfPressure(INCREDIBLY_LARGE_NUMBER);
  the_planet->setBoilPoint(INCREDIBLY_LARGE_NUMBER);
  the_planet->setGreenhsRise(0);
  the_planet->setHydrosphere(1);
  the_planet->setCloudCover(1);
  the_planet->setIceCover(0);
  the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));

  if (force_gas_giant) {
    the_planet->setMolecWeight(about(0.5, 0.1));
  } else {
    the_planet->setMolecWeight(min_molec_weight(the_planet));
  }

  gas_giant_temperature_albedo(the_planet, parent_mass, is_moon);

  the_planet->setDensity(
      volume_density(the_planet->getMass(), the_planet->getRadius()));

  the_planet->setEstimatedTerrTemp(est_temp(
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      the_planet->getA(), EARTH_ALBEDO));

  if (do_gases) {
    the_sun = the_planet->getTheSun();
    calculate_gases(the_sun, the_planet, planet_id);
  }

  long double temp = the_planet->getEstimatedTerrTemp();

  if (is_habitable_jovian(the_planet)) {
    gen->sim_context.system_habitable_jovians++;

    if ((flag_verbose & 0x8000) != 0) {
      std::string planet_type = type_string(the_planet);
      std::string moon_string = the_planet->first_moon != nullptr ? " WITH MOON" : "";

      std::cerr << planet_id << "\t" << planet_type << " ("
           << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << " EM " << toString(the_sun.getAge() / 1.0E9) << " By)"
           << moon_string << " with earth-like temperature ("
           << toString(temp - FREEZING_POINT_OF_WATER) << " C, "
           << toString(32.0 + ((temp - FREEZING_POINT_OF_WATER) * 1.8))
           << " F, " << (temp - EARTH_AVERAGE_KELVIN) << " C Earth).\n";
    }
  }
}

/**
 * @brief Generate moons for a planet
 * @param the_planet Planet to generate moons for
 * @param planet_no Planet number for ID generation
 * @param the_sun Parent star
 * @param random_tilt Whether to randomize tilt
 * @param planet_id Planet identifier for logging
 * @param do_gases Whether to calculate detailed atmospheric gases
 * @param do_moons Whether to generate moons
 * @param is_moon Whether this planet is itself a moon
 */
/**
 * @brief Calculate the Hill sphere radius for a planet or moon
 * @param semi_major_axis Semi-major axis in AU
 * @param body_mass Mass of the body in solar masses
 * @param primary_mass Mass of the primary body in solar masses
 * @return Hill sphere radius in km
 */
static long double calculate_hill_sphere(long double semi_major_axis,
                                         long double body_mass,
                                         long double primary_mass) {
  return semi_major_axis * KM_PER_AU *
         std::pow(body_mass / (3.0 * primary_mass), 1.0 / 3.0);
}

/**
 * @brief Calculate the Roche limit for a moon around a planet
 * @param planet_radius Radius of the planet in km
 * @param planet_density Density of the planet in g/cc
 * @param moon_density Density of the moon in g/cc
 * @return Roche limit in km
 */
static long double calculate_roche_limit(long double planet_radius,
                                         long double planet_density,
                                         long double moon_density) {
  return 2.44 * planet_radius *
         std::pow(planet_density / moon_density, 1.0 / 3.0);
}

/**
 * @brief Check if a proposed moon orbit collides with existing moons
 * @param the_planet The planet hosting the moons
 * @param distance Proposed orbital distance in km
 * @param moon_mass Mass of the proposed moon in solar masses
 * @param planet_mass Mass of the planet in solar masses
 * @return true if collision detected, false if orbit is clear
 */
static bool check_moon_collision(planet *the_planet, long double distance,
                                long double moon_mass, long double planet_mass) {
  planet *the_moon = nullptr;
  long double hill_sphere2 = 0;
  long double temp_hill_sphere = 0;

  temp_hill_sphere = distance * std::pow(moon_mass / (3.0 * planet_mass), 1.0 / 3.0);

  for (the_moon = the_planet->first_moon; the_moon != nullptr;
       the_moon = the_moon->next_planet) {
    hill_sphere2 = the_moon->getMoonA() * KM_PER_AU *
                   std::pow(the_moon->getMass() / (3.0 * planet_mass), 1.0 / 3.0);

    if (((the_moon->getMoonA() * KM_PER_AU) >= (distance - temp_hill_sphere) &&
         (the_moon->getMoonA() * KM_PER_AU) <= (distance + temp_hill_sphere)) ||
        (distance >= ((the_moon->getMoonA() * KM_PER_AU) - hill_sphere2) &&
         distance <= ((the_moon->getMoonA() * KM_PER_AU) + hill_sphere2))) {
      return true;  // Collision detected
    }
  }
  return false;  // No collision
}

static void generate_moons(StarGenerator* gen, planet *the_planet, int planet_no, sun &the_sun,
                          bool random_tilt, const std::string &planet_id,
                          bool do_gases, bool do_moons, bool is_moon) {
  if (!do_moons || is_moon) {
    return;
  }

  bool skip_minor_moons = false;
  long double moon_mass = 0.0;
  int n = 0;
  std::stringstream ss;
  std::string moon_id;
  long double temp_hill_sphere = 0;
  long double min_rand_distance = 0;

  if (the_planet->first_moon != nullptr) {
    planet *ptr = nullptr;
    planet *prev = nullptr;
    planet *next_moon = nullptr;

    // reset moon distances for predefined moons
    /*for (n = 0, ptr = the_planet->first_moon; ptr != NULL; ptr = next_moon)
    {
      next_moon = ptr->next_planet;
      ptr->setMoonA(0);
      ptr->setMoonE(0);
    }*/

    long double hill_sphere = calculate_hill_sphere(
        the_planet->getA(), the_planet->getMass(), the_sun.getMass());

    for (n = 0, ptr = the_planet->first_moon; ptr != nullptr; ptr = next_moon)
    // for (n = 0; n < the_planet->getMoonCount(); n++)
    {
      next_moon = ptr->next_planet;
      moon_mass += ptr->getMass() * SUN_MASS_IN_EARTH_MASSES;

      long double roche_limit = 0.0;
      long double distance = 0.0;
      long double eccentricity = 0.0;

      ptr->setA(the_planet->getA());
      ptr->setE(the_planet->getE());
      ptr->setOrbitZone(the_planet->getOrbitZone());

      assign_composition(ptr, the_sun, true);

      ss.str("");
      ss << planet_id << "-" << toString(n + 1);
      moon_id = ss.str();
      ss.str("");

      // generate_planet(ptr, n, the_sun, random_tilt, moon_id, do_gases,
      // do_moons, true, the_planet->getMass());
      ptr->setRadius(radius_improved(ptr->getMass(), ptr->getImf(),
                                     ptr->getRmf(), ptr->getCmf(), false,
                                     ptr->getOrbitZone(), ptr));
      ptr->setDensity(volume_density(ptr->getMass(), ptr->getRadius()));

      roche_limit = calculate_roche_limit(the_planet->getRadius(),
                                          the_planet->getDensity(),
                                          ptr->getDensity());

      if ((roche_limit * 1.5) < (hill_sphere / 3.0) &&
          (hill_sphere / 3.0) > (the_planet->getRadius() * 2.5)) {
        bool done = false;
        bool bad_place = false;
        bool to_many_tries = false;
        planet *the_moon = nullptr;
        long double hill_sphere2 = 0;
        int tries = 0;
        while (!done) {
          tries++;
          if (tries > 20) {
            to_many_tries = true;
            break;
          }
          bad_place = false;

          if ((roche_limit * 1.5) > (the_planet->getRadius() * 2.5)) {
            min_rand_distance = roche_limit * 1.5;
          } else {
            min_rand_distance = the_planet->getRadius() * 2.5;
          }

          distance = random_number(min_rand_distance, hill_sphere / 3.0);
          eccentricity = random_number(
              0, 0.01);  // I know I should use random_eccentricity here but I
                         // want to prevent orbits from crossing each other

          bad_place = check_moon_collision(the_planet, distance, ptr->getMass(),
                                           the_planet->getMass());
          if (!bad_place) {
            done = true;
          }
        }

        if (to_many_tries) {
          bool dont_break = false;
          if ((flag_verbose & 0x1000) != 0) {
            std::cerr << "  " << planet_id << ": can't fit anymore moons!\n";
          }
          skip_minor_moons = true;
          planet *node = nullptr;
          planet *next = nullptr;
          for (node = ptr; node != nullptr; node = next) {
            next = node->next_planet;
            if (node->getDeletable()) {
              delete node;
            } else {
              node->next_planet = node->reconnect_to;
              n = 0;
              the_planet->first_moon = the_planet->first_moon_backup;
              // ptr = the_planet->first_moon;
              dont_break = true;
              continue;
            }
          }
          if (prev != nullptr) {
            prev->next_planet = nullptr;
          } else {
            the_planet->first_moon = nullptr;
          }
          /*for (int m = n; m < the_planet->getMoonCount(); m++)
          {
            the_planet->deleteMoon(m);
          }*/
          if (!dont_break) {
            break;
          }
        } else {
          ptr->setMoonA(distance / KM_PER_AU);
          ptr->setMoonE(eccentricity);

          ptr->setOrbPeriod(
              period(ptr->getMoonA(), ptr->getMass(), the_planet->getMass()));
          ptr->setType(tUnknown);
          generate_planet(gen, ptr, n, the_sun, random_tilt, moon_id, do_gases,
                          do_moons, true, the_planet->getMass());

          if ((flag_verbose & 0x40000) != 0) {
            std::cerr << "   Roche limit: R = "
                 << toString(the_planet->getRadius())
                 << ", rM = " << toString(the_planet->getDensity())
                 << ", rm = " << toString(ptr->getDensity()) << " -> "
                 << toString(roche_limit) << " km\n";
            std::cerr << "   Hill Sphere: a = "
                 << toString(the_planet->getA() * KM_PER_AU) << ", m = "
                 << toString(the_planet->getMass() * SOLAR_MASS_IN_KILOGRAMS)
                 << ", M = "
                 << toString(the_sun.getMass() * SOLAR_MASS_IN_KILOGRAMS)
                 << " -> " << toString(hill_sphere) << " km\n";
            std::cerr << moon_id << " Moon orbit: a = "
                 << toString(ptr->getMoonA() * KM_PER_AU)
                 << " km, e = " << toString(ptr->getMoonE()) << "\n";
          }

          if ((flag_verbose & 0x1000) != 0) {
            std::cerr << "  " << planet_id << ": ("
                 << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
                 << " EM) " << n << " "
                 << toString(ptr->getMass() * SUN_MASS_IN_EARTH_MASSES)
                 << " EM\n";
          }
          prev = ptr;
        }
      } else {
        if ((flag_verbose & 0x1000) != 0) {
          std::cerr << "  " << planet_id
               << " lost moons due to gravity of the sun!\n";
        }
        skip_minor_moons = true;
        // delete moons
        planet *node = nullptr;
        planet *next = nullptr;
        for (node = the_planet->first_moon; node != nullptr; node = next) {
          next = node->next_planet;
          if (node->getDeletable()) {
            delete node;
          } else {
            node->next_planet = node->reconnect_to;
            // n = 0;
            the_planet->first_moon = the_planet->first_moon_backup;
            // ptr = the_planet->first_moon;
            continue;
          }
        }
        n = 0;
        the_planet->first_moon = nullptr;
        /*for (int m = 0; m < the_planet->getMoonCount(); m++)
         *	    {
         *	      the_planet->deleteMoon(m);
         }*/
        break;
      }
    }
  }

  if (skip_minor_moons) {
    the_planet->setMinorMoons(0);
  } else {
    the_planet->setMinorMoons(
        poisson(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES));
  }

  if (the_planet->getMinorMoons() > 0) {
    long double max_total_moon_mass = NAN;
    long double remaining_moon_mass = NAN;
    long double new_moon_mass = NAN;
    long double new_moon_gas_mass = NAN;
    long double min_new_moon_mass = 1.0 / 100000000000.0;
    planet *tmp = nullptr;
    planet *ref = nullptr;
    int attempts = 0;
    bool done2 = false;
    bool too_many_tries = false;
    bool bad_place2 = false;
    long double hill_sphere3 = calculate_hill_sphere(
        the_planet->getA(), the_planet->getMass(), the_sun.getMass());
    long double hill_sphere4 = 0;
    long double roche_limit2 = 0;
    int tries2 = 0;
    if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 1000.0) {
      max_total_moon_mass =
          the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * 0.05;
    } else if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 10.0) {
      max_total_moon_mass =
          the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * (1.5 / 10000.0);
    } else if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 0.5) {
      if (the_planet->getRadius() > 15000.0) {
        max_total_moon_mass =
            the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * 0.01;
      } else {
        max_total_moon_mass =
            the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * 0.05;
      }
    } else {
      max_total_moon_mass =
          the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * (1.5 / 10000.0);
    }
    remaining_moon_mass = max_total_moon_mass - moon_mass;

    // get the pointer to the last moon in the list
    if (the_planet->first_moon != nullptr) {
      tmp = the_planet->first_moon;
      while (true) {
        if (tmp->next_planet == nullptr) {
          break;
        }
        tmp = tmp->next_planet;
      }
    } else {
      tmp = nullptr;
    }

    n++;
    int moon_count = 0;
    while (true) {
      done2 = false;
      ss.str("");
      ss << planet_id << "-" << toString(n);
      moon_id = ss.str();
      ss.str("");

      if (remaining_moon_mass <= 0.0 || attempts > 5 ||
          moon_count > the_planet->getMinorMoons() ||
          (hill_sphere3 / 3.0) <= the_planet->getRadius()) {
        break;
      }
      planet *new_moon = nullptr;
      new_moon = new planet();
      new_moon->setPlanetNo(n);
      new_moon->setOrbitZone(the_planet->getOrbitZone());
      new_moon->setA(the_planet->getA());
      new_moon->setE(the_planet->getE());
      new_moon->setHzd(the_planet->getHzd());

      double r1 = randf();
      double maxln = exp(1.0);
      double r2 = log(1.0 + (maxln * r1)) / 1.33;

      new_moon_mass = r2 * remaining_moon_mass;

      if (new_moon_mass < min_new_moon_mass) {
        new_moon_mass = min_new_moon_mass;
      }

      if (new_moon_mass > 10.0) {
        new_moon_gas_mass = new_moon_mass * random_number(0.05, 1.0);
        new_moon->setGasGiant(true);
      } else {
        new_moon_gas_mass = 0;
        new_moon->setGasGiant(false);
      }
      new_moon->setDustMass((new_moon_mass - new_moon_gas_mass) /
                            SUN_MASS_IN_EARTH_MASSES);
      new_moon->setGasMass(new_moon_gas_mass / SUN_MASS_IN_EARTH_MASSES);

      assign_composition(new_moon, the_sun, true);

      // generate_planet(new_moon, n, the_sun, random_tilt, moon_id, do_gases,
      // do_moons, true, the_planet->getMass());
      new_moon->setRadius(radius_improved(
          new_moon->getMass(), new_moon->getImf(), new_moon->getRmf(),
          new_moon->getCmf(), false, new_moon->getOrbitZone(), new_moon));
      new_moon->setDensity(
          volume_density(new_moon->getMass(), new_moon->getRadius()));

      long double distance2 = NAN;
      long double eccentricity2 = NAN;

      roche_limit2 = calculate_roche_limit(the_planet->getRadius(),
                                           the_planet->getDensity(),
                                           new_moon->getDensity());
      if ((roche_limit2 * 1.5) >= (hill_sphere3 / 3.0)) {
        if ((flag_verbose & 0x1000) != 0) {
          std::cerr << "  " << planet_id << ": Can't add anymore moons!\n";
        }
        delete new_moon;
        break;
      }
      tries2 = 0;
      while (!done2) {
        tries2++;
        bad_place2 = false;
        too_many_tries = false;
        distance2 = random_number(roche_limit2 * 1.5, hill_sphere3 / 3.0);
        eccentricity2 = random_number(
            0, 0.01);  // I know I should use random_eccentricity here but I
                       // want to prevent orbits from crossing each other
        if ((flag_verbose & 0x1000) != 0) {
          std::cerr << "  " << planet_id << ": Attempting to add moon ("
               << toString(new_moon->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " EU) at " << toString(distance2)
               << " km with eccentricity of " << toString(eccentricity2)
               << "\n";
        }

        for (ref = the_planet->first_moon; ref != nullptr;
             ref = ref->next_planet) {
          hill_sphere4 =
              ref->getMoonA() * KM_PER_AU *
              std::pow(ref->getMass() / (3.0 * the_planet->getMass()), 1.0 / 3.0);
          temp_hill_sphere =
              distance2 *
              std::pow(new_moon->getMass() / (3.0 * the_planet->getMass()),
                  1.0 / 3.0);
          // std::cout << toString(distance2) << " " << toString(distance2 -
          // temp_hill_sphere) << " " << toString(distance2 +
          // temp_hill_sphere) << "\n";
          if (((ref->getMoonA() * KM_PER_AU) >=
                   (distance2 - temp_hill_sphere) &&
               (ref->getMoonA() * KM_PER_AU) <=
                   (distance2 + temp_hill_sphere)) ||
              (distance2 >= ((ref->getMoonA() * KM_PER_AU) - hill_sphere4) &&
               distance2 <= ((ref->getMoonA() * KM_PER_AU) + hill_sphere4))) {
            bad_place2 = true;
            if ((flag_verbose & 0x1000) != 0) {
              std::cerr << "  Failed due to neighboring moon.\n";
            }
            break;
          }
        }
        if (!bad_place2 || tries2 > 20) {
          done2 = true;
        }
        if (tries2 > 20) {
          too_many_tries = true;
        }
      }
      new_moon->setMoonA(distance2 / KM_PER_AU);
      new_moon->setMoonE(eccentricity2);
      if (bad_place2 || too_many_tries) {
        delete new_moon;
        attempts++;
        if ((flag_verbose & 0x1000) != 0) {
          std::cerr << "  Failed to add moon.\n";
        }
      } else {
        attempts = 0;
        generate_planet(gen, new_moon, n, the_sun, random_tilt, moon_id, do_gases,
                        do_moons, true, the_planet->getMass());
        if (tmp != nullptr) {
          tmp->next_planet = new_moon;
          tmp = tmp->next_planet;
        } else {
          the_planet->first_moon = new_moon;
          tmp = the_planet->first_moon;
        }
        // the_planet->addMoon(new_moon);
        remaining_moon_mass -= new_moon_mass;
        moon_mass += new_moon_mass;
        n++;
        moon_count++;

        if ((flag_verbose & 0x40000) != 0) {
          std::cerr << "   Roche limit: R = " << toString(the_planet->getRadius())
               << ", rM = " << toString(the_planet->getDensity())
               << ", rm = " << toString(new_moon->getDensity()) << " -> "
               << toString(roche_limit2) << " km\n";
          std::cerr << "   Hill Sphere: a = "
               << toString(the_planet->getA() * KM_PER_AU) << ", m = "
               << toString(the_planet->getMass() * SOLAR_MASS_IN_KILOGRAMS)
               << ", M = "
               << toString(the_sun.getMass() * SOLAR_MASS_IN_KILOGRAMS)
               << " -> " << toString(hill_sphere3) << " km\n";
          std::cerr << moon_id << " Moon orbit: a = "
               << toString(new_moon->getMoonA() * KM_PER_AU)
               << " km, e = " << toString(new_moon->getMoonE()) << "\n";
        }

        if ((flag_verbose & 0x1000) != 0) {
          std::cerr << "  New Moon for " << planet_id << ": ("
               << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " EM) " << n << " "
               << toString(new_moon->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " EM\n";
        }
      }
    }

    if (the_planet->first_moon != nullptr) {
      // reorder the moons
      the_planet->sortMoons();

      // Build moon vector for efficient iteration
      the_planet->buildMoonVector();

      // make some modifications to the moons
      int moon_number = 1;
      for (planet* tmp : the_planet->moons) {
        tmp->setPlanetNo(moon_number);
        ss.str("");
        ss << planet_id << "-" << tmp->getPlanetNo();
        moon_id = ss.str();

        // change inclinations
        long double fdist =
            (tmp->getMoonA() * KM_PER_AU) / (hill_sphere3 / 3.0);
        long double fmass =
            std::pow(moon_mass / (tmp->getMass() * SUN_MASS_IN_EARTH_MASSES), 0.2);
        tmp->setInclination(fdist * fmass * tmp->getInclination());

        if (tmp->getGasMass() == 0) {
          // we don't want moons with too thick an atmosphere
          if (tmp->getSurfPressure() > 6000) {
            tmp->setType(tUnknown);
            tmp->setSurfPressure(calcPhlPressure(tmp) *
                                 EARTH_SURF_PRES_IN_MILLIBARS);
            while (tmp->getSurfPressure() > 6000) {
              tmp->setSurfPressure(tmp->getSurfPressure() - 1.0);
            }
            iterate_surface_temp(tmp, do_gases);
            if (do_gases) {
              tmp->clearGases();
              calculate_gases(the_sun, tmp, moon_id);
            }
            assign_type(gen, the_sun, tmp, moon_id, true, do_gases, false);
          }
        }

        tmp->setHzc(calcHzc(tmp));
        tmp->setHza(calcHza(tmp));
        tmp->setEsi(calcEsi(tmp));
        tmp->setSph(calcSph(tmp));
        moon_number++;
      }
    }
  }
}

/**
 * @brief Finalize rocky planet properties (atmosphere, temperature, type)
 * @param the_planet Planet to finalize
 * @param the_sun Parent star
 * @param planet_id Planet identifier for logging
 * @param do_gases Whether to calculate detailed atmospheric gases
 * @param is_moon Whether this is a moon
 * @param the_fudged_radius Fudged radius for calculations
 */
static void finalize_rocky_planet_properties(StarGenerator* gen, planet *the_planet, sun &the_sun,
                                            const std::string &planet_id, bool do_gases,
                                            bool is_moon, long double the_fudged_radius) {
  the_planet->setEstimatedTemp(est_temp(
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      the_planet->getA(), EARTH_ALBEDO));
  the_planet->setEstimatedTerrTemp(est_temp(
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      the_planet->getA(), EARTH_ALBEDO));

  the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));
  the_planet->setMolecWeight(min_molec_weight(the_planet));

  the_planet->setGreenhouseEffect(grnhouse(
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      the_planet->getA()));

  long double fudged_escape_velocity =
      escape_vel(the_planet->getMass(), the_fudged_radius);
  the_planet->setVolatileGasInventory(vol_inventory(
      the_planet->getMass(), fudged_escape_velocity,
      the_planet->getRmsVelocity(), the_sun.getMass(),
      the_planet->getOrbitZone(), the_planet->getGreenhouseEffect(),
      the_planet->getGasMass() > 0.0));
  the_planet->setSurfPressure(pressure(the_planet->getVolatileGasInventory(),
                                       the_fudged_radius,
                                       the_planet->getSurfGrav()));

  if (the_planet->getSurfPressure() == 0.0) {
    the_planet->setBoilPoint(0.0);
  } else {
    the_planet->setBoilPoint(boiling_point(the_planet->getSurfPressure()));
  }

  iterate_surface_temp(the_planet, do_gases);

  if (do_gases) {
    the_sun = the_planet->getTheSun();
    calculate_gases(the_sun, the_planet, planet_id);
  }

  assign_type(gen, the_sun, the_planet, planet_id, is_moon, do_gases, false);
}

/**
 * @brief generate planet
 *
 * @param the_planet
 * @param planet_no
 * @param the_sun
 * @param random_tilt
 * @param planet_id
 * @param do_gases
 * @param do_moons
 * @param is_moon
 * @param parent_mass
 */
void generate_planet(StarGenerator* gen, planet *the_planet, int planet_no, sun &the_sun,
                     bool random_tilt, const std::string &planet_id, bool do_gases,
                     bool do_moons, bool is_moon, long double parent_mass) {
    do_gases = (gen->config.flags & fDoGases) != 0;
  do_moons = (gen->config.flags & fDoMoons) != 0;
  long double tmp = NAN;
  long double ecc_coef = 0.077;
  long double lambda = NAN;
  long double the_fudged_radius = 0.0;

  if (do_moons && !is_moon) {
    srandf(gen->sim_context.current_system_seed + (1000 * planet_no));
  }

  the_planet->setTheSun(the_sun);

  the_planet->setOrbitZone(orbital_zone(
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      the_planet->getA()));

  the_planet->setHzd(calcHzd(the_planet));

  assign_composition(the_planet, the_sun, is_moon);

  if (!is_moon) {
    the_planet->setOrbPeriod(
        period(the_planet->getA(), the_planet->getMass(), parent_mass));
    if (random_tilt || the_planet->getAxialTilt() == 0) {
      the_planet->setAxialTilt(inclination(the_planet->getA(), parent_mass));
    }
  } else {
    the_planet->setOrbPeriod(
        period(the_planet->getMoonA(), the_planet->getMass(), parent_mass));
    if (random_tilt || the_planet->getAxialTilt() == 0) {
      the_planet->setAxialTilt(
          inclination(the_planet->getMoonA(), parent_mass));
    }
  }

  the_planet->setInclination(gaussian(1.0));

  the_planet->setAscendingNode(random_number(0.0, 360.0));
  the_planet->setLongitudeOfPericenter(random_number(0.0, 360.0));
  the_planet->setMeanLongitude(random_number(0.0, 360.0));

  the_planet->setExosphericTemp(
      EARTH_EXOSPHERE_TEMP /
      pow2(the_planet->getA() /
           the_sun.getREcosphere(the_planet->getMass() *
                                 SUN_MASS_IN_EARTH_MASSES)));
  the_planet->setRmsVelocity(
      rms_vel(MOL_NITROGEN, the_planet->getExosphericTemp()));
  the_planet->setCoreRadius(radius_improved(
      the_planet->getDustMass(), the_planet->getImf(), the_planet->getRmf(),
      the_planet->getCmf(), false, the_planet->getOrbitZone(), the_planet));

  the_planet->setDensity(empirical_density(
      the_planet->getMass(), the_planet->getA(),
      the_sun.getREcosphere(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
      true));
  the_planet->setRadius(
      volume_radius(the_planet->getMass(), the_planet->getDensity()));

  the_planet->setSurfAccel(
      acceleration(the_planet));
  the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));

  the_planet->setMolecWeight(min_molec_weight(the_planet));

  bool force_gas_giant = false;
  if ((the_planet->getGasMass() / the_planet->getMass()) > 0.05 &&
      (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 10.0) {
    force_gas_giant = true;
  }
  if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 1.0 &&
       (the_planet->getGasMass() / the_planet->getMass()) > 0.05 &&
       min_molec_weight(the_planet) <= 4.0) ||
      ((the_planet->getGasMass() / the_planet->getMass()) > 0.2 &&
       the_planet->getA() < 0.8) ||
      force_gas_giant)  // it's a gas planet
  {
    if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 10.0) {
      the_planet->setType(tSubSubGasGiant);  // it's a gas dwarf
    } else if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 50.0) {
      the_planet->setType(tSubGasGiant);  // it's a neptunian
    } else if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) /
                JUPITER_MASS) >= 13.0) {
      the_planet->setType(tBrownDwarf);  // it's a brown dwarf
    } else {
      the_planet->setType(tGasGiant);  // it's a jovian
    }
  } else  // If not, it's rocky.
  {
    the_fudged_radius = fudged_radius(
        the_planet->getMass(), the_planet->getImf(), the_planet->getRmf(),
        the_planet->getCmf(), the_planet->getGasGiant(),
        the_planet->getOrbitZone(), the_planet);
    the_planet->setRadius(radius_improved(
        the_planet->getMass(), the_planet->getImf(), the_planet->getRmf(),
        the_planet->getCmf(), false, the_planet->getOrbitZone(), the_planet));
    the_planet->setDensity(
        volume_density(the_planet->getMass(), the_planet->getRadius()));

    the_planet->setSurfAccel(
        acceleration(the_planet));
    the_planet->setSurfGrav(gravity(the_planet->getSurfAccel()));

    // Calculate and apply atmospheric gas loss (H2 and He)
    calculate_gas_loss(the_planet, the_sun, planet_id);
  }

  the_planet->setDay(day_length(the_planet, parent_mass, is_moon));
  the_planet->setOblateness(calcOblateness(the_planet));

  the_planet->setEscVelocity(
      escape_vel(the_planet->getMass(), the_planet->getRadius()));

  // Finalize planet properties based on type
  if (is_gas_planet(the_planet)) {
    finalize_gas_giant_properties(gen, the_planet, the_sun, planet_id, do_gases,
                                  force_gas_giant, parent_mass, is_moon);
  } else {
    finalize_rocky_planet_properties(gen, the_planet, the_sun, planet_id, do_gases,
                                     is_moon, the_fudged_radius);
  }

  the_planet->setHzc(calcHzc(the_planet));
  the_planet->setHza(calcHza(the_planet));
  the_planet->setEsi(calcEsi(the_planet));
  the_planet->setSph(calcSph(the_planet));
  set_habitability_flags(the_planet);

  // Generate moons for this planet (if not a moon itself)
  generate_moons(gen, the_planet, planet_no, the_sun, random_tilt, planet_id,
                 do_gases, do_moons, is_moon);

  ZoneScoped;
}

/**
 * @brief Update min/max statistic for a value
 * @param value The current value to check
 * @param min_stat Reference to minimum statistic to update
 * @param max_stat Reference to maximum statistic to update
 * @return true if a new min or max was set, false otherwise
 */
static auto update_min_max_stat(long double value, long double& min_stat, long double& max_stat) -> bool {
  bool updated = false;
  if (min_stat > value || min_stat == 0.0) {
    min_stat = value;
    updated = true;
  }
  if (max_stat < value) {
    max_stat = value;
    updated = true;
  }
  return updated;
}

/**
 * @brief Update breathable planet statistics
 * @param the_planet The planet to track statistics for
 * @param illumination The planet's illumination value
 * @return true if any new min/max was set (for verbose logging)
 */
static auto update_breathable_statistics(SimulationContext& sc, planet* the_planet, long double illumination) -> bool {
  // Accumulate into the caller's per-context stats. On the parallel path this is
  // the per-worker gen->sim_context (no shared global, no lock); the run-wide
  // totals are reduced from these after the threads join. On the sequential path
  // this is g_generator.sim_context, accumulated directly across systems.
  bool stats_updated = false;

  stats_updated |= update_min_max_stat(the_planet->getSurfTemp(), sc.min_breathable_temp, sc.max_breathable_temp);
  stats_updated |= update_min_max_stat(the_planet->getSurfGrav(), sc.min_breathable_g, sc.max_breathable_g);
  stats_updated |= update_min_max_stat(illumination, sc.min_breathable_l, sc.max_breathable_l);
  stats_updated |= update_min_max_stat(the_planet->getSurfPressure(), sc.min_breathable_p, sc.max_breathable_p);
  stats_updated |= update_min_max_stat(the_planet->getMass(), sc.min_breathable_mass, sc.max_breathable_mass);

  // Update terrestrial-specific stats if applicable
  if (the_planet->getType() == tTerrestrial) {
    stats_updated |= update_min_max_stat(the_planet->getSurfGrav(), sc.min_breathable_terrestrial_g, sc.max_breathable_terrestrial_g);
    stats_updated |= update_min_max_stat(illumination, sc.min_breathable_terrestrial_l, sc.max_breathable_terrestrial_l);
  }

  return stats_updated;
}

/**
 * @brief Update potentially habitable planet statistics
 * @param the_planet The planet to track statistics for
 * @param illumination The planet's illumination value
 * @return true if any new min/max was set (for verbose logging)
 */
static auto update_potential_statistics(SimulationContext& sc, planet* the_planet, long double illumination) -> bool {
  // Accumulate into the caller's per-context stats (see update_breathable_statistics).
  bool stats_updated = false;

  stats_updated |= update_min_max_stat(the_planet->getSurfTemp(), sc.min_potential_temp, sc.max_potential_temp);
  stats_updated |= update_min_max_stat(the_planet->getSurfGrav(), sc.min_potential_g, sc.max_potential_g);
  stats_updated |= update_min_max_stat(illumination, sc.min_potential_l, sc.max_potential_l);
  stats_updated |= update_min_max_stat(the_planet->getSurfPressure(), sc.min_potential_p, sc.max_potential_p);
  stats_updated |= update_min_max_stat(the_planet->getMass(), sc.min_potential_mass, sc.max_potential_mass);

  // Update terrestrial-specific stats if applicable
  if (the_planet->getType() == tTerrestrial ||
      (the_planet->getType() == t1Face &&
       the_planet->getHydrosphere() >= 0.05 &&
       the_planet->getHydrosphere() <= 0.8)) {
    stats_updated |= update_min_max_stat(the_planet->getSurfGrav(), sc.min_potential_terrestrial_g, sc.max_potential_terrestrial_g);
    stats_updated |= update_min_max_stat(illumination, sc.min_potential_terrestrial_l, sc.max_potential_terrestrial_l);
  }

  return stats_updated;
}

/**
 * @brief Log planet information for verbose output
 */
static void log_planet_info(planet* the_planet, const std::string& planet_id, long double illumination) {
  std::cerr << type_string(the_planet)
       << "\tp=" << toString(the_planet->getSurfPressure())
       << "\tm=" << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
       << "\tg=" << toString(the_planet->getSurfGrav())
       << "\tt=" << toString(the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN)
       << "\tl=" << toString(illumination)
       << "\t" << planet_id << "\n";
}

/**
 * @brief Check planet and update statistics
 */
void check_planet(StarGenerator* gen, planet *the_planet, const std::string &planet_id, bool is_moon) {
  int tIndex = 0;

  tIndex = the_planet->getType();

  if (gen->sim_context.type_counts[tIndex] == 0) {
    ++gen->sim_context.type_count;
  }

  ++gen->sim_context.type_counts[tIndex];

  unsigned int breathe = 0;

  // if (breathe == BREATHABLE && the_planet->getA() >
  // habitable_zone_distance(the_planet->getTheSun(), RECENT_VENUS) &&
  // the_planet->getA() < habitable_zone_distance(the_planet->getTheSun(),
  // EARLY_MARS))
  gen->sim_context.total_worlds++;
  if (is_habitable(the_planet)) {
    bool list_it = false;
    long double illumination = calcLuminosity(the_planet);

    gen->sim_context.system_habitable++;

    if (is_habitable_earth_like(the_planet)) {
      gen->sim_context.total_habitable_earthlike++;
      gen->sim_context.total_habitable_conservative++;
      gen->sim_context.total_habitable_optimistic++;
    } else if (is_habitable_conservative(the_planet)) {
      gen->sim_context.total_habitable_conservative++;
      gen->sim_context.total_habitable_optimistic++;
    } else if (is_habitable_optimistic(the_planet)) {
      gen->sim_context.total_habitable_optimistic++;
    }

    if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) >= 5.0 &&
         (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) < 10.0) ||
        (convert_km_to_eu(the_planet->getRadius()) >= 1.5 &&
         convert_km_to_eu(the_planet->getRadius()) <= 2.5)) {
      gen->sim_context.system_habitable_superterrans++;
    }

    // Update breathable planet statistics
    if (update_breathable_statistics(gen->sim_context, the_planet, illumination) && ((gen->config.verbose_level & 0x0002) != 0)) {
      list_it = true;
    }

    if ((gen->config.verbose_level & 0x0004) != 0) {
      list_it = true;
    }

    if (list_it) {
      log_planet_info(the_planet, planet_id, illumination);
    }
  } else if (is_potentialy_habitable(the_planet)) {
    gen->sim_context.system_potentially_habitable++;
    bool list_it = false;
    long double illumination = calcLuminosity(the_planet);

    if (is_potentialy_habitable_earth_like(the_planet)) {
      gen->sim_context.total_potentially_habitable_earthlike++;
      gen->sim_context.total_potentially_habitable_conservative++;
      gen->sim_context.total_potentially_habitable_optimistic++;
    } else if (is_potentialy_habitable_conservative(the_planet)) {
      gen->sim_context.total_potentially_habitable_conservative++;
      gen->sim_context.total_potentially_habitable_optimistic++;
    } else if (is_potentialy_habitable_optimistic(the_planet)) {
      gen->sim_context.total_potentially_habitable_optimistic++;
    }

    // Update potentially habitable planet statistics
    if (update_potential_statistics(gen->sim_context, the_planet, illumination) && ((gen->config.verbose_level & 0x0002) != 0)) {
      list_it = true;
    }

    if ((gen->config.verbose_level & 0x0004) != 0) {
      list_it = true;
    }

    if (list_it) {
      log_planet_info(the_planet, planet_id, illumination);
    }
  }

  if (is_moon && gen->sim_context.system_max_moon_mass < the_planet->getMass()) {
    gen->sim_context.system_max_moon_mass = the_planet->getMass();

    if ((gen->config.verbose_level & 0x0002) != 0) {
      std::cerr << type_string(the_planet)
           << "\tp=" << toString(the_planet->getSurfPressure()) << "\tm="
           << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << "\tg=" << toString(the_planet->getSurfGrav()) << "\tt="
           << toString(the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN) << "\t"
           << planet_id << "\n";
    }
  }

  if (((gen->config.verbose_level & 0x0800) != 0) &&
      (the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.0006 &&
      (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.0006 &&
      the_planet->getType() != tGasGiant &&
      the_planet->getType() != tSubGasGiant &&
      the_planet->getType() != tBrownDwarf) {
    int core_size =
        (int)((50.0 * the_planet->getCoreRadius()) / the_planet->getRadius());

    if (core_size <= 49) {
      std::cerr << type_string(the_planet)
           << "\tp=" << toString(the_planet->getCoreRadius())
           << "\tr=" << toString(the_planet->getRadius()) << "\tm="
           << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << "\t"
           << planet_id << "\t" << (50 - core_size) << "\n";
    }
  }

  long double rel_temp = (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) -
                         EARTH_AVERAGE_CELSIUS;
  // long double seas = the_planet->getHydrosphere() * 100.0;
  // long double clouds = the_planet->getCloudCover() * 100.0;
  // long double pressure = the_planet->getSurfPressure() /
  // EARTH_SURF_PRES_IN_MILLIBARS;
  long double ice = the_planet->getIceCover() * 100.0;
  long double gravity = the_planet->getSurfGrav();
  // unsigned int breathe = breathability(the_planet);
  breathe = breathability(the_planet);

  if (is_earth_like(the_planet)) {
    gen->sim_context.system_earthlike++;

    if ((gen->config.verbose_level & 0x0008) != 0) {
      std::cerr << type_string(the_planet)
           << "\tp=" << toString(the_planet->getSurfPressure()) << "\tm="
           << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << "\tg=" << toString(the_planet->getSurfGrav()) << "\tt="
           << toString(the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN) << "\t"
           << gen->sim_context.system_habitable << " " << planet_id << "\tEarth-like\n";
    }
  } else if (breathe == BREATHABLE && gravity > 1.3 && rel_temp < -2.0 &&
             ice < 10.0 && gen->sim_context.system_habitable > 1) {
    if ((gen->config.verbose_level & 0x0008) != 0) {
      std::cerr << type_string(the_planet)
           << "\tp=" << toString(the_planet->getSurfPressure()) << "\tm="
           << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << "\tg=" << toString(the_planet->getSurfGrav()) << "\tt="
           << toString(the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN) << "\t"
           << gen->sim_context.system_habitable << " " << planet_id << "\tSphinx-like\n";
    }
  }
  ZoneScoped;
}

/**
 * @brief assign type
 * 
 * @param the_sun 
 * @param the_planet 
 * @param planet_id 
 * @param is_moon 
 * @param do_gases 
 * @param second_time 
 */
void assign_type(StarGenerator* gen, sun &the_sun, planet *the_planet, const std::string &planet_id,
                 bool is_moon, bool do_gases, bool second_time) {
    if (the_planet->getSurfPressure() < 1.0) {
    if (the_planet->getRadius() < round_threshold(the_planet->getDensity())) {
      the_planet->setType(tAsteroids);
    } else {
      if (the_planet->getImf() > 0.5 || the_planet->getIceCover() >= 0.5) {
        the_planet->setType(tIce);
      } else if ((the_planet->getImf() + the_planet->getRmf()) < 0.2) {
        the_planet->setType(tIron);
      } else if (the_planet->getCmf() >= 0.75) {
        the_planet->setType(tCarbon);
      } else {
        the_planet->setType(tRock);
      }
    }
  }
  /*else if (the_planet->getSurfPressure() > 6000.0 &&
  the_planet->getMolecWeight() <= 2.0) // Retains Hydrogen
  {
    the_planet->setType(tSubSubGasGiant);
  }*/
  else {
    if ((((int)the_planet->getDay() ==
          (int)(the_planet->getOrbPeriod() * 24.0)) ||
         the_planet->getResonantPeriod()) &&
        !is_moon) {
      the_planet->setType(t1Face);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getImf() >= 0.05 &&
               the_planet->getHydrosphere() == 0.0) {
      the_planet->setType(tIce);
      /*the_planet->setIceCover(the_planet->getIceCover() +
      the_planet->getHydrosphere()); if (the_planet->getIceCover() > 1.0)
      {
        the_planet->setIceCover(1.0);
      }*/
      the_planet->setHydrosphere(0.0);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getHydrosphere() >=
               0.8)  // In Star Trek, any planet that is 80% covered with water,
                     // it is considered an ocean planet
    {
      if (the_planet->getCmf() >= 0.75) {
        the_planet->setType(tOil);
      } else {
        the_planet->setType(tWater);
      }
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getIceCover() >=
               0.8)  // In Star Trek, any planet that is 80% covered with ice,
                     // it is considered an ice planet
    {
      the_planet->setType(tIce);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getHydrosphere() >= 0.05) {
      if (the_planet->getCmf() >= 0.75) {
        the_planet->setType(tOil);
      } else {
        the_planet->setType(tTerrestrial);
      }
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getSurfTemp() > the_planet->getBoilPoint()) {
      the_planet->setType(tVenusian);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if ((the_planet->getGasMass() / the_planet->getMass()) > 0.0001) {
      the_planet->setType(
          tIce);  // Accreted gas but no Greenhouseor or liquid water
      the_planet->setIceCover(1.0);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getSurfPressure() < 250) {
      the_planet->setType(tMartian);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else if (the_planet->getSurfTemp() < FREEZING_POINT_OF_WATER) {
      the_planet->setType(tIce);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }
    } else {
      the_planet->setType(tUnknown);
      if (!second_time) {
        makeHabitable(gen, the_sun, the_planet, planet_id, is_moon, do_gases);
      }

      if ((gen->config.verbose_level & 0x0001) != 0) {
        std::string one_face_string;
        if (((int)the_planet->getDay() ==
             (int)(the_planet->getOrbPeriod() * 24.0)) ||
            the_planet->getResonantPeriod()) {
          one_face_string = "(1-Face)";
        } else {
          one_face_string = "";
        }
        std::cerr << type_string(the_planet)
             << "\tp=" << toString(the_planet->getSurfPressure()) << "\tm="
             << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
             << "\tg=" << toString(the_planet->getSurfGrav()) << "\tt="
             << toString(the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN)
             << "\t" << planet_id << "\t Unknown " << one_face_string << "\n";
      }
    }
  }
  ZoneScoped;
}