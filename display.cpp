#include <variant>
#include "display.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "PerformanceMonitor.h"
#include "SimulationContext.h"
#include "const.h"
#include "units.hh"   // au-backed exact length conversions (sgu::)
#include "elements.h"
#include "enviro.h"
#include "stargen.h"
#include "utils.h"
#include "PlanetRow.h"


void ensure_directory_exists(const std::string& path) {
  std::filesystem::path dir_path(path);
  if (!std::filesystem::exists(dir_path)) {
    std::filesystem::create_directories(dir_path);
    std::cout << "Created directory: " << path << std::endl;
  }
}

int random_numberInt(int min, int max) {
  // Draw from the active seeded RandomContext (see utils.cpp) rather than a
  // static std::random_device generator. The old static RNG was (a) seeded
  // from system entropy -> Celestia (-c) texture indices differed every run,
  // (b) a shared-mutable static -> a latent data race if ever reached from the
  // parallel path, and (c) buggy: the static uniform_int_distribution was
  // constructed once and ignored min/max on later calls. This routes through
  // the same deterministic path as random_number() and respects min/max.
  return random_number_int(min, max);
}

/**
 * @brief Output system to text
 *
 * @param innermost_planet
 * @param do_gases
 * @param seed
 * @param do_moons
 */
/**
 * @brief Get planet classification symbol for text output
 */
static auto get_planet_symbol(planet* the_planet) -> char {
  if (the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0) {
    return '+';  // Greenhouse effect
  }
  if ((the_planet->getHydrosphere() > 0.05) && (the_planet->getHydrosphere() < 0.8)) {
    return '*';  // Water world
  }
  if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 0.1) {
    return 'o';  // Significant mass
  }
  return '.';  // Small body
}

/**
 * @brief Print system characteristics header
 */
static void text_print_system_header(sun& the_sun, long int seed) {
  std::cout << std::format("Stargen - V{}; seed={}\n", stargen_revision, seed)
            << "                          SYSTEM  CHARACTERISTICS\n";

  if (!the_sun.getIsCircumbinary()) {
    std::cout << std::format("Stellar mass: {} solar masses\n", the_sun.getMass())
              << std::format("Stellar luminosity: {}\n", the_sun.getLuminosity());
  } else {
    std::cout << std::format("Mass of Primary: {} solar masses\n", the_sun.getMass())
              << std::format("Luminosity of Primary: {}\n", the_sun.getLuminosity())
              << std::format("Mass of Secondary: {} solar masses\n", the_sun.getSecondaryMass())
              << std::format("Luminosity of Secondary: {}\n", the_sun.getSecondaryLuminosity());
  }

  std::cout << std::format("Stellar metallicity: [Fe/H] = {} dex\n", the_sun.getMetallicity());

  std::cout << std::format("Age: {} billion years\t({} billion left on main sequence)\n",
                           the_sun.getAge() / 1.0E9,
                           (the_sun.getLife() - the_sun.getAge()) / 1.0E9)
            << std::format("Habitable ecosphere radius: {} AU\n\n",
                           AVE(habitable_zone_distance(the_sun, RECENT_VENUS, 1.0),
                               habitable_zone_distance(the_sun, EARLY_MARS, 1.0)));
}

/**
 * @brief Print compact planet list
 */
static void text_print_planet_list() {
  std::cout << "Planets present at:\n";
  int counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    std::cout << std::format(
        "{}\t{} AU\t{} EM\t{}\n",
        counter,
        the_planet->getA(),
        the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES,
        get_planet_symbol(the_planet));
    counter++;
  }
  std::cout << "\n\n\n";
}

/**
 * @brief Print detailed planet properties
 */
static void text_print_planet_details(planet* the_planet, int counter) {
  std::cout << std::format("Planet {}", counter);
  if (is_gas_planet(the_planet)) {
    std::cout << "\t*gas giant*";
  }
  std::cout << '\n';

  // Tidal locking status
  if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
    std::cout << "Planet is tidally locked with one face to star.\n";
  } else if (the_planet->getResonantPeriod()) {
    std::cout << "Planet's rotation is in a resonant spin lock with the star.\n";
  }

  // Habitability caveats (notably for M-dwarf hosts); see set_habitability_flags.
  if (the_planet->getPmsDesiccationRisk()) {
    std::cout << "Likely desiccated during the host M-dwarf's pre-main-sequence "
                 "phase (possible abiotic-O2 \"mirage Earth\").\n";
  }
  if (the_planet->getHighXuvEscapeRisk()) {
    std::cout << "High atmospheric-escape risk from M-dwarf XUV/flare "
                 "irradiation.\n";
  }
  if (the_planet->getCo2CollapseRisk()) {
    std::cout << "Cold for the habitable zone: outgassed CO2 may freeze out, "
                 "risking irreversible glaciation.\n";
  }

  // Orbital properties
  std::cout << std::format("   Distance from primary star:\t{} AU\n", the_planet->getA())
            << std::format("   Eccentricity of orbit:\t{}\n", the_planet->getE())
            << std::format("   Length of year:\t\t{} days\n", the_planet->getOrbPeriod())
            << std::format("   Length of day:\t\t{} hours\n", the_planet->getDay())
            << std::format("   Mass:\t\t\t{} Earth masses\n",
                           the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  // Surface properties (rocky planets only)
  if (!is_gas_planet(the_planet)) {
    std::cout << std::format("   Surface gravity:\t\t{} Earth gees\n", the_planet->getSurfGrav())
              << std::format("   Surface pressure:\t\t{} Earth atmospheres",
                             the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS);
    if (the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0) {
      std::cout << " GREENHOUSE EFFECT";
    }
    std::cout << '\n'
              << std::format("   Surface temperature:\t\t{} degrees Celcius\n",
                             the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER)
              << std::format("   Boiling point of water:\t{} degrees Celcius\n",
                             the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER)
              << std::format("   Hydrosphere percentage:\t{}%\n", the_planet->getHydrosphere() * 100.0)
              << std::format("   Cloud cover percentage:\t{}%\n", the_planet->getCloudCover() * 100.0)
              << std::format("   Ice cover percentage:\t{}%\n", the_planet->getIceCover() * 100.0);
    if (the_planet->getClimateModelTemp() > 0.0) {
      std::cout << std::format(
          "   1D-climate surface temp:\t{} degrees Celcius (Lehmer et al. 2020)\n",
          the_planet->getClimateModelTemp() - FREEZING_POINT_OF_WATER);
    }
  }

  // Physical properties (all planets)
  std::cout << std::format("   Equatorial radius:\t\t{} Km\n", the_planet->getRadius())
            << std::format("   Density:\t\t\t{} grams/cc\n", the_planet->getDensity())
            << std::format("   Escape Velocity:\t\t{} Km/sec\n",
                           sgu::cm_to_km(the_planet->getEscVelocity()))
            << std::format("   Molecular weight retained:\t{} and above\n",
                           the_planet->getMolecWeight())
            << std::format("   Surface acceleration:\t{} cm/sec2\n", the_planet->getSurfAccel())
            << std::format("   Axial tilt:\t\t\t{} degrees\n", the_planet->getAxialTilt())
            << std::format("   Planetary albedo:\t\t{}\n", the_planet->getAlbedo()) << "\n\n";
}

void text_describe_system(planet* innermost_planet, bool do_gases, long int seed, bool do_moons) {
  performanceMonitor.recordFileOperation("text_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;

  sun the_sun = innermost_planet->getTheSun();

  // Print system header
  text_print_system_header(the_sun, seed);

  // Print compact planet list
  text_print_planet_list();

  // Print detailed planet information
  int counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    text_print_planet_details(the_planet, counter);
    counter++;
  }
}

/**
 * @brief Output to csv file
 *
 * @param the_file
 * @param innermost_planet
 * @param do_gases
 * @param seed
 * @param do_moons
 */
void csv_describe_system(std::fstream& the_file, planet* innermost_planet, bool do_gases, long int seed,
                         bool do_moons) {
  performanceMonitor.recordFileOperation("csv_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet*      the_planet;
  sun          the_sun = innermost_planet->getTheSun();
  int          counter;
  std::stringstream ss;
  std::string       id;
  planet*      moon;
  int          moons;

  if (!the_sun.getIsCircumbinary()) {
    the_file << "'Seed', 'Star Name', 'Luminosity', 'Mass', 'Temperature', "
                "'Spectral Type', 'Total Time on Main Sequence', 'Age', "
                "'Earth-like Distance'\n";
    the_file << seed << ", '" << escapeCsvField(the_sun.getName()) << "', " << toString(the_sun.getLuminosity())
             << ", " << toString(the_sun.getMass()) << ", " << toString(the_sun.getEffTemp())
             << ", '" << escapeCsvField(the_sun.getSpecType()) << "', " << toString(the_sun.getLife()) << ", "
             << toString(the_sun.getAge()) << ", " << toString(the_sun.getREcosphere(1.0)) << "\n";
  } else {
    the_file << "'Seed', 'Star Name', 'Luminosity of Primary', 'Mass of Primary', "
                "'Temperature of Primary', 'Spectral Type of Primary', 'Luminosity "
                "of Secondary', 'Mass of Secondary', 'Temperature of Secondary', "
                "'Spectral Type of Secondary', 'Seperation', 'Eccentricity', "
                "'Combined Temperature', 'Primary's Total Time on Main Sequence', "
                "'Age', 'Earth-like Distance'\n";
    the_file << seed << ", '" << escapeCsvField(the_sun.getName()) << "', " << toString(the_sun.getLuminosity())
             << ", " << toString(the_sun.getMass()) << ", " << toString(the_sun.getEffTemp())
             << ", '" << escapeCsvField(the_sun.getSpecType()) << "', "
             << toString(the_sun.getSecondaryLuminosity()) << ", "
             << toString(the_sun.getSecondaryMass()) << ", "
             << toString(the_sun.getSecondaryEffTemp()) << ", '" << escapeCsvField(the_sun.getSecondarySpecType())
             << "', " << toString(the_sun.getSeperation()) << ", "
             << toString(the_sun.getEccentricity()) << ", "
             << toString(the_sun.getCombinedEffTemp()) << ", " << toString(the_sun.getLife())
             << ", " << toString(the_sun.getAge()) << ", " << toString(the_sun.getREcosphere(1.0))
             << "\n";
  }
  the_file << emit_csv_header();
  counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    csv_row(the_file, the_planet, do_gases, false, id, ss);
    if (do_moons && !the_planet->moons.empty()) {
      moons = 1;
      for (planet* moon : the_planet->moons) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        csv_row(the_file, moon, do_gases, true, id, ss);
        moons++;
      }
    }
    counter++;
  }
  ZoneScoped;
}

/**
 * @brief Output to JSON
 *
 * @param the_file
 * @param innermost_planet
 * @param do_gases
 * @param seed
 * @param do_moons
 */
void jsonDescribeSystem(std::fstream& the_file, planet* innermost_planet, bool do_gases, long int seed,
                        bool do_moons) {
  performanceMonitor.recordFileOperation("json_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet*      the_planet;
  sun          the_sun = innermost_planet->getTheSun();
  int          counter;
  std::stringstream ss;
  std::string       id;
  planet*      moon;
  int          moons;

  nlohmann::json header;
  if (!the_sun.getIsCircumbinary()) {
    header = {
        {"schema_version",              "1.0.0"                   },
        {"generator",                   "stargen"                 },
        {"Body Type",                   "Star"                    },
        {"seed",                        seed                      },
        {"Star Name",                   the_sun.getName()         },
        {"Luminosity",                  the_sun.getLuminosity()   },
        {"Mass",                        the_sun.getMass()         },
        {"Temperature",                 the_sun.getEffTemp()      },
        {"Spectral Type",               the_sun.getSpecType()     },
        {"Metallicity",                 the_sun.getMetallicity()  },
        {"Total Time on Main Sequence", the_sun.getLife()         },
        {"Age",                         the_sun.getAge()          },
        {"Earth-like Distance",         the_sun.getREcosphere(1.0)}
    };
  } else {
    header = {
        {"schema_version",              "1.0.0"                         },
        {"generator",                   "stargen"                       },
        {"Body Type",                   "Binary Star"                   },
        {"seed",                        seed                            },
        {"Star Name",                   the_sun.getName()               },
        {"Luminosity of Primary",       the_sun.getLuminosity()         },
        {"Mass of Primary",             the_sun.getMass()               },
        {"Temperature of Primary",      the_sun.getEffTemp()            },
        {"Spectral Type of Primary",    the_sun.getSpecType()           },
        {"Luminosity of Secondary",     the_sun.getSecondaryLuminosity()},
        {"Mass of Secondary",           the_sun.getSecondaryMass()      },
        {"Temperature of Secondary",    the_sun.getSecondaryEffTemp()   },
        {"Spectral Type of Secondary",  the_sun.getSecondarySpecType()  },
        {"Seperation",                  the_sun.getSeperation()         },
        {"Eccentricity",                the_sun.getEccentricity()       },
        {"Combined Temperature",        the_sun.getCombinedEffTemp()    },
        {"Metallicity",                 the_sun.getMetallicity()        },
        {"Total Time on Main Sequence", the_sun.getLife()               },
        {"Age",                         the_sun.getAge()                },
        {"Earth-like Distance",         the_sun.getREcosphere(1.0)      }
    };
  }

  nlohmann::json planets = nlohmann::json::array();

  counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    nlohmann::json planet_json = jsonRow(the_planet, do_gases, false, id, ss);
    if (do_moons && !the_planet->moons.empty()) {
      nlohmann::json moon_array = nlohmann::json::array();
      moons = 1;
      for (planet* moon : the_planet->moons) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        moon_array.push_back(jsonRow(moon, do_gases, true, id, ss));
        moons++;
      }
      planet_json["Moons"] = std::move(moon_array);
    }
    planets.push_back(std::move(planet_json));
    counter++;
  }

  nlohmann::json document;
  document["star"]    = std::move(header);
  document["planets"] = std::move(planets);
  the_file << document.dump(4) << "\n";
  ZoneScoped;
}

/**
 * @brief Create a "row" for cvs output. A row is basically a planetoid
 *
 * @param the_file
 * @param the_planet
 * @param do_gases
 * @param is_moon
 * @param id
 * @param ss
 */
