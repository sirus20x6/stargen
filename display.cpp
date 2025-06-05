#include <variant>
#include "display.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "PerformanceMonitor.h"
#include "const.h"
#include "elements.h"
#include "enviro.h"
#include "stargen.h"
#include "utils.h"

using namespace std;

void ensure_directory_exists(const std::string& path) {
  std::filesystem::path dir_path(path);
  if (!std::filesystem::exists(dir_path)) {
    std::filesystem::create_directories(dir_path);
    std::cout << "Created directory: " << path << std::endl;
  }
}

int random_numberInt(int min, int max) {
  static std::random_device              rd;
  static std::mt19937                    gen(rd());
  static std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

/**
 * @brief Output system to text
 *
 * @param innermost_planet
 * @param do_gases
 * @param seed
 * @param do_moons
 */
void text_describe_system(planet* innermost_planet, bool do_gases, long int seed, bool do_moons) {
  performanceMonitor.recordFileOperation("text_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  sun     the_sun = innermost_planet->getTheSun();
  int     counter;

  std::cout << std::format("Stargen - V{}; seed={}\n", stargen_revision, seed)
            << "                          SYSTEM  CHARACTERISTICS\n";
  if (!the_sun.getIsCircumbinary()) {
    std::cout << std::format("Stellar mass: {} solar masses\n", the_sun.getMass());
    std::cout << std::format("Stellar luminosity: {}\n", the_sun.getLuminosity());
  } else {
    std::cout << std::format("Mass of Primary: {} solar masses\n", the_sun.getMass());
    std::cout << std::format("Luminosity of Primary: {}\n", the_sun.getLuminosity());
    std::cout << std::format("Mass of Secondary: {} solar masses\n", the_sun.getSecondaryMass());
    std::cout << std::format("Luminosity of Secondary: {}\n", the_sun.getSecondaryLuminosity());
  }
  std::cout << std::format("Age: {} billion years\t({} billion left on main sequence)\n",
                           the_sun.getAge() / 1.0E9,
                           (the_sun.getAge() - the_sun.getLife()) / 1.0E9);
  std::cout << std::format("Habitable ecosphere radius: {} AU\n\n",
                           AVE(habitable_zone_distance(the_sun, RECENT_VENUS, 1.0),
                               habitable_zone_distance(the_sun, EARLY_MARS, 1.0)));
  std::cout << "Planets present at:\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    std::cout << std::format(
        "{}\t{} AU\t{} EM\t{}\n", counter, the_planet->getA(),
        the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES,
        ((the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0)       ? '+'
         : ((the_planet->getHydrosphere() > .05) && (the_planet->getHydrosphere() < 0.8)) ? '*'
         : ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > .1)                      ? 'o'
                                                                                          : '.'));
  }
  std::cout << "\n\n\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    std::cout << std::format("Planet {}", counter);
    if (is_gas_planet(the_planet)) {
      std::cout << "\t*gas giant*";
    }
    std::cout << '\n';
    if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
      std::cout << "Planet is tidally locked with one face to star.\n";
    } else if (the_planet->getResonantPeriod()) {
      std::cout << "Planet's rotation is in a resonant spin lock with the star.\n";
    }
    std::cout << std::format("   Distance from primary star:\t{} AU\n", the_planet->getA())
              << std::format("   Eccentricity of orbit:\t{}\n", the_planet->getE())
              << std::format("   Length of year:\t\t{} days\n", the_planet->getOrbPeriod())
              << std::format("   Length of day:\t\t{} hours\n", the_planet->getDay())
              << std::format("   Mass:\t\t\t{} Earth masses\n",
                             the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
    if (!is_gas_planet(the_planet)) {
      std::cout << std::format("   Surface gravity:\t\t{} Earth gees\n", the_planet->getSurfGrav())
                << std::format("   Surface pressure:\t\t{} Earth atmospheres",
                               the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS);
      if (the_planet->getGreenhouseEffect() && the_planet->getSurfPressure() > 0.0) {
        std::cout << " GREENHOUSE EFFECT";
      }
      std::cout
          << '\n'
          << std::format("   Surface temperature:\t\t{} degrees Celcius\n",
                         the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER)
          << std::format("   Boiling point of water:\t{} degrees Celcius\n",
                         the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER)
          << std::format("   Hydrosphere percentage:\t{}%\n", the_planet->getHydrosphere() * 100.0)
          << std::format("   Cloud cover percentage:\t{}%\n", the_planet->getCloudCover() * 100.0)
          << std::format("   Ice cover percentage:\t{}%\n", the_planet->getIceCover() * 100.0);
    }
    std::cout << std::format("   Equatorial radius:\t\t{} Km\n", the_planet->getRadius())
              << std::format("   Density:\t\t\t{} grams/cc\n", the_planet->getDensity())
              << std::format("   Escape Velocity:\t\t{} Km/sec\n",
                             the_planet->getEscVelocity() / CM_PER_KM)
              << std::format("   Molecular weight retained:\t{} and above\n",
                             the_planet->getMolecWeight())
              << std::format("   Surface acceleration:\t{} cm/sec2\n", the_planet->getSurfAccel())
              << std::format("   Axial tilt:\t\t\t{} degrees\n", the_planet->getAxialTilt())
              << std::format("   Planetary albedo:\t\t{}\n", the_planet->getAlbedo()) << "\n\n";
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
void csv_describe_system(fstream& the_file, planet* innermost_planet, bool do_gases, long int seed,
                         bool do_moons) {
  performanceMonitor.recordFileOperation("csv_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet*      the_planet;
  sun          the_sun = innermost_planet->getTheSun();
  int          counter;
  stringstream ss;
  string       id;
  planet*      moon;
  int          moons;

  if (!the_sun.getIsCircumbinary()) {
    the_file << "'Seed', 'Star Name', 'Luminosity', 'Mass', 'Temperature', "
                "'Spectral Type', 'Total Time on Main Sequence', 'Age', "
                "'Earth-like Distance'\n";
    the_file << seed << ", '" << the_sun.getName() << "', " << toString(the_sun.getLuminosity())
             << ", " << toString(the_sun.getMass()) << ", " << toString(the_sun.getEffTemp())
             << ", '" << the_sun.getSpecType() << "', " << toString(the_sun.getLife()) << ", "
             << toString(the_sun.getAge()) << ", " << toString(the_sun.getREcosphere(1.0)) << "\n";
  } else {
    the_file << "'Seed', 'Star Name', 'Luminosity of Primary', 'Mass of Primary', "
                "'Temperature of Primary', 'Spectral Type of Primary', 'Luminosity "
                "of Secondary', 'Mass of Secondary', 'Temperature of Secondary', "
                "'Spectral Type of Secondary', 'Seperation', 'Eccentricity', "
                "'Combined Temperature', 'Primary's Total Time on Main Sequence', "
                "'Age', 'Earth-like Distance'\n";
    the_file << seed << ", '" << the_sun.getName() << "', " << toString(the_sun.getLuminosity())
             << ", " << toString(the_sun.getMass()) << ", " << toString(the_sun.getEffTemp())
             << ", '" << the_sun.getSpecType() << "', "
             << toString(the_sun.getSecondaryLuminosity()) << ", "
             << toString(the_sun.getSecondaryMass()) << ", "
             << toString(the_sun.getSecondaryEffTemp()) << ", '" << the_sun.getSecondarySpecType()
             << "', " << toString(the_sun.getSeperation()) << ", "
             << toString(the_sun.getEccentricity()) << ", "
             << toString(the_sun.getCombinedEffTemp()) << ", " << toString(the_sun.getLife())
             << ", " << toString(the_sun.getAge()) << ", " << toString(the_sun.getREcosphere(1.0))
             << "\n";
  }
  the_file << "'Planet #', 'Distance', 'Eccentricity', 'Inclination', 'Longitude of "
              "the Ascending Node', 'Longitude of the Pericenter', 'Mean "
              "Longitude', 'Axial Tilt', 'Ice Mass Fraction', 'Rock Mass Fraction', "
              "'Carbon Mass Fraction', 'Total Mass', 'Is Gas Giant', 'Dust Mass', "
              "'Gas Mass', 'Radius of Core', 'Total Radius', 'Orbit Zone', "
              "'Density', 'Orbit Period', 'Rotation Period', 'Has Spin Orbit "
              "Resonance', 'Escape Velocity', 'Surface Acceleration', 'Surface "
              "Gravity', 'RMS Velocity', 'Minimum Molecular Weight', 'Volatile Gas "
              "Inventory', 'Surface Pressure', 'Greenhouse Effect', 'Boiling "
              "Point', 'Albedo', 'Exospheric Temperature', 'Estimated Temperature', "
              "'Estimated Terran Temperature', 'Surface Temperature', 'Greenhouse "
              "Rise', 'High Temperature', 'Low Temperature', 'Maximum Temperature', "
              "'Minimum Temperature', 'Hydrosphere', 'Cloud Cover', 'Ice Cover', "
              "'Atmosphere', 'Type', 'Minor Moons'\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    csv_row(the_file, the_planet, do_gases, false, id, ss);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
           moon = moon->next_planet, moons++) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        csv_row(the_file, moon, do_gases, true, id, ss);
      }
    }
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
void jsonDescribeSystem(fstream& the_file, planet* innermost_planet, bool do_gases, long int seed,
                        bool do_moons) {
  performanceMonitor.recordFileOperation("json_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet*      the_planet;
  sun          the_sun = innermost_planet->getTheSun();
  int          counter;
  stringstream ss;
  string       id;
  planet*      moon;
  int          moons;

  if (!the_sun.getIsCircumbinary()) {
    nlohmann::json header = {
        {"Body Type",                   "Binary Star"             },
        {"seed",                        seed                      },
        {"Star Name",                   the_sun.getName()         },
        {"Luminosity",                  the_sun.getLuminosity()   },
        {"Mass",                        the_sun.getMass()         },
        {"Temperature",                 the_sun.getEffTemp()      },
        {"Spectral Type",               the_sun.getSpecType()     },
        {"Total Time on Main Sequence", the_sun.getLife()         },
        {"Age",                         the_sun.getAge()          },
        {"Earth-like Distance",         the_sun.getREcosphere(1.0)}
    };
    the_file << header.dump(4) << "\n";
  } else {
    nlohmann::json header = {
        {"Body Type",                   "Star"                          },
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
        {"Total Time on Main Sequence", the_sun.getLife()               },
        {"Age",                         the_sun.getAge()                },
        {"Earth-like Distance",         the_sun.getREcosphere(1.0)      }
    };
    the_file << header.dump(4) << "\n";
  }
  nlohmann::json body;

  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    jsonRow(the_file, the_planet, do_gases, false, id, ss);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
           moon = moon->next_planet, moons++) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        jsonRow(the_file, moon, do_gases, true, id, ss);
      }
    }
  }
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
void csv_row(std::fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             const std::string& id, std::stringstream& ss) {
  do_gases = (flags_arg_clone & fDoGases) != 0;

  std::string atmosphere = generate_atmosphere_string(the_planet, do_gases);

  using variant_type                    = std::variant<double, long double, int, bool, std::string>;
  std::vector<variant_type> planet_data = {id,
                                           is_moon ? the_planet->getMoonA() : the_planet->getA(),
                                           is_moon ? the_planet->getMoonE() : the_planet->getE(),
                                           the_planet->getInclination(),
                                           the_planet->getAscendingNode(),
                                           the_planet->getLongitudeOfPericenter(),
                                           the_planet->getMeanLongitude(),
                                           the_planet->getAxialTilt(),
                                           the_planet->getImf(),
                                           the_planet->getRmf(),
                                           the_planet->getCmf(),
                                           the_planet->getMass(),
                                           the_planet->getGasGiant(),
                                           the_planet->getDustMass(),
                                           the_planet->getGasMass(),
                                           the_planet->getCoreRadius(),
                                           the_planet->getRadius(),
                                           the_planet->getOrbitZone(),
                                           the_planet->getDensity(),
                                           the_planet->getOrbPeriod(),
                                           the_planet->getDay(),
                                           the_planet->getResonantPeriod(),
                                           the_planet->getEscVelocity(),
                                           the_planet->getSurfAccel(),
                                           the_planet->getSurfGrav(),
                                           the_planet->getRmsVelocity(),
                                           the_planet->getMolecWeight(),
                                           the_planet->getVolatileGasInventory(),
                                           the_planet->getSurfPressure(),
                                           the_planet->getGreenhouseEffect(),
                                           the_planet->getBoilPoint(),
                                           the_planet->getAlbedo(),
                                           the_planet->getExosphericTemp(),
                                           the_planet->getEstimatedTemp(),
                                           the_planet->getEstimatedTerrTemp(),
                                           the_planet->getSurfTemp(),
                                           the_planet->getGreenhsRise(),
                                           the_planet->getHighTemp(),
                                           the_planet->getLowTemp(),
                                           the_planet->getMaxTemp(),
                                           the_planet->getMinTemp(),
                                           the_planet->getHydrosphere(),
                                           the_planet->getCloudCover(),
                                           the_planet->getIceCover(),
                                           atmosphere,
                                           type_string(the_planet),
                                           the_planet->getMinorMoons()};

  std::string row;
  for (const auto& data : planet_data) {
    row += std::visit(
        [](const auto& value) {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, std::string>) {
            return std::format("'{}'", value);
          } else if constexpr (std::is_floating_point_v<T>) {
            return std::format("{:.6f}", value);
          } else {
            return std::format("{}", value);
          }
        },
        data);
    row += ", ";
  }

  // Remove the trailing ", " and add a newline
  row.pop_back();
  row.pop_back();
  row += '\n';

  the_file << row;

  ZoneScoped;
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