// Single population point for the per-planet output row consumed by the
// reflective JSON/CSV emitters (PlanetRow.h). Members hold RAW getter values.
// Two atmosphere strings are computed because JSON and CSV format gas pressures
// differently: JSON via toString() (legacy jsonRow formatting), CSV via {:.2f}
// (generate_atmosphere_string).
auto to_row(planet* the_planet, bool is_moon, const std::string& id, bool do_gases) -> PlanetRow {
  do_gases = (flags_arg_clone & fDoGases) != 0;

  // JSON-style atmosphere string, byte-for-byte the legacy jsonRow inline build.
  std::string atmosphere_json;
  if (do_gases) {
    std::stringstream ss;
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      int  index     = gases.count();
      bool poisonous = false;
      for (int n = 0; n < gases.count(); n++) {
        if (gases[n].getNum() == the_planet->getGas(i).getNum()) {
          index = n;
          break;
        }
      }
      long double ipp = inspired_partial_pressure(the_planet->getSurfPressure(),
                                                  the_planet->getGas(i).getSurfPressure());
      if (ipp < 0.0) {
        ipp = 0.0;
      }
      if (ipp > gases[index].getMaxIpp()) {
        poisonous = true;
      }
      ss << gases[index].getSymbol() << " "
         << toString(100.0 *
                     (the_planet->getGas(i).getSurfPressure() / the_planet->getSurfPressure()))
         << " " << toString(the_planet->getGas(i).getSurfPressure()) << " (" << toString(ipp) << ")";
      if (poisonous) {
        ss << " poisonous";
      }
      ss << ";";
    }
    atmosphere_json = ss.str();
  }

  PlanetRow r;
  r.planet_no                    = id;
  r.distance                     = is_moon ? the_planet->getMoonA() : the_planet->getA();
  r.eccentricity                 = is_moon ? the_planet->getMoonE() : the_planet->getE();
  r.inclination                  = the_planet->getInclination();
  r.ascending_node               = the_planet->getAscendingNode();
  r.longitude_of_pericenter      = the_planet->getLongitudeOfPericenter();
  r.mean_longitude               = the_planet->getMeanLongitude();
  r.axial_tilt                   = the_planet->getAxialTilt();
  r.ice_mass_fraction            = the_planet->getImf();
  r.rock_mass_fraction           = the_planet->getRmf();
  r.carbon_mass_fraction         = the_planet->getCmf();
  r.total_mass                   = the_planet->getMass();
  r.is_gas_giant                 = the_planet->getGasGiant();
  r.dust_mass                    = the_planet->getDustMass();
  r.gas_mass                     = the_planet->getGasMass();
  r.radius_of_core               = the_planet->getCoreRadius();
  r.total_radius                 = the_planet->getRadius();
  r.orbit_zone                   = the_planet->getOrbitZone();
  r.density                      = the_planet->getDensity();
  r.orbit_period                 = the_planet->getOrbPeriod();
  r.rotation_period              = the_planet->getDay();
  r.has_spin_orbit_resonance     = the_planet->getResonantPeriod();
  r.escape_velocity              = the_planet->getEscVelocity();
  r.surface_acceleration         = the_planet->getSurfAccel();
  r.surface_gravity              = the_planet->getSurfGrav();
  r.rms_velocity                 = the_planet->getRmsVelocity();
  r.minimum_molecular_weight     = the_planet->getMolecWeight();
  r.volatile_gas_inventory       = the_planet->getVolatileGasInventory();
  r.surface_pressure             = the_planet->getSurfPressure();
  r.greenhouse_effect            = the_planet->getGreenhouseEffect();
  r.boiling_point                = the_planet->getBoilPoint();
  r.albedo                       = the_planet->getAlbedo();
  r.exospheric_temperature       = the_planet->getExosphericTemp();
  r.estimated_temperature        = the_planet->getEstimatedTemp();
  r.estimated_terran_temperature = the_planet->getEstimatedTerrTemp();
  r.surface_temperature          = the_planet->getSurfTemp();
  r.greenhouse_rise              = the_planet->getGreenhsRise();
  r.high_temperature             = the_planet->getHighTemp();
  r.low_temperature              = the_planet->getLowTemp();
  r.maximum_temperature          = the_planet->getMaxTemp();
  r.minimum_temperature          = the_planet->getMinTemp();
  r.hydrosphere                  = the_planet->getHydrosphere();
  r.cloud_cover                  = the_planet->getCloudCover();
  r.ice_cover                    = the_planet->getIceCover();
  r.atmosphere                   = generate_atmosphere_string(the_planet, do_gases);  // CSV-style
  r.type                         = type_string(the_planet);
  r.minor_moons                  = the_planet->getMinorMoons();
  r.tidally_locked               = the_planet->getTidallyLocked();
  r.pms_desiccation_risk         = the_planet->getPmsDesiccationRisk();
  r.high_xuv_escape_risk         = the_planet->getHighXuvEscapeRisk();
  r.co2_collapse_risk            = the_planet->getCo2CollapseRisk();
  r.climate_model_temp           = the_planet->getClimateModelTemp();
  r.atmosphere_json              = atmosphere_json;
  r.is_moon                      = is_moon;
  return r;
}

void csv_row(std::fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             const std::string& id, std::stringstream& /*ss*/) {
  ZoneScoped;
  the_file << emit_csv_row(to_row(the_planet, is_moon, id, do_gases));
}

std::string generate_atmosphere_string(planet* the_planet, bool do_gases) {
  if (!do_gases) return "";

  std::string atmosphere;
  for (int i = 0; i < the_planet->getNumGases(); i++) {
    auto gas      = the_planet->getGas(i);
    auto gas_info = gases[find_gas_index(gas.getNum())];

    long double ipp = std::max(
        0.0L, inspired_partial_pressure(the_planet->getSurfPressure(), gas.getSurfPressure()));
    bool poisonous = ipp > gas_info.getMaxIpp();

    atmosphere += std::format("{} {:.2f} {:.2f} ({:.2f}){};", gas_info.getSymbol(),
                              100.0 * (gas.getSurfPressure() / the_planet->getSurfPressure()),
                              gas.getSurfPressure(), ipp, poisonous ? " poisonous" : "");
  }

  return atmosphere;
}

auto jsonRow(planet* the_planet, bool do_gases, bool is_moon, std::string id,
             std::stringstream& /*ss*/) -> nlohmann::json {
  ZoneScoped;
  return emit_planet_json(to_row(the_planet, is_moon, id, do_gases));
}

/**
 * @brief type string
 *
 * @param the_planet
 * @return string
 */
std::string type_string(planet* the_planet) {
  switch (the_planet->getType()) {
    case tUnknown:
      return "Unknown";
    case tRock:
      return "Rock";
    case tVenusian:
      return "Venusian";
    case tTerrestrial:
      return "Terrestrial";
    case tSubSubGasGiant:
      return std::format("{} Gas Dwarf", cloud_type_string(the_planet));
    case tSubGasGiant:
      return std::format("{} Neptunian", cloud_type_string(the_planet));
    case tGasGiant:
      return std::format("{} Jovian", cloud_type_string(the_planet));
    case tMartian:
      return "Martian";
    case tWater:
      return "Water";
    case tIce:
      return "Ice";
    case tAsteroids:
      return "Asteroids";
    case t1Face:
      return "1Face";
    case tBrownDwarf:
      return "Brown Dwarf";
    case tIron:
      return "Iron";
    case tCarbon:
      return "Carbon";
    case tOil:
      return "Oil";
    default:
      return "Unknown";
  }
}

/**
 * @brief cloud type string
 *
 * @param the_planet
 * @return string
 */
std::string cloud_type_string(planet* the_planet) {
  long double temp = the_planet->getEstimatedTemp();
  ZoneScoped;
  if (temp > 2240) {
    return "Carbon";
  } else if (temp > 1400) {
    return "Silicate";
  } else if (temp > 900) {
    return "Alkali";
  } else if (temp > 360) {
    return "Cloudless";
  } else if (temp > 320) {
    return "Sulfur";
  } else if (temp > 150) {
    return "Water";
  } else if (temp > 81) {
    return "Ammonia";
  } else {
    return "Methane";
  }
}

/**
 * @brief Create a svg file
 *
 * @param innermost_planet
 * @param path
 * @param file_name
 * @param svg_ext
 * @param prognam
 * @param do_moons
 */
/**
 * @brief SVG scaling parameters
 */
struct SVGScaleParams {
  long double max_x;
  long double max_y;
  long double margin;
  long double min_log;
  long double max_log;
  long double mult;
  long double offset;
  long double em_scale;
  int floor;
  int ceiling;
};

/**
 * @brief Calculate SVG scaling parameters for logarithmic distance display
 */
static auto calculate_svg_scale(planet* innermost_planet, planet* outermost_planet) -> SVGScaleParams {
  SVGScaleParams params;
  params.max_x = 760.0;
  params.max_y = 120.0;
  params.margin = 20.0;
  params.em_scale = 5.0;

  long double inner_edge = innermost_planet->getA() * (1.0 - innermost_planet->getE());
  long double outer_edge = outermost_planet->getA() * (1.0 + outermost_planet->getE());

  params.floor = (int)(log10(inner_edge) - 1.0);
  params.min_log = params.floor;
  params.ceiling = (int)(log10(outer_edge) + 1.0);
  params.max_log = 0.0;

  // Find precise min and max log values
  for (int x = params.floor; x <= params.ceiling; x++) {
    for (float n = 1.0; n < 9.9; n++) {
      if (inner_edge > std::pow(10.0, x + log10(n))) {
        params.min_log = x + log10(n);
      }
      if (params.max_log == 0.0 && outer_edge < std::pow(10.0, x + log10(n))) {
        params.max_log = x + log10(n);
      }
    }
  }

  params.mult = params.max_x / (params.max_log - params.min_log);
  params.offset = -params.mult * (1.0 + params.min_log);

  return params;
}

/**
 * @brief Write SVG header and document setup
 */
static void svg_write_header(std::fstream& output, planet* innermost_planet,
                             const std::string& prognam, const SVGScaleParams& params) {
  output << "<?xml version='1.0' standalone='no'?>\n";
  output << "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' \n";
  output << "  'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>\n";
  output << "\n";
  output << "<svg width='100%' height='100%' version='1.1'\n";
  output << "     xmlns='http://www.w3.org/2000/svg'\n";
  output << "     viewBox='-" << params.margin << " -" << params.margin << " "
         << (params.max_x + (params.margin * 2.0)) << " "
         << (params.max_y + (params.margin * 2.0)) << "' >\n";
  output << "\n";
  output << "<title>" << escapeXmlText(innermost_planet->getTheSun().getName()) << "</title>\n";
  output << "<desc>Created by: " << escapeXmlText(prognam) << " - " << stargen_revision << "</desc>\n";
  output << "\n";
}

/**
 * @brief Draw axis and tick marks
 */
static void svg_draw_axis(std::fstream& output, const SVGScaleParams& params) {
  output << "<g stroke='#8893b5' stroke-width='1'>\n";
  output << "    <line x1='" << ((params.offset + params.mult) + (params.min_log * params.mult))
         << "' y1='" << (params.max_y - params.margin) << "' x2='"
         << ((params.offset + params.mult) + (params.max_log * params.mult)) << "' y2='"
         << (params.max_y - params.margin) << "' />\n";

  for (int x = params.floor; x <= params.ceiling; x++) {
    for (float n = 1.0; n < 9.9; n++) {
      if (params.min_log <= (x + log10(n)) && params.max_log >= (x + log10(n))) {
        float value = (n == 1) ? 10 : 5;
        output << "    <line x1='" << ((params.offset + params.mult) + ((x + log10(n)) * params.mult))
               << "' y1='" << (params.max_y - params.margin - value) << "' x2='"
               << ((params.offset + params.mult) + ((x + log10(n)) * params.mult)) << "' y2='"
               << (params.max_y - params.margin + value) << "' />\n";
      }
    }
  }

  output << "</g>\n\n";
}

/**
 * @brief Draw habitable zone indicators
 */
static void svg_draw_habitable_zone(std::fstream& output, sun& the_sun, const SVGScaleParams& params) {
  long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
  long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

  // Center line of habitable zone
  output << "<line x1='" << ((params.offset + params.mult) + (log10(the_sun.getREcosphere(1.0)) * params.mult))
         << "' y1='" << ((params.max_y - params.margin) - 5) << "' x2='"
         << ((params.offset + params.mult) + (log10(the_sun.getREcosphere(1.0)) * params.mult))
         << "' y2='" << ((params.max_y - params.margin) + 5)
         << "' stroke='#9fe6b4' stroke-width='1' />\n";

  // Habitable zone range
  output << "<line x1='" << ((params.offset + params.mult) + (log10(min_r_ecosphere) * params.mult))
         << "' y1='" << (params.max_y - params.margin) << "' x2='"
         << ((params.offset + params.mult) + (log10(max_r_ecosphere) * params.mult)) << "' y2='"
         << (params.max_y - params.margin)
         << "' stroke='#3ddc84' stroke-width='10' stroke-opacity='0.30' />\n";
}

/**
 * @brief Draw axis labels (AU distances)
 */
static void svg_draw_axis_labels(std::fstream& output, const SVGScaleParams& params) {
  output << "<g font-family='Arial' font-size='10' \n";
  output << "   font-style='normal' font-weight='normal'\n";
  output << "   fill='#aab2cc' text-anchor='middle'>\n";

  for (int x = params.floor; x <= params.ceiling; x++) {
    if (params.min_log <= x && params.max_log >= x) {
      output << "    <text x='" << ((params.offset + params.mult) + (x * params.mult))
             << "' y='120'> " << std::pow(10.0, x) << " AU </text>\n";
    }
  }

  output << "\n";
}

/**
 * @brief Draw all planets in the system
 */
static void svg_draw_planets(std::fstream& output, planet* innermost_planet, const SVGScaleParams& params) {
  for (planet* a_planet : g_sim_context.planets) {
    long double x = (params.offset + params.mult) + (log10(a_planet->getA()) * params.mult);
    long double r = std::pow((a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES), 1.0 / 3.0) * params.em_scale;
    long double x1 = (params.offset + params.mult) + (log10(a_planet->getA() * (1.0 - a_planet->getE())) * params.mult);
    long double x2 = (params.offset + params.mult) + (log10(a_planet->getA() * (1.0 + a_planet->getE())) * params.mult);

    output << "    <circle cx='" << x << "' cy='30' r='" << r
           << "' fill='none' stroke='#cdd6ee' stroke-width='1' />\n";
    output << "    <line x1='" << x1 << "' y1='" << ((params.max_y - params.margin) - 15.0)
           << "' x2='" << x2 << "' y2='" << ((params.max_y - params.margin) - 15.0)
           << "' stroke='#cdd6ee' stroke-width='8' stroke-opacity='0.35'/>\n";
    output << "    <text x='" << x << "' y='" << (params.max_y - (params.margin * 2.0)) << "'>";
    output << (a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
    output << "</text>\n\n";
  }
}

/**
 * @brief Build the hover-title string for a body in the icon strip.
 */
static auto strip_icon_title(planet* body, const std::string& label) -> std::string {
  std::ostringstream info;
  info << label << " " << type_string(body) << ": " << (body->getMass() * SUN_MASS_IN_EARTH_MASSES)
       << " EM, " << toString(body->getA(), 3) << " AU";
  if (!is_gas_planet(body) && body->getType() != tUnknown) {
    info << ", " << (body->getSurfTemp() - FREEZING_POINT_OF_WATER) << " C";
  }
  return info.str();
}

/**
 * @brief Emit one <a>-wrapped type icon (image) centered at (cx, cy).
 */
static void strip_icon_image(std::fstream& output, planet* body, double cx, double cy, double w,
                             const std::string& href, const std::string& url_path,
                             const std::string& title) {
  output << "<a href='" << href << "'>";
  output << "<title>" << escapeXmlText(title) << "</title>";
  output << "<image href='" << escapeXmlAttr(url_path) << "ref/" << image_type_string(body)
         << "Planet.webp' x='" << (cx - (w / 2.0)) << "' y='" << (cy - (w / 2.0)) << "' width='" << w
         << "' height='" << w << "' />";
  output << "</a>\n";
}

// Icon side lengths (in viewBox units), shared by the layout pre-pass and the
// draw pass so the two can never diverge.
static auto planet_icon_width(planet* p) -> double {
  if (p->getType() == tAsteroids) {
    return 11.0;
  }
  double w = sqrt(convert_km_to_eu(p->getRadius())) * 14.0;
  if (w < 9.0) {
    w = 9.0;
  }
  if (w > 40.0) {
    w = 40.0;
  }
  return w;
}

static auto moon_icon_width(planet* m) -> double {
  double mw = sqrt(convert_km_to_eu(m->getRadius())) * 18.0;
  if (mw < 5.0) {
    mw = 5.0;
  }
  if (mw > 14.0) {
    mw = 14.0;
  }
  return mw;
}

/**
 * @brief Draw the whole system as ONE continuous SVG: type icons positioned by
 *        log-distance, each connected straight down to its place on the AU axis,
 *        with the habitable-zone band and distance labels in the same figure.
 *
 * Because the icons and the scale live in a single coordinate system there is no
 * separator or gap between them — a planet's icon sits directly above its tick on
 * the axis and its eccentricity bar, and the figure reads top-to-bottom as one
 * unit. Moons (when do_moons) stack vertically above their parent planet at the
 * same x. Sets the three habitable-category flags so the caller can decide
 * whether to emit the terrestrials table.
 */
static void svg_draw_system_map(std::fstream& output, const SVGScaleParams& params,
                                const std::string& url_path, const std::string& system_url,
                                bool int_link, bool do_gases, bool do_moons, sun& the_sun,
                                bool& terrestrials_seen, bool& habitable_jovians_seen,
                                bool& potentialy_habitables_seen) {
  const double view_w      = (double)(params.max_x + (params.margin * 2.0));
  const double view_bottom = (double)(params.max_y + params.margin);  // keeps the AU labels (y=120) in view
  const double axis_y      = (double)(params.max_y - params.margin);  // where svg_draw_axis puts the line
  const double baseline    = 50.0;                                    // y of planet icon centers
  const double ecc_y       = axis_y - 12.0;                           // eccentricity bars sit just above the axis
  const double moon_gap    = 2.0;

  // Pre-pass: find the highest point any icon/moon reaches so the viewBox can
  // grow upward to fit the tallest moon stack (nothing clips) yet stay snug when
  // there are few moons. Seed with the sun icon's top edge.
  double min_top = baseline - 16.0;
  for (planet* p : g_sim_context.planets) {
    double w = planet_icon_width(p);
    if ((baseline - (w / 2.0)) < min_top) {
      min_top = baseline - (w / 2.0);
    }
    if (do_moons) {
      double cursor = (baseline - (w / 2.0)) - 3.0;
      for (planet* moon : p->moons) {
        double mw  = moon_icon_width(moon);
        double top = cursor - mw;  // top edge = (cursor - mw/2) - mw/2
        if (top < min_top) {
          min_top = top;
        }
        cursor = top - moon_gap;
      }
    }
  }
  const double view_top = min_top - 4.0;
  const double view_h   = view_bottom - view_top;

  // Full-width, single figure. Generous height cap so it is not cramped; because
  // everything is one SVG. No height cap: width fills the cell and the height
  // follows from the viewBox aspect ratio (height:auto), so it spans the full
  // page width like the old GIF overview and grows as tall as it needs.
  output << "<svg xmlns='http://www.w3.org/2000/svg' version='1.1' width='100%' "
            "preserveAspectRatio='xMidYMid meet'\n";
  output << "     viewBox='-" << params.margin << " " << view_top << " " << view_w << " " << view_h
         << "' style='display:block;width:100%;height:auto'>\n";

  // Scale first, so icons/connectors paint on top of it.
  svg_draw_axis(output, params);
  svg_draw_habitable_zone(output, the_sun, params);

  // The sun has no log-distance position; pin it at the far left edge.
  output << "<image href='" << escapeXmlAttr(url_path) << "ref/Sun.gif' x='"
         << (-params.margin + 2.0) << "' y='" << (baseline - 16.0) << "' width='10' height='32' />\n";

  int counter = 1;
  for (planet* p : g_sim_context.planets) {
    if ((p->getSurfPressure() > 0 || p->getGasGiant()) && p->getNumGases() == 0 && do_gases) {
      std::string pid = std::to_string(counter);
      calculate_gases(the_sun, p, pid);
    }

    double x = (double)((params.offset + params.mult) + (log10(p->getA()) * params.mult));
    double w = planet_icon_width(p);

    // Eccentricity bar + a connector from the icon straight down to the axis, so
    // the icon and its position on the scale read as one continuous mark.
    double x1 = (double)((params.offset + params.mult) + (log10(p->getA() * (1.0 - p->getE())) * params.mult));
    double x2 = (double)((params.offset + params.mult) + (log10(p->getA() * (1.0 + p->getE())) * params.mult));
    output << "<line x1='" << x1 << "' y1='" << ecc_y << "' x2='" << x2 << "' y2='" << ecc_y
           << "' stroke='#cdd6ee' stroke-width='8' stroke-opacity='0.35' />\n";
    output << "<line x1='" << x << "' y1='" << (baseline + (w / 2.0)) << "' x2='" << x << "' y2='" << axis_y
           << "' stroke='#cdd6ee' stroke-width='1' stroke-opacity='0.30' />\n";

    std::string p_href = (int_link ? "" : escapeXmlAttr(system_url)) + "#" + std::to_string(counter);
    strip_icon_image(output, p, x, baseline, w, p_href, url_path,
                     strip_icon_title(p, "#" + std::to_string(counter)));

    if (is_terrestrial(p)) {
      terrestrials_seen = true;
    }
    if (is_habitable_jovian(p)) {
      habitable_jovians_seen = true;
    }
    if (is_potentialy_habitable(p)) {
      potentialy_habitables_seen = true;
    }

    // Moons: stack upward from just above the planet icon, at the same x. The
    // pre-pass already sized the viewBox so the whole stack fits without clipping.
    if (do_moons) {
      double cursor = (baseline - (w / 2.0)) - 3.0;  // top edge of planet, minus a gap
      int    midx   = 1;
      for (planet* moon : p->moons) {
        if (is_terrestrial(moon)) {
          terrestrials_seen = true;
        }
        if (is_habitable_jovian(moon)) {
          habitable_jovians_seen = true;
        }
        if (is_potentialy_habitable(moon)) {
          potentialy_habitables_seen = true;
        }

        double      mw      = moon_icon_width(moon);
        double      mcy     = cursor - (mw / 2.0);
        std::string m_label = "#" + std::to_string(counter) + "." + std::to_string(midx);
        std::string m_href  = (int_link ? "" : escapeXmlAttr(system_url)) + m_label;
        strip_icon_image(output, moon, x, mcy, mw, m_href, url_path, strip_icon_title(moon, m_label));
        cursor = (mcy - (mw / 2.0)) - moon_gap;
        midx++;
      }
    }

    counter++;
  }

  // AU distance labels last; this opens a <g> that the closing </g> wraps.
  svg_draw_axis_labels(output, params);
  output << "</g>\n</svg>\n";
}

void create_svg_file(planet* innermost_planet, std::string path, std::string file_name, std::string svg_ext,
                     std::string prognam, bool do_moons) {
  performanceMonitor.recordFileOperation("svg_output");

  // path already carries a trailing separator (see stargen.cpp), and a plain
  // concat is byte-identical for every input incl. an absolute -o name (fs::path
  // operator/ would instead let an absolute file_name override the directory).
  const std::string the_file_spec = path + file_name + svg_ext;

  std::fstream output;
  output.open(the_file_spec.c_str(), std::ios::out);
  if (!output) {
    throw std::runtime_error("StarGen: cannot open SVG file '" + the_file_spec +
                             "' for writing");
  }

  // Find outermost planet (last in vector)
  planet* outermost_planet = g_sim_context.planets.empty() ? innermost_planet : g_sim_context.planets.back();

  // Calculate scaling parameters
  SVGScaleParams params = calculate_svg_scale(innermost_planet, outermost_planet);

  // Generate SVG document
  svg_write_header(output, innermost_planet, prognam, params);
  svg_draw_axis(output, params);

  sun the_sun = innermost_planet->getTheSun();
  svg_draw_habitable_zone(output, the_sun, params);
  svg_draw_axis_labels(output, params);
  svg_draw_planets(output, innermost_planet, params);

  output << "</g>\n";
  output << "</svg>\n";

  ZoneScoped;
  output.close();
}

/**
 * @brief openCVSorJson
 *
 * @param path
 * @param the_filename
 * @param output
 */
void openCVSorJson(std::string path, std::string the_filename, std::fstream& output) {
  ensure_directory_exists(path);
  const std::string the_file_spec = path + the_filename;

  output.open(the_file_spec.c_str(), std::ios::out);
  if (!output) {
    throw std::runtime_error("StarGen: cannot open output file '" +
                             the_file_spec + "' for writing");
  }
  ZoneScoped;
}

/**
 * @brief Refresh file stream
 *
 * @param output
 * @param path
 * @param file_name
 * @param ext
 */
void refresh_file_stream(std::fstream& output, const std::string& path, const std::string& file_name,
                         const std::string& ext) {
  output.close();

  const std::string the_file_spec = path + file_name + ext;

  output.open(the_file_spec.c_str(), std::fstream::out | std::fstream::app);
  if (!output) {
    throw std::runtime_error("StarGen: cannot reopen output file '" +
                             the_file_spec + "' for writing");
  }
}

/**
 * @brief Open HTML file
 *
 * @param system_name
 * @param seed
 * @param path
 * @param url_path
 * @param file_name
 * @param ext
 * @param prognam
 * @param output
 */
void open_html_file(const std::string& system_name, long seed, const std::string& path,
                    const std::string& url_path, const std::string& file_name, const std::string& ext,
                    const std::string& prognam, std::fstream& output) {
  ensure_directory_exists(path);
  const std::string the_file_spec = path + file_name + ext;
  const bool noname = system_name.empty();

  output.open(the_file_spec.c_str(), std::fstream::out);
  if (!output) {
    throw std::runtime_error("StarGen: cannot open HTML file '" + the_file_spec +
                             "' for writing");
  }

  output << "<!DOCTYPE html>\n";
  output << "<html lang='en'>\n";
  output << "<head>\n";
  output << "<meta charset='utf-8'>\n";
  output << "<meta name='viewport' content='width=device-width, initial-scale=1'>\n";
  output << "\t<title>System " << seed << (noname ? "" : " - ")
         << (noname ? "" : escapeXmlText(system_name)) << "</title>\n";
  output << "\t<meta name='generator' content='" << escapeXmlAttr(prognam) << " - "
         << stargen_revision << "'>\n";
  // Modern "deep space" stylesheet. The dark palette comes from the BG*/TX*
  // constants (emitted as presentational attributes, which CSS overrides); this
  // sheet layers on typography and turns the bordered (border=3) panels into
  // rounded cards. It deliberately does NOT set a global img max-width or cell
  // padding, so the system-overview strip keeps its intrinsic, radius-proportional
  // planet icons and reads as a horizontal "solar system" lineup (wide systems
  // scroll horizontally, as before). Only the bordered panels get card chrome;
  // the black orbit strip (border=0) is left as plain layout.
  output << R"CSS(<style>
:root{--ink:#dfe4f0;--muted:#9aa3c0;--line:#283353;--accent:#7fd1e0;}
*{box-sizing:border-box;}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;
  background:radial-gradient(1200px 700px at 50% -10%,#16203c 0%,#0b0f1c 55%,#05070e 100%) fixed;
  color:var(--ink);line-height:1.5;margin:0;padding:24px 12px;font-size:15px;}
a{color:var(--accent);text-decoration:none;}
a:hover{color:#bdeef7;text-decoration:underline;}
img{border:0;vertical-align:middle;}
hr{border:0;border-top:1px solid var(--line);margin:18px 0;}
/* Card chrome only on the bordered panels; no overflow:hidden so a wide orbit
   strip is never clipped. */
table[border='3']{border-collapse:separate;border-spacing:0;border:1px solid var(--line);
  border-radius:12px;box-shadow:0 8px 26px rgba(0,0,0,.40);margin:18px auto;}
/* Tame the oversized legacy inline planet thumbnails in data tables. */
td img[width='600']{width:auto;max-width:128px;height:auto;}
font{font-style:normal;}
center{color:var(--muted);font-size:13px;}
/* --- Readability layer (HTML-only, value-coded) ------------------------- */
/* Zebra + hover on the TOP-LEVEL value cells of the bordered detail tables.
   Scoped with '>tbody>tr>td' so nested tables (temperature range, gases,
   oblateness) and the label/header <th> cells are never touched. */
table[border='3']>tbody>tr:nth-of-type(even)>td{background:rgba(255,255,255,.022);}
table[border='3']>tbody>tr:hover>td{background:rgba(127,209,224,.075);}
/* Temperature-coded headline cell: blue=frozen -> green=temperate -> red=hot.
   !important so the tint wins over the zebra stripe on even rows. */
.t-frozen{background:rgba(120,170,255,.20)!important;color:#e6eeff!important;}
.t-cold{background:rgba(90,200,230,.16)!important;color:#dcf6fc!important;}
.t-temperate{background:rgba(110,225,140,.20)!important;color:#e3ffe9!important;}
.t-warm{background:rgba(255,190,90,.18)!important;color:#ffeccd!important;}
.t-hot{background:rgba(255,105,75,.22)!important;color:#ffdcd3!important;}
/* Status chips / glyphs shown under the planet type. */
.chips{margin-top:8px;line-height:2.1;}
.chip{display:inline-block;font-size:12px;line-height:1;padding:5px 9px;margin:0 6px 2px 0;
  border-radius:999px;border:1px solid var(--line);background:rgba(255,255,255,.04);
  white-space:nowrap;}
.chip-good{color:#8aeaa6;border-color:rgba(127,230,160,.45);background:rgba(127,230,160,.10);}
.chip-warn{color:#ffce7a;border-color:rgba(255,197,107,.45);background:rgba(255,197,107,.10);}
.chip-bad{color:#ff957a;border-color:rgba(255,138,107,.45);background:rgba(255,138,107,.10);}
.glyph{font-style:normal;font-weight:600;}
.poison{color:#ff957a;font-weight:600;}
/* Potentially/▶ habitable planet: green halo + header badge. */
table.habitable{box-shadow:0 0 0 2px rgba(127,230,160,.55),0 10px 30px rgba(0,0,0,.5)!important;}
.badge{display:inline-block;font-size:13px;font-weight:700;letter-spacing:.04em;
  padding:3px 11px;margin-left:12px;border-radius:999px;vertical-align:middle;
  color:#04230f;background:#7fe6a0;}
.badge-maybe{color:#11210a;background:#cfe89a;}
</style>
)CSS";
  output << "<link rel='icon' type='image/png' href='" << escapeXmlAttr(url_path) << "ref/favicon.png'>\n";
  output << "</head>\n";
  output << "<body bgcolor='" << BGCOLOR << "' text='" << TXCOLOR << "' link='" << LINKCOLOR
         << "' vlink='" << TXCOLOR << "' alink='" << ALINKCOLOR << "'>\n\n";
}

/**
 * @brief Close HTML file
 *
 * @param the_file
 */
void close_html_file(std::fstream& the_file) {
  the_file << "<p>\n\n";
  the_file << "<center>\n";
  the_file << "This page was created by omega13a's variant of <a href='" << STARGEN_URL
           << "'>StarGen</a> (" << stargen_revision << ").\n";
  the_file << "</center>\n";
  the_file << "</p>\n\n";
  the_file << "</body>\n</html>\n";
  the_file.close();
}

/**
 * @brief Print description
 *
 * @param the_file
 * @param opening
 * @param the_planet
 * @param closing
 */
/**
 * @brief Classify planet's gravity level
 */
static auto describe_gravity_level(long double gravity) -> std::string {
  switch (static_cast<int>(gravity * 2)) {
    case 0:
    case 1:
      return "Very Low-G";
    case 2:
    case 3:
      return "Low-G";
    case 4:
    case 5:
      return "Earth-Like-G";
    case 6:
    case 7:
      return "High-G";
    default:
      return "Very-High-G";
  }
}

/**
 * @brief Classify planet's temperature relative to Earth
 */
static auto describe_temperature_level(long double rel_temp) -> std::string {
  switch (static_cast<int>(rel_temp + 5) / 5) {
    case 0:
      return "Cold";
    case 1:
      return "Cool";
    case 2:
      return "Mild";
    case 3:
      return "Warm";
    default:
      return "Hot";
  }
}

// === HTML readability helpers (value-coded cells, status glyphs) ===========
// Map an absolute temperature (Kelvin) to a CSS tier class used to tint the
// headline temperature cell (blue=frozen -> green=temperate -> red=hot). The
// bands bracket the liquid-water range so a temperate world reads green at a
// glance. HTML-only; the golden TEXT output does not use this.
static auto temp_tier_class(long double kelvin) -> const char* {
  if (kelvin < 240.0L) {
    return "t-frozen";
  }
  if (kelvin < FREEZING_POINT_OF_WATER) {
    return "t-cold";
  }
  if (kelvin < 323.15L) {
    return "t-temperate";
  }
  if (kelvin < 373.15L) {
    return "t-warm";
  }
  return "t-hot";
}

/**
 * @brief Describe iron core size
 */
static auto describe_iron_core(planet* the_planet, long double iron) -> std::string {
  if (the_planet->getType() != tIron && the_planet->getType() != tAsteroids) {
    switch (static_cast<int>(iron / 20)) {
      case 5:
        return "'Cannonball'";
      case 4:
      case 3:
        return "Large Iron Core";
      case 2:
      case 1:
        return "Medium Iron Core";
      case 0:
        return iron < 1 ? "Coreless" : "Small Iron Core";
    }
  } else if (the_planet->getType() == tAsteroids) {
    if (iron > 80)
      return "Metallic";
    else if (iron < 20)
      return "Rocky";
  }
  return "";
}

/**
 * @brief Describe ice coverage
 */
static auto describe_ice_coverage(planet* the_planet, long double ice) -> std::string {
  if (the_planet->getType() == tIce) {
    switch (static_cast<int>(ice / 25)) {
      case 4:
      case 3:
        return "Mostly Ice";
      case 2:
        return "Half Ice";
      case 1:
        return "Significant Ice";
      default:
        return ice > 10.0 ? "A Bit Icey" : "";
    }
  } else if (ice > 90.0) {
    return "Ice Covered";
  }
  return "";
}

/**
 * @brief Write atmosphere and surface descriptions
 */
static void describe_atmosphere_surface(std::fstream& the_file, planet* the_planet, bool& first,
                                        long double atmosphere, long double seas,
                                        long double clouds) {
  if (atmosphere < 0.001) {
    lprint(the_file, first, "Airless");
  } else {
    // Hydrosphere description
    if (the_planet->getType() != tWater) {
      switch (static_cast<int>(seas / 10)) {
        case 0:
          lprint(the_file, first, "Desert");
          break;
        case 1:
          lprint(the_file, first, "Semi-Arid");
          break;
        case 2:
          lprint(the_file, first, "Arid");
          break;
        case 3:
        case 4:
          lprint(the_file, first, "Dry");
          break;
        case 5:
        case 6:
        case 7:
          lprint(the_file, first, "Moist");
          break;
        case 8:
          lprint(the_file, first, "Wet");
          break;
        default:
          lprint(the_file, first, "Water World");
          break;
      }
    }

    // Cloud description
    if (clouds < 10.0)
      lprint(the_file, first, "Cloudless");
    else if (clouds < 40.0)
      lprint(the_file, first, "Few clouds");
    else if (clouds > 80.0)
      lprint(the_file, first, "Cloudy");

    // Boiling ocean check
    if (the_planet->getMaxTemp() > the_planet->getBoilPoint() && seas > 0.0) {
      lprint(the_file, first, "Boiling ocean");
    }

    // Atmosphere thickness
    if (atmosphere < (MIN_O2_IPP / EARTH_SURF_PRES_IN_MILLIBARS)) {
      lprint(the_file, first, "Unbreathably thin atmosphere");
    } else if (atmosphere < 0.5) {
      lprint(the_file, first, "Thin atmosphere");
    } else if (atmosphere > (MAX_HABITABLE_PRESSURE / EARTH_SURF_PRES_IN_MILLIBARS)) {
      lprint(the_file, first, "Unbreathably thick atmosphere");
    } else if (atmosphere > 2.0) {
      lprint(the_file, first, "Thick atmosphere");
    } else if (the_planet->getType() != tTerrestrial) {
      lprint(the_file, first, "Normal atmosphere");
    }
  }
}

/**
 * @brief Thermal adjective (Hot/Warm/Cold/Frigid) from the planet's most
 *        meaningful temperature.
 *
 * Uses the greenhouse surface temperature for worlds that have a surface, and the
 * (reflective cloud-top) estimated temperature for gas giants/dwarfs, which have
 * no surface. This replaces the old orbit-position-only word, which labelled a
 * bright, cold gas dwarf "Warm" merely for sitting in the habitable zone even
 * though its displayed Estimated Temperature was far below freezing. Note we do
 * NOT use the equilibrium temp for rocky worlds: Earth's own equilibrium temp is
 * ~255 K (-18 C), which would mislabel Earth-likes as "Cold".
 */
static auto describe_thermal_class(planet* the_planet) -> std::string {
  long double temp_k =
      is_gas_planet(the_planet) ? the_planet->getEstimatedTemp() : the_planet->getSurfTemp();
  if (temp_k >= FREEZING_POINT_OF_WATER + 50.0) {
    return "Hot ";
  }
  if (temp_k >= FREEZING_POINT_OF_WATER) {
    return "Warm ";
  }
  if (temp_k >= FREEZING_POINT_OF_WATER - 50.0) {
    return "Cold ";
  }
  return "Frigid ";
}

/**
 * @brief Describe habitability classification
 */
static auto describe_habitability(planet* the_planet, sun& the_sun) -> std::string {
  long double min_r_ecosphere = habitable_zone_distance(
      the_sun, RECENT_VENUS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
  long double max_r_ecosphere = habitable_zone_distance(
      the_sun, EARLY_MARS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  std::string result;

  if (the_planet->getA() < min_r_ecosphere || the_planet->getA() > max_r_ecosphere) {
    if (is_habitable_extended(the_planet)) {
      result = "Habitable (Extended Definition) ";
    } else if (is_potentialy_habitable_extended(the_planet)) {
      result = "Potentially Habitable (Extended Definition) ";
    }
  } else {
    if (is_habitable(the_planet)) {
      if (is_habitable_earth_like(the_planet)) {
        result = "Habitable (Earth-like Definition) ";
      } else if (is_habitable_conservative(the_planet)) {
        result = "Habitable (Conservative Definition) ";
      } else if (is_habitable_optimistic(the_planet)) {
        result = "Habitable (Optimistic Definition) ";
      } else if (is_habitable_extended(the_planet)) {
        result = "Habitable (Extended Definition) ";
      }
    } else if (is_potentialy_habitable(the_planet)) {
      if (is_potentialy_habitable_earth_like(the_planet)) {
        result = "Potentially Habitable (Earth-like Definition) ";
      } else if (is_potentialy_habitable_conservative(the_planet)) {
        result = "Potentially Habitable (Conservative Definition) ";
      } else if (is_potentialy_habitable_optimistic(the_planet)) {
        result = "Potentially Habitable (Optimistic Definition) ";
      } else if (is_potentialy_habitable_extended(the_planet)) {
        result = "Potentially Habitable (Extended Definition) ";
      }
    }
  }

  // Thermal adjective from actual temperature, not orbital position.
  result += describe_thermal_class(the_planet);

  return result;
}

/**
 * @brief Classify planet size category
 */
static auto describe_size_class(long double earth_masses, long double earth_radii) -> std::string {
  if (earth_masses <= 0.00001 || earth_radii <= 0.03) {
    return "Asteroidan";
  } else if (earth_masses <= 0.1 || earth_radii <= 0.4) {
    return "Mercurian";
  } else if (earth_masses <= 0.5 || earth_radii <= 0.8) {
    return "Subterran";
  } else if (earth_masses <= 5.0 || earth_radii <= 1.5) {
    return "Terran";
  } else if (earth_masses <= 10.0 || earth_radii <= 2.5) {
    return "Superterran";
  } else if (earth_masses <= 50.0 || earth_radii <= 6.0) {
    return "Neptunian";
  } else {
    return "Jovian";
  }
}

void print_description(std::fstream& the_file, const std::string& opening, planet* the_planet,
                       const std::string& closing) {
  bool         first        = true;
  long double  earth_masses = the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES;
  long double  earth_radii  = convert_km_to_eu(the_planet->getRadius());

  the_file << opening;

  // Day length classification
  if (the_planet->getDay() < 10.0) {
    lprint(the_file, first, "Short Day");
  } else if (the_planet->getDay() > 48.0) {
    lprint(the_file, first, "Long Day");
  }

  // Dwarf planet check
  if (the_planet->getMoonA() == 0 && calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0 &&
      the_planet->getType() != tAsteroids) {
    lprint(the_file, first, "Dwarf planet");
  }

  if (is_gas_planet(the_planet)) {
    long double temp = the_planet->getEstimatedTemp() - FREEZING_POINT_OF_WATER;
    lprint(the_file, first, std::format("{} Earth Masses", earth_masses));
    lprint(the_file, first, std::format("{} C", temp));
  } else {
    // Rocky planet descriptions
    long double rel_temp   = (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) - EARTH_AVERAGE_CELSIUS;
    long double seas       = 100.0 * the_planet->getHydrosphere();
    long double clouds     = 100.0 * the_planet->getCloudCover();
    long double atmosphere = the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS;
    long double ice        = 100.0 * the_planet->getIceCover();
    long double gravity    = the_planet->getSurfGrav();
    long double iron       = (1.0 - (the_planet->getRmf() + the_planet->getImf())) * 100.0;

    // Apply classification helpers
    lprint(the_file, first, describe_gravity_level(gravity));
    lprint(the_file, first, describe_temperature_level(rel_temp));

    std::string iron_desc = describe_iron_core(the_planet, iron);
    if (!iron_desc.empty()) {
      lprint(the_file, first, iron_desc);
    }

    std::string ice_desc = describe_ice_coverage(the_planet, ice);
    if (!ice_desc.empty()) {
      lprint(the_file, first, ice_desc);
    }

    describe_atmosphere_surface(the_file, the_planet, first, atmosphere, seas, clouds);

    if (is_earth_like(the_planet)) {
      lprint(the_file, first, "Earth-like");
    }

    if (the_planet->getNumGases() > 0) {
      first = true;
      unsigned int temp;

      the_file << " (";

      for (int i = 0; i < the_planet->getNumGases(); i++) {
        int index = gases.count();

        for (int n = 0; n < gases.count(); n++) {
          if (gases[n].getNum() == the_planet->getGas(i).getNum()) {
            index = n;
            break;
          }
        }
        if ((the_planet->getGas(i).getSurfPressure() / the_planet->getSurfPressure()) > 0.01) {
          lprint(the_file, first, gases[index].getHtmlSymbol());
        }
      }

      temp = breathability(the_planet);
      if (temp != NONE) {
        the_file << " - " << breathability_phrase[temp] << ")";
      }
    }
  }

  // Habitability and size classification
  the_file << " - ";

  sun the_sun = the_planet->getTheSun();
  the_file << describe_habitability(the_planet, the_sun);
  the_file << describe_size_class(earth_masses, earth_radii);
  the_file << " (" << the_planet->getOrbPeriod() << " day long orbit)";

  the_file << closing;
}

/**
 * @brief List molecules
 *
 * @param the_file
 * @param weight
 */
void list_molecules(std::fstream& the_file, long double weight) {
  int  count = 0;
  int  max   = 8;
  bool first = true;

  mol_print(the_file, first, count, max, weight, "H", ATOMIC_HYDROGEN);
  mol_print(the_file, first, count, max, weight, "H<sub><small>2</small></sub>", MOL_HYDROGEN);
  mol_print(the_file, first, count, max, weight, "He", HELIUM);
  mol_print(the_file, first, count, max, weight, "N", ATOMIC_NITROGEN);
  mol_print(the_file, first, count, max, weight, "O", ATOMIC_OXYGEN);
  mol_print(the_file, first, count, max, weight, "CH<sub><small>4</small></sub>", METHANE);
  mol_print(the_file, first, count, max, weight, "NH<sub><small>3</small></sub>", AMMONIA);
  mol_print(the_file, first, count, max, weight, "H<sub><small>2</small></sub>O", WATER_VAPOR);
  mol_print(the_file, first, count, max, weight, "Ne", NEON);
  mol_print(the_file, first, count, max, weight, "N<sub><small>2</small></sub>", MOL_NITROGEN);
  mol_print(the_file, first, count, max, weight, "CO", CARBON_MONOXIDE);
  mol_print(the_file, first, count, max, weight, "NO", NITRIC_OXIDE);
  mol_print(the_file, first, count, max, weight, "O<sub><small>2</small></sub>", MOL_OXYGEN);
  mol_print(the_file, first, count, max, weight, "H<sub><small>2</small></sub>S",
            HYDROGEN_SULPHIDE);
  mol_print(the_file, first, count, max, weight, "Ar", ARGON);
  mol_print(the_file, first, count, max, weight, "CO<sub><small>2</small></sub>", CARBON_DIOXIDE);
  mol_print(the_file, first, count, max, weight, "N<sub><small>2</small></sub>O", NITROUS_OXIDE);
  mol_print(the_file, first, count, max, weight, "NO<sub><small>2</small></sub>", NITROGEN_DIOXIDE);
  mol_print(the_file, first, count, max, weight, "O<sub><small>3</small></sub>", OZONE);
  mol_print(the_file, first, count, max, weight, "SO<sub><small>2</small></sub>", SULPH_DIOXIDE);
  mol_print(the_file, first, count, max, weight, "SO<sub><small>3</small></sub>", SULPH_TRIOXIDE);
  mol_print(the_file, first, count, max, weight, "Kr", KRYPTON);
  mol_print(the_file, first, count, max, weight, "Xe", XENON);
}

/**
 * @brief html star details helper
 *
 * @param the_file
 * @param header
 * @param mass
 * @param luminosity
 * @param temperature
 * @param age
 * @param life
 * @param spec_type
 */
void html_star_details_helper(std::fstream& the_file, const std::string& header, long double mass,
                              long double luminosity, long double temperature, long double age,
                              long double life, const std::string& spec_type,
                              long double metallicity) {
  the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>";
  the_file << "<font size='+1' color='" << TXHEADER << "'><b>" << header << "</b></font>";
  the_file << "</td></tr>\n";
  the_file << "<tr><td>Stellar mass</td>\n";
  the_file << "\t<td>" << toString(mass) << " solar masses</td></tr>\n";
  the_file << "<tr><td>Stellar luminosity</td>\n";
  the_file << "\t<td>" << toString(luminosity) << "</td></tr>\n";
  the_file << "<tr><td>Temperature</td>\n";
  the_file << "\t<td>" << toString(temperature) << "</td></tr>\n";
  the_file << "<tr><td>Spectral Type</td>\n";
  the_file << "\t<td>" << escapeXmlText(spec_type) << "</td></tr>\n";
  the_file << "<tr><td>Metallicity [Fe/H]</td>\n";
  the_file << "\t<td>" << toString(metallicity) << " dex</td></tr>\n";
  the_file << "<tr><td>Age</td>\n";
  the_file << "\t<td>" << toString(age / 1.0E9) << " billion years<br>("
           << toString((life - age) / 1.0E9) << " billion left on main sequence)<br></td></tr>";
}

/**
 * @brief Count system objects (planets, dwarf planets, asteroids, moons)
 */
struct SystemObjectCounts {
  int planet_count;
  int dwarf_planet_count;
  int asteroid_belt_count;
  int moon_count;
  int object_count;
};

static auto count_system_objects(planet* innermost_planet, bool do_moons) -> SystemObjectCounts {
  SystemObjectCounts counts = {0, 0, 0, 0, 0};

  for (planet* the_planet : g_sim_context.planets) {
    if (the_planet->getType() == tAsteroids) {
      counts.asteroid_belt_count++;
    } else if (calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0) {
      counts.dwarf_planet_count++;
    } else {
      counts.planet_count++;
    }
    counts.object_count++;

    if (do_moons && !the_planet->moons.empty()) {
      counts.moon_count += the_planet->moons.size();
    }
  }

  return counts;
}

/**
 * @brief Write HTML thumbnail table header
 */
static void html_write_thumbnail_header(std::fstream& the_file, const std::string& system_name,
                                        const std::string& url_path, const std::string& system_url,
                                        const std::string& file_name, const std::string& svg_url,
                                        int graphic_format, const SystemObjectCounts& counts) {
  the_file << "<p>\n\n";
  the_file << "<table border=3 cellspacing=2 cellpadding=2 align=center ";
  the_file << "bgcolor='" << BGTABLE << "' width='90%'>\n";
  the_file << "<tr><th colspan=2 bgcolor='" << BGTABLE << "' align=center>\n";
  the_file << "\t<font size='+2' color='" << TXTABLE << "'>";

  if (file_name.empty()) {
    the_file << escapeXmlText(system_name);
  } else {
    the_file << "<a href='" << escapeXmlAttr(system_url) << "'>" << escapeXmlText(system_name) << "</a>";
  }

  the_file << "</font></th></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td colspan=2>\n";

  if (graphic_format == gfSVG) {
    the_file << "<object data='" << escapeXmlAttr(svg_url) << "' type='image/svg+xml'\n";
    the_file << "        width='100%' height='100%' border=1 "
                "style='background-color:#05070e;'>\n";
  }

  the_file << "<table border=0 cellspacing=0 cellpadding=3 bgcolor='" << BGSPACE
           << "' width='100%'>\n";

  the_file << "<tr><td colspan=" << (counts.object_count + 2) << " bgcolor='" << BGHEADER
           << "' align=center>\n";
  the_file << "\t<font size='+1'  color='" << TXHEADER << "'><b>" << counts.planet_count
           << " Planets,</b> <b>" << counts.dwarf_planet_count << " Dwarf Planets,</b> <b>"
           << counts.asteroid_belt_count << " Asteroid Belts</b>, <b>" << counts.moon_count
           << " Moons</b></font>\n";
  the_file << "\t(<font size='-1' color='" << TXHEADER
           << "'>size proportional to Sqrt(Radius)</font>)\n";
  the_file << "</td></tr>\n";
  the_file << "<tr valign=middle bgcolor='" << BGSPACE << "'>\n";
  the_file << "<td bgcolor='" << BGSPACE << "'><img alt='Sun' src='" << escapeXmlAttr(url_path) << "ref/Sun.gif' ";
  the_file << "width=15 height=63 border=0></td>\n";
}

/**
 * @brief Write a planet thumbnail image and link
 */
static auto html_write_planet_thumbnail(std::fstream& the_file, planet* the_planet, sun& the_sun,
                                        int counter, bool do_gases, const std::string& url_path,
                                        const std::string& system_url, bool int_link) -> bool {
  std::stringstream ss;
  std::string planet_id;

  int ppixels = ((int)(sqrt(convert_km_to_eu(the_planet->getRadius())) * 50.0)) + 1;
  std::string ptype = type_string(the_planet);
  std::string info;

  if ((the_planet->getSurfPressure() > 0 || the_planet->getGasGiant()) &&
      the_planet->getNumGases() == 0 && do_gases) {
    ss.str("");
    ss << counter;
    planet_id = ss.str();
    calculate_gases(the_sun, the_planet, planet_id);
  }

  if (the_planet->getType() == tAsteroids) {
    ppixels = (int)(25.0 + (8.0 * log((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) /
                                      ASTEROID_MASS_LIMIT)));
  }

  if (ppixels < 10) {
    ppixels = 10;
  }

  ss.str("");
  ss << ptype << ": " << (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << " EM";
  if (is_gas_planet(the_planet)) {
    ss << " (c. " << the_planet->getEstimatedTemp() << "&deg;)";
  } else if (the_planet->getType() == tUnknown) {
    ss << ", " << (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) << " EM from gas ("
       << (100.0 * (the_planet->getGasMass() / the_planet->getMass())) << "%)";
  } else {
    ss << " " << the_planet->getSurfGrav() << " g, "
       << (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) << "&deg;";
  }
  ss << " Zone = " << the_planet->getOrbitZone();
  info = ss.str();

  the_file << "\t<td bgcolor='" << BGSPACE << "' align=center><a href='"
           << (int_link ? "" : escapeXmlAttr(system_url)) << "#" << counter << "' title='#" << counter << " - "
           << info << "'>";
  the_file << "<img alt='" << ptype << "' src='" << escapeXmlAttr(url_path) << "ref/"
           << image_type_string(the_planet) << "Planet.webp' width=" << ppixels
           << " height=" << ppixels << " border=0>";
  the_file << "</a>";

  return is_terrestrial(the_planet) || is_habitable_jovian(the_planet) || is_potentialy_habitable(the_planet);
}

/**
 * @brief Result of checking moon interest categories
 */
struct MoonInterestFlags {
  bool has_terrestrials;
  bool has_habitable_jovians;
  bool has_potentialy_habitables;
};

/**
 * @brief Write moon thumbnails for a planet
 */
static auto html_write_moon_thumbnails(std::fstream& the_file, planet* the_planet, sun& the_sun,
                                       int counter, bool do_gases, const std::string& url_path,
                                       const std::string& system_url, bool int_link) -> MoonInterestFlags {
  MoonInterestFlags flags = {false, false, false};
  std::stringstream ss;
  std::string planet_id;
  int moons = 1;

  for (planet* moon : the_planet->moons) {
    ss.str("");
    std::string mtype = type_string(moon);
    int mpixels = ((int)(sqrt(convert_km_to_eu(moon->getRadius())) * 100.)) + 1;

    if ((the_planet->getSurfPressure() > 0 || the_planet->getGasGiant()) &&
        the_planet->getNumGases() == 0 && do_gases) {
      ss.str("");
      ss << counter;
      planet_id = ss.str();
      calculate_gases(the_sun, the_planet, planet_id);
    }

    ss.str("");
    ss << mtype << ": " << (moon->getMass() * SUN_MASS_IN_EARTH_MASSES) << " EM";
    if (is_gas_planet(moon)) {
      ss << " (c. " << moon->getEstimatedTemp() << "&deg;)";
    } else if (moon->getType() == tUnknown) {
      ss << ", " << (moon->getGasMass() * SUN_MASS_IN_EARTH_MASSES) << " EM from gas ("
         << (100.0 * (moon->getGasMass() / moon->getMass())) << "%)";
    } else {
      ss << " " << moon->getSurfGrav() << " g, "
         << (moon->getSurfTemp() - FREEZING_POINT_OF_WATER) << "&deg;";
    }
    ss << " Zone = " << moon->getOrbitZone();
    std::string info = ss.str();

    the_file << "\n\t\t<br><a href='" << (int_link ? "" : escapeXmlAttr(system_url)) << "#" << counter << "."
             << moons << "' title='#" << counter << "." << moons << " - " << info << "'>";
    the_file << "<img alt='" << mtype << "' src='" << escapeXmlAttr(url_path) << "ref/"
             << image_type_string(moon) << "Planet.webp' width=" << mpixels
             << " height=" << mpixels << " border=0>";
    the_file << "</a>";

    if (is_terrestrial(moon)) {
      flags.has_terrestrials = true;
    }
    if (is_habitable_jovian(moon)) {
      flags.has_habitable_jovians = true;
    }
    if (is_potentialy_habitable(moon)) {
      flags.has_potentialy_habitables = true;
    }
    moons++;
  }

  return flags;
}

/**
 * @brief Write terrestrial/habitable planets description table
 */
static void html_write_terrestrials_table(std::fstream& the_file, planet* innermost_planet,
                                          const std::string& system_url, bool int_link,
                                          bool do_moons) {
  the_file << "<tr><td colspan=2><table width='100%'>";

  int counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    if (is_habitable_jovian(the_planet) || is_terrestrial(the_planet) ||
        is_potentialy_habitable(the_planet)) {
      the_file << "\n\t<tr><td align=right width='5%'>";
      the_file << "<a href='" << (int_link ? "" : escapeXmlAttr(system_url)) << "#" << counter << "'><small>#"
               << counter << "</small></a>";
      the_file << "</td>\n\t\t<td><small>" << type_string(the_planet) << ": </small>";

      print_description(the_file, "", the_planet, "");

      the_file << "</td></tr>";
    }

    if (do_moons && !the_planet->moons.empty()) {
      int moons = 1;
      for (planet* moon : the_planet->moons) {
        if (is_habitable_jovian(moon) || is_terrestrial(moon) || is_potentialy_habitable(moon)) {
          the_file << "\n\t<tr><td align=right width='5%'>";
          the_file << "<a href='" << (int_link ? "" : escapeXmlAttr(system_url)) << "#" << counter << "."
                   << moons << "'><small>#" << counter << "." << moons << "</small></a>";
          the_file << "</td>\n\t\t<td><small>" << type_string(moon) << ": </small>";

          print_description(the_file, "", moon, "");

          the_file << "</td></tr>";
        }
        moons++;
      }
    }
    counter++;
  }

  the_file << "</table></td></tr>\n";
}

/**
 * @brief Write star system details section
 */
static void html_write_system_details(std::fstream& the_file, sun& the_sun) {
  long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
  long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

  if (!the_sun.getIsCircumbinary()) {
    html_star_details_helper(the_file, "Stellar Characteristics", the_sun.getMass(),
                             the_sun.getLuminosity(), the_sun.getEffTemp(), the_sun.getAge(),
                             the_sun.getLife(), the_sun.getSpecType(), the_sun.getMetallicity());
  } else {
    html_star_details_helper(the_file, "Stellar Characteristics of Primary", the_sun.getMass(),
                             the_sun.getLuminosity(), the_sun.getEffTemp(), the_sun.getAge(),
                             the_sun.getLife(), the_sun.getSpecType(), the_sun.getMetallicity());
    html_star_details_helper(the_file, "Stellar Characteristics of Secondary",
                             the_sun.getSecondaryMass(), the_sun.getSecondaryLuminosity(),
                             the_sun.getSecondaryEffTemp(), the_sun.getAge(),
                             the_sun.getSecondaryLife(), the_sun.getSecondarySpecType(),
                             the_sun.getMetallicity());
    the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>";
    the_file << "<font size='+1' color='" << TXHEADER << "'><b>Orbit Characteristics</b></font>";
    the_file << "</td></tr>\n";
    the_file << "<tr><td>Seperation</td>\n";
    the_file << "\t<td>" << toString(the_sun.getSeperation()) << " AU</td></tr>";
    the_file << "<tr><td>Eccentricity</td>\n";
    the_file << "\t<td>" << toString(the_sun.getEccentricity()) << " AU</td></tr>";
  }

  the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>";
  the_file << "<font size='+1' color='" << TXHEADER
           << "'><b>Habitable Zone Characteristics</b></font>";
  the_file << "</td></tr>\n";

  if (the_sun.getIsCircumbinary()) {
    the_file << "<tr><td>Minimum Stable Orbit</td>\n";
    the_file << "\t<td>" << toString(the_sun.getMinStableDistance()) << " AU</td></tr>\n";
  }

  the_file << "<tr><td>Range</td>\n";
  the_file << "\t<td>" << toString(min_r_ecosphere) << " AU to " << toString(max_r_ecosphere)
           << " AU</td></tr>\n";
  the_file << "<tr><td>Radius</td>\n";
  the_file << "\t<td>" << toString(AVE(min_r_ecosphere, max_r_ecosphere)) << " AU</td></tr>\n";
  the_file << "<tr><td>Recent Venus Distance</td>\n";
  the_file << "\t<td>" << toString(min_r_ecosphere) << " AU</td></tr>\n";
  the_file << "<tr><td>Runaway Greenhouse Distance</td>\n";
  the_file << "\t<td>" << toString(habitable_zone_distance(the_sun, RUNAWAY_GREENHOUSE, 1.0))
           << " AU</td></tr>\n";
  the_file << "<tr><td>Moist Greenhouse Distance</td>\n";
  the_file << "\t<td>" << toString(habitable_zone_distance(the_sun, MOIST_GREENHOUSE, 1.0))
           << " AU</td></tr>\n";
  the_file << "<tr><td>Earth-like Distance</td>\n";
  the_file << "\t<td>" << toString(habitable_zone_distance(the_sun, EARTH_LIKE, 1.0))
           << " AU</td></tr>\n";
  the_file << "<tr><td>First CO2 Condensation Limit</td>\n";
  the_file << "\t<td>"
           << toString(habitable_zone_distance(the_sun, FIRST_CO2_CONDENSATION_LIMIT, 1.0))
           << " AU</td></tr>\n";
  the_file << "<tr><td>Maximum Greenhouse Distance</td>\n";
  the_file << "\t<td>" << toString(habitable_zone_distance(the_sun, MAXIMUM_GREENHOUSE, 1.0))
           << " AU</td></tr>\n";
  the_file << "<tr><td>Early Mars Distance</td>\n";
  the_file << "\t<td>" << toString(max_r_ecosphere) << " AU</td></tr>\n";
  the_file << "<tr><td>Two AU Cloud Limit</td>\n";
  the_file << "\t<td>" << toString(habitable_zone_distance(the_sun, TWO_AU_CLOUD_LIMIT, 1.0))
           << " AU</td></tr>\n";
}

/**
 * @brief HTML thumbnails
 *
 * @param innermost_planet
 * @param the_file
 * @param system_name
 * @param url_path
 * @param system_url
 * @param svg_url
 * @param file_name
 * @param details
 * @param terrestrials
 * @param int_link
 * @param do_moons
 * @param graphic_format
 * @param do_gases
 */
void html_thumbnails(planet* innermost_planet, std::fstream& the_file, const std::string& system_name,
                     const std::string& url_path, const std::string& system_url, const std::string& svg_url,
                     const std::string& file_name, bool details, bool terrestrials, bool int_link,
                     bool do_moons, int graphic_format, bool do_gases, bool orbit_map) {
  if (!the_file) {
    throw std::runtime_error("StarGen: thumbnail output stream is not open");
  }

  sun the_sun = innermost_planet->getTheSun();
  bool terrestrials_seen = false;
  bool habitable_jovians_seen = false;
  bool potentialy_habitables_seen = false;

  // Count all system objects
  SystemObjectCounts counts = count_system_objects(innermost_planet, do_moons);

  // System-page path (orbit_map, GIF mode): render the whole system as ONE
  // continuous, full-width SVG — type icons positioned by log-distance, each
  // connected straight down to the AU axis and habitable-zone band, with moons
  // stacked above their planet. No separator between "strip" and "scale": they
  // are the same figure. SVG mode keeps the classic table strip (its <object>
  // already carries the orbits).
  if (orbit_map && graphic_format != gfSVG && !g_sim_context.planets.empty()) {
    SVGScaleParams sp = calculate_svg_scale(innermost_planet, g_sim_context.planets.back());

    the_file << "<p>\n\n<table border=3 cellspacing=0 cellpadding=0 align=center bgcolor='"
             << BGTABLE << "' width='100%'>\n";
    the_file << "<tr><th colspan=2 bgcolor='" << BGTABLE << "' align=center>\n\t<font size='+2' color='"
             << TXTABLE << "'>";
    if (file_name.empty()) {
      the_file << escapeXmlText(system_name);
    } else {
      the_file << "<a href='" << escapeXmlAttr(system_url) << "'>" << escapeXmlText(system_name) << "</a>";
    }
    the_file << "</font></th></tr>\n";

    the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>\n\t<font size='+1' color='"
             << TXHEADER << "'><b>" << counts.planet_count << " Planets,</b> <b>" << counts.dwarf_planet_count
             << " Dwarf Planets,</b> <b>" << counts.asteroid_belt_count << " Asteroid Belts</b>, <b>"
             << counts.moon_count << " Moons</b></font>\n\t(<font size='-1' color='" << TXHEADER
             << "'>size proportional to Sqrt(Radius); position &amp; habitable zone by distance</font>)\n</td></tr>\n";

    the_file << "<tr><td colspan=2 bgcolor='" << BGSPACE << "'>\n";
    svg_draw_system_map(the_file, sp, url_path, system_url, int_link, do_gases, do_moons, the_sun,
                        terrestrials_seen, habitable_jovians_seen, potentialy_habitables_seen);
    the_file << "</td></tr>\n";

    if (terrestrials && (terrestrials_seen || habitable_jovians_seen || potentialy_habitables_seen)) {
      html_write_terrestrials_table(the_file, innermost_planet, system_url, int_link, do_moons);
    }
    if (details) {
      html_write_system_details(the_file, the_sun);
    }

    the_file << "</table>\n<br clear='all'>\n</p>\n";
    return;
  }

  // Write table header with system name and counts
  html_write_thumbnail_header(the_file, system_name, url_path, system_url, file_name, svg_url, graphic_format, counts);

  // Write planet and moon thumbnails
  int counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    bool planet_interesting = html_write_planet_thumbnail(the_file, the_planet, the_sun, counter,
                                                          do_gases, url_path, system_url, int_link);

    if (planet_interesting) {
      if (is_terrestrial(the_planet)) {
        terrestrials_seen = true;
      }
      if (is_habitable_jovian(the_planet)) {
        habitable_jovians_seen = true;
      }
      if (is_potentialy_habitable(the_planet)) {
        potentialy_habitables_seen = true;
      }
    }

    if (do_moons) {
      MoonInterestFlags moon_flags = html_write_moon_thumbnails(the_file, the_planet, the_sun, counter,
                                                                do_gases, url_path, system_url, int_link);
      if (moon_flags.has_terrestrials) {
        terrestrials_seen = true;
      }
      if (moon_flags.has_habitable_jovians) {
        habitable_jovians_seen = true;
      }
      if (moon_flags.has_potentialy_habitables) {
        potentialy_habitables_seen = true;
      }
    }

    the_file << "</td>\n";
    counter++;
  }

  the_file << "<td bgcolor='" << BGSPACE << "' align=right valign=bottom>";
  the_file << "<a href='" << escapeXmlAttr(url_path) << "ref/Key.html'><font size='-3' color='" << TXSPACE
           << "'>See<br>Key</font></a></td>\n";
  the_file << "</tr></table>\n";

  if (graphic_format == gfSVG) {
    the_file << "</object>\n";
  }

  the_file << "</td></tr>\n";

  // Write terrestrial/habitable planets table
  if (terrestrials && (terrestrials_seen || habitable_jovians_seen || potentialy_habitables_seen)) {
    html_write_terrestrials_table(the_file, innermost_planet, system_url, int_link, do_moons);
  }

  // Write star system details
  if (details) {
    html_write_system_details(the_file, the_sun);
  }

  the_file << "</table>\n<br clear='all'>\n";
  the_file << "</p>\n";
}

/**
 * @brief HTML thumbnail totals
 *
 * @param the_file
 */
void html_thumbnail_totals(std::fstream& the_file) {
  const std::string row_template = R"(
    <tr bgcolor='{0}'>
        <td align=right>{1}</td>
        <td align=center>{2}</td>
    </tr>)";

  the_file << std::format(R"(
<p>
<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='{0}'>
    <tr>
        <th colspan=2 bgcolor='{1}' align=center>
            <font size='+1' color='{2}'><b>Summary</b></font>
        </th>
    </tr>)",
                          BGTABLE, BGHEADER, TXHEADER);

  auto add_row = [&the_file, &row_template](const std::string& label, const auto& value) {
    the_file << std::vformat(row_template, std::make_format_args(BGTABLE, label, value));
  };

  // Load atomic values for formatting
  add_row("Earthlike worlds", g_sim_context.total_earthlike.load());
  add_row("Total Habitable worlds (Earth-like Definition)", g_sim_context.total_habitable_earthlike.load());
  add_row("Total Habitable worlds (Conservative Definition)", g_sim_context.total_habitable_conservative.load());
  add_row("Total Habitable worlds (Optimistic Definition)", g_sim_context.total_habitable_optimistic.load());
  add_row("Total Habitable worlds (Extended Definition)", g_sim_context.total_habitable.load());
  add_row("Total Potentially Habitable worlds (Earth-like Definition)",
          g_sim_context.total_potentially_habitable_earthlike.load());
  add_row("Total Potentially Habitable worlds (Conservative Definition)",
          g_sim_context.total_potentially_habitable_conservative.load());
  add_row("Total Potentially Habitable worlds (Optimistic Definition)",
          g_sim_context.total_potentially_habitable_optimistic.load());
  add_row("Total Potentially Habitable worlds (Extended Definition)", g_sim_context.total_potentially_habitable.load());
  add_row("Total worlds", g_sim_context.total_worlds.load());

  add_row("Habitable mass range",
          std::format("{:.2f} EM - {:.2f} EM", min_breathable_mass * SUN_MASS_IN_EARTH_MASSES,
                      max_breathable_mass * SUN_MASS_IN_EARTH_MASSES));
  add_row("Potentially Habitable mass range",
          std::format("{:.2f} EM - {:.2f} EM", min_potential_mass * SUN_MASS_IN_EARTH_MASSES,
                      max_potential_mass * SUN_MASS_IN_EARTH_MASSES));
  add_row("Habitable g range", std::format("{:.2f} - {:.2f}", min_breathable_g, max_breathable_g));
  add_row("Potentially Habitable g range",
          std::format("{:.2f} - {:.2f}", min_potential_g, max_potential_g));
  add_row("Terrestrial g range", std::format("{:.2f} - {:.2f}", min_breathable_terrestrial_g,
                                             max_breathable_terrestrial_g));
  add_row("Habitable pressure range",
          std::format("{:.2f} - {:.2f}", min_breathable_p, max_breathable_p));
  add_row("Potentially Habitable pressure range",
          std::format("{:.2f} - {:.2f}", min_potential_p, max_potential_p));
  add_row("Habitable temp range",
          std::format("{:.2f} C - {:.2f} C", min_breathable_temp - EARTH_AVERAGE_KELVIN,
                      max_breathable_temp - EARTH_AVERAGE_KELVIN));
  add_row("Potentially Habitable temp range",
          std::format("{:.2f} C - {:.2f} C", min_potential_temp - EARTH_AVERAGE_KELVIN,
                      max_potential_temp - EARTH_AVERAGE_KELVIN));
  add_row("Habitable illumination range",
          std::format("{:.2f} - {:.2f}", min_breathable_l, max_breathable_l));
  add_row("Potentially Habitable illumination range",
          std::format("{:.2f} - {:.2f}", min_potential_l, max_potential_l));
  add_row(
      "Terrestrial illumination range",
      std::format("{:.2f} - {:.2f}", min_breathable_terrestrial_l, max_breathable_terrestrial_l));

  the_file << "\n</table>\n</p>\n";
}

/**
 * @brief HTML describe planet
 *
 * @param the_planet
 * @param counter
 * @param moons
 * @param do_gases
 * @param url_path
 * @param the_file
 */
/**
 * @brief Write orbital properties to HTML table
 */
static void html_write_orbital_properties(planet* the_planet, int moons,
                                          std::fstream& the_file) {
  if (moons == 0) {
    the_file << std::format(R"(
<tr><th>Distance from primary star</th><td>{:.2f} KM</td>
<td>{:.2f} AU</td></tr>
)",
                            the_planet->getA() * KM_PER_AU, the_planet->getA());
  } else {
    the_file << std::format(R"(
<tr><th>Distance from primary planet</th><td>{:.2f} KM</td>
<td>{:.2f} AU</td></tr>
)",
                            the_planet->getMoonA() * KM_PER_AU, the_planet->getMoonA());
  }

  the_file << std::format(R"(
<tr><th>Eccentricity of orbit</th><td>{:.6f}<td></td></tr>
<tr><th>Inclination</th><td>{:.2f}&deg;</td><td></td></tr>
<tr><th>Longitude of the Ascending Node</th><td>{:.2f}&deg;</td><td></td></tr>
<tr><th>Longitude of the Pericenter</th><td>{:.2f}&deg;</td><td></td></tr>
<tr><th>Mean Longitude</th><td>{:.2f}&deg;</td><td></td></tr>
<tr><th>Habitable Zone Distance (HZD)</th><td>{:.2f}</td><td></td></tr>
)",
                          moons == 0 ? the_planet->getE() : the_planet->getMoonE(),
                          the_planet->getInclination(), the_planet->getAscendingNode(),
                          the_planet->getLongitudeOfPericenter(), the_planet->getMeanLongitude(),
                          the_planet->getHzd());
}

/**
 * @brief Write mass and composition data to HTML table
 */
static void html_write_mass_composition(planet* the_planet, const std::string& url_path,
                                        std::fstream& the_file) {
  the_file << std::format(
      R"(
<tr><th>Mass</th><td>{:.2f} Kg</td>
<td>
)",
      (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES * EARTH_MASS_IN_GRAMS) / 1000.0);

  if ((the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006 &&
       the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006 &&
       !is_gas_planet(the_planet)) ||
      (the_planet->getType() == tSubSubGasGiant &&
       the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006)) {
    int core_size =
        static_cast<int>((50.0 * the_planet->getCoreRadius()) / the_planet->getRadius());

    if (core_size <= 49) {
      the_file << std::format(R"(
<table border=0 cellspacing=0 cellpadding=0 bgcolor='#000000' background='{0}ref/Atmosphere.gif' align=right>
<tr align=right valign=bottom background='{0}ref/Atmosphere.gif'>
<td width=50 height=50 align=right valign=bottom bgcolor='#000000' background='{0}ref/Atmosphere.gif'>
<img src='{0}ref/Core.gif' alt='' width={1} height={1}>
</td></tr></table>
)",
                              url_path, core_size);
    }
  }

  the_file << std::format("{:.2f} Earth masses", the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  if (the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006 &&
      the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006) {
    the_file << std::format(" ({:.2f} Jupiter masses)",
                            (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) / JUPITER_MASS);
  }
  the_file << "<br>";

  if (the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006 &&
      the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES > 0.0006) {
    the_file << std::format(R"(
{:.2f} Earth masses dust ({:.2f} Jupiter masses)<br>
{:.2f} Earth masses gas ({:.2f} Jupiter masses)<br>
)",
                            the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES,
                            (the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) / JUPITER_MASS,
                            the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES,
                            (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) / JUPITER_MASS);
  }

  if (is_gas_planet(the_planet)) {
    the_file << "Dust mass composition:<br>\n";
  }

  the_file << std::format(R"(
{:.2f}% ice<br>
{:.2f}% rock ({:.2f}% carbon, {:.2f}% silicates)<br>
{:.2f}% iron
)",
                          the_planet->getImf() * 100.0, the_planet->getRmf() * 100.0,
                          the_planet->getCmf() * 100.0, (1.0 - the_planet->getCmf()) * 100.0,
                          (1.0 - (the_planet->getImf() + the_planet->getRmf())) * 100.0);

  the_file << "</td></tr>\n";

  the_file << std::format(R"(
<tr><th>Habitable Zone Composition (HZC)</th><td>{:.2f}</td><td></td></tr>
)",
                          the_planet->getHzc());
}

/**
 * @brief Write physical properties (surface conditions, radius, density) to HTML table
 */
static void html_write_physical_properties(planet* the_planet, std::fstream& the_file) {
  if (!is_gas_planet(the_planet)) {
    long double celsius = the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER;

    the_file << std::format(R"(
<tr><th>Surface gravity</th>
<td>{:.2f} cm/sec<sup>2</sup></td>
<td>{:.2f} Earth gees</td></tr>

<tr><th>Surface pressure</th>
<td>{:.2f} millibars</td>
<td>{:.2f} Earth atmospheres</td></tr>

<tr><th>Surface temperature</th>
<td class='{}'>{:.2f}&deg; Celcius<br>{:.2f}&deg; Fahrenheit</td>
<td rowspan=2 valign=top>{:.2f}&deg; C Earth temperature<br>{:.2f}&deg; F Earth temperature
)",
                            the_planet->getSurfAccel(), the_planet->getSurfGrav(),
                            the_planet->getSurfPressure(),
                            the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS,
                            temp_tier_class(the_planet->getSurfTemp()), celsius,
                            32.0 + (celsius * 1.8), celsius - EARTH_AVERAGE_CELSIUS,
                            (celsius - EARTH_AVERAGE_CELSIUS) * 1.8);

    if (the_planet->getGreenhsRise() > 0.1) {
      the_file << std::format("<br>{:.2f}&deg; C greenhouse effect", the_planet->getGreenhsRise());
      if (the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0) {
        the_file << " (GH)";
      }
    }

    the_file << "</td></tr>\n";

    the_file << R"(<tr><th>Normal temperature range</th><td><center><table>)";
    if (std::abs(the_planet->getHighTemp() - the_planet->getMaxTemp()) > 10.0 ||
        std::abs(the_planet->getLowTemp() - the_planet->getMinTemp()) > 10.0) {
      the_file << std::format(R"(
<tr><th>Night</th><th></th><th>Day</th></tr>
<tr><td>{:.2f}&deg; C<br>{:.2f}&deg; F</td>
<td></td>
<td>{:.2f}&deg; C<br>{:.2f}&deg; F</td>
)",
                              the_planet->getLowTemp() - FREEZING_POINT_OF_WATER,
                              32.0 + (1.8 * (the_planet->getLowTemp() - FREEZING_POINT_OF_WATER)),
                              the_planet->getHighTemp() - FREEZING_POINT_OF_WATER,
                              32.0 + (1.8 * (the_planet->getHighTemp() - FREEZING_POINT_OF_WATER)));
    }
    the_file << std::format(R"(
<tr><th>Min</th><th></th><th>Max</th></tr>
<tr><td>{:.2f}&deg; C<br>{:.2f}&deg; F</td>
<td> - </td>
<td>{:.2f}&deg; C<br>{:.2f}&deg; F</td>
</table></center></td></tr>
)",
                            the_planet->getMinTemp() - FREEZING_POINT_OF_WATER,
                            32.0 + (1.8 * (the_planet->getMinTemp() - FREEZING_POINT_OF_WATER)),
                            the_planet->getMaxTemp() - FREEZING_POINT_OF_WATER,
                            32.0 + (1.8 * (the_planet->getMaxTemp() - FREEZING_POINT_OF_WATER)));

    the_file << std::format(R"(
<tr><th>Standard Primary Habitability (SPH)</th><td>{:.2f}</td><td></td></tr>
)",
                            the_planet->getSph());
  } else {
    the_file << std::format(R"(
<tr><th>Estimated Temperature<br><small>radiative equilibrium; excludes greenhouse &amp; internal heat</small></th>
<td class='{}'>{:.2f}&deg; K</td>
<td>{:.2f}&deg; C Earth temperature</td></tr>
)",
                            temp_tier_class(the_planet->getEstimatedTemp()),
                            the_planet->getEstimatedTemp(),
                            the_planet->getEstimatedTemp() - EARTH_AVERAGE_KELVIN);
  }

  the_file << std::format(R"(
<tr><th>Average radius</th>
<td>{:.2f} Km</td>
<td>{:.2f} Earth radii
)",
                          the_planet->getRadius(), convert_km_to_eu(the_planet->getRadius()));

  if (is_gas_planet(the_planet)) {
    the_file << std::format("<br>{:.2f} Jupiter radii",
                            the_planet->getRadius() / KM_JUPITER_RADIUS);
  }
  the_file << "</td></tr>\n";

  if (the_planet->getGasMass()) {
    the_file << std::format(R"(
<tr><th>Radius of Core</th>
<td>{:.2f} km</td>
<td>{:.2f} Earth radii</td></tr>
)",
                            the_planet->getCoreRadius(),
                            convert_km_to_eu(the_planet->getCoreRadius()));
  }

  the_file << std::format(R"(
<tr><th>Oblateness</th>
<td>{:.6f}</td>
<td><table>
<tr><th>Equatorial Radius</th><th>Polar Radius</th></tr>
<tr><td>{:.2f} Km<br>{:.2f} Earth radii<br>{:.2f} Jupiter radii</td>
<td>{:.2f} Km<br>{:.2f} Earth radii<br>{:.2f} Jupiter radii</td>
</tr></table></td></tr>

<tr><th>Density</th>
<td>{:.2f} grams/cc</td>
<td>{:.2f} Earth densities</td></tr>

<tr><th>Escape Velocity</th>
<td>{:.2f} Km/sec</td>
<td></td></tr>

<tr><th>Molecular weight retained</th>
<td>{:.2f} and above</td>
<td>
)",
                          the_planet->getOblateness(), the_planet->getEquatrorialRadius(),
                          convert_km_to_eu(the_planet->getEquatrorialRadius()),
                          the_planet->getEquatrorialRadius() / KM_JUPITER_RADIUS,
                          the_planet->getPolarRadius(),
                          convert_km_to_eu(the_planet->getPolarRadius()),
                          the_planet->getPolarRadius() / KM_JUPITER_RADIUS,
                          the_planet->getDensity(), the_planet->getDensity() / EARTH_DENSITY,
                          sgu::cm_to_km(the_planet->getEscVelocity()), the_planet->getMolecWeight());

  list_molecules(the_file, the_planet->getMolecWeight());
}