void jsonRow(fstream& the_file, planet* the_planet, bool do_gases, bool is_moon, string id,
             stringstream& ss) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  string      atmosphere;
  long double ipp;
  int         index;
  bool        poisonous;

  if (do_gases) {
    ss.str();
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      index     = gases.count();
      poisonous = false;

      for (int n = 0; n < gases.count(); n++) {
        if (gases[n].getNum() == the_planet->getGas(i).getNum()) {
          index = n;
          break;
        }
      }

      ipp = inspired_partial_pressure(the_planet->getSurfPressure(),
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
         << " " << toString(the_planet->getGas(i).getSurfPressure()) << " (" << toString(ipp)
         << ")";
      if (poisonous) {
        ss << " poisonous";
      }
      ss << ";";
    }
    atmosphere = ss.str();
    ss.str("");
  }

  nlohmann::json body;
  body["Planet #"] = id;
  if (!is_moon) {
    body["Distance"]     = the_planet->getA();
    body["Eccentricity"] = the_planet->getE();
  } else {
    body["Distance au"]  = the_planet->getMoonA();
    body["Eccentricity"] = the_planet->getMoonE();
  }
  body["Inclination"]                     = the_planet->getInclination();
  body["Longitude of the Ascending Node"] = the_planet->getAscendingNode();
  body["Longitude of the Pericenter"]     = the_planet->getLongitudeOfPericenter();
  body["Mean Longitude"]                  = the_planet->getMeanLongitude();
  body["Axial Tilt"]                      = the_planet->getAxialTilt();
  body["Ice Mass Fraction"]               = the_planet->getImf();
  body["Rock Mass Fraction"]              = the_planet->getRmf();
  body["Carbon Mass Fraction"]            = the_planet->getCmf();
  body["Total Mass"]                      = the_planet->getMass();
  body["Is Gas Giant"]                    = the_planet->getGasMass();
  body["Dust Mass"]                       = the_planet->getDustMass();
  body["Gas Mass"]                        = the_planet->getMass();
  body["Radius of Core"]                  = the_planet->getCoreRadius();
  body["Total Radius"]                    = the_planet->getRadius();
  body["Orbit Zone"]                      = the_planet->getOrbitZone();
  body["Density"]                         = the_planet->getDensity();
  body["Orbit Period"]                    = the_planet->getOrbPeriod();
  body["Rotation Period"]                 = the_planet->getDay();
  body["Has Spin Orbit Resonance"]        = the_planet->getResonantPeriod();
  body["Escape Velocity"]                 = the_planet->getEscVelocity();
  body["Surface Acceleration"]            = the_planet->getSurfAccel();
  body["Surface Gravity"]                 = the_planet->getSurfGrav();
  body["RMS Velocity"]                    = the_planet->getRmsVelocity();
  body["Minimum Molecular Weight"]        = the_planet->getMolecWeight();
  body["Volatile Gas Inventory"]          = the_planet->getVolatileGasInventory();
  body["Get Surface Pressure"]            = the_planet->getSurfPressure();
  body["Greenhouse Effect"]               = the_planet->getGreenhouseEffect();
  body["Boiling Point"]                   = the_planet->getBoilPoint();
  body["Albedo"]                          = the_planet->getAlbedo();
  body["Exospheric Temperature"]          = the_planet->getExosphericTemp();
  body["Estimated Temperature"]           = the_planet->getEstimatedTemp();
  body["Estimated Terran Temperature"]    = the_planet->getEstimatedTerrTemp();
  body["Surface Temperature"]             = the_planet->getSurfTemp();
  body["Greenhouse Rise"]                 = the_planet->getGreenhsRise();
  body["High Temperature"]                = the_planet->getHighTemp();
  body["Low Temperature"]                 = the_planet->getLowTemp();
  body["Maximum Temperature"]             = the_planet->getMaxTemp();
  body["Minimum Temperature"]             = the_planet->getMinTemp();
  body["Hydrosphere"]                     = the_planet->getHydrosphere();
  body["Cloud Cover"]                     = the_planet->getCloudCover();
  body["Ice Cover"]                       = the_planet->getIceCover();
  body["Atmosphere"]                      = atmosphere;
  body["Type"]                            = type_string(the_planet);
  body["Minor Moons"]                     = the_planet->getMinorMoons();
  the_file << body.dump(4) << "\n";
  ZoneScoped;
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
      return format("{} Gas Dwarf", cloud_type_string(the_planet));
    case tSubGasGiant:
      return format("{} Neptunian", cloud_type_string(the_planet));
    case tGasGiant:
      return format("{} Jovian", cloud_type_string(the_planet));
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
string cloud_type_string(planet* the_planet) {
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
void create_svg_file(planet* innermost_planet, string path, string file_name, string svg_ext,
                     string prognam, bool do_moons) {
  performanceMonitor.recordFileOperation("svg_output");
  planet*      outermost_planet;
  planet*      a_planet;
  fstream      output;
  string       the_file_spec;
  stringstream ss;

  ss.str("");
  ss << path << file_name << svg_ext;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype    = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), ios::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype    = 'TEXT';
#endif

  for (outermost_planet = innermost_planet; outermost_planet != nullptr;
       outermost_planet = outermost_planet->next_planet) {
    long double max_x      = 760.0;
    long double max_y      = 120.0;
    long double margin     = 20.0;
    long double inner_edge = innermost_planet->getA() * (1.0 - innermost_planet->getE());
    long double outer_edge = outermost_planet->getA() * (1.0 + outermost_planet->getE());
    long double floor      = (int)(log10(inner_edge) - 1.0);
    long double min_log    = floor;
    long double ceiling    = (int)(log10(outer_edge) + 1.0);
    long double max_log    = 0.0;
    long double mult;
    long double offset;
    long double em_scale = 5;

    // todo: fix loop counter
    for (int x = floor; x <= ceiling; x++) {
      float n;

      for (n = 1.0; n < 9.9; n++) {
        if (inner_edge > std::pow(10.0, x + log10(n))) {
          min_log = x + log10(n);
        }

        if (max_log == 0.0 && outer_edge < std::pow(10.0, x + log10(n))) {
          max_log = x + log10(n);
        }
      }
    }

    mult     = max_x / (max_log - min_log);
    offset   = -mult * (1.0 + min_log);
    em_scale = 5;

    output << "<?xml version='1.0' standalone='no'?>\n";
    output << "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' \n";
    output << "  'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>\n";
    output << "\n";
    output << "<svg width='100%' height='100%' version='1.1'\n";
    output << "     xmlns='http://www.w3.org/2000/svg'\n";
    output << "     viewBox='-" << margin << " -" << margin << " " << (max_x + (margin * 2.0))
           << " " << (max_y + (margin * 2.0)) << "' >\n";
    output << "\n";
    output << "<title>" << innermost_planet->getTheSun().getName() << "</title>\n";
    output << "<desc>Created by: " << prognam << " - " << stargen_revision << "</desc>\n";
    output << "\n";
    output << "<g stroke='black' stroke-width='1'>\n";
    output << "    <line x1='" << ((offset + mult) + (min_log * mult)) << "' y1='"
           << (max_y - margin) << "' x2='" << ((offset + mult) + (max_log * mult)) << "' y2='"
           << (max_y - margin) << "' />\n";

    // todo: fix loop counter
    for (int x = floor; x <= ceiling; x++) {
      float n;

      for (n = 1.0; n < 9.9; n++) {
        if (min_log <= (x + log10(n)) && max_log >= (x + log10(n))) {
          float value;
          if (n == 1) {
            value = 10;
          } else {
            value = 5;
          }
          output << "    <line x1='" << ((offset + mult) + ((x + log10(n)) * mult)) << "' y1='"
                 << (max_y - margin - value) << "' x2='"
                 << ((offset + mult) + ((x + log10(n)) * mult)) << "' y2='"
                 << (max_y - margin + value) << "' />\n";
        }
      }
    }

    output << "</g>\n\n";

    sun         the_sun         = innermost_planet->getTheSun();
    long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
    long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

    output << "<line x1='"
           << ((offset + mult) + (log10(innermost_planet->getTheSun().getREcosphere(1.0)) * mult))
           << "' y1='" << ((max_y - margin) - 5) << "' x2='"
           << ((offset + mult) + (log10(innermost_planet->getTheSun().getREcosphere(1.0)) * mult))
           << "' y2='" << ((max_y - margin) + 5) << "' stroke='blue' stroke-width='1' />\n";

    output << "<line x1='" << ((offset + mult) + (log10(min_r_ecosphere) * mult)) << "' y1='"
           << (max_y - margin) << "' x2='" << ((offset + mult) + (log10(max_r_ecosphere) * mult))
           << "' y2='" << (max_y - margin)
           << "' stroke='#66c' stroke-width='10' stroke-opacity='0.5' />\n";

    output << "<g font-family='Arial' font-size='10' \n";
    output << "   font-style='normal' font-weight='normal'\n";
    output << "   fill='black' text-anchor='middle'>\n";

    // todo: fix loop counter
    for (int x = floor; x <= ceiling; x++) {
      if (min_log <= x && max_log >= x) {
        output << "    <text x='" << ((offset + mult) + (x * mult)) << "' y='120'> "
               << std::pow(10.0, x) << " AU </text>\n";
      }
    }

    output << "\n";

    for (a_planet = innermost_planet; a_planet != nullptr; a_planet = a_planet->next_planet) {
      long double x = (offset + mult) + (log10(a_planet->getA()) * mult);
      long double r =
          std::pow((a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES), 1.0 / 3.0) * em_scale;
      long double x1 =
          (offset + mult) + (log10(a_planet->getA() * (1.0 - a_planet->getE())) * mult);
      long double x2 =
          (offset + mult) + (log10(a_planet->getA() * (1.0 + a_planet->getE())) * mult);

      output << "    <circle cx='" << x << "' cy='30' r='" << r
             << "' fill='none' stroke='black' stroke-width='1' />\n";
      output << "    <line x1='" << x1 << "' y1='" << ((max_y - margin) - 15.0) << "' x2='" << x2
             << "' y2='" << ((max_y - margin) - 15.0)
             << "' stroke='black' stroke-width='8' stroke-opacity='0.3'/>\n";
      output << "    <text x='" << x << "' y='" << (max_y - (margin * 2.0)) << "'>";
      output << (a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
      output << "</text>\n\n";
    }

    output << "</g>\n";
    output << "</svg>\n";
  }
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
void openCVSorJson(string path, string the_filename, fstream& output) {
  ensure_directory_exists(path);
  string       the_file_spec;
  stringstream ss;

  ss.str("");
  ss << path << the_filename;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype    = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), ios::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype    = 'TEXT';
#endif
  if (!output) {
    exit(EXIT_FAILURE);
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
void refresh_file_stream(fstream& output, const string& path, const string& file_name,
                         const string& ext) {
  string       the_file_spec;
  stringstream ss;

  output.close();

  ss.str("");
  ss << path << file_name << ext;
  the_file_spec = ss.str();

  output.open(the_file_spec.c_str(), fstream::out | fstream::app);
  if (!output) {
    exit(EXIT_FAILURE);
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
void open_html_file(const string& system_name, long seed, const string& path,
                    const string& url_path, const string& file_name, const string& ext,
                    const string& prognam, fstream& output) {
  ensure_directory_exists(path);
  string       the_file_spec;
  bool         noname = system_name.empty();
  stringstream ss;

  ss.str("");
  ss << path << file_name << ext;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype    = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), fstream::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype    = 'TEXT';
#endif
  if (!output) {
    exit(EXIT_FAILURE);
  }

  output << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n";
  output << "        \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  output << "<html>\n";
  output << "<head>\n";
  output << "<meta http-equiv=content-type content='text/html; charset=utf-8'>\n";
  output << "\t<title>System " << seed << (noname ? "" : " - ") << (noname ? "" : system_name)
         << "</title>\n";
  output << "\t<meta name='generator' content='" << prognam << " - " << stargen_revision << "'>\n";
  output << "<style type='text/css'>\n";
  output << "<!--\n";
  output << "table {border-color: #ffd;}\n";
  output << "-->\n";
  output << "</style>\n";
  output << "<link rel='icon' type='image/png' href='" << url_path << "ref/favicon.png'>\n";
  output << "</head>\n";
  output << "<body bgcolor='" << BGCOLOR << "' text='" << TXCOLOR << "' link='" << LINKCOLOR
         << "' vlink='" << TXCOLOR << "' alink='" << ALINKCOLOR << "'>\n\n";
}

/**
 * @brief Close HTML file
 *
 * @param the_file
 */
void close_html_file(fstream& the_file) {
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
void print_description(fstream& the_file, const string& opening, planet* the_planet,
                       const string& closing) {
  bool         first        = true;
  long double  earth_masses = the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES;
  long double  earth_radii  = convert_km_to_eu(the_planet->getRadius());
  stringstream ss;

  the_file << opening;

  if (the_planet->getDay() < 10.0) {
    lprint(the_file, first, "Short Day");
  } else if (the_planet->getDay() > 48.0) {
    lprint(the_file, first, "Long Day");
  }

  if (the_planet->getMoonA() == 0 && calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0 &&
      the_planet->getType() != tAsteroids) {
    lprint(the_file, first, "Dwarf planet");
  }

  if (is_gas_planet(the_planet)) {
    long double temp = the_planet->getEstimatedTemp() - FREEZING_POINT_OF_WATER;
    lprint(the_file, first, format("{} Earth Masses", earth_masses));
    lprint(the_file, first, format("{} C", temp));
  } else {
    long double rel_temp =
        (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) - EARTH_AVERAGE_CELSIUS;
    long double seas       = 100.0 * the_planet->getHydrosphere();
    long double clouds     = 100.0 * the_planet->getCloudCover();
    long double atmosphere = the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS;
    long double ice        = 100.0 * the_planet->getIceCover();
    long double gravity    = the_planet->getSurfGrav();
    long double iron       = (1.0 - (the_planet->getRmf() + the_planet->getImf())) * 100.0;

    // Gravity description
    switch (static_cast<int>(gravity * 2)) {
      case 0:
      case 1:
        lprint(the_file, first, "Very Low-G");
        break;
      case 2:
      case 3:
        lprint(the_file, first, "Low-G");
        break;
      case 4:
      case 5:
        lprint(the_file, first, "Earth-Like-G");
        break;
      case 6:
      case 7:
        lprint(the_file, first, "High-G");
        break;
      default:
        lprint(the_file, first, "Very-High-G");
        break;
    }

    // Temperature description
    switch (static_cast<int>(rel_temp + 5) / 5) {
      case 0:
        lprint(the_file, first, "Cold");
        break;
      case 1:
        lprint(the_file, first, "Cool");
        break;
      case 2:
        lprint(the_file, first, "Mild");
        break;
      case 3:
        lprint(the_file, first, "Warm");
        break;
      default:
        lprint(the_file, first, "Hot");
        break;
    }

    // Iron core description
    if (the_planet->getType() != tIron && the_planet->getType() != tAsteroids) {
      switch (static_cast<int>(iron / 20)) {
        case 5:
          lprint(the_file, first, "'Cannonball'");
          break;
        case 4:
        case 3:
          lprint(the_file, first, "Large Iron Core");
          break;
        case 2:
        case 1:
          lprint(the_file, first, "Medium Iron Core");
          break;
        case 0:
          if (iron < 1)
            lprint(the_file, first, "Coreless");
          else
            lprint(the_file, first, "Small Iron Core");
          break;
      }
    } else if (the_planet->getType() == tAsteroids) {
      if (iron > 80)
        lprint(the_file, first, "Metallic");
      else if (iron < 20)
        lprint(the_file, first, "Rocky");
    }

    // Ice description
    if (the_planet->getType() == tIce) {
      switch (static_cast<int>(ice / 25)) {
        case 4:
        case 3:
          lprint(the_file, first, "Mostly Ice");
          break;
        case 2:
          lprint(the_file, first, "Half Ice");
          break;
        case 1:
          lprint(the_file, first, "Significant Ice");
          break;
        default:
          if (ice > 10.0) lprint(the_file, first, "A Bit Icey");
          break;
      }
    } else if (ice > 90.0) {
      lprint(the_file, first, "Ice Covered");
    }

    // Atmosphere and surface description
    if (atmosphere < 0.001) {
      lprint(the_file, first, "Airless");
    } else {
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

      if (clouds < 10.0)
        lprint(the_file, first, "Cloudless");
      else if (clouds < 40.0)
        lprint(the_file, first, "Few clouds");
      else if (clouds > 80.0)
        lprint(the_file, first, "Cloudy");

      if (the_planet->getMaxTemp() > the_planet->getBoilPoint() && seas > 0.0) {
        lprint(the_file, first, "Boiling ocean");
      }

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
  the_file << " - ";

  sun         the_sun         = the_planet->getTheSun();
  long double min_r_ecosphere = habitable_zone_distance(
      the_sun, RECENT_VENUS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
  long double max_r_ecosphere = habitable_zone_distance(
      the_sun, EARLY_MARS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  if (the_planet->getA() < min_r_ecosphere || the_planet->getA() > max_r_ecosphere) {
    if (is_habitable_extended(the_planet)) {
      the_file << "Habitable (Extended Definition) ";
    } else if (is_potentialy_habitable_extended(the_planet)) {
      the_file << "Potentially Habitable (Extended Definition) ";
    }
    the_file << (the_planet->getA() < min_r_ecosphere ? "Hot " : "Cold ");
  } else {
    if (is_habitable(the_planet)) {
      switch (0) {
        case 0:
          if (is_habitable_earth_like(the_planet)) {
            the_file << "Habitable (Earth-like Definition) ";
            break;
          }
        case 1:
          if (is_habitable_conservative(the_planet)) {
            the_file << "Habitable (Conservative Definition) ";
            break;
          }
        case 2:
          if (is_habitable_optimistic(the_planet)) {
            the_file << "Habitable (Optimistic Definition) ";
            break;
          }
        case 3:
          if (is_habitable_extended(the_planet)) {
            the_file << "Habitable (Extended Definition) ";
            break;
          }
      }
    } else if (is_potentialy_habitable(the_planet)) {
      switch (0) {
        case 0:
          if (is_potentialy_habitable_earth_like(the_planet)) {
            the_file << "Potentially Habitable (Earth-like Definition) ";
            break;
          }
        case 1:
          if (is_potentialy_habitable_conservative(the_planet)) {
            the_file << "Potentially Habitable (Conservative Definition) ";
            break;
          }
        case 2:
          if (is_potentialy_habitable_optimistic(the_planet)) {
            the_file << "Potentially Habitable (Optimistic Definition) ";
            break;
          }
        case 3:
          if (is_potentialy_habitable_extended(the_planet)) {
            the_file << "Potentially Habitable (Extended Definition) ";
            break;
          }
      }
    }
    the_file << "Warm ";
  }

  switch (0) {
    case 0:
      if (earth_masses <= 0.00001 || earth_radii <= 0.03) {
        the_file << "Asteroidan";
        break;
      }
    case 1:
      if (earth_masses <= 0.1 || earth_radii <= 0.4) {
        the_file << "Mercurian";
        break;
      }
    case 2:
      if (earth_masses <= 0.5 || earth_radii <= 0.8) {
        the_file << "Subterran";
        break;
      }
    case 3:
      if (earth_masses <= 5.0 || earth_radii <= 1.5) {
        the_file << "Terran";
        break;
      }
    case 4:
      if (earth_masses <= 10.0 || earth_radii <= 2.5) {
        the_file << "Superterran";
        break;
      }
    case 5:
      if (earth_masses <= 50.0 || earth_radii <= 6.0) {
        the_file << "Neptunian";
        break;
      }
    default:
      the_file << "Jovian";
  }

  the_file << " (" << the_planet->getOrbPeriod() << " day long orbit)";

  the_file << closing;
}

/**
 * @brief List molecules
 *
 * @param the_file
 * @param weight
 */
void list_molecules(fstream& the_file, long double weight) {
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
void html_star_details_helper(fstream& the_file, const string& header, long double mass,
                              long double luminosity, long double temperature, long double age,
                              long double life, const string& spec_type) {
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
  the_file << "\t<td>" << spec_type << "</td></tr>\n";
  the_file << "<tr><td>Age</td>\n";
  the_file << "\t<td>" << toString(age / 1.0E9) << " billion years<br>("
           << toString((life - age) / 1.0E9) << " billion left on main sequence)<br></td></tr>";
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
void html_thumbnails(planet* innermost_planet, fstream& the_file, const string& system_name,
                     const string& url_path, const string& system_url, const string& svg_url,
                     const string& file_name, bool details, bool terrestrials, bool int_link,
                     bool do_moons, int graphic_format, bool do_gases) {
  planet*      the_planet;
  sun          the_sun = innermost_planet->getTheSun();
  int          counter;
  int          planet_count               = 0;
  bool         terrestrials_seen          = false;
  bool         habitable_jovians_seen     = false;
  bool         potentialy_habitables_seen = false;
  planet*      moon;
  int          moons;
  int          dwarf_planet_count  = 0;
  int          asteroid_belt_count = 0;
  int          moon_count          = 0;
  int          object_count        = 0;
  stringstream ss;
  string       planet_id;

  if (!the_file) {
    std::cout << "We have a serious error!\n";
    exit(EXIT_FAILURE);
  }

  for (the_planet = innermost_planet; the_planet != nullptr; the_planet = the_planet->next_planet) {
    if (the_planet->getType() == tAsteroids) {
      asteroid_belt_count++;
    } else if (calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0) {
      dwarf_planet_count++;
    } else {
      planet_count++;
    }
    object_count++;
    if (do_moons) {
      for (moon = the_planet->first_moon; moon != nullptr; moon = moon->next_planet) {
        moon_count++;
      }
    }
  }

  the_file << "<p>\n\n";
  the_file << "<table border=3 cellspacing=2 cellpadding=2 align=center ";
  the_file << "bgcolor='" << BGTABLE << "' width='90%'>\n";
  the_file << "<tr><th colspan=2 bgcolor='" << BGTABLE << "' align=center>\n";
  the_file << "\t<font size='+2' color='" << TXTABLE << "'>";

  if (file_name.empty()) {
    the_file << system_name;
  } else {
    the_file << "<a href='" << system_url << "'>" << system_name << "</a>";
  }

  the_file << "</font></th></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td colspan=2>\n";

  if (graphic_format == gfSVG) {
    the_file << "<object data='" << svg_url << "' type='image/svg+xml'\n";
    the_file << "        width='100%' height='100%' border=1 "
                "style='background-color:white;'>\n";
  }

  the_file << "<table border=0 cellspacing=0 cellpadding=3 bgcolor='" << BGSPACE
           << "' width='100%'>\n";

  the_file << "<tr><td colspan=" << (object_count + 2) << " bgcolor='" << BGHEADER
           << "' align=center>\n";
  the_file << "\t<font size='+1'  color='" << TXHEADER << "'><b>" << planet_count
           << " Planets,</b> <b>" << dwarf_planet_count << " Dwarf Planets,</b> <b>"
           << asteroid_belt_count << " Asteroid Belts</b>, <b>" << moon_count
           << " Moons</b></font>\n";
  the_file << "\t(<font size='-1' color='" << TXHEADER
           << "'>size proportional to Sqrt(Radius)</font>)\n";
  the_file << "</td></tr>\n";
  the_file << "<tr valign=middle bgcolor='" << BGSPACE << "'>\n";
  the_file << "<td bgcolor='" << BGSPACE << "'><img alt='Sun' src='" << url_path << "ref/Sun.gif' ";
  the_file << "width=15 height=63 border=0></td>\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    ss.str("");
    int    ppixels = ((int)(sqrt(convert_km_to_eu(the_planet->getRadius())) * 50.0)) + 1;
    string ptype   = type_string(the_planet);
    string info;

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
             << (int_link ? "" : system_url) << "#" << counter << "' title='#" << counter << " - "
             << info << "'>";
    the_file << "<img alt='" << ptype << "' src='" << url_path << "ref/"
             << image_type_string(the_planet) << "Planet.webp' width=" << ppixels
             << " height=" << ppixels << " border=0>";
    the_file << "</a>";

    if (is_terrestrial(the_planet)) {
      terrestrials_seen = true;
    }

    if (is_habitable_jovian(the_planet)) {
      habitable_jovians_seen = true;
    }

    if (is_potentialy_habitable(the_planet)) {
      potentialy_habitables_seen = true;
    }

    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; do_moons && moon != nullptr;
           moon = moon->next_planet, moons++) {
        ss.str("");
        string mtype   = type_string(moon);
        int    mpixels = ((int)(sqrt(convert_km_to_eu(moon->getRadius())) * 100.)) + 1;

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
        info = ss.str();

        the_file << "\n\t\t<br><a href='" << (int_link ? "" : system_url) << "#" << counter << "."
                 << moons << "' title='#" << counter << "." << moons << " - " << info << "'>";
        the_file << "<img alt='" << mtype << "' src='" << url_path << "ref/"
                 << image_type_string(moon) << "Planet.webp' width=" << mpixels
                 << " height=" << mpixels << " border=0>";
        the_file << "</a>";

        if (is_terrestrial(moon)) {
          terrestrials_seen = true;
        }

        if (is_habitable_jovian(moon)) {
          habitable_jovians_seen = true;
        }

        if (is_potentialy_habitable(moon)) {
          potentialy_habitables_seen = true;
        }
      }
    }
    the_file << "</td>\n";
  }
  the_file << "<td bgcolor='" << BGSPACE << "' align=right valign=bottom>";
  the_file << "<a href='" << url_path << "ref/Key.html'><font size='-3' color='" << TXSPACE
           << "'>See<br>Key</font></a></td>\n";
  the_file << "</tr></table>\n";

  if (graphic_format == gfSVG) {
    the_file << "</object>\n";
  }

  the_file << "</td></tr>\n";

  // Table of data on the star system

  if (terrestrials && (terrestrials_seen || habitable_jovians_seen || potentialy_habitables_seen)) {
    the_file << "<tr><td colspan=2><table width='100%'>";

    for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
         the_planet = the_planet->next_planet, counter++) {
      if (is_habitable_jovian(the_planet) || is_terrestrial(the_planet) ||
          is_potentialy_habitable(the_planet)) {
        the_file << "\n\t<tr><td align=right width='5%'>";
        the_file << "<a href='" << (int_link ? "" : system_url) << "#" << counter << "'><small>#"
                 << counter << "</small></a>";
        the_file << "</td>\n\t\t<td><small>" << type_string(the_planet) << ": </small>";

        print_description(the_file, "", the_planet, "");

        the_file << "</td></tr>";
      }

      if (do_moons) {
        for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
             moon = moon->next_planet, moons++) {
          if (is_habitable_jovian(moon) || is_terrestrial(moon) || is_potentialy_habitable(moon)) {
            the_file << "\n\t<tr><td align=right width='5%'>";
            the_file << "<a href='" << (int_link ? "" : system_url) << "#" << counter << "."
                     << moons << "'><small>#" << counter << "." << moons << "</small></a>";
            the_file << "</td>\n\t\t<td><small>" << type_string(moon) << ": </small>";

            print_description(the_file, "", moon, "");

            the_file << "</td></tr>";
          }
        }
      }
    }

    the_file << "</table></td></tr>\n";
  }

  if (details) {
    long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
    long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

    if (!the_sun.getIsCircumbinary()) {
      html_star_details_helper(the_file, "Stellar Characteristics", the_sun.getMass(),
                               the_sun.getLuminosity(), the_sun.getEffTemp(), the_sun.getAge(),
                               the_sun.getLife(), the_sun.getSpecType());
    } else {
      html_star_details_helper(the_file, "Stellar Characteristics of Primary", the_sun.getMass(),
                               the_sun.getLuminosity(), the_sun.getEffTemp(), the_sun.getAge(),
                               the_sun.getLife(), the_sun.getSpecType());
      html_star_details_helper(the_file, "Stellar Characteristics of Secondary",
                               the_sun.getSecondaryMass(), the_sun.getSecondaryLuminosity(),
                               the_sun.getSecondaryEffTemp(), the_sun.getAge(),
                               the_sun.getSecondaryLife(), the_sun.getSecondarySpecType());
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

  add_row("Earthlike worlds", total_earthlike);
  add_row("Total Habitable worlds (Earth-like Definition)", total_habitable_earthlike);
  add_row("Total Habitable worlds (Conservative Definition)", total_habitable_conservative);
  add_row("Total Habitable worlds (Optimistic Definition)", total_habitable_optimistic);
  add_row("Total Habitable worlds (Extended Definition)", total_habitable);
  add_row("Total Potentially Habitable worlds (Earth-like Definition)",
          total_potentially_habitable_earthlike);
  add_row("Total Potentially Habitable worlds (Conservative Definition)",
          total_potentially_habitable_conservative);
  add_row("Total Potentially Habitable worlds (Optimistic Definition)",
          total_potentially_habitable_optimistic);
  add_row("Total Potentially Habitable worlds (Extended Definition)", total_potentially_habitable);
  add_row("Total worlds", total_worlds);

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
void html_describe_planet(planet* the_planet, int counter, int moons, bool do_gases,
                          const std::string& url_path, std::fstream& the_file) {
  std::string planet_id =
      moons > 0 ? std::format("{}.{}", counter, moons) : std::to_string(counter);
  std::string typeString = type_string(the_planet);

  do_gases = (flags_arg_clone & fDoGases) != 0;

  the_file << std::format(R"(
<p>
<a name='{0}'></a>
<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='{1}' width='{2}%'>
<colgroup span=1 align=left valign=middle>
<colgroup span=2 align=left valign=middle>
<tr><th colspan=3 bgcolor='{3}' align=center>
<font size='+2' color='{4}'>{5} #{0} Statistics</font></th></tr>
)",
                          planet_id, BGTABLE, (moons == 0) ? 95 : 90, BGHEADER, TXHEADER,
                          (moons == 0) ? "Planet" : "Moon");

  the_file << std::format(R"(
<tr><th>Planet type</th>
<td colspan=2><img alt='{0}' src='{1}ref/{2}Planet.webp' align=middle width=150 height=150>{0}
)",
                          typeString, url_path, image_type_string(the_planet));

  if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
    the_file << "<br>Tidally Locked 1 Face\n";
  } else if (the_planet->getResonantPeriod()) {
    the_file << std::format("<br>Resonant Spin Locked ({} Resonance)\n",
                            moons == 0 ? printSpinResonanceFactor(the_planet->getE())
                                       : printSpinResonanceFactor(the_planet->getMoonE()));
  }

  print_description(the_file, "<br>", the_planet, "");

  the_file << "</td></tr>\n";

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
<td>{:.2f}&deg; Celcius<br>{:.2f}&deg; Fahrenheit</td>
<td rowspan=2 valign=top>{:.2f}&deg; C Earth temperature<br>{:.2f}&deg; F Earth temperature
)",
                            the_planet->getSurfAccel(), the_planet->getSurfGrav(),
                            the_planet->getSurfPressure(),
                            the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS, celsius,
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
<tr><th>Estimated Temperature</th>
<td>{:.2f}&deg; K</td>
<td>{:.2f}&deg; C Earth temperature</td></tr>
)",
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
                          the_planet->getEscVelocity() / CM_PER_KM, the_planet->getMolecWeight());

  list_molecules(the_file, the_planet->getMolecWeight());

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
                                gases[index].getName(), percentage,
                                the_planet->getGas(i).getSurfPressure(), ipp,
                                poisonous ? "poisonous" : "");
      }
    }
    the_file << "</table>\n";
  }
  the_file << "</td></tr>\n";

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

  long double esir = calcEsiHelper(convert_km_to_eu(the_planet->getRadius()), 1.0, 0.57);
  long double esid = calcEsiHelper(the_planet->getDensity() / EARTH_DENSITY, 1.0, 1.07);
  long double esiv = calcEsiHelper((the_planet->getEscVelocity() / CM_PER_KM) / 11.186, 1.0, 0.70);
  long double esit = calcEsiHelper(the_planet->getSurfTemp(), EARTH_AVERAGE_KELVIN, 5.58);

  the_file << std::format(R"(
<tr><th>Earth Similarity Index (ESI)</th>
<td>{:.4f}</td>
<td><!--esir: {:.4f}<br />esid: {:.4f}<br />esiv: {:.4f}<br />esit: {:.4f}--></td></tr>
)",
                          the_planet->getEsi(), esir, esid, esiv, esit);

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
                          const string& url_path, fstream& the_file) {
  performanceMonitor.recordFileOperation("html_output");
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  int     counter;
  planet* moon;
  int     moons;
  string  typeString;

  the_file << "\n<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='" << BGTABLE
           << "' width='90%'>\n";
  the_file << "<tr><th colspan=7 bgcolor='" << BGHEADER << "' align=center>\n";
  the_file << "<font size='+2' color='" << TXHEADER << "'>Planetary Overview</font></th></tr>\n\n";
  the_file << "<tr align=center>\n";
  the_file << "\t<th>#</th><th "
              "colspan=3>Type</th><th>Dist.</th><th>Mass</th><th>Radius</th>\n";
  the_file << "</tr>\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    typeString = type_string(the_planet);

    the_file << "<tr align=right>\n\t<td><a href='#" << counter << "'>" << counter
             << "</a></td>\n\t<td align=center><img alt='" << typeString << "' src='" << url_path
             << "ref/" << image_type_string(the_planet) << "Planet.webp' width='600'></td>\n\t<td>"
             << typeString << "</td>\n\t<td>" << toString(the_planet->getA(), 4)
             << "  AU</td>\n\t<td>" << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES, 4)
             << " EM</td>\n\t<td>" << toString(convert_km_to_eu(the_planet->getRadius()), 4)
             << " ER";
    if (is_gas_planet(the_planet)) {
      the_file << " (" << toString(the_planet->getRadius() / KM_JUPITER_RADIUS, 4) << " JR)";
    }
    the_file << "</td></tr>\n";

    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
           moon = moon->next_planet, moons++) {
        typeString = type_string(moon);

        the_file << "<tr align=right>\n";
        the_file << "\n";
        the_file << "\t<td align=center><a href='#" << counter << "." << moons << "'>" << counter
                 << "." << moons << "</a></td>\n";
        the_file << "\t<td align=center><img alt='" << typeString << "' src='" << url_path << "ref/"
                 << image_type_string(moon) << "Planet.webp' width='400'></td>\n";
        the_file << "\t<td>" << typeString << "</td>\n";
        the_file << "\t<td>" << toString(moon->getMoonA() * KM_PER_AU, 4) << " km</td>\n";
        the_file << "\t<td>" << toString(moon->getMass() * SUN_MASS_IN_EARTH_MASSES, 4)
                 << " EM</td>\n";
        the_file << "\t<td>" << toString(convert_km_to_eu(moon->getRadius()), 4) << " ER";
        if (is_gas_planet(moon)) {
          the_file << " (" << toString(moon->getRadius() / KM_JUPITER_RADIUS, 4) << " JR)";
        }
        the_file << "</td>";
        the_file << "</tr>\n";
      }
    }
  }
  the_file << "</table>\n<br clear=all>\n";

  // Tables for individual planets
  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    html_describe_planet(the_planet, counter, 0, do_gases, url_path, the_file);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
           moon = moon->next_planet, moons++) {
        html_describe_planet(moon, counter, moons, do_gases, url_path, the_file);
      }
    }
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
void celestia_describe_system(planet* innermost_planet, string designation, string system_name,
                              long int seed, long double inc, long double an, bool do_moons) {
  planet *    the_planet, *moon;
  sun         the_sun = innermost_planet->getTheSun();
  int         counter, moons;
  long double min_r_ecosphere = habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
  long double max_r_ecosphere = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

  std::cout << "# Stargen - " << stargen_revision << "; seed=" << seed << "\n"
            << "#\n"
            << "# " << designation << ", " << system_name << "\n"
            << "#\n"
            << "# Stellar mass: " << toString(the_sun.getMass()) << " solar masses\n"
            << "# Stellar luminosity: " << toString(the_sun.getLuminosity()) << "\n"
            << "# Stellar temperature: " << toString(the_sun.getEffTemp()) << " Kelvin\n"
            << "# Age: " << toString(the_sun.getAge() / 1.0E9) << "  billion years\t("
            << toString((the_sun.getLife() - the_sun.getAge()) / 1.0E9)
            << " billion left on main sequence)\n"
            << "# Habitable ecosphere: " << toString(min_r_ecosphere) << " AU to "
            << toString(max_r_ecosphere) << " AU\n\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, counter++) {
    celestia_describe_world(the_planet, designation, system_name, seed, inc, an, counter, the_sun,
                            false, counter);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != nullptr;
           moon = moon->next_planet, moons++) {
        celestia_describe_world(moon, designation, system_name, seed, inc, an, moons, the_sun, true,
                                counter);
      }
    }
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
#include <format>
#include <iostream>
#include <string>
#include <string_view>