/**
 * @brief Write atmospheric gas composition to HTML table
 */
static void html_write_atmospheric_gases(planet* the_planet, bool do_gases,
                                         std::fstream& the_file) {
  if (do_gases && the_planet->getNumGases() > 0) {
    the_file << R"(
<table border=0 cellspacing=0 cellpadding=0>
)";
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      int         index     = gases.count();
      bool        poisonous = false;
      long double ipp       = inspired_partial_pressure(the_planet->getSurfPressure(),
                                                        the_planet->getGas(i).getSurfPressure());

      for (int n = 0; n < gases.count(); n++) {
        if (gases[n].getNum() == the_planet->getGas(i).getNum()) {
          index = n;
          break;
        }
      }

      ipp = std::max(0.0L, ipp);
      if (ipp > gases[index].getMaxIpp()) {
        poisonous = true;
      }

      long double percentage =
          100.0 * (the_planet->getGas(i).getSurfPressure() / the_planet->getSurfPressure());
      if (percentage > 0.001 || poisonous) {
        the_file << std::format(R"(
<tr><th align=left>{}&nbsp;</th>
<td align=right>{:.3f}%&nbsp;</td>
<td align=right>{:.2f} mb&nbsp;</td>
<td align=right>(ipp:{:.2f})</td>
<td align=right>&nbsp;{}</td></tr>
)",
                                escapeXmlText(gases[index].getName()), percentage,
                                the_planet->getGas(i).getSurfPressure(), ipp,
                                poisonous ? "<span class='poison'>&#9888; poisonous</span>" : "");
      }
    }
    the_file << "</table>\n";
  }
  the_file << "</td></tr>\n";
}

/**
 * @brief Write additional properties (orbital period, hydrosphere, ESI) to HTML table
 */
static void html_write_additional_properties(planet* the_planet, std::fstream& the_file) {
  the_file << std::format(
      R"(
<tr><th>Habitable Zone Atmosphere (HZA)</th>
<td>{:.2f}</td><td></td></tr>

<tr><th>Axial tilt</th>
<td>{:.2f}&deg;</td>
<td></td></tr>

<tr><th>Planetary albedo</th>
<td>{:.2f}</td>
<td></td></tr>

<tr><th>Exospheric Temperature</th>
<td>{:.2f}&deg; K</td>
<td>{:.2f}&deg; C Earth temperature</td></tr>

<tr><th>Length of year</th>
<td>{:.2f} Earth days</td>
<td>{:.2f} Local days
)",
      the_planet->getHza(), the_planet->getAxialTilt(), the_planet->getAlbedo(),
      the_planet->getExosphericTemp(), the_planet->getExosphericTemp() - EARTH_EXOSPHERE_TEMP,
      the_planet->getOrbPeriod(), (the_planet->getOrbPeriod() * 24.0) / the_planet->getDay());

  if (the_planet->getOrbPeriod() > DAYS_IN_A_YEAR) {
    the_file << std::format("<br>{:.2f} Earth years", the_planet->getOrbPeriod() / DAYS_IN_A_YEAR);
  }
  the_file << "</td></tr>\n";

  the_file << std::format(R"(
<tr><th>Length of day</th>
<td>{:.2f} hours</td>
<td></td></tr>
)",
                          the_planet->getDay());

  if (!is_gas_planet(the_planet)) {
    the_file << std::format(R"(
<tr><th>Boiling point of water</th>
<td>{:.2f}&deg; Celcius<br>{:.2f}&deg; Fahrenheit</td>
<td></td></tr>

<tr><th>Hydrosphere</th>
<td>{:.2f}%</td>
<td></td></tr>

<tr><th>Cloud cover</th>
<td>{:.2f}%</td>
<td></td></tr>

<tr><th>Ice cover</th>
<td>{:.2f}%</td>
<td></td></tr>
)",
                            the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER,
                            32.0 + ((the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER) * 1.8),
                            the_planet->getHydrosphere() * 100.0,
                            the_planet->getCloudCover() * 100.0, the_planet->getIceCover() * 100.0);
  }

  // Sub-indices come from the single calcEsiComponents() source (the headline
  // ESI cell below uses the_planet->getEsi(), itself the product of these).
  EsiComponents esi_c = calcEsiComponents(the_planet);
  long double esir = esi_c.r;
  long double esid = esi_c.d;
  long double esiv = esi_c.v;
  long double esit = esi_c.t;

  the_file << std::format(R"(
<tr><th>Earth Similarity Index (ESI)</th>
<td>{:.4f}</td>
<td><!--esir: {:.4f}<br />esid: {:.4f}<br />esiv: {:.4f}<br />esit: {:.4f}--></td></tr>
)",
                          the_planet->getEsi(), esir, esid, esiv, esit);
}