void celestia_describe_world(planet* the_planet, const std::string& designation,
                             const std::string& system_name, long seed, long double inc,
                             long double an, int counter, sun& the_sun, bool is_moon,
                             int planet_num) {
  const auto name =
      is_moon ? std::format("\"p{}-{}\"", counter, planet_num) : std::format("\"p{}\"", counter);
  const auto parent = is_moon ? std::format("\"{}/p{}\"", designation, planet_num)
                              : std::format("\"{}\"", designation);

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
  print_detail("Escape Velocity:", the_planet->getEscVelocity() / CM_PER_KM, "Km/sec");
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

/*
void moongen_describe_system(planet* innermost_planet, string designation,
string system_name, long int seed)
{
  planet *the_planet, *moon;
  sun the_sun = innermost_planet->getTheSun();
  long tmp;
  int num_moons = 0;
  int num_planets = 0;
  int counter;
  long double mass = 0;
  long double total_moon_mass = 0;

  std::cout << "#! /bin/sh -x\n";
  std::cout << "# Stargen - " << stargen_revision << "; seed=" << seed << "\n";
  std::cout << "#\n";
  std::cout << "# Script to generate moons for some planets in the system " <<
designation << " (" << system_name << ")\n"; std::cout << "#\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != nullptr;
the_planet = the_planet->next_planet, counter++)
  {
    mass = the_planet->getMass();
    total_moon_mass = 0;
    for (moon = the_planet->first_moon; moon != nullptr; moon = moon->next_planet)
    {
      total_moon_mass += moon->getMass();
    }

    tmp = the_planet->getMinorMoons();

    num_moons += tmp;
    if (tmp > 0)
    {
      num_planets++;
      std::cout << "moon_orbits -a " << toString(the_planet->getA()) << " -m " <<
toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << " -R " <<
toString(the_planet->getRadius()) << " -M " << toString(the_sun.getMass()) << "
-n p" << counter << " -N " << tmp << " -U " << toString(total_moon_mass *
SUN_MASS_IN_EARTH_MASSES) << " -s " << (seed + (counter * 1000)) << " -S " <<
designation << "\n";
    }
  }
}
*/

/**
 * @brief lprint
 *
 * @param the_file
 * @param first
 * @param text
 */
void lprint(fstream& the_file, bool& first, const string& text) {
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
string image_type_string(planet* the_planet) {
  string       typeString;
  stringstream ss;
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
string printSpinResonanceFactor(long double eccentricity) {
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
void mol_print(fstream& the_file, bool& first, int& count, int max, long double min_weight,
               string molecule, long double weight) {
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
string texture_name(planet_type type) {
  string typeString = "Unknown";
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