void html_describe_planet(planet* the_planet, int counter, int moons, bool do_gases,
                          const std::string& url_path, std::fstream& the_file) {
  std::string planet_id =
      moons > 0 ? std::format("{}.{}", counter, moons) : std::to_string(counter);
  std::string typeString = type_string(the_planet);

  do_gases = (flags_arg_clone & fDoGases) != 0;

  // Highlight a (potentially) habitable world: green halo on the card + a badge
  // in the title row, so the standout planet pops out of the system at a glance.
  const bool  is_hab   = is_habitable(the_planet);
  const bool  is_phab  = !is_hab && is_potentialy_habitable(the_planet);
  const char* tbl_class = (is_hab || is_phab) ? "habitable" : "";
  std::string badge;
  if (is_hab) {
    badge = "<span class='badge'>&#127757; HABITABLE</span>";
  } else if (is_phab) {
    badge = "<span class='badge badge-maybe'>&#127759; POTENTIALLY HABITABLE</span>";
  }

  // Write table header
  the_file << std::format(R"(
<p>
<a name='{0}'></a>
<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='{1}' width='{2}%' class='{6}'>
<colgroup span=1 align=left valign=middle>
<colgroup span=2 align=left valign=middle>
<tr><th colspan=3 bgcolor='{3}' align=center>
<font size='+2' color='{4}'>{5} #{0} Statistics</font>{7}</th></tr>
)",
                          planet_id, BGTABLE, (moons == 0) ? 95 : 90, BGHEADER, TXHEADER,
                          (moons == 0) ? "Planet" : "Moon", tbl_class, badge);

  // Write planet type and image
  the_file << std::format(R"(
<tr><th>Planet type</th>
<td colspan=2><img alt='{0}' src='{1}ref/{2}Planet.webp' align=middle width=150 height=150>{0}
)",
                          typeString, escapeXmlAttr(url_path), image_type_string(the_planet));

  // Quick-glance status chips shown under the planet type/description: spin
  // state and air breathability. Detailed caveats (XUV, desiccation, CO2) stay
  // in the "Habitability notes" row below to avoid duplication.
  std::string chips;
  if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
    chips += "<span class='chip chip-warn'><span class='glyph'>&#128274;</span> Tidally locked</span>";
  } else if (the_planet->getResonantPeriod()) {
    chips += std::format(
        "<span class='chip chip-warn'><span class='glyph'>&#128274;</span> Spin&ndash;orbit resonance ({})</span>",
        moons == 0 ? printSpinResonanceFactor(the_planet->getE())
                   : printSpinResonanceFactor(the_planet->getMoonE()));
  }
  if (!is_gas_planet(the_planet) && the_planet->getNumGases() > 0) {
    switch (breathability(the_planet)) {
      case BREATHABLE:
        chips += "<span class='chip chip-good'><span class='glyph'>&#10003;</span> Breathable air</span>";
        break;
      case UNBREATHABLE:
        chips += "<span class='chip chip-warn'>Unbreathable air</span>";
        break;
      case POISONOUS:
        chips += "<span class='chip chip-bad'><span class='glyph'>&#9888;</span> Toxic air</span>";
        break;
      default:
        break;
    }
  }

  print_description(the_file, "<br>", the_planet, "");

  if (!chips.empty()) {
    the_file << "\n<div class='chips'>" << chips << "</div>";
  }

  the_file << "</td></tr>\n";

  // Write all planet properties using helper functions
  html_write_orbital_properties(the_planet, moons, the_file);
  html_write_mass_composition(the_planet, url_path, the_file);
  html_write_physical_properties(the_planet, the_file);
  html_write_atmospheric_gases(the_planet, do_gases, the_file);
  html_write_additional_properties(the_planet, the_file);

  // Modern habitability caveats + the Lehmer et al. 2020 1-D-climate diagnostic
  // (tidal-lock status is already shown with the planet type above).
  {
    std::string notes;
    if (the_planet->getPmsDesiccationRisk()) {
      notes += "<li><span class='glyph' style='color:#ffce7a'>&#9888;</span> Likely desiccated "
               "during the host M-dwarf's pre-main-sequence "
               "phase (possible abiotic-O2 \"mirage Earth\").</li>";
    }
    if (the_planet->getHighXuvEscapeRisk()) {
      notes += "<li><span class='glyph' style='color:#ffce7a'>&#9728;</span> High atmospheric-escape "
               "risk from M-dwarf XUV/flare irradiation.</li>";
    }
    if (the_planet->getCo2CollapseRisk()) {
      notes += "<li><span class='glyph' style='color:#9fd4ff'>&#10052;</span> Cold for the habitable "
               "zone: outgassed CO2 may freeze out, "
               "risking irreversible glaciation.</li>";
    }
    if (the_planet->getClimateModelTemp() > 0.0) {
      notes += std::format(
          "<li><span class='glyph' style='color:#8aeaa6'>&#127777;</span> 1-D-climate surface "
          "temperature: {:.1f} &deg;C (Lehmer et al. 2020).</li>",
          (double)(the_planet->getClimateModelTemp() - FREEZING_POINT_OF_WATER));
    }
    if (!notes.empty()) {
      the_file << std::format(
          "<tr><th>Habitability notes</th><td colspan=2>"
          "<ul style='margin:4px 0;padding-left:18px'>{}</ul></td></tr>\n",
          notes);
    }
  }

  // Close table
  the_file << "</table>\n\n</p>\n<br>\n\n";
}
/**
 * @brief HTML describe system
 *
 * @param innermost_planet
 * @param do_gases
 * @param do_moons
 * @param url_path
 * @param the_file
 */
void html_describe_system(planet* innermost_planet, bool do_gases, bool do_moons,
                          const std::string& url_path, std::fstream& the_file) {
  performanceMonitor.recordFileOperation("html_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  int counter;
  int moons;

  // The system overview (planet icon strip + orbit map / habitable-zone scale) is
  // emitted by html_thumbnails above; here we emit only the per-planet detail
  // tables. (The old "Planetary Overview" summary table was removed as redundant
  // with that figure.)

  // Tables for individual planets
  counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    html_describe_planet(the_planet, counter, 0, do_gases, url_path, the_file);
    if (do_moons && !the_planet->moons.empty()) {
      moons = 1;
      for (planet* moon : the_planet->moons) {
        html_describe_planet(moon, counter, moons, do_gases, url_path, the_file);
        moons++;
      }
    }
    counter++;
  }
}

/**
 * @brief Celestia describe system
 *
 * @param innermost_planet
 * @param designation
 * @param system_name
 * @param seed
 * @param inc
 * @param an
 * @param do_moons
 */
void celestia_describe_system(planet* innermost_planet, std::string designation, std::string system_name,
                              long int seed, long double inc, long double an, bool do_moons) {
  planet *    the_planet, *moon;
  sun         the_sun = innermost_planet->getTheSun();
  int         counter, moons;
  long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
  long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

  std::cout << "# Stargen - " << stargen_revision << "; seed=" << seed << "\n"
            << "#\n"
            << "# " << escapeCelestiaId(designation) << ", " << escapeCelestiaId(system_name) << "\n"
            << "#\n"
            << "# Stellar mass: " << toString(the_sun.getMass()) << " solar masses\n"
            << "# Stellar luminosity: " << toString(the_sun.getLuminosity()) << "\n"
            << "# Stellar temperature: " << toString(the_sun.getEffTemp()) << " Kelvin\n"
            << "# Stellar metallicity: [Fe/H] = " << toString(the_sun.getMetallicity()) << " dex\n"
            << "# Age: " << toString(the_sun.getAge() / 1.0E9) << "  billion years\t("
            << toString((the_sun.getLife() - the_sun.getAge()) / 1.0E9)
            << " billion left on main sequence)\n"
            << "# Habitable ecosphere: " << toString(min_r_ecosphere) << " AU to "
            << toString(max_r_ecosphere) << " AU\n\n";
  counter = 1;
  for (planet* the_planet : g_sim_context.planets) {
    celestia_describe_world(the_planet, designation, system_name, seed, inc, an, counter, the_sun,
                            false, counter);
    if (do_moons && !the_planet->moons.empty()) {
      moons = 1;
      for (planet* moon : the_planet->moons) {
        celestia_describe_world(moon, designation, system_name, seed, inc, an, moons, the_sun, true,
                                counter);
        moons++;
      }
    }
    counter++;
  }
}

/**
 * @brief Celestia describe world
 *
 * @param the_planet
 * @param designation
 * @param system_name
 * @param seed
 * @param inc
 * @param an
 * @param counter
 * @param the_sun
 * @param is_moon
 * @param planet_num
 */
void celestia_describe_world(planet* the_planet, const std::string& designation,
                             const std::string& system_name, long seed, long double inc,
                             long double an, int counter, sun& the_sun, bool is_moon,
                             int planet_num) {
  const std::string designation_esc = escapeCelestiaId(designation);
  const auto name =
      is_moon ? std::format("\"p{}-{}\"", counter, planet_num) : std::format("\"p{}\"", counter);
  const auto parent = is_moon ? std::format("\"{}/p{}\"", designation_esc, planet_num)
                              : std::format("\"{}\"", designation_esc);

  std::cout << std::format("\t{} {}\n{{\n", name, parent);

  if (!is_moon) {
    if (the_planet->getType() == tAsteroids ||
        the_planet->getRadius() < round_threshold(the_planet->getDensity())) {
      std::cout << "\tClass \"asteroid\"\n";
    } else if (calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0) {
      std::cout << "\tClass \"dwarfplanet\"\n";
    } else {
      std::cout << "\tClass \"planet\"\n";
    }
  } else {
    std::cout << "\tClass \"moon\"\n";
  }

  std::cout << std::format("\tRadius {}\n", the_planet->getEquatrorialRadius());
  std::cout << std::format("\tMass {}\n\n", the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  if (is_gas_planet(the_planet) && the_planet->getType() != tBrownDwarf) {
    handle_gas_planet_texture(the_planet);
  } else {
    handle_rocky_planet_texture(the_planet);
  }

  std::cout << '\n';

  print_planet_details(the_planet);

  if (the_planet->getNumGases() > 0) {
    print_atmosphere_composition(the_planet);
  }

  std::cout << "}\n";
}

// Helper functions

void handle_gas_planet_texture(planet* the_planet) {
  constexpr double ts = TEMPERATURE_SATURN + 1.0;
  constexpr double tu = TEMPERATURE_URANUS + 1.0;
  constexpr double tn = TEMPERATURE_NEPTUNE + 1.0;

  const double temp = the_planet->getEstimatedTemp();

  if (temp < tn) {
    std::cout << "\tTexture \"tgasgiant.*\"\n\tColor [ 0.37 0.5 0.87 ]\n\tBlendTexture true\n";
  } else if (temp < tu) {
    std::cout << "\tTexture \"tgasgiant.*\"\n";
    assignTemperatureColors(the_planet, tu, 0.58, 0.69, 0.74, tn, 0.37, 0.5, 0.87);
  } else if (temp < ts) {
    std::cout << "\tTexture \"tgasgiant.*\"\n";
    assignTemperatureColors(the_planet, ts, 0.91, 0.87, 0.76, tu, 0.58, 0.69, 0.74);
  } else if (temp < TEMPERATURE_CLASS_II) {
    std::cout << "\tTexture \"exo-class1.*\"\n";
    assignTemperatureColors(the_planet, TEMPERATURE_CLASS_II, 1, 1, 1, ts, 0.91, 0.87, 0.76);
  } else if (temp < TEMPERATURE_SULFUR_GIANT) {
    std::cout << "\tTexture \"exo-class2.*\"\n";
  } else if (temp < TEMPERATURE_CLASS_III) {
    std::cout << "\tTexture \"venus.jpg\"\n";
  } else if (temp < TEMPERATURE_CLASS_IV) {
    std::cout << "\tTexture \"exo-class3.*\"\n";
  } else if (temp < TEMPERATURE_CLASS_V) {
    std::cout << "\tTexture \"exo-class4.*\"\n\tNightTexture \"exo-class4night.*\"\n";
  } else if (temp < TEMPERATURE_CARBON_GIANT) {
    std::cout << "\tTexture \"exo-class5.*\"\n\tNightTexture \"exo-class5night.*\"\n";
  } else {
    std::cout << "\tTexture \"exo-class4night.*\"\n\tNightTexture \"exo-class4night.*\"\n\tColor [ "
                 "0 0 0 ]\n\tBlendTexture true\n";
  }
}

void handle_rocky_planet_texture(planet* the_planet) {
  switch (the_planet->getType()) {
    case tIce:
      handle_ice_planet_texture(the_planet);
      break;
    case tWater:
      handle_water_planet_texture(the_planet);
      break;
    case tCarbon:
    case tIron:
    case tOil:
    case tRock:
      handle_rocky_texture(the_planet);
      break;
    case t1Face:
      handle_1face_planet_texture(the_planet);
      break;
    case tVenusian:
      handle_venusian_texture(the_planet);
      break;
    case tMartian:
      handle_martian_texture(the_planet);
      break;
    case tTerrestrial:
      handle_terrestrial_texture(the_planet);
      break;
    case tAsteroids:
      handle_asteroid_texture(the_planet);
      break;
    default:
      std::cout << std::format("\tTexture \"{}\"\n", texture_name(the_planet->getType()));
  }
}

void print_planet_details(planet* the_planet) {
  const auto print_detail = [](std::string_view label, auto value, std::string_view unit = "") {
    std::cout << std::format("#   {:<30} {:<10} {}\n", label, value, unit);
  };

  print_detail("HZD:", the_planet->getHzd());
  print_detail("HZC:", the_planet->getHzc());
  print_detail("HZA:", the_planet->getHza());
  print_detail("SPH:", the_planet->getSph());
  print_detail("ESI:", the_planet->getEsi());
  print_detail("Density:", the_planet->getDensity(), "grams/cc");
  print_detail("Escape Velocity:", sgu::cm_to_km(the_planet->getEscVelocity()), "Km/sec");
  print_detail("Surface acceleration:", the_planet->getSurfAccel(), "cm/sec2");
  print_detail("Surface gravity:", the_planet->getSurfGrav(), "Earth gees");
  print_detail("Surface temperature:", the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER,
               "degrees Celsius");
  print_detail("Estimated temperature:", the_planet->getEstimatedTemp() - FREEZING_POINT_OF_WATER,
               "degrees Celsius");
  print_detail("Estimated terrestrial temperature:",
               the_planet->getEstimatedTerrTemp() - FREEZING_POINT_OF_WATER, "degrees Celsius");
  print_detail("Boiling point of water:", the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER,
               "degrees Celsius");

  std::cout << std::format("#   {:<30} {:<10} Earth atmospheres", "Surface pressure:",
                           the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS);
  if (the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0) {
    std::cout << "\tGREENHOUSE EFFECT";
  }
  std::cout << '\n';

  print_detail("Molecular weight retained:", the_planet->getMolecWeight(), "and above");
}

void print_atmosphere_composition(planet* the_planet) {
  for (int i = 0; i < the_planet->getNumGases(); i++) {
    auto gas      = the_planet->getGas(i);
    auto gas_info = gases[find_gas_index(gas.getNum())];

    double percentage = (gas.getSurfPressure() / the_planet->getSurfPressure()) * 100.0;
    double ipp = inspired_partial_pressure(the_planet->getSurfPressure(), gas.getSurfPressure());
    ipp        = std::max(0.0, ipp);
    bool poisonous = ipp > gas_info.getMaxIpp();

    if (percentage > 0.05 || poisonous) {
      std::cout << std::format("#           {}: {:.2f}% {:.2f} mb (ipp: {:.2f})",
                               gas_info.getName(), percentage, gas.getSurfPressure(), ipp);
      if (poisonous) {
        std::cout << " poisonous";
      }
      std::cout << '\n';
    }
  }
}

void handle_ice_planet_texture(planet* the_planet) {
  std::cout << std::format("# Hydrosphere percentage: {:.2f}\n",
                           the_planet->getHydrosphere() * 100.0);
  std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                           the_planet->getIceCover() * 100.0);

  int ran_itex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"tice{}.*\"\n", ran_itex);
  std::cout << "\tSpecularColor [ 0.1 0.1 0.13 ]\n";
  std::cout << "\tSpecularPower 25.0\n";
  std::cout << std::format("\tBumpMap \"tice{}-bump.*\"\n", ran_itex);
  std::cout << "\tBumpHeight 1.5\n";
  std::cout << "\tColor [ 1.0 0.9 0.75 ]\n";
}

void handle_water_planet_texture(planet* the_planet) {
  std::cout << std::format("# Hydrosphere percentage: {:.2f}\n",
                           the_planet->getHydrosphere() * 100.0);
  std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                           the_planet->getIceCover() * 100.0);

  int ran_wtex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"twater{}.*\"\n", ran_wtex);
  std::cout << "\tSpecularColor [ 0.5 0.5 0.55 ]\n";
  std::cout << "\tSpecularPower 25.0\n";
  std::cout << "\tColor [ 0.75 0.75 1.0 ]\n";
}

void handle_rocky_texture(planet* the_planet) {
  if (the_planet->getType() == tOil) {
    std::cout << std::format("# Hydrosphere percentage: {:.2f}\n",
                             the_planet->getHydrosphere() * 100.0);
    std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                             the_planet->getIceCover() * 100.0);
  }

  int ran_rtex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"trock{}.*\"\n", ran_rtex);
  std::cout << std::format("\tBumpMap \"trock{}-bump.*\"\n", ran_rtex);
  std::cout << "\tBumpHeight 1.5\n";

  if (the_planet->getType() != tOil) {
    assignDistanceColors(the_planet, 0.52, 0.47, 0.42);
  }
}

void handle_1face_planet_texture(planet* the_planet) {
  std::cout << std::format("# Hydrosphere percentage: {:.2f}\n",
                           the_planet->getHydrosphere() * 100.0);
  std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                           the_planet->getIceCover() * 100.0);

  int ran_1tex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"t1face{}.*\"\n", ran_1tex);
  std::cout << std::format("\tSpecularTexture \"t1face{}-spec.*\"\n", ran_1tex);
  std::cout << "\tSpecularColor [ 0.1 0.1 0.13 ]\n";
  std::cout << "\tSpecularPower 25.0\n";
  std::cout << std::format("\tBumpMap \"t1face{}-bump.*\"\n", ran_1tex);
  std::cout << "\tBumpHeight 1.5\n";
  std::cout << "\tColor [ 0.52 0.47 0.42 ]\n";
}

void handle_venusian_texture(planet* the_planet) {
  int ran_vtex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"tvenus{}.*\"\n", ran_vtex);
  std::cout << std::format("\tBumpMap \"tvenus{}-bump.*\"\n", ran_vtex);
  std::cout << "\tBumpHeight 1.5\n";
}

void handle_martian_texture(planet* the_planet) {
  std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                           the_planet->getIceCover() * 100.0);

  int ran_mtex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"tmars{}.*\"\n", ran_mtex);
  std::cout << std::format("\tBumpMap \"tmars{}-bump.*\"\n", ran_mtex);
  std::cout << "\tBumpHeight 1.5\n";

  assignDistanceColors(the_planet, 1, 0.75, 0.7);
}

void handle_terrestrial_texture(planet* the_planet) {
  std::cout << std::format("# Hydrosphere percentage: {:.2f}\n",
                           the_planet->getHydrosphere() * 100.0);
  std::cout << std::format("# Ice cover percentage:   {:.2f}\n\n",
                           the_planet->getIceCover() * 100.0);

  int ran_etex = random_numberInt(1, 5);
  std::cout << std::format("\tTexture \"tearth{}.*\"\n", ran_etex);
  std::cout << std::format("\tSpecularTexture \"tearth{}-spec.*\"\n", ran_etex);
  std::cout << "\tSpecularColor [ 0.8 0.8 0.85 ]\n";
  std::cout << "\tSpecularPower 25.0\n";
  std::cout << std::format("\tBumpMap \"tearth{}-bump.*\"\n", ran_etex);
  std::cout << "\tBumpHeight 1.5\n";
  std::cout << "\tColor [ 0.9 0.9 0.95 ]\n";
}

void handle_asteroid_texture(planet* the_planet) {
  int ran_atex = random_numberInt(1, 3);
  std::cout << "\tTexture \"tasteroid.*\"\n";

  if (the_planet->getRadius() < (round_threshold(the_planet->getDensity()) * (2.0 / 3.0))) {
    std::cout << "\tMesh \"asteroid.cms\"\n";
  } else {
    std::cout << "\tBumpMap \"tasteroid-bump.*\"\n";
    std::cout << "\tBumpHeight 1.5\n";
  }

  switch (ran_atex) {
    case 1:
      std::cout << "\tColor [ 0.52 0.46 0.43 ]\n";
      break;
    case 2:
      std::cout << "\tColor [ 0.37 0.37 0.37 ]\n";
      break;
    default:
      std::cout << "\tColor [ 0.7 0.7 0.7 ]\n";
  }

  std::cout << "\tBlendTexture true\n";
}

int find_gas_index(int gas_num) {
  for (int i = 0; i < gases.count(); ++i) {
    if (gases[i].getNum() == gas_num) {
      return i;
    }
  }
  return -1;  // Return -1 if the gas is not found
}

void assignTemperatureColors(planet* the_planet, double temp1, double red1, double green1,
                             double blue1, double temp2, double red2, double green2, double blue2) {
  double temp  = the_planet->getEstimatedTemp();
  double ratio = (temp - temp2) / (temp1 - temp2);
  ratio        = std::clamp(ratio, 0.0, 1.0);

  double red   = red2 + (ratio * (red1 - red2));
  double green = green2 + (ratio * (green1 - green2));
  double blue  = blue2 + (ratio * (blue1 - blue2));

  std::cout << std::format("\tColor [ {:.2f} {:.2f} {:.2f} ]\n", red, green, blue);
  std::cout << "\tBlendTexture true\n";
}

void assignDistanceColors(planet* the_planet, double red, double green, double blue) {
  double min_r_ecosphere = habitable_zone_distance(the_planet->getTheSun(), RECENT_VENUS, 1.0);
  double max_r_ecosphere = habitable_zone_distance(the_planet->getTheSun(), EARLY_MARS, 1.0);
  double r_ecosphere = (the_planet->getA() - min_r_ecosphere) / (max_r_ecosphere - min_r_ecosphere);
  r_ecosphere        = std::clamp(r_ecosphere, 0.0, 1.0);

  red   = red + (0.5 * r_ecosphere);
  green = green + (0.5 * r_ecosphere);
  blue  = blue + (0.5 * r_ecosphere);

  std::cout << std::format("\tColor [ {:.2f} {:.2f} {:.2f} ]\n", red, green, blue);
}

/**
 * @brief lprint
 *
 * @param the_file
 * @param first
 * @param text
 */
void lprint(std::fstream& the_file, bool& first, const std::string& text) {
  if (first) {
    first = false;
  } else {
    the_file << ", ";
  }
  the_file << text;
}

/**
 * @brief Image type string
 *
 * @param the_planet
 * @return string
 */
std::string image_type_string(planet* the_planet) {
  std::string       typeString;
  std::stringstream ss;
  if (is_gas_planet(the_planet) && the_planet->getType() != tBrownDwarf) {
    ss << cloud_type_string(the_planet) << " Gas";
    typeString = ss.str();
  } else {
    typeString = type_string(the_planet);
  }
  return remove_spaces(typeString);
}

fraction stern_brocot_search(long double f, long double tol) {
  int base = floor(f);
  f -= base;
  if (f < tol) return fraction(base, 1);
  if (1 - tol < f) return fraction(base + 1, 1);

  fraction lower(0, 1);
  fraction upper(1, 1);

  while (true) {
    fraction middle(lower.n + upper.n, lower.d + upper.d);

    if (middle.d * (f + tol) < middle.n) {
      upper = middle;
    } else if (middle.n < middle.d * (f - tol)) {
      lower = middle;
    } else {
      return fraction(middle.d * base + middle.n, middle.d);
    }
  }
}

/**
 * @brief Print Spin Resonance Factor
 *
 * @param eccentricity
 * @return string
 */
std::string printSpinResonanceFactor(long double eccentricity) {
  long double top       = 1.0 - eccentricity;
  long double bottom    = 1.0 + eccentricity;
  long double fraction  = top / bottom;
  long double tolerance = .01;

  // this is a test to replace the hard coded check with something formulaic
  // with a tolerance of .01 the ratios shouldn't get too crazy
  // decrease tollerance to increase accuracy but you may get ratios like 1549/1322
  // for the number 0.85345349958 at a tolerance of .000001 instead of something like
  // 7/6 for tolerances between .007 and .015
  // see https://gist.github.com/mikeando/7073d62385a34a61a6f7
  struct fraction sb = stern_brocot_search(fraction, tolerance);
  // this is setup to have numerator and denomenator flipped.
  return (std::to_string(sb.d)) + ":" + (std::to_string(sb.n));
}

/**
 * @brief Mol Print
 *
 * @param the_file
 * @param first
 * @param count
 * @param max
 * @param min_weight
 * @param molecule
 * @param weight
 */
void mol_print(std::fstream& the_file, bool& first, int& count, int max, long double min_weight,
               std::string molecule, long double weight) {
  if (weight >= min_weight) {
    count++;
    if (count > max) {
      the_file << "...";
    } else {
      lprint(the_file, first, molecule);
    }
  }
}

/**
 * @brief Texture name
 *
 * @param type
 * @return string
 */
std::string texture_name(planet_type type) {
  std::string typeString = "Unknown";
  switch (type) {
    case tUnknown:
      typeString = "tunknown.*";
      break;
    case tIron:
    case tCarbon:
    case tOil:
    case tRock:
      typeString = "trock.*";
      break;
    case tVenusian:
      typeString = "tvenus.*";
      break;
    case tTerrestrial:
      typeString = "tearth.*";
      break;
    case tSubSubGasGiant:
      typeString = "tsubsubgasgiant.*";
      break;
    case tSubGasGiant:
      typeString = "tsubgasgiant.*";
      break;
    case tGasGiant:
      typeString = "tgasgiant.*";
      break;
    case tMartian:
      typeString = "tmars.*";
      break;
    case tWater:
      typeString = "twater.*";
      break;
    case tIce:
      typeString = "tice.*";
      break;
    case tAsteroids:
      typeString = "tasteroid.*";
      break;
    case t1Face:
      typeString = "t1face.*";
      break;
    case tBrownDwarf:
      typeString = "browndwarf.*";
      break;
  }
  return typeString;
}

/**
 * @brief Display clouds
 *
 * @param the_planet
 */
void display_clouds(planet* the_planet) {
  long double ranCtex = random_number(1.5, 5.5);
  std::cout << "# Cloud cover percentage: " << (the_planet->getCloudCover() * 100.0) << "%\n";
  if (the_planet->getCloudCover() > 0.25) {
    std::cout << "\n";
    std::cout << "\t\tCloudHeight 7\n";
    if (the_planet->getDay() > (14.0 * 24.0)) {
      std::cout << "\t\tCloudSpeed 300\n";
    } else if (the_planet->getDay() > (4.0 * 24.0)) {
      std::cout << "\t\tCloudSpeed 130\n";
    } else {
      std::cout << "\t\tCloudSpeed 65\n";
    }
  }

  if (the_planet->getCloudCover() > 0.75) {
    std::cout << "\t\tCloudMap \"t100-clouds" << ranCtex << ".*\"\n";
  } else if (the_planet->getCloudCover() > 0.25) {
    std::cout << "\t\tCloudMap \"t50-clouds" << ranCtex << ".*\"\n";
  }
}

/**
 * @brief Assign Distance Colors
 *
 * @param the_planet
 * @param r0
 * @param g0
 * @param b0
 */
void assignDistanceColors(planet* the_planet, long double r0, long double g0, long double b0) {
  long double r, g, b;

  r = r0 + ((1.0 - r0) *
            (the_planet->getA() / (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
                                                                      SUN_MASS_IN_EARTH_MASSES))));
  g = g0 + ((1.0 - g0) *
            (the_planet->getA() / (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
                                                                      SUN_MASS_IN_EARTH_MASSES))));
  b = b0 + ((1.0 - b0) *
            (the_planet->getA() / (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
                                                                      SUN_MASS_IN_EARTH_MASSES))));
  if (r < 0.0) {
    r = 0.0;
  } else if (r > 1.0) {
    r = 1.0;
  }
  if (g < 0.0) {
    g = 0.0;
  } else if (g > 1.0) {
    g = 1.0;
  }
  if (b < 0.0) {
    b = 0.0;
  } else if (b > 1.0) {
    b = 1.0;
  }
  std::cout << "\tColor [ " << toString(r) << " " << toString(g) << " " << toString(b) << " ]\n";
  std::cout << "\tBlendTexture true\n";
}

/**
 * @brief Assign Temperature Colors
 *
 * @param the_planet
 * @param t0
 * @param r0
 * @param g0
 * @param b0
 * @param t1
 * @param r1
 * @param g1
 * @param b1
 */
void assignTemperatureColors(planet* the_planet, long double t0, long double r0, long double g0,
                             long double b0, long double t1, long double r1, long double g1,
                             long double b1) {
  long double r, g, b, te;

  te = the_planet->getEstimatedTemp();
  // modify color proportional to temperature
  r = r0 + ((r1 - r0) * ((te - t0) / (t1 - t0)));
  g = g0 + ((g1 - g0) * ((te - t0) / (t1 - t0)));
  b = b0 + ((b1 - b0) * ((te - t0) / (t1 - t0)));

  std::cout << "\tColor [ " << toString(r) << " " << toString(g) << " " << toString(b) << " ]\n";
  std::cout << "\tBlendTexture true\n";
}