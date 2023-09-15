#include "display.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "const.h"
#include "elements.h"
#include "enviro.h"
#include "stargen.h"
#include "utils.h"


using namespace std;

/**
 * @brief Output system to text
 * 
 * @param innermost_planet 
 * @param do_gases 
 * @param seed 
 * @param do_moons 
 */
void text_describe_system(planet* innermost_planet, bool do_gases,
                          long int seed, bool do_moons) {
  // do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  sun the_sun = innermost_planet->getTheSun();
  int counter;

  cout << "Stargen - V" << stargen_revision << "; seed=" << seed << endl;
  cout << "                          SYSTEM  CHARACTERISTICS\n";
  if (!the_sun.getIsCircumbinary()) {
    cout << "Stellar mass: " << toString(the_sun.getMass()) << " solar masses\n";
    cout << "Stellar luminosity: " << toString(the_sun.getLuminosity()) << endl;
  } else {
    cout <<
    "Mass of Primary: " << toString(the_sun.getMass()) << " solar masses\n"
    "Luminosity of Primary: " << toString(the_sun.getLuminosity()) << endl <<
    "Mass of Secondary: " << toString(the_sun.getSecondaryMass()) << " solar masses\n"
    "Luminosity of Primary: " << toString(the_sun.getSecondaryLuminosity()) << endl;
  }
  cout << "Age: " << toString(the_sun.getAge() / 1.0E9)
       << " billion years	("
       << toString((the_sun.getAge() - the_sun.getLife()) / 1.0E9)
       << " billion left on main sequence)\n"
          "Habitable ecosphere radius: "
       << toString(AVE(habitable_zone_distance(the_sun, RECENT_VENUS, 1.0),
                       habitable_zone_distance(the_sun, EARLY_MARS, 1.0)))
       << " AU\n\n";
  cout << "Planets present at:\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    cout << counter << "\t" << toString(the_planet->getA()) << " AU\t"
         << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
         << " EM\t"
         << (((the_planet->getGreenhouseEffect()) &&
              (the_planet->getSurfPressure() > 0.0))
                 ? '+'
             : ((the_planet->getHydrosphere() > .05) &&
                (the_planet->getHydrosphere() < 0.8))
                 ? '*'
             : ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > .1) ? 'o'
                                                                         : '.')
         << endl;
  }
  cout << endl << endl << endl;
  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    cout << "Planet " << counter;
    if (is_gas_planet(the_planet)) {
      cout << "\t*gas giant*";
    }
    cout << endl;
    if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
      cout << "Planet is tidally locked with one face to star.\n";
    } else if (the_planet->getResonantPeriod()) {
      cout << "Planet's rotation is in a resonant spin lock with the star.\n";
    }
    cout << "   Distance from primary star:\t" << toString(the_planet->getA()) << " AU\n"
    "   Eccentricity of orbit:\t" << toString(the_planet->getE()) << endl <<
    "   Length of year:\t\t" << toString(the_planet->getOrbPeriod()) << " days\n"
    "   Length of day:\t\t" << toString(the_planet->getDay()) << " hours\n"
    "   Mass:\t\t\t" << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << " Earth masses\n";
    if (!is_gas_planet(the_planet)) {
      cout << "   Surface gravity:\t\t" << toString(the_planet->getSurfGrav()) << " Earth gees\n"
      "   Surface pressure:\t\t" << toString(the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS) << " Earth atmospheres";
      if (the_planet->getGreenhouseEffect() &&
          the_planet->getSurfPressure() > 0.0) {
        cout << " GREENHOUSE EFFECT";
      }
      cout << endl;
      cout << 
      "   Surface temperature:\t\t" << toString(the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) << " degrees Celcius\n"
      "   Boiling point of water:\t" << toString(the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER) << " degrees Celcius\n"
      "   Hydrosphere percentage:\t" << toString(the_planet->getHydrosphere() * 100.0) << "%\n"
      "   Cloud cover percentage:\t" << toString(the_planet->getCloudCover() * 100.0) << "%\n"
      "   Ice cover percentage:\t" << toString(the_planet->getIceCover() * 100.0) << "%\n";
    }
    cout << "   Equatorial radius:\t\t" << toString(the_planet->getRadius()) << " Km\n"
    "   Density:\t\t\t" << toString(the_planet->getDensity()) << " grams/cc\n"
    "   Escape Velocity:\t\t" << toString(the_planet->getEscVelocity() / CM_PER_KM) << " Km/sec\n"
    "   Molecular weight retained:\t" << toString(the_planet->getMolecWeight()) << " and above\n"
    "   Surface acceleration:\t" << toString(the_planet->getSurfAccel()) << "cm/sec2\n"
    "   Axial tilt:\t\t\t" << toString(the_planet->getAxialTilt()) << " degrees\n"
    "   Planetary albedo:\t\t" << toString(the_planet->getAlbedo()) << endl;
    cout << endl << endl;
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
void csv_describe_system(fstream& the_file, planet* innermost_planet,
                         bool do_gases, long int seed, bool do_moons) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  sun the_sun = innermost_planet->getTheSun();
  int counter;
  stringstream ss;
  string id;
  planet* moon;
  int moons;

  if (!the_sun.getIsCircumbinary()) {
    the_file << "'Seed', 'Star Name', 'Luminosity', 'Mass', 'Temperature', "
                "'Spectral Type', 'Total Time on Main Sequence', 'Age', "
                "'Earth-like Distance'\n";
    the_file << seed << ", '" << the_sun.getName() << "', "
             << toString(the_sun.getLuminosity()) << ", "
             << toString(the_sun.getMass()) << ", "
             << toString(the_sun.getEffTemp()) << ", '" << the_sun.getSpecType()
             << "', " << toString(the_sun.getLife()) << ", "
             << toString(the_sun.getAge()) << ", "
             << toString(the_sun.getREcosphere(1.0)) << "\n";
  } else {
    the_file
        << "'Seed', 'Star Name', 'Luminosity of Primary', 'Mass of Primary', "
           "'Temperature of Primary', 'Spectral Type of Primary', 'Luminosity "
           "of Secondary', 'Mass of Secondary', 'Temperature of Secondary', "
           "'Spectral Type of Secondary', 'Seperation', 'Eccentricity', "
           "'Combined Temperature', 'Primary's Total Time on Main Sequence', "
           "'Age', 'Earth-like Distance'\n";
    the_file << seed << ", '" << the_sun.getName() << "', "
             << toString(the_sun.getLuminosity()) << ", "
             << toString(the_sun.getMass()) << ", "
             << toString(the_sun.getEffTemp()) << ", '" << the_sun.getSpecType()
             << "', " << toString(the_sun.getSecondaryLuminosity()) << ", "
             << toString(the_sun.getSecondaryMass()) << ", "
             << toString(the_sun.getSecondaryEffTemp()) << ", '"
             << the_sun.getSecondarySpecType() << "', "
             << toString(the_sun.getSeperation()) << ", "
             << toString(the_sun.getEccentricity()) << ", "
             << toString(the_sun.getCombinedEffTemp()) << ", "
             << toString(the_sun.getLife()) << ", "
             << toString(the_sun.getAge()) << ", "
             << toString(the_sun.getREcosphere(1.0)) << "\n";
  }
  the_file
      << "'Planet #', 'Distance', 'Eccentricity', 'Inclination', 'Longitude of "
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
  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    csv_row(the_file, the_planet, do_gases, false, id, ss);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != NULL;
           moon = moon->next_planet, moons++) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        csv_row(the_file, moon, do_gases, true, id, ss);
      }
    }
  }
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
void jsonDescribeSystem(fstream& the_file, planet* innermost_planet,
                         bool do_gases, long int seed, bool do_moons) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  sun the_sun = innermost_planet->getTheSun();
  int counter;
  stringstream ss;
  string id;
  planet* moon;
  int moons;

  if (!the_sun.getIsCircumbinary()) {

    nlohmann::json header = {
      {"Body Type", "Binary Star"},
      {"seed", seed},
      {"Star Name",the_sun.getName()},
      {"Luminosity",the_sun.getLuminosity()},
      {"Mass",the_sun.getMass()},
      {"Temperature",the_sun.getEffTemp()},
      {"Spectral Type",the_sun.getSpecType()},
      {"Total Time on Main Sequence",the_sun.getLife()},
      {"Age",the_sun.getAge()},
      {"Earth-like Distance",the_sun.getREcosphere(1.0)}
    };
          the_file << header.dump(4) << std::endl;
  } else {
    
      nlohmann::json header = {
        {"Body Type", "Star"},
        {"seed", seed},
        {"Star Name",the_sun.getName()},
        {"Luminosity of Primary",the_sun.getLuminosity()},
        {"Mass of Primary",the_sun.getMass()},
        {"Temperature of Primary",the_sun.getEffTemp()},
        {"Spectral Type of Primary",the_sun.getSpecType()},
        {"Luminosity of Secondary",the_sun.getSecondaryLuminosity()},
        {"Mass of Secondary",the_sun.getSecondaryMass()},
        {"Temperature of Secondary",the_sun.getSecondaryEffTemp()},
        {"Spectral Type of Secondary",the_sun.getSecondarySpecType()},
        {"Seperation",the_sun.getSeperation()},
        {"Eccentricity",the_sun.getEccentricity()},
        {"Combined Temperature",the_sun.getCombinedEffTemp()},
        {"Total Time on Main Sequence",the_sun.getLife()},
        {"Age",the_sun.getAge()},
        {"Earth-like Distance",the_sun.getREcosphere(1.0)}
      };
    the_file << header.dump(4) <<std::endl;
  }
  nlohmann::json body;

  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    ss << the_sun.getName() << " " << counter;
    id = ss.str();
    ss.str("");
    jsonRow(the_file, the_planet, do_gases, false, id, ss);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != NULL;
           moon = moon->next_planet, moons++) {
        ss << the_sun.getName() << " " << counter << "." << moons;
        id = ss.str();
        ss.str("");
        jsonRow(the_file, moon, do_gases, true, id, ss);
      }
    }
  }
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
void csv_row(fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             string id, stringstream& ss) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  string atmosphere;
  long double ipp;
  int index;
  bool poisonous;

  if (do_gases) {
    ss.str();
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      index = gases.count();
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
         << toString(100.0 * (the_planet->getGas(i).getSurfPressure() /
                              the_planet->getSurfPressure()))
         << " " << toString(the_planet->getGas(i).getSurfPressure()) << " ("
         << toString(ipp) << ")";
      if (poisonous) {
        ss << " poisonous";
      }
      ss << ";";
    }
    atmosphere = ss.str();
    ss.str("");
  }

  the_file << "'" << id << "', ";
  if (!is_moon) {
    the_file << toString(the_planet->getA()) << ", "
             << toString(the_planet->getE());
  } else {
    the_file << toString(the_planet->getMoonA()) << ", "
             << toString(the_planet->getMoonE());
  }
  the_file << ", " << toString(the_planet->getInclination()) << ", "
           << toString(the_planet->getAscendingNode()) << ", "
           << toString(the_planet->getLongitudeOfPericenter()) << ", "
           << toString(the_planet->getMeanLongitude()) << ", "
           << toString(the_planet->getAxialTilt()) << ", "
           << toString(the_planet->getImf()) << ", "
           << toString(the_planet->getRmf()) << ", "
           << toString(the_planet->getCmf()) << ", "
           << toString(the_planet->getMass()) << ", "
           << the_planet->getGasGiant() << ", "
           << toString(the_planet->getDustMass()) << ", "
           << toString(the_planet->getGasMass()) << ", "
           << toString(the_planet->getCoreRadius()) << ", "
           << toString(the_planet->getRadius()) << ", "
           << the_planet->getOrbitZone() << ", "
           << toString(the_planet->getDensity()) << ", "
           << toString(the_planet->getOrbPeriod()) << ", "
           << toString(the_planet->getDay()) << ", "
           << the_planet->getResonantPeriod() << ", "
           << toString(the_planet->getEscVelocity()) << ", "
           << toString(the_planet->getSurfAccel()) << ", "
           << toString(the_planet->getSurfGrav()) << ", "
           << toString(the_planet->getRmsVelocity()) << ", "
           << toString(the_planet->getMolecWeight()) << ", "
           << toString(the_planet->getVolatileGasInventory()) << ", "
           << toString(the_planet->getSurfPressure()) << ", "
           << toString(the_planet->getGreenhouseEffect()) << ", "
           << toString(the_planet->getBoilPoint()) << ", "
           << toString(the_planet->getAlbedo()) << ", "
           << toString(the_planet->getExosphericTemp()) << ", "
           << toString(the_planet->getEstimatedTemp()) << ", "
           << toString(the_planet->getEstimatedTerrTemp()) << ", "
           << toString(the_planet->getSurfTemp()) << ", "
           << toString(the_planet->getGreenhsRise()) << ", "
           << toString(the_planet->getHighTemp()) << ", "
           << toString(the_planet->getLowTemp()) << ", "
           << toString(the_planet->getMaxTemp()) << ", "
           << toString(the_planet->getMinTemp()) << ", "
           << toString(the_planet->getHydrosphere()) << ", "
           << toString(the_planet->getCloudCover()) << ", "
           << toString(the_planet->getIceCover()) << ", '" << atmosphere
           << "', '" << type_string(the_planet) << "', "
           << the_planet->getMinorMoons() << "\n";
}

void jsonRow(fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             string id, stringstream& ss) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  string atmosphere;
  long double ipp;
  int index;
  bool poisonous;

  if (do_gases) {
    ss.str();
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      index = gases.count();
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
         << toString(100.0 * (the_planet->getGas(i).getSurfPressure() /
                              the_planet->getSurfPressure()))
         << " " << toString(the_planet->getGas(i).getSurfPressure()) << " ("
         << toString(ipp) << ")";
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
      body["Distance"] = the_planet->getA();
      body["Eccentricity"] =the_planet->getE();
    }
    else {
      body["Distance au"] = the_planet->getMoonA();
      body["Eccentricity"] =the_planet->getMoonE();
    }
    body["Inclination"] = the_planet->getInclination();
    body["Longitude of the Ascending Node"] = the_planet->getAscendingNode();
    body["Longitude of the Pericenter"] = the_planet->getLongitudeOfPericenter();
    body["Mean Longitude"] = the_planet->getMeanLongitude();
    body["Axial Tilt"] = the_planet->getAxialTilt();
    body["Ice Mass Fraction"] = the_planet->getImf();
    body["Rock Mass Fraction"] = the_planet->getRmf();
    body["Carbon Mass Fraction"] = the_planet->getCmf();
    body["Total Mass"] = the_planet->getMass();
    body["Is Gas Giant"] = the_planet->getGasMass();
    body["Dust Mass"] = the_planet->getDustMass();
    body["Gas Mass"] = the_planet->getMass();
    body["Radius of Core"] = the_planet->getCoreRadius();
    body["Total Radius"] = the_planet->getRadius();
    body["Orbit Zone"] = the_planet->getOrbitZone();
    body["Density"] = the_planet->getDensity();
    body["Orbit Period"] = the_planet->getOrbPeriod();
    body["Rotation Period"] = the_planet->getDay();
    body["Has Spin Orbit Resonance"] = the_planet->getResonantPeriod();
    body["Escape Velocity"] = the_planet->getEscVelocity();
    body["Surface Acceleration"] = the_planet->getSurfAccel();
    body["Surface Gravity"] = the_planet->getSurfGrav();
    body["RMS Velocity"] = the_planet->getRmsVelocity();
    body["Minimum Molecular Weight"] = the_planet->getMolecWeight();
    body["Volatile Gas Inventory"] = the_planet->getVolatileGasInventory();
    body["Get Surface Pressure"] = the_planet->getSurfPressure();
    body["Greenhouse Effect"] = the_planet->getGreenhouseEffect();
    body["Boiling Point"] = the_planet->getBoilPoint();
    body["Albedo"] = the_planet->getAlbedo();
    body["Exospheric Temperature"] = the_planet->getExosphericTemp();
    body["Estimated Temperature"] = the_planet->getEstimatedTemp();
    body["Estimated Terran Temperature"] = the_planet->getEstimatedTerrTemp();
    body["Surface Temperature"] = the_planet->getSurfTemp();
    body["Greenhouse Rise"] = the_planet->getGreenhsRise();
    body["High Temperature"] = the_planet->getHighTemp();
    body["Low Temperature"] = the_planet->getLowTemp();
    body["Maximum Temperature"] = the_planet->getMaxTemp();
    body["Minimum Temperature"] = the_planet->getMinTemp();
    body["Hydrosphere"] = the_planet->getHydrosphere();
    body["Cloud Cover"] = the_planet->getCloudCover();
    body["Ice Cover"] = the_planet->getIceCover();
    body["Atmosphere"] = atmosphere;
    body["Type"] = type_string(the_planet);
    body["Minor Moons"] = the_planet->getMinorMoons();
    the_file << body.dump(4) << std::endl;
}

/**
 * @brief type string
 * 
 * @param the_planet 
 * @return string 
 */
string type_string(planet* the_planet) {
  stringstream ss;
  planet_type type = the_planet->getType();
  string ptype;

  ss.str("");
  if (type == tUnknown) {
    ss << "Unknown";
  } else if (type == tRock) {
    ss << "Rock";
  } else if (type == tVenusian) {
    ss << "Venusian";
  } else if (type == tTerrestrial) {
    ss << "Terrestrial";
  } else if (type == tSubSubGasGiant) {
    ss << cloud_type_string(the_planet) << " Gas Dwarf";
  } else if (type == tSubGasGiant) {
    ss << cloud_type_string(the_planet) << " Neptunian";
  } else if (type == tGasGiant) {
    ss << cloud_type_string(the_planet) << " Jovian";
  } else if (type == tMartian) {
    ss << "Martian";
  } else if (type == tWater) {
    ss << "Water";
  } else if (type == tIce) {
    ss << "Ice";
  } else if (type == tAsteroids) {
    ss << "Asteroids";
  } else if (type == t1Face) {
    ss << "1Face";
  } else if (type == tBrownDwarf) {
    ss << "Brown Dwarf";
  } else if (type == tIron) {
    ss << "Iron";
  } else if (type == tCarbon) {
    ss << "Carbon";
  } else if (type == tOil) {
    ss << "Oil";
  }

  ptype = ss.str();
  return ptype;
}

/**
 * @brief cloud type string
 * 
 * @param the_planet 
 * @return string 
 */
string cloud_type_string(planet* the_planet) {
  long double temp = the_planet->getEstimatedTemp();

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
void create_svg_file(planet* innermost_planet, string path, string file_name,
                     string svg_ext, string prognam, bool do_moons) {
  planet* outermost_planet;
  planet* a_planet;
  fstream output;
  string the_file_spec;
  stringstream ss;

  ss.str("");
  ss << path << file_name << svg_ext;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), ios::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype = 'TEXT';
#endif

  for (outermost_planet = innermost_planet; outermost_planet != NULL;
       outermost_planet = outermost_planet->next_planet) {
    long double max_x = 760.0;
    long double max_y = 120.0;
    long double margin = 20.0;
    long double inner_edge =
        innermost_planet->getA() * (1.0 - innermost_planet->getE());
    long double outer_edge =
        outermost_planet->getA() * (1.0 + outermost_planet->getE());
    long double floor = (int)(log10(inner_edge) - 1.0);
    long double min_log = floor;
    long double ceiling = (int)(log10(outer_edge) + 1.0);
    long double max_log = 0.0;
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

    mult = max_x / (max_log - min_log);
    offset = -mult * (1.0 + min_log);
    em_scale = 5;

    output << "<?xml version='1.0' standalone='no'?>\n";
    output << "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' \n";
    output << "  'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>\n";
    output << "\n";
    output << "<svg width='100%' height='100%' version='1.1'\n";
    output << "     xmlns='http://www.w3.org/2000/svg'\n";
    output << "     viewBox='-" << margin << " -" << margin << " "
           << (max_x + (margin * 2.0)) << " " << (max_y + (margin * 2.0))
           << "' >\n";
    output << "\n";
    output << "<title>" << innermost_planet->getTheSun().getName()
           << "</title>\n";
    output << "<desc>Created by: " << prognam << " - " << stargen_revision
           << "</desc>\n";
    output << "\n";
    output << "<g stroke='black' stroke-width='1'>\n";
    output << "    <line x1='" << ((offset + mult) + (min_log * mult))
           << "' y1='" << (max_y - margin) << "' x2='"
           << ((offset + mult) + (max_log * mult)) << "' y2='"
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
          output << "    <line x1='"
                 << ((offset + mult) + ((x + log10(n)) * mult)) << "' y1='"
                 << (max_y - margin - value) << "' x2='"
                 << ((offset + mult) + ((x + log10(n)) * mult)) << "' y2='"
                 << (max_y - margin + value) << "' />\n";
        }
      }
    }

    output << "</g>\n\n";

    sun the_sun = innermost_planet->getTheSun();
    long double min_r_ecosphere =
        habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
    long double max_r_ecosphere =
        habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

    output << "<line x1='"
           << ((offset + mult) +
               (log10(innermost_planet->getTheSun().getREcosphere(1.0)) * mult))
           << "' y1='" << ((max_y - margin) - 5) << "' x2='"
           << ((offset + mult) +
               (log10(innermost_planet->getTheSun().getREcosphere(1.0)) * mult))
           << "' y2='" << ((max_y - margin) + 5)
           << "' stroke='blue' stroke-width='1' />\n";

    output << "<line x1='"
           << ((offset + mult) + (log10(min_r_ecosphere) * mult)) << "' y1='"
           << (max_y - margin) << "' x2='"
           << ((offset + mult) + (log10(max_r_ecosphere) * mult)) << "' y2='"
           << (max_y - margin)
           << "' stroke='#66c' stroke-width='10' stroke-opacity='0.5' />\n";

    output << "<g font-family='Arial' font-size='10' \n";
    output << "   font-style='normal' font-weight='normal'\n";
    output << "   fill='black' text-anchor='middle'>\n";

    // todo: fix loop counter
    for (int x = floor; x <= ceiling; x++) {
      if (min_log <= x && max_log >= x) {
        output << "    <text x='" << ((offset + mult) + (x * mult))
               << "' y='120'> " << std::pow(10.0, x) << " AU </text>\n";
      }
    }

    output << "\n";

    for (a_planet = innermost_planet; a_planet != NULL;
         a_planet = a_planet->next_planet) {
      long double x = (offset + mult) + (log10(a_planet->getA()) * mult);
      long double r =
          std::pow((a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES), 1.0 / 3.0) *
          em_scale;
      long double x1 =
          (offset + mult) +
          (log10(a_planet->getA() * (1.0 - a_planet->getE())) * mult);
      long double x2 =
          (offset + mult) +
          (log10(a_planet->getA() * (1.0 + a_planet->getE())) * mult);

      output << "    <circle cx='" << x << "' cy='30' r='" << r
             << "' fill='none' stroke='black' stroke-width='1' />\n";
      output << "    <line x1='" << x1 << "' y1='" << ((max_y - margin) - 15.0)
             << "' x2='" << x2 << "' y2='" << ((max_y - margin) - 15.0)
             << "' stroke='black' stroke-width='8' stroke-opacity='0.3'/>\n";
      output << "    <text x='" << x << "' y='" << (max_y - (margin * 2.0))
             << "'>";
      output << (a_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
      output << "</text>\n\n";
    }

    output << "</g>\n";
    output << "</svg>\n";
  }

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
  string the_file_spec;
  stringstream ss;

  ss.str("");
  ss << path << the_filename;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), ios::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype = 'TEXT';
#endif
  if (!output) {
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Refresh file stream
 * 
 * @param output 
 * @param path 
 * @param file_name 
 * @param ext 
 */
void refresh_file_stream(fstream& output, const string &path, const string &file_name,
                         const string &ext) {
  string the_file_spec;
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
void open_html_file(const string &system_name, long seed, const string &path, const string &url_path,
                    const string &file_name, const string &ext, const string &prognam,
                    fstream& output) {
  string the_file_spec;
  bool noname = system_name.empty();
  stringstream ss;

  ss.str("");
  ss << path << file_name << ext;
  the_file_spec = ss.str();

#ifdef macintosh
  _fcreator = 'MSIE';
  _ftype = 'TEXT';
#endif
  output.open(the_file_spec.c_str(), fstream::out);
#ifdef macintosh
  _fcreator = 'R*ch';
  _ftype = 'TEXT';
#endif
  if (!output) {
    exit(EXIT_FAILURE);
  }

  output
      << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n";
  output << "        \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  output << "<html>\n";
  output << "<head>\n";
  output
      << "<meta http-equiv=content-type content='text/html; charset=utf-8'>\n";
  output << "\t<title>System " << seed << (noname ? "" : " - ")
         << (noname ? "" : system_name) << "</title>\n";
  output << "\t<meta name='generator' content='" << prognam << " - "
         << stargen_revision << "'>\n";
  output << "<style type='text/css'>\n";
  output << "<!--\n";
  output << "table {border-color: #ffd;}\n";
  output << "-->\n";
  output << "</style>\n";
  output << "<link rel='icon' type='image/png' href='" << url_path
         << "ref/favicon.png'>\n";
  output << "</head>\n";
  output << "<body bgcolor='" << BGCOLOR << "' text='" << TXCOLOR << "' link='"
         << LINKCOLOR << "' vlink='" << TXCOLOR << "' alink='" << ALINKCOLOR
         << "'>\n\n";
}

/**
 * @brief Close HTML file
 * 
 * @param the_file 
 */
void close_html_file(fstream& the_file) {
  the_file << "<p>\n\n";
  the_file << "<center>\n";
  the_file << "This page was created by omega13a's variant of <a href='"
           << STARGEN_URL << "'>StarGen</a> (" << stargen_revision << ").\n";
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
void print_description(fstream& the_file, const string &opening, planet* the_planet,
                       const string &closing) {
  bool first = true;
  long double earth_masses = the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES;
  long double earth_radii = convert_km_to_eu(the_planet->getRadius());
  stringstream ss;

  the_file << opening;

  if (the_planet->getDay() < 10.0) {
    lprint(the_file, first, "Short Day");
  } else if (the_planet->getDay() > 48.0) {
    lprint(the_file, first, "Long Day");
  }

  if (the_planet->getMoonA() == 0 &&
      calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0 &&
      the_planet->getType() != tAsteroids) {
    lprint(the_file, first, "Dwarf planet");
  }

  if (is_gas_planet(the_planet)) {
    long double temp = the_planet->getEstimatedTemp() - FREEZING_POINT_OF_WATER;

    ss << earth_masses << " Earth Masses";
    lprint(the_file, first, ss.str());
    ss.str("");
    ss << temp << " C";
    lprint(the_file, first, ss.str());
  } else {
    long double rel_temp =
        (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) -
        EARTH_AVERAGE_CELSIUS;
    long double seas = 100.0 * the_planet->getHydrosphere();
    long double clouds = 100.0 * the_planet->getCloudCover();
    long double atmosphere =
        the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS;
    long double ice = 100.0 * the_planet->getIceCover();
    long double gravity = the_planet->getSurfGrav();
    long double iron =
        (1.0 - (the_planet->getRmf() + the_planet->getImf())) * 100.0;

    if (gravity < 0.5) {
      lprint(the_file, first, "Very Low-G");
    }
    else if (gravity < .8) {
      lprint(the_file, first, "Low-G");
    }
    else if (gravity < 1.2) {
      lprint(the_file, first, "Earth-Like-G");
    }
    else if (gravity < 1.4) {
      lprint(the_file, first, "High-G");
    }
    else {
      lprint(the_file, first, "Very-High-G");
    }

    if (rel_temp < -5.0) {
      lprint(the_file, first, "Cold");
    }
    else if (rel_temp < -2.0) {
      lprint(the_file, first, "Cool");
    }
    else if (rel_temp < 3.0) {
      lprint(the_file, first, "Mild");
    }
    else if (rel_temp < 7.5) {
      lprint(the_file, first, "Warm");
    }
    else {
      lprint(the_file, first, "Hot");
    }



    if (the_planet->getType() != tIron && the_planet->getType() != tAsteroids) {
      if (iron > 90.0) {
        lprint(the_file, first, "'Cannonball'");
      } else if (iron > 60) {
        lprint(the_file, first, "Large Iron Core");
      } else if (iron >= 20) {
        lprint(the_file, first, "Medium Iron Core");
      } else if (iron < 1) {
        lprint(the_file, first, "Coreless");
      } else if (iron < 20) {
        lprint(the_file, first, "Small Iron Core");
      }
    } else if (the_planet->getType() == tAsteroids) {
      if (iron > 80) {
        lprint(the_file, first, "Metallic");
      } else if (iron < 20) {
        lprint(the_file, first, "Rocky");
      }
    }

    if (the_planet->getType() != tIce) {
      if (ice > 90.0) {
        lprint(the_file, first, "Ice Covered");
      }
    } else if (ice > 75.0) {
        lprint(the_file, first, "Mostly Ice");
      }
      else if (ice > 50.0) {
        lprint(the_file, first, "Half Ice");
      }
      else if (ice > 25.0) {
        lprint(the_file, first, "Significant Ice");
      }
      else if (ice > 10.0) {
        lprint(the_file, first, "A Bit Icey");
      }

    if (atmosphere < 0.001) {
      lprint(the_file, first, "Airless");
    } else {
      if (the_planet->getType() != tWater) {
        if (seas < 10.0) {
          lprint(the_file, first, "Desert");
        } 
        else if (seas < 15.0) {
          lprint(the_file, first, "Semi-Arid");
        }
        else if (seas < 25.0) {
          lprint(the_file, first, "Arid");
        }
        else if (seas < 50.0) {
          lprint(the_file, first, "Dry");
        }
        else if (seas <= 75.0) {
          lprint(the_file, first, "Moist");
        }
        else if (seas < 85.00) {
          lprint(the_file, first, "Wet");
        }
        else {
          lprint(the_file, first, "Water World");
        }
      }

      if (clouds < 10.0) {
        lprint(the_file, first, "Cloudless");
      } else if (clouds < 40.0) {
        lprint(the_file, first, "Few clouds");
      } else if (clouds > 80.0) {
        lprint(the_file, first, "Cloudy");
      }

      if (the_planet->getMaxTemp() > the_planet->getBoilPoint() && seas > 0.0) {
        lprint(the_file, first, "Boiling ocean");
      }

      if (atmosphere < (MIN_O2_IPP / EARTH_SURF_PRES_IN_MILLIBARS)) {
        lprint(the_file, first, "Unbreathably thin atmosphere");
      } else if (atmosphere < 0.5) {
        lprint(the_file, first, "Thin atmosphere");
      } else if (atmosphere >
                 (MAX_HABITABLE_PRESSURE / EARTH_SURF_PRES_IN_MILLIBARS)) {
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
        if ((the_planet->getGas(i).getSurfPressure() /
             the_planet->getSurfPressure()) > 0.01) {
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

  sun the_sun = the_planet->getTheSun();
  long double min_r_ecosphere = habitable_zone_distance(
      the_sun, RECENT_VENUS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
  long double max_r_ecosphere = habitable_zone_distance(
      the_sun, EARLY_MARS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);

  if (the_planet->getA() < min_r_ecosphere) {
    if (is_habitable_extended(the_planet)) {
      the_file << "Habitable (Extended Definition) ";
    } else if (is_potentialy_habitable_extended(the_planet)) {
      the_file << "Potentially Habitable (Extended Definition) ";
    }
    the_file << "Hot ";
  } else if (the_planet->getA() > max_r_ecosphere) {
    if (is_habitable_extended(the_planet)) {
      the_file << "Habitable (Extended Definition) ";
    } else if (is_potentialy_habitable_extended(the_planet)) {
      the_file << "Potentially Habitable (Extended Definition) ";
    }
    the_file << "Cold ";
  } else {
    if (is_habitable(the_planet)) {
      if (is_habitable_earth_like(the_planet)) {
        the_file << "Habitable (Earth-like Definition) ";
      } else if (is_habitable_conservative(the_planet)) {
        the_file << "Habitable (Conservative Definition) ";
      } else if (is_habitable_optimistic(the_planet)) {
        the_file << "Habitable (Optimistic Definition) ";
      } else if (is_habitable_extended(the_planet)) {
        the_file << "Habitable (Extended Definition) ";
      }
    } else if (is_potentialy_habitable(the_planet)) {
      if (is_potentialy_habitable_earth_like(the_planet)) {
        the_file << "Potentially Habitable (Earth-like Definition) ";
      } else if (is_potentialy_habitable_conservative(the_planet)) {
        the_file << "Potentially Habitable (Conservative Definition) ";
      } else if (is_potentialy_habitable_optimistic(the_planet)) {
        the_file << "Potentially Habitable (Optimistic Definition) ";
      } else if (is_potentialy_habitable_extended(the_planet)) {
        the_file << "Potentially Habitable (Extended Definition) ";
      }
    }
    the_file << "Warm ";
  }

  if (earth_masses <= 0.00001 || earth_radii <= 0.03) {
    the_file << "Asteroidan";
  } else if (earth_masses <= 0.1 || earth_radii <= 0.4) {
    the_file << "Mercurian";
  } else if (earth_masses <= 0.5 || earth_radii <= 0.8) {
    the_file << "Subterran";
  } else if (earth_masses <= 5.0 || earth_radii <= 1.5) {
    the_file << "Terran";
  } else if (earth_masses <= 10.0 || earth_radii <= 2.5) {
    the_file << "Superterran";
  } else if (earth_masses <= 50.0 || earth_radii <= 6.0) {
    the_file << "Neptunian";
  } else {
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
  int count = 0;
  int max = 8;
  bool first = true;

  mol_print(the_file, first, count, max, weight, "H", ATOMIC_HYDROGEN);
  mol_print(the_file, first, count, max, weight, "H<sub><small>2</small></sub>",
            MOL_HYDROGEN);
  mol_print(the_file, first, count, max, weight, "He", HELIUM);
  mol_print(the_file, first, count, max, weight, "N", ATOMIC_NITROGEN);
  mol_print(the_file, first, count, max, weight, "O", ATOMIC_OXYGEN);
  mol_print(the_file, first, count, max, weight,
            "CH<sub><small>4</small></sub>", METHANE);
  mol_print(the_file, first, count, max, weight,
            "NH<sub><small>3</small></sub>", AMMONIA);
  mol_print(the_file, first, count, max, weight,
            "H<sub><small>2</small></sub>O", WATER_VAPOR);
  mol_print(the_file, first, count, max, weight, "Ne", NEON);
  mol_print(the_file, first, count, max, weight, "N<sub><small>2</small></sub>",
            MOL_NITROGEN);
  mol_print(the_file, first, count, max, weight, "CO", CARBON_MONOXIDE);
  mol_print(the_file, first, count, max, weight, "NO", NITRIC_OXIDE);
  mol_print(the_file, first, count, max, weight, "O<sub><small>2</small></sub>",
            MOL_OXYGEN);
  mol_print(the_file, first, count, max, weight,
            "H<sub><small>2</small></sub>S", HYDROGEN_SULPHIDE);
  mol_print(the_file, first, count, max, weight, "Ar", ARGON);
  mol_print(the_file, first, count, max, weight,
            "CO<sub><small>2</small></sub>", CARBON_DIOXIDE);
  mol_print(the_file, first, count, max, weight,
            "N<sub><small>2</small></sub>O", NITROUS_OXIDE);
  mol_print(the_file, first, count, max, weight,
            "NO<sub><small>2</small></sub>", NITROGEN_DIOXIDE);
  mol_print(the_file, first, count, max, weight, "O<sub><small>3</small></sub>",
            OZONE);
  mol_print(the_file, first, count, max, weight,
            "SO<sub><small>2</small></sub>", SULPH_DIOXIDE);
  mol_print(the_file, first, count, max, weight,
            "SO<sub><small>3</small></sub>", SULPH_TRIOXIDE);
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
void html_star_details_helper(fstream& the_file, const string& header,
                              long double mass, long double luminosity,
                              long double temperature, long double age,
                              long double life, const string &spec_type) {
  the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>";
  the_file << "<font size='+1' color='" << TXHEADER << "'><b>" << header
           << "</b></font>";
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
           << toString((life - age) / 1.0E9)
           << " billion left on main sequence)<br></td></tr>";
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
void html_thumbnails(planet* innermost_planet, fstream& the_file,
                     const string &system_name, const string &url_path, const string &system_url,
                     const string &svg_url, const string &file_name, bool details,
                     bool terrestrials, bool int_link, bool do_moons,
                     int graphic_format, bool do_gases) {
  planet* the_planet;
  sun the_sun = innermost_planet->getTheSun();
  int counter;
  int planet_count = 0;
  bool terrestrials_seen = false;
  bool habitable_jovians_seen = false;
  bool potentialy_habitables_seen = false;
  planet* moon;
  int moons;
  int dwarf_planet_count = 0;
  int asteroid_belt_count = 0;
  int moon_count = 0;
  int object_count = 0;
  stringstream ss;
  string planet_id;

  if (!the_file) {
    cout << "We have a serious error!\n";
    exit(EXIT_FAILURE);
  }

  for (the_planet = innermost_planet; the_planet != NULL;
       the_planet = the_planet->next_planet) {
    if (the_planet->getType() == tAsteroids) {
      asteroid_belt_count++;
    } else if (calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0) {
      dwarf_planet_count++;
    } else {
      planet_count++;
    }
    object_count++;
    if (do_moons) {
      for (moon = the_planet->first_moon; moon != NULL;
           moon = moon->next_planet) {
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

  the_file << "<tr><td colspan=" << (object_count + 2) << " bgcolor='"
           << BGHEADER << "' align=center>\n";
  the_file << "\t<font size='+1'  color='" << TXHEADER << "'><b>"
           << planet_count << " Planets,</b> <b>" << dwarf_planet_count
           << " Dwarf Planets,</b> <b>" << asteroid_belt_count
           << " Asteroid Belts</b>, <b>" << moon_count << " Moons</b></font>\n";
  the_file << "\t(<font size='-1' color='" << TXHEADER
           << "'>size proportional to Sqrt(Radius)</font>)\n";
  the_file << "</td></tr>\n";
  the_file << "<tr valign=middle bgcolor='" << BGSPACE << "'>\n";
  the_file << "<td bgcolor='" << BGSPACE << "'><img alt='Sun' src='" << url_path
           << "ref/Sun.gif' ";
  the_file << "width=15 height=63 border=0></td>\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    ss.str("");
    int ppixels =
        ((int)(sqrt(convert_km_to_eu(the_planet->getRadius())) * 30.0)) + 1;
    string ptype = type_string(the_planet);
    string info;

    if ((the_planet->getSurfPressure() > 0 || the_planet->getGasGiant()) &&
        the_planet->getNumGases() == 0 && do_gases) {
      ss.str("");
      ss << counter;
      planet_id = ss.str();
      calculate_gases(the_sun, the_planet, planet_id);
    }

    if (the_planet->getType() == tAsteroids) {
      ppixels =
          (int)(25.0 +
                (5.0 * log((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) /
                           ASTEROID_MASS_LIMIT)));
    }

    if (ppixels < 3) {
      ppixels = 3;
    }

    ss.str("");
    ss << ptype << ": " << (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
       << " EM";
    if (is_gas_planet(the_planet)) {
      ss << " (c. " << the_planet->getEstimatedTemp() << "&deg;)";
    } else if (the_planet->getType() == tUnknown) {
      ss << ", " << (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES)
         << " EM from gas ("
         << (100.0 * (the_planet->getGasMass() / the_planet->getMass()))
         << "%)";
    } else {
      ss << " " << the_planet->getSurfGrav() << " g, "
         << (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) << "&deg;";
    }
    ss << " Zone = " << the_planet->getOrbitZone();
    info = ss.str();

    the_file << "\t<td bgcolor='" << BGSPACE << "' align=center><a href='"
             << (int_link ? "" : system_url) << "#" << counter << "' title='#"
             << counter << " - " << info << "'>";
    the_file << "<img alt='" << ptype << "' src='" << url_path << "ref/"
             << image_type_string(the_planet) << "Planet.gif' width=" << ppixels
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
      for (moon = the_planet->first_moon, moons = 1; do_moons && moon != NULL;
           moon = moon->next_planet, moons++) {
        ss.str("");
        string mtype = type_string(moon);
        int mpixels =
            ((int)(sqrt(convert_km_to_eu(moon->getRadius())) * 30.)) + 1;

        if ((the_planet->getSurfPressure() > 0 || the_planet->getGasGiant()) &&
            the_planet->getNumGases() == 0 && do_gases) {
          ss.str("");
          ss << counter;
          planet_id = ss.str();
          calculate_gases(the_sun, the_planet, planet_id);
        }

        ss.str("");
        ss << mtype << ": " << (moon->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << " EM";
        if (is_gas_planet(moon)) {
          ss << " (c. " << moon->getEstimatedTemp() << "&deg;)";
        } else if (moon->getType() == tUnknown) {
          ss << ", " << (moon->getGasMass() * SUN_MASS_IN_EARTH_MASSES)
             << " EM from gas ("
             << (100.0 * (moon->getGasMass() / moon->getMass())) << "%)";
        } else {
          ss << " " << moon->getSurfGrav() << " g, "
             << (moon->getSurfTemp() - FREEZING_POINT_OF_WATER) << "&deg;";
        }
        ss << " Zone = " << moon->getOrbitZone();
        info = ss.str();

        the_file << "\n\t\t<br><a href='" << (int_link ? "" : system_url) << "#"
                 << counter << "." << moons << "' title='#" << counter << "."
                 << moons << " - " << info << "'>";
        the_file << "<img alt='" << mtype << "' src='" << url_path << "ref/"
                 << image_type_string(moon) << "Planet.gif' width=" << mpixels
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
  the_file << "<a href='" << url_path << "ref/Key.html'><font size='-3' color='"
           << TXSPACE << "'>See<br>Key</font></a></td>\n";
  the_file << "</tr></table>\n";

  if (graphic_format == gfSVG) {
    the_file << "</object>\n";
  }

  the_file << "</td></tr>\n";

  // Table of data on the star system

  if (terrestrials && (terrestrials_seen || habitable_jovians_seen ||
                       potentialy_habitables_seen)) {
    the_file << "<tr><td colspan=2><table width='100%'>";

    for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
         the_planet = the_planet->next_planet, counter++) {
      if (is_habitable_jovian(the_planet) || is_terrestrial(the_planet) ||
          is_potentialy_habitable(the_planet)) {
        the_file << "\n\t<tr><td align=right width='5%'>";
        the_file << "<a href='" << (int_link ? "" : system_url) << "#"
                 << counter << "'><small>#" << counter << "</small></a>";
        the_file << "</td>\n\t\t<td><small>" << type_string(the_planet)
                 << ": </small>";

        print_description(the_file, "", the_planet, "");

        the_file << "</td></tr>";
      }

      if (do_moons) {
        for (moon = the_planet->first_moon, moons = 1; moon != NULL;
             moon = moon->next_planet, moons++) {
          if (is_habitable_jovian(moon) || is_terrestrial(moon) ||
              is_potentialy_habitable(moon)) {
            the_file << "\n\t<tr><td align=right width='5%'>";
            the_file << "<a href='" << (int_link ? "" : system_url) << "#"
                     << counter << "." << moons << "'><small>#" << counter
                     << "." << moons << "</small></a>";
            the_file << "</td>\n\t\t<td><small>" << type_string(moon)
                     << ": </small>";

            print_description(the_file, "", moon, "");

            the_file << "</td></tr>";
          }
        }
      }
    }

    the_file << "</table></td></tr>\n";
  }

  if (details) {
    long double min_r_ecosphere =
        habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
    long double max_r_ecosphere =
        habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

    if (!the_sun.getIsCircumbinary()) {
      html_star_details_helper(the_file, "Stellar Characteristics",
                               the_sun.getMass(), the_sun.getLuminosity(),
                               the_sun.getEffTemp(), the_sun.getAge(),
                               the_sun.getLife(), the_sun.getSpecType());
    } else {
      html_star_details_helper(the_file, "Stellar Characteristics of Primary",
                               the_sun.getMass(), the_sun.getLuminosity(),
                               the_sun.getEffTemp(), the_sun.getAge(),
                               the_sun.getLife(), the_sun.getSpecType());
      html_star_details_helper(
          the_file, "Stellar Characteristics of Secondary",
          the_sun.getSecondaryMass(), the_sun.getSecondaryLuminosity(),
          the_sun.getSecondaryEffTemp(), the_sun.getAge(),
          the_sun.getSecondaryLife(), the_sun.getSecondarySpecType());
      the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER
               << "' align=center>";
      the_file << "<font size='+1' color='" << TXHEADER
               << "'><b>Orbit Characteristics</b></font>";
      the_file << "</td></tr>\n";
      the_file << "<tr><td>Seperation</td>\n";
      the_file << "\t<td>" << toString(the_sun.getSeperation())
               << " AU</td></tr>";
      the_file << "<tr><td>Eccentricity</td>\n";
      the_file << "\t<td>" << toString(the_sun.getEccentricity())
               << " AU</td></tr>";
    }
    the_file << "<tr><td colspan=2 bgcolor='" << BGHEADER << "' align=center>";
    the_file << "<font size='+1' color='" << TXHEADER
             << "'><b>Habitable Zone Characteristics</b></font>";
    the_file << "</td></tr>\n";
    if (the_sun.getIsCircumbinary()) {
      the_file << "<tr><td>Minimum Stable Orbit</td>\n";
      the_file << "\t<td>" << toString(the_sun.getMinStableDistance())
               << " AU</td></tr>\n";
    }
    the_file << "<tr><td>Range</td>\n";
    the_file << "\t<td>" << toString(min_r_ecosphere) << " AU to "
             << toString(max_r_ecosphere) << " AU</td></tr>\n";
    the_file << "<tr><td>Radius</td>\n";
    the_file << "\t<td>" << toString(AVE(min_r_ecosphere, max_r_ecosphere))
             << " AU</td></tr>\n";
    the_file << "<tr><td>Recent Venus Distance</td>\n";
    the_file << "\t<td>" << toString(min_r_ecosphere) << " AU</td></tr>\n";
    the_file << "<tr><td>Runaway Greenhouse Distance</td>\n";
    the_file << "\t<td>"
             << toString(
                    habitable_zone_distance(the_sun, RUNAWAY_GREENHOUSE, 1.0))
             << " AU</td></tr>\n";
    the_file << "<tr><td>Moist Greenhouse Distance</td>\n";
    the_file << "\t<td>"
             << toString(
                    habitable_zone_distance(the_sun, MOIST_GREENHOUSE, 1.0))
             << " AU</td></tr>\n";
    the_file << "<tr><td>Earth-like Distance</td>\n";
    the_file << "\t<td>"
             << toString(habitable_zone_distance(the_sun, EARTH_LIKE, 1.0))
             << " AU</td></tr>\n";
    the_file << "<tr><td>First CO2 Condensation Limit</td>\n";
    the_file << "\t<td>"
             << toString(habitable_zone_distance(
                    the_sun, FIRST_CO2_CONDENSATION_LIMIT, 1.0))
             << " AU</td></tr>\n";
    the_file << "<tr><td>Maximum Greenhouse Distance</td>\n";
    the_file << "\t<td>"
             << toString(
                    habitable_zone_distance(the_sun, MAXIMUM_GREENHOUSE, 1.0))
             << " AU</td></tr>\n";
    the_file << "<tr><td>Early Mars Distance</td>\n";
    the_file << "\t<td>" << toString(max_r_ecosphere) << " AU</td></tr>\n";
    the_file << "<tr><td>Two AU Cloud Limit</td>\n";
    the_file << "\t<td>"
             << toString(
                    habitable_zone_distance(the_sun, TWO_AU_CLOUD_LIMIT, 1.0))
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
void html_thumbnail_totals(fstream& the_file) {
  the_file << "\n<p>\n\n";
  the_file
      << "<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='"
      << BGTABLE << "'>\n";
  the_file << "<tr><th colspan=2 bgcolor='" << BGHEADER << "' align=center>\n";
  the_file << "\t<font size='+1' color='" << TXHEADER
           << "'><b>Summary</b></font></th></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tEarthlike worlds\n</td><td align=center>\n\t"
           << total_earthlike << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Habitable worlds (Earth-like Definition)\n</td><td "
              "align=center>\n\t"
           << total_habitable_earthlike << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Habitable worlds (Conservative Definition)\n</td><td "
              "align=center>\n\t"
           << total_habitable_conservative << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Habitable worlds (Optimistic Definition)\n</td><td "
              "align=center>\n\t"
           << total_habitable_optimistic << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Habitable worlds (Extended Definition)\n</td><td "
              "align=center>\n\t"
           << total_habitable << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Potentially Habitable worlds (Earth-like "
              "Definition)\n</td><td align=center>\n\t"
           << total_potentially_habitable_earthlike << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Potentially Habitable worlds (Conservative "
              "Definition)\n</td><td align=center>\n\t"
           << total_potentially_habitable_conservative << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Potentially Habitable worlds (Optimistic "
              "Definition)\n</td><td align=center>\n\t"
           << total_potentially_habitable_optimistic << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal Potentially Habitable worlds (Extended "
              "Definition)\n</td><td align=center>\n\t"
           << total_potentially_habitable << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTotal worlds\n</td><td align=center>\n\t" << total_worlds
           << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tHabitable mass range\n</td><td align=center>\n\t"
           << (min_breathable_mass * SUN_MASS_IN_EARTH_MASSES) << " EM -  "
           << (max_breathable_mass * SUN_MASS_IN_EARTH_MASSES)
           << " EM\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tPotentially Habitable mass range\n</td><td align=center>\n\t"
           << (min_potential_mass * SUN_MASS_IN_EARTH_MASSES) << " EM -  "
           << (max_potential_mass * SUN_MASS_IN_EARTH_MASSES)
           << " EM\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tHabitable g range\n</td><td align=center>\n\t"
           << min_breathable_g << " -  " << max_breathable_g
           << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tPotentially Habitable g range\n</td><td align=center>\n\t"
           << min_potential_g << " -  " << max_potential_g << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTerrestrial g range\n</td><td align=center>\n\t"
           << min_breathable_terrestrial_g << " -  "
           << max_breathable_terrestrial_g << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tHabitable pressure range\n</td><td align=center>\n\t"
           << min_breathable_p << " -  " << max_breathable_p
           << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file
      << "\tPotentially Habitable pressure range\n</td><td align=center>\n\t"
      << min_potential_p << " -  " << max_potential_p << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tHabitable temp range\n</td><td align=center>\n\t"
           << (min_breathable_temp - EARTH_AVERAGE_KELVIN) << " C -  "
           << (max_breathable_temp - EARTH_AVERAGE_KELVIN)
           << " C\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tPotentially Habitable temp range\n</td><td align=center>\n\t"
           << (min_potential_temp - EARTH_AVERAGE_KELVIN) << " C -  "
           << (max_potential_temp - EARTH_AVERAGE_KELVIN) << " C\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tHabitable illumination range\n</td><td align=center>\n\t"
           << min_breathable_l << " -  " << max_breathable_l
           << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tPotentially Habitable illumination range\n</td><td "
              "align=center>\n\t"
           << min_potential_l << " -  " << max_potential_l << "\n</td></tr>\n";
  the_file << "<tr bgcolor='" << BGTABLE << "'><td align=right>\n";
  the_file << "\tTerrestrial illumination range\n</td><td align=center>\n\t"
           << min_breathable_terrestrial_l << " - "
           << max_breathable_terrestrial_l << "\n</td></tr>\n";
  the_file << "</table>\n\n";
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
void html_describe_planet(planet* the_planet, int counter, int moons,
                         bool do_gases, const string &url_path, fstream& the_file) {
  string planet_id;
  string typeString = type_string(the_planet);
  stringstream ss;

  do_gases = (flags_arg_clone & fDoGases) != 0;

  ss << counter;
  if (moons > 0) {
    ss << "." << moons;
  }
  planet_id = ss.str();

  the_file << "<p>\n<a name='" << planet_id
           << "'></a><table border=3 cellspacing=2 cellpadding=2 align=center ";
  the_file << "bgcolor='" << BGTABLE << "' width='" << ((moons == 0) ? 95 : 90)
           << "%'>\n";
  the_file << "<colgroup span=1 align=left valign=middle>\n";
  the_file << "<colgroup span=2 align=left valign=middle>\n";
  the_file << "<tr><th colspan=3 bgcolor='" << BGHEADER << "' align=center>\n";
  the_file << "<font size='+2' color='" << TXHEADER << "'>"
           << ((moons == 0) ? "Planet" : "Moon") << " #" << planet_id
           << " Statistics</font></th></tr>\n";

  the_file << "<tr><th>Planet type</th>";
  the_file << "<td colspan=2><img alt='" << typeString << "' src='" << url_path
           << "ref/" << image_type_string(the_planet)
           << ".gif' align=middle width=20 height=20>" << typeString;

  if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0)) {
    the_file << "<br>Tidally Locked 1 Face\n";
  } else if (the_planet->getResonantPeriod()) {
    the_file << "<br>Resonant Spin Locked (";
    if (moons == 0) {
      the_file << printSpinResonanceFactor(the_planet->getE());
    } else {
      the_file << printSpinResonanceFactor(the_planet->getMoonE());
    }
    the_file << " Resonance)\n";
  }

  print_description(the_file, "<br>", the_planet, "");

  the_file << "</td></tr>\n";

  if (moons == 0) {
    the_file << "<tr><th>Distance from primary star</th><td>"
             << toString(the_planet->getA() * KM_PER_AU) << " KM</td>";
    the_file << "<td>" << toString(the_planet->getA()) << " AU</td></tr>\n";
  } else {
    the_file << "<tr><th>Distance from primary planet</th><td>"
             << toString(the_planet->getMoonA() * KM_PER_AU) << " KM</td>";
    the_file << "<td>" << toString(the_planet->getMoonA()) << " AU</td></tr>\n";
  }

  the_file << "<tr><th>Eccentricity of orbit</th><td>";
  if (moons == 0) {
    the_file << toString(the_planet->getE());
  } else {
    the_file << toString(the_planet->getMoonE());
  }
  the_file << "<td></td></tr>\n";

  the_file << "<tr><th>Inclination</th><td>"
           << toString(the_planet->getInclination())
           << "&deg;</td><td></td></tr>\n";
  the_file << "<tr><th>Longitude of the Ascending Node</th><td>"
           << toString(the_planet->getAscendingNode())
           << "&deg;</td><td></td></tr>\n";
  the_file << "<tr><th>Longitude of the Pericenter</th><td>"
           << toString(the_planet->getLongitudeOfPericenter())
           << "&deg;</td><td></td></tr>\n";
  the_file << "<tr><th>Mean Longitude</th><td>"
           << toString(the_planet->getMeanLongitude())
           << "&deg;</td><td></td></tr>\n";

  the_file << "<tr><th>Habitable Zone Distance (HZD)</th><td>"
           << toString(the_planet->getHzd()) << "</td><td></td></tr>\n";

  the_file << "<tr><th>Mass</th><td> "
           << toString((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES *
                        EARTH_MASS_IN_GRAMS) /
                       1000.0)
           << " Kg</td>";
  the_file << "<td>";

  if (((the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006 &&
       (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006 &&
       !is_gas_planet(the_planet)) ||
      (the_planet->getType() == tSubSubGasGiant &&
       (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006)) {
    int core_size =
        (int)((50.0 * the_planet->getCoreRadius()) / the_planet->getRadius());

    if (core_size <= 49) {
      the_file << "<table border=0 cellspacing=0 cellpadding=0 "
                  "bgcolor='#000000' background='"
               << url_path << "ref/Atmosphere.gif' align=right>";
      the_file << "<tr align=right valign=bottom background='" << url_path
               << "ref/Atmosphere.gif'>";
      the_file << "<td width=50 height=50 align=right valign=bottom "
                  "bgcolor='#000000' background='"
               << url_path << "ref/Atmosphere.gif'>";
      the_file << "<img src='" << url_path
               << "ref/Core.gif' alt='' width=" << core_size
               << " height=" << core_size << ">";
      the_file << "</td></tr></table>";
      the_file << "\n";
    }
  }

  the_file << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
           << " Earth masses";
  if ((the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006 &&
      (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006) {
    the_file << " ("
             << toString((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) /
                         JUPITER_MASS)
             << " Jupiter masses)";
  }
  the_file << "<br>";

  if ((the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006 &&
      (the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) > 0.0006) {
    the_file << toString(the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES)
             << " Earth masses dust ("
             << toString(
                    (the_planet->getDustMass() * SUN_MASS_IN_EARTH_MASSES) /
                    JUPITER_MASS)
             << " Jupiter masses)<br />";
    the_file << toString(the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES)
             << " Earth masses gas ("
             << toString((the_planet->getGasMass() * SUN_MASS_IN_EARTH_MASSES) /
                         JUPITER_MASS)
             << " Jupiter masses)<br />";
  }

  if (is_gas_planet(the_planet)) {
    the_file << "Dust mass composition:<br />\n";
  }
  the_file << toString(the_planet->getImf() * 100.0) << "% ice<br>"
           << toString(the_planet->getRmf() * 100.0) << "% rock ("
           << toString(the_planet->getCmf() * 100.0) << "% carbon, "
           << toString((1.0 - the_planet->getCmf()) * 100.0)
           << "% silicates)<br>"
           << toString((1.0 - (the_planet->getImf() + the_planet->getRmf())) *
                       100.0)
           << "% iron";

  the_file << "</td></tr>\n";

  the_file << "<tr><th>Habitable Zone Composition (HZC)</th><td>"
           << toString(the_planet->getHzc()) << "</td><td></td></tr>\n";

  if (!is_gas_planet(the_planet)) {
    long double celsius = (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER);

    the_file << "<tr><th>Surface gravity</th>";
    the_file << "<td>" << toString(the_planet->getSurfAccel())
             << " cm/sec<sup>2</sup></td>";
    the_file << "<td>" << toString(the_planet->getSurfGrav())
             << " Earth gees</td></tr>\n";

    the_file << "<tr><th>Surface pressure</th><td>"
             << toString(the_planet->getSurfPressure()) << " millibars";
    the_file << "</td><td>"
             << toString(the_planet->getSurfPressure() /
                         EARTH_SURF_PRES_IN_MILLIBARS)
             << " Earth atmospheres</td></tr>\n";

    the_file << "<tr><th>Surface temperature</th>";
    the_file << "<td>" << toString(celsius) << "&deg; Celcius";
    the_file << "<br>" << toString(32.0 + (celsius * 1.8))
             << "&deg; Fahrenheit</td>";
    the_file << "<td rowspan=2 valign=top>"
             << toString(celsius - EARTH_AVERAGE_CELSIUS)
             << "&deg; C Earth temperature";
    the_file << "<br>" << toString((celsius - EARTH_AVERAGE_CELSIUS) * 1.8)
             << "&deg; F Earth temperature";

    if (the_planet->getGreenhsRise() > 0.1) {
      the_file << "<br>" << toString(the_planet->getGreenhsRise())
               << "&deg; C greenhouse effect";

      if (the_planet->getGreenhouseEffect() &&
          the_planet->getSurfPressure() > 0.0) {
        the_file << " (GH)";
      }
    }

    the_file << "</td></tr>\n";

    the_file << "<tr><th>Normal temperature range</th>";
    the_file << "<td><center><table>\n";
    if (fabs(the_planet->getHighTemp() - the_planet->getMaxTemp()) > 10.0 ||
        fabs(the_planet->getLowTemp() - the_planet->getMinTemp()) > 10.0) {
      the_file << "\t<tr><th>Night</th><th></th><th>Day</th></tr>\n";
      the_file << "\t<tr><td>"
               << toString(the_planet->getLowTemp() - FREEZING_POINT_OF_WATER)
               << "&deg; C<br>"
               << toString(32.0 + (1.8 * (the_planet->getLowTemp() -
                                          FREEZING_POINT_OF_WATER)))
               << "&deg; F</td>";
      the_file << "<td></td>\n";
      the_file << "<td>"
               << toString(the_planet->getHighTemp() - FREEZING_POINT_OF_WATER)
               << "&deg; C<br>"
               << toString(32.0 + (1.8 * (the_planet->getHighTemp() -
                                          FREEZING_POINT_OF_WATER)))
               << "&deg; F</td>";
    }
    the_file << "\t<tr><th>Min</th><th></th><th>Max</th></tr>\n";
    the_file << "\t<tr><td>"
             << toString(the_planet->getMinTemp() - FREEZING_POINT_OF_WATER)
             << "&deg; C<br>"
             << toString(32.0 + (1.8 * (the_planet->getMinTemp() -
                                        FREEZING_POINT_OF_WATER)))
             << "&deg; F</td>";
    the_file << "<td> - </td>";
    the_file << "<td>"
             << toString(the_planet->getMaxTemp() - FREEZING_POINT_OF_WATER)
             << "&deg; C<br>"
             << toString(32.0 + (1.8 * (the_planet->getMaxTemp() -
                                        FREEZING_POINT_OF_WATER)))
             << "&deg; F</td>";
    the_file << "</table></center></td></tr>\n";

    the_file << "<tr><th>Standard Primary Habitability (SPH)</th><td>"
             << toString(the_planet->getSph()) << "</td><td></td></tr>\n";
  } else {
    the_file << "<tr><th>Estimated Temperature</th><td>"
             << toString(the_planet->getEstimatedTemp()) << "&deg; K</td><td>"
             << toString(the_planet->getEstimatedTemp() - EARTH_AVERAGE_KELVIN)
             << "&deg; C Earth temperature</td></tr>";
  }

  the_file << "<tr><th>Average radius</th><td>"
           << toString(the_planet->getRadius()) << " Km</td><td>"
           << toString(convert_km_to_eu(the_planet->getRadius()))
           << " Earth radii";
  if (is_gas_planet(the_planet)) {
    the_file << "<br />"
             << toString(the_planet->getRadius() / KM_JUPITER_RADIUS)
             << " Jupiter radii";
  }
  the_file << "</td></tr>\n";
  if (the_planet->getGasMass()) {
    the_file << "<tr><th>Radius of Core</th><td>"
             << toString(the_planet->getCoreRadius()) << " km</td><td>"
             << toString(convert_km_to_eu(the_planet->getCoreRadius()))
             << " Earth radii</td></tr>";
  }
  the_file << "<tr><th>Oblateness</th><td>"
           << toString(the_planet->getOblateness())
           << "</td><td><table><tr><th>Equatorial Radius</th><th>Polar "
              "Radius</th></tr><tr><td>"
           << toString(the_planet->getEquatrorialRadius()) << " Km<br />"
           << toString(convert_km_to_eu(the_planet->getEquatrorialRadius()))
           << " Earth radii<br />"
           << toString(the_planet->getEquatrorialRadius() / KM_JUPITER_RADIUS)
           << " Jupiter radii</td><td>"
           << toString(the_planet->getPolarRadius()) << " Km<br />"
           << toString(convert_km_to_eu(the_planet->getPolarRadius()))
           << " Earth radii<br />"
           << toString(the_planet->getPolarRadius() / KM_JUPITER_RADIUS)
           << " Jupiter radii</td></tr></table>";
  the_file << "</td></tr>\n";

  the_file << "<tr><th>Density</th>";
  the_file << "<td>" << toString(the_planet->getDensity()) << " grams/cc</td>";
  the_file << "<td>" << toString(the_planet->getDensity() / EARTH_DENSITY)
           << " Earth densities</td></tr>\n";

  the_file << "<tr><th>Escape Velocity</th><td>"
           << toString(the_planet->getEscVelocity() / CM_PER_KM)
           << " Km/sec</td>";
  the_file << "<td></td></tr>\n";

  the_file << "<tr><th>Molecular weight retained</th><td>"
           << toString(the_planet->getMolecWeight()) << " and above</td>";
  the_file << "<td>";

  list_molecules(the_file, the_planet->getMolecWeight());

  if (do_gases && the_planet->getNumGases() > 0) {
    the_file << "\n<table border=0 cellspacing=0 cellpadding=0>\n";
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      int index = gases.count();
      bool poisonous = false;
      long double ipp;

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
      if ((the_planet->getGas(i).getSurfPressure() /
           the_planet->getSurfPressure()) > 0.001 ||
          poisonous) {
        the_file << "<tr><th align=left>" << gases[index].getName()
                 << "&nbsp;</th>";
        the_file << "<td align=right>"
                 << (100.0 * (the_planet->getGas(i).getSurfPressure() /
                              the_planet->getSurfPressure()))
                 << "%&nbsp;</td>";
        the_file << "<td align=right>"
                 << the_planet->getGas(i).getSurfPressure() << " mb&nbsp;</td>";
        the_file << "<td align=right>(ipp:" << ipp << ")</td>";
        the_file << "<td align=right>&nbsp;" << (poisonous ? "poisonous" : "")
                 << "</td></tr>\n";
      }
    }
    the_file << "</table>\n";
  }
  the_file << "</td></tr>\n";

  the_file << "<tr><th>Habitable Zone Atmosphere (HZA)</th><td>"
           << toString(the_planet->getHza()) << "</td><td></td></tr>\n";

  the_file << "<tr><th>Axial tilt</th><td>"
           << toString(the_planet->getAxialTilt()) << "&deg;</td>";
  the_file << "<td></td></tr>\n";

  the_file << "<tr><th>Planetary albedo</th><td>"
           << toString(the_planet->getAlbedo()) << "</td>";
  the_file << "<td></td></tr>\n";

  the_file << "<tr><th>Exospheric Temperature</th><td>"
           << toString(the_planet->getExosphericTemp()) << "&deg; K</td>";
  the_file << "<td>"
           << toString(the_planet->getExosphericTemp() - EARTH_EXOSPHERE_TEMP)
           << "&deg; C Earth temperature</td></tr>\n";

  the_file << "<tr><th>Length of year</th>";
  the_file << "<td>" << toString(the_planet->getOrbPeriod())
           << " Earth days</td>";
  the_file << "<td>"
           << toString((the_planet->getOrbPeriod() * 24.0) /
                       the_planet->getDay())
           << " Local days";
  if (the_planet->getOrbPeriod() > DAYS_IN_A_YEAR) {
    the_file << "<br>" << toString(the_planet->getOrbPeriod() / DAYS_IN_A_YEAR)
             << " Earth years";
  }
  the_file << "</td></tr>\n";

  the_file << "<tr><th>Length of day</th><td>" << toString(the_planet->getDay())
           << " hours</td>";
  the_file << "<td></td></tr>\n";

  if (!is_gas_planet(the_planet)) {
    the_file << "<tr><th>Boiling point of water</th>";
    the_file << "<td>"
             << toString(the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER)
             << "&deg; Celcius";
    the_file << "<br>"
             << toString(32.0 + ((the_planet->getBoilPoint() -
                                  FREEZING_POINT_OF_WATER) *
                                 1.8))
             << "&deg; Fahrenheit</td>";
    the_file << "<td></td></tr>\n";

    the_file << "<tr><th>Hydrosphere</th><td>"
             << toString(the_planet->getHydrosphere() * 100.0) << "%</td>";
    the_file << "<td></td></tr>\n";

    the_file << "<tr><th>Cloud cover</th><td>"
             << toString(the_planet->getCloudCover() * 100.0) << "%</td>";
    the_file << "<td></td></tr>\n";

    the_file << "<tr><th>Ice cover</th><td>"
             << toString(the_planet->getIceCover() * 100.0) << "%</td>";
    the_file << "<td></td></tr>\n";
  }

  long double esir =
      calcEsiHelper(convert_km_to_eu(the_planet->getRadius()), 1.0, 0.57);
  long double esid =
      calcEsiHelper(the_planet->getDensity() / EARTH_DENSITY, 1.0, 1.07);
  long double esiv = calcEsiHelper(
      (the_planet->getEscVelocity() / CM_PER_KM) / 11.186, 1.0, 0.70);
  long double esit =
      calcEsiHelper(the_planet->getSurfTemp(), EARTH_AVERAGE_KELVIN, 5.58);

  the_file << "<tr><th>Earth Similarity Index (ESI)</th><td>"
           << toString(the_planet->getEsi())
           << "</td><td><!--esir: " << toString(esir)
           << "<br />esid: " << toString(esid)
           << "<br />esiv: " << toString(esiv)
           << "<br />esit: " << toString(esit) << "--></td></tr>\n";

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
void html_describe_system(planet* innermost_planet, bool do_gases,
                          bool do_moons, const string &url_path, fstream& the_file) {
  do_gases = (flags_arg_clone & fDoGases) != 0;
  planet* the_planet;
  int counter;
  planet* moon;
  int moons;
  string typeString;

  the_file
      << "\n<table border=3 cellspacing=2 cellpadding=2 align=center bgcolor='"
      << BGTABLE << "' width='90%'>\n";
  the_file << "<tr><th colspan=7 bgcolor='" << BGHEADER << "' align=center>\n";
  the_file << "<font size='+2' color='" << TXHEADER
           << "'>Planetary Overview</font></th></tr>\n\n";
  the_file << "<tr align=center>\n";
  the_file << "\t<th>#</th><th "
              "colspan=3>Type</th><th>Dist.</th><th>Mass</th><th>Radius</th>\n";
  the_file << "</tr>\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    typeString = type_string(the_planet);

    the_file << "<tr align=right>\n\t<td><a href='#" << counter << "'>"
             << counter << "</a></td>\n\t<td align=center><img alt='"
             << typeString << "' src='" << url_path << "ref/"
             << image_type_string(the_planet) << ".gif'></td>\n\t<td colspan=2>"
             << typeString << "</td>\n\t<td>" << toString(the_planet->getA(), 4)
             << "  AU</td>\n\t<td>"
             << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES, 4)
             << " EM</td>\n\t<td>"
             << toString(convert_km_to_eu(the_planet->getRadius()), 4) << " ER";
    if (is_gas_planet(the_planet)) {
      the_file << " ("
               << toString(the_planet->getRadius() / KM_JUPITER_RADIUS, 4)
               << " JR)";
    }
    the_file << "</td></tr>\n";

    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != NULL;
           moon = moon->next_planet, moons++) {
        typeString = type_string(moon);

        the_file << "<tr align=right>\n";
        the_file << "\t<td></td>\n";
        the_file << "\t<td align=center><a href='#" << counter << "." << moons
                 << "'>" << counter << "." << moons << "</a></td>\n";
        the_file << "\t<td align=center><img alt='" << typeString << "' src='"
                 << url_path << "ref/" << image_type_string(moon)
                 << ".gif'></td>\n";
        the_file << "\t<td>" << typeString << "</td>\n";
        the_file << "\t<td>" << toString(moon->getMoonA() * KM_PER_AU, 4)
                 << " km</td>\n";
        the_file << "\t<td>"
                 << toString(moon->getMass() * SUN_MASS_IN_EARTH_MASSES, 4)
                 << " EM</td>\n";
        the_file << "\t<td>" << toString(convert_km_to_eu(moon->getRadius()), 4)
                 << " ER";
        if (is_gas_planet(moon)) {
          the_file << " (" << toString(moon->getRadius() / KM_JUPITER_RADIUS, 4)
                   << " JR)";
        }
        the_file << "</td>";
        the_file << "</tr>\n";
      }
    }
  }
  the_file << "</table>\n<br clear=all>\n";

  // Tables for individual planets
  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    html_describe_planet(the_planet, counter, 0, do_gases, url_path, the_file);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != NULL;
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
void celestia_describe_system(planet* innermost_planet, string designation,
                              string system_name, long int seed,
                              long double inc, long double an, bool do_moons) {
  planet *the_planet, *moon;
  sun the_sun = innermost_planet->getTheSun();
  int counter, moons;
  long double min_r_ecosphere =
      habitable_zone_distance(the_sun, RECENT_VENUS, 1.0);
  long double max_r_ecosphere =
      habitable_zone_distance(the_sun, EARLY_MARS, 1.0);

  cout << "# Stargen - " << stargen_revision << "; seed=" << seed << endl;
  cout << "#\n";
  cout << "# " << designation << ", " << system_name << endl;
  cout << "#\n";
  cout << "# Stellar mass: " << toString(the_sun.getMass()) << " solar masses\n";
  cout << "# Stellar luminosity: " << toString(the_sun.getLuminosity()) << endl;
  cout << "# Stellar temperature: " << toString(the_sun.getEffTemp())
       << " Kelvin\n";
  cout << "# Age: " << toString(the_sun.getAge() / 1.0E9)
       << "  billion years\t("
       << toString((the_sun.getLife() - the_sun.getAge()) / 1.0E9)
       << " billion left on main sequence)\n";
  cout << "# Habitable ecosphere: " << toString(min_r_ecosphere) << " AU to "
       << toString(max_r_ecosphere) << " AU\n\n";
  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
       the_planet = the_planet->next_planet, counter++) {
    celestia_describe_world(the_planet, designation, system_name, seed, inc, an,
                            counter, the_sun, false, counter);
    if (do_moons) {
      for (moon = the_planet->first_moon, moons = 1; moon != NULL;
           moon = moon->next_planet, moons++) {
        celestia_describe_world(moon, designation, system_name, seed, inc, an,
                                moons, the_sun, true, counter);
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
void celestia_describe_world(planet* the_planet, const string &designation,
                             const string &system_name, long int seed, long double inc,
                             long double an, int counter, sun& the_sun,
                             bool is_moon, int planet_num) {
  long double tmp, local_inc, mean_long;
  long double min_r_ecosphere;  // = habitable_zone_distance(the_sun,
                                // RECENT_VENUS, 1.0);
  long double
      max_r_ecosphere;  // = habitable_zone_distance(the_sun, EARLY_MARS, 1.0);
  string typeString;
  stringstream ss;
  string parent;
  string name;

  if (is_moon) {
    ss << "\"p" << counter << "-" << planet_num;
  } else {
    ss << "\"p" << counter;
  }
  name = ss.str();

  ss.str("");
  if (is_moon) {
    ss << designation << "/p" << planet_num;
  } else {
    ss << designation;
  }
  parent = ss.str();

  cout << "\t" << name << "\" \"" << parent << "\"\n";
  cout << "{\n";
  if (!is_moon) {
    if (the_planet->getType() == tAsteroids ||
        the_planet->getRadius() < round_threshold(the_planet->getDensity())) {
      cout << "\tClass \"asteroid\"\n";
    } else if (calcLambda(the_planet->getA(), the_planet->getMass()) < 1.0) {
      cout << "\tClass \"dwarfplanet\"\n";
    } else {
      cout << "\tClass \"planet\"\n";
    }
  } else {
    cout << "\tClass \"moon\"\n";
  }
  cout << "\tRadius " << toString(the_planet->getEquatrorialRadius()) << endl;
  cout << "\tMass "
       << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << endl;
  cout << endl;

  if (is_gas_planet(the_planet) && the_planet->getType() != tBrownDwarf) {
    long double ts = TEMPERATURE_SATURN + 1.0;
    long double tu = TEMPERATURE_URANUS + 1.0;
    long double tn = TEMPERATURE_NEPTUNE + 1.0;

    if (the_planet->getEstimatedTemp() < tn)  // Neptune est temp = 48.1
    {
      cout << "\tTexture \"tgasgiant.*\"\n";
      cout << "\tColor [ 0.37 0.5 0.87 ]\n";
      cout << "\tBlendTexture true\n";
    } else if (the_planet->getEstimatedTemp() < tu)  // Uranus est temp = 60.3
    {
      cout << "\tTexture \"tgasgiant.*\"\n";
      assignTemperatureColors(the_planet, tu, 0.58, 0.69, 0.74, tn, 0.37, 0.5,
                              0.87);
    } else if (the_planet->getEstimatedTemp() < ts)  // Saturn est temp = 85.5
    {
      cout << "\tTexture \"tgasgiant.*\"\n";
      assignTemperatureColors(the_planet, ts, 0.91, 0.87, 0.76, tu, 0.58, 0.69,
                              0.74);
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_CLASS_II) {
      cout << "\tTexture \"exo-class1.*\"\n";
      assignTemperatureColors(the_planet, TEMPERATURE_CLASS_II, 1, 1, 1, ts,
                              0.91, 0.87, 0.76);
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_SULFUR_GIANT) {
      cout << "\tTexture \"exo-class2.*\"\n";
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_CLASS_III) {
      cout << "\tTexture \"venus.jpg\"\n";
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_CLASS_IV) {
      cout << "\tTexture \"exo-class3.*\"\n";
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_CLASS_V) {
      cout << "\tTexture \"exo-class4.*\"\n";
      cout << "\tNightTexture \"exo-class4night.*\"\n";
    } else if (the_planet->getEstimatedTemp() < TEMPERATURE_CARBON_GIANT) {
      cout << "\tTexture \"exo-class5.*\"\n";
      cout << "\tNightTexture \"exo-class5night.*\"\n";
    } else {
      cout << "\tTexture \"exo-class4night.*\"\n";
      cout << "\tNightTexture \"exo-class4night.*\"\n";
      cout << "\tColor [ 0 0 0 ]\n";
      cout << "\tBlendTexture true\n";
    }
  } else if (the_planet->getType() == tIce) {
    cout << "# Hydrosphere percentage: "
         << toString(the_planet->getHydrosphere() * 100.0) << endl;
    cout << "# Ice cover percentage:   "
         << toString(the_planet->getIceCover() * 100.0) << endl;
    cout << endl;
    int ranItex = random_number(1.5, 5.5);
    cout << "\tTexture \"tice" << ranItex << ".*\"\n";
    cout << "\tSpecularColor [ 0.1 0.1 0.13 ]\n";
    cout << "\tSpecularPower 25.0\n";
    cout << "\tBumpMap \"tice" << ranItex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";
    cout << "\tColor [ 1.0 0.9 0.75 ]\n";
  } else if (the_planet->getType() == tWater) {
    cout << "# Hydrosphere percentage: "
         << toString(the_planet->getHydrosphere() * 100.0) << endl;
    cout << "# Ice cover percentage:   "
         << toString(the_planet->getIceCover() * 100.0) << endl;
    cout << endl;
    int ranWtex = random_number(1.5, 5.5);
    cout << "\tTexture \"twater" << ranWtex << ".*\"\n";
    cout << "\tSpecularColor [ 0.5 0.5 0.55 ]\n";
    cout << "\tSpecularPower 25.0\n";
    cout << "\tColor [ 0.75 0.75 1.0 ]\n";
  } else if (the_planet->getType() == tCarbon ||
             the_planet->getType() == tIron || the_planet->getType() == tOil ||
             the_planet->getType() == tRock) {
    if (the_planet->getType() == tOil) {
      cout << "# Hydrosphere percentage: "
           << toString(the_planet->getHydrosphere() * 100.0) << endl;
      cout << "# Ice cover percentage:   "
           << toString(the_planet->getIceCover() * 100.0) << endl;
      cout << endl;
    }
    int ranRtex = random_number(1.5, 5.5);
    cout << "\tTexture \"trock" << ranRtex << ".*\"\n";
    cout << "\tBumpMap \"trock" << ranRtex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";

    if (the_planet->getType() != tOil) {
      assignDistanceColors(the_planet, 0.52, 0.47, 0.42);
    }
  } else if (the_planet->getType() == t1Face) {
    cout << "# Hydrosphere percentage: "
         << toString(the_planet->getHydrosphere() * 100.0) << endl;
    cout << "# Ice cover percentage:   "
         << toString(the_planet->getIceCover() * 100.0) << endl;
    cout << endl;
    int ran1tex = random_number(1.5, 5.5);
    cout << "\tTexture \"t1face" << ran1tex << ".*\"\n";
    cout << "\tSpecularTexture \"t1face" << ran1tex << "-spec.*\"\n";
    cout << "\tSpecularColor [ 0.1 0.1 0.13 ]\n";
    cout << "\tSpecularPower 25.0\n";
    cout << "\tBumpMap \"t1face" << ran1tex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";
    cout << "\tColor [ 0.52 0.47 0.42 ]\n";
  } else if (the_planet->getType() == tVenusian) {
    int ranVtex = random_number(1.5, 5.5);
    cout << "\tTexture \"tvenus" << ranVtex << ".*\"\n";
    cout << "\tBumpMap \"tvenus" << ranVtex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";
  } else if (the_planet->getType() == tMartian) {
    cout << "# Ice cover percentage:   "
         << toString(the_planet->getIceCover() * 100.0) << endl;
    cout << endl;
    int ranMtex = random_number(1.5, 5.5);
    cout << "\tTexture \"tmars" << ranMtex << ".*\"\n";
    cout << "\tBumpMap \"tmars" << ranMtex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";

    assignDistanceColors(the_planet, 1, 0.75, 0.7);
  } else if (the_planet->getType() == tTerrestrial) {
    cout << "# Hydrosphere percentage: "
         << toString(the_planet->getHydrosphere() * 100.0) << endl;
    cout << "# Ice cover percentage:   "
         << toString(the_planet->getIceCover() * 100.0) << endl;
    cout << endl;
    int ranEtex = random_number(1.5, 5.5);
    cout << "\tTexture \"tearth" << ranEtex << ".*\"\n";
    cout << "\tSpecularTexture \"tearth" << ranEtex << "-spec.*\"\n";
    cout << "\tSpecularColor [ 0.8 0.8 0.85 ]\n";
    cout << "\tSpecularPower 25.0\n";
    cout << "\tBumpMap \"tearth" << ranEtex << "-bump.*\"\n";
    cout << "\tBumpHeight 1.5\n";
    cout << "\tColor [ 0.9 0.9 0.95 ]\n";
  } else if (the_planet->getType() == tAsteroids) {
    int ranAtex = random_number(1.5, 3.5);
    cout << "\tTexture \"tasteroid.*\"\n";
    if (the_planet->getRadius() <
        (round_threshold(the_planet->getDensity()) * (2.0 / 3.0))) {
      cout << "\tMesh \"asteroid.cms\"\n";
    } else {
      cout << "\tBumpMap \"tasteroid-bump.*\"\n";
      cout << "\tBumpHeight 1.5\n";
    }
    if (ranAtex == 1) {
      cout << "\tColor   [ 0.52 0.46 0.43 ]\n";
    } else if (ranAtex == 2) {
      cout << "\tColor   [ 0.37 0.37 0.37 ]\n";
    } else {
      cout << "\tColor   [ 0.7 0.7 0.7 ]\n";
    }
    cout << "\tBlendTexture true\n";
  } else {
    cout << "\tTexture \"" << texture_name(the_planet->getType()) << "\""
         << endl;
  }

  cout << endl;

  cout << "#   HZD:\t\t\t" << toString(the_planet->getHzd()) << endl;
  cout << "#   HZC:\t\t\t" << toString(the_planet->getHzc()) << endl;
  cout << "#   HZA:\t\t\t" << toString(the_planet->getHza()) << endl;
  cout << "#   SPH:\t\t\t" << toString(the_planet->getSph()) << endl;
  cout << "#   ESI:\t\t\t" << toString(the_planet->getEsi()) << endl;
  cout << "#   Density:\t\t\t" << toString(the_planet->getDensity())
       << "\tgrams/cc\n";
  cout << "#   Escape Velocity:\t\t"
       << toString(the_planet->getEscVelocity() / CM_PER_KM) << "\tKm/sec"
       << endl;
  cout << "#   Surface acceleration:\t" << toString(the_planet->getSurfAccel())
       << "\tcm/sec2\n";
  cout << "#   Surface gravity:\t\t" << toString(the_planet->getSurfGrav())
       << "\tEarth gees\n";
  cout << "#   Surface temperature:\t"
       << toString(the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER)
       << "\tdegrees Celcius\n";
  cout << "#   Estimated temperature:\t"
       << toString(the_planet->getEstimatedTemp() - FREEZING_POINT_OF_WATER)
       << "\tdegrees Celcius\n";
  cout << "#   Estimated terrestrial temperature:\t"
       << toString(the_planet->getEstimatedTerrTemp() - FREEZING_POINT_OF_WATER)
       << "\tdegrees Celcius\n";
  cout << "#   Boiling point of water:\t"
       << toString(the_planet->getBoilPoint() - FREEZING_POINT_OF_WATER)
       << "\tdegrees Celcius\n";
  cout << "#   Surface pressure:\t\t"
       << toString(the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS)
       << "\tEarth atmospheres";
  if (the_planet->getGreenhouseEffect() &&
      the_planet->getSurfPressure() > 0.0) {
    cout << "\tGREENHOUSE EFFECT";
  }
  cout << endl;
  cout << "#   Molecular weight retained:\t"
       << toString(the_planet->getMolecWeight()) << " and above\n";

  if (the_planet->getNumGases() > 0) {
    for (int i = 0; i < the_planet->getNumGases(); i++) {
      int index = gases.count();
      bool poisonous = false;
      long double ipp;

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

      if ((the_planet->getGas(i).getSurfPressure() /
           the_planet->getSurfPressure()) > .0005 ||
          poisonous) {
        cout << "#           " << gases[index].getName() << ": "
             << toString((the_planet->getGas(i).getSurfPressure() /
                          the_planet->getSurfPressure()) *
                         100.0)
             << "% " << toString(the_planet->getGas(i).getSurfPressure())
             << " mb (ipp: " << toString(ipp) << ")";
        if (poisonous) {
          cout << " poisonous";
        }
        cout << endl;
      }
    }
  }

  cout << endl;

  switch (the_planet->getType()) {
    case t1Face:
      cout << "\tHazeColor [ 1 1 1 ]\n"
      "\tHazeDensity 0.3\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 30\n"
      "\t\tLower [ 0.8 0.4 0.1 ]\n"
      "\t\tUpper [ 0.0 0.0 0.9 ]\n"
      "\t\tSky [ 0.8 0.4 0.1 ]\n"
      "\t\tSunset [ 0.8 0.5 0.2 ]\n"
	  "\t\tMie 0.001\n"
	  "\t\tMieAsymmetry -0.25\n"
	  "\t\tRayleigh [ 0.008 0.004 0.001 ]\n"
	  "\t\tAbsorption [ 0.002 0.005 0.008 ]\n"
	  "\t\tMieScaleHeight 9\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tIce:
      cout << "\tHazeColor [ 0.2 0.5 1 ]\n"
      "\tHazeDensity 1\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 60\n"
      "\t\tLower [ 0.8 0.4 0.1 ]\n"
      "\t\tUpper [ 0.0 0.0 0.9 ]\n"
      "\t\tSky [ 0.8 0.4 0.1 ]\n"
      "\t\tSunset [ 0.8 0.5 0.2 ]\n"
	  "\t\tMie 0.001\n"
	  "\t\tMieAsymmetry -0.25\n"
	  "\t\tRayleigh [ 0.008 0.004 0.001 ]\n"
	  "\t\tAbsorption [ 0.002 0.005 0.008 ]\n"
	  "\t\tMieScaleHeight 12\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tMartian:
      cout << "\tHazeColor [ 1 1 1 ]\n"
      "\tHazeDensity 0.45\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 30\n"
      "\t\tLower [ 0.8 0.6 0.6 ]\n"
      "\t\tUpper [ 0.7 0.3 0.3 ]\n"
      "\t\tSky [ 0.83 0.75 0.65 ]\n"
      "\t\tSunset [ 0.7 0.7 0.8 ]\n"
	  "\t\tMie 0.0024\n"
	  "\t\tMieAsymmetry -0.15\n"
	  "\t\tRayleigh [ 0.0083 0.0075 0.0065 ]\n"
	  "\t\tAbsorption [ 0.003 0.003 0.002 ]\n"
	  "\t\tMieScaleHeight 20\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tTerrestrial:
      cout << "\tHazeColor [ 1 1 1 ]\n"
      "\tHazeDensity 0.3\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 60\n"
      "\t\tLower [ 0.5 0.5 0.65 ]\n"
      "\t\tUpper [ 0.3 0.3 0.6 ]\n"
      "\t\tSky [ 0.3 0.6 0.9 ]\n"
      "\t\tSunset [ 1.0 0.6 0.2 ]\n"
	  "\t\tMie 0.001\n"
	  "\t\tMieAsymmetry -0.25\n"
	  "\t\tRayleigh [ 0.003 0.006 0.009 ]\n"
	  "\t\tAbsorption [ 0.000 0.004 0.008 ]\n"
	  "\t\tMieScaleHeight 12\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tWater:
      cout << "\tHazeColor [ 1 1 1 ]\n"
      "\tHazeDensity 0.3\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 90\n"
      "\t\tLower [ 0.4 0.4 0.7 ]\n"
      "\t\tUpper [ 0.2 0.2 0.6 ]\n"
      "\t\tSky [ 0.4 0.7 0.9 ]\n"
      "\t\tSunset [ 1.0 0.6 0.2 ]\n"
	  "\t\tMie 0.001\n"
	  "\t\tMieAsymmetry -0.25\n"
	  "\t\tRayleigh [ 0.004 0.007 0.009 ]\n"
	  "\t\tAbsorption [ 0.000 0.004 0.008 ]\n"
	  "\t\tMieScaleHeight 15\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tOil:
    case tVenusian:
      cout << "\tHazeColor [ 0.5 0.35 0.2 ]\n"
      "\tHazeDensity 0.35\n\n"
      "\tAtmosphere {\n"
      "\t\tHeight 60\n"
      "\t\tLower [ 0.8 0.8 0.5 ]\n"
      "\t\tUpper [ 0.6 0.6 0.6 ]\n"
      "\t\tSky [ 0.8 0.8 0.5 ]\n"
	  "\t\tMie 0.002\n"
	  "\t\tMieAsymmetry -0.33\n"
	  "\t\tRayleigh [ 0.008 0.008 0.005 ]\n"
	  "\t\tMieScaleHeight 20\n";
      display_clouds(the_planet);
      cout << "\t}\n\n";
      break;
    case tSubSubGasGiant:
    case tSubGasGiant:
      cout << "\tOblateness " << toString(the_planet->getOblateness()) << endl;
      cout << "\tHazeColor [ 0.5 0.8 1.0 ]\n";
      cout << "\tHazeDensity 0.2\n";
	  cout << "\tAtmosphere {\n"
      "\t\tHeight 500\n"
	  "\t\tMie 0.0001\n"
	  "\t\tMieAsymmetry -0.1\n"
	  "\t\tRayleigh [ 0.00104 0.00193 0.00400 ]\n"
	  "\t\tAbsorption [ 0.00000 0.00050 0.00100 ]\n"
	  "\t\tMieScaleHeight 20.3\n";
      cout << "\t}\n\n";
      break;
    case tGasGiant:
      cout << "\tOblateness " << toString(the_planet->getOblateness()) << endl;
      cout << "\tHazeColor [ 0.4 0.45 0.5 ]\n";
      cout << "\tHazeDensity 0.3\n";
	  cout << "\tAtmosphere {\n"
      "\t\tHeight 1000\n"
	  "\t\tMie 0.0001\n"
	  "\t\tMieAsymmetry -0.15\n"
	  "\t\tRayleigh [ 0.00095 0.00145 0.00298 ]\n"
	  "\t\tAbsorption [ 0.00033 0.00010 0.00001 ]\n"
	  "\t\tMieScaleHeight 28\n";
      cout << "\t}\n\n";
      break;
    case tBrownDwarf:
      cout << "\tHazeColor [ 0.5 0.45 0.45 ]\n";
      cout << "\tHazeDensity 0.3\n";
      break;
    default:
      break;
  }
  // cout << "\tOrbitFrame { EclipticJ2000{} }\n";
  cout << "\tOrbitFrame { \n"
  "\t\tBodyFixed { \n"
  "\t\t\tCenter \"" << parent << "\"\n"
  "\t\t}\n"
  "\t}\n"
  "\tEllipticalOrbit {\n";
  if (!is_moon) {
    cout << "\t\tPeriod            "
         << toString(the_planet->getOrbPeriod() / DAYS_IN_A_YEAR) << " # Years"
         << endl;
    cout << "\t\tSemiMajorAxis     " << toString(the_planet->getA()) << " # AU"
         << endl;
    cout << "\t\tEccentricity      " << toString(the_planet->getE()) << endl;
  } else {
    cout << "\t\tPeriod            " << toString(the_planet->getOrbPeriod())
         << " # Days\n";
    cout << "\t\tSemiMajorAxis     "
         << toString(the_planet->getMoonA() * KM_PER_AU) << " # Km\n";
    cout << "\t\tEccentricity      " << toString(the_planet->getMoonE())
         << endl;
  }
  local_inc = the_planet->getInclination();
  cout << "\t\tInclination       " << toString(local_inc) << endl;
  cout << "\t\tAscendingNode     " << toString(the_planet->getAscendingNode())
       << endl;
  cout << "\t\tLongOfPericenter  "
       << toString(the_planet->getLongitudeOfPericenter()) << endl;
  cout << "\t\tMeanLongitude     " << toString(the_planet->getMeanLongitude())
       << endl;
  cout << "\t}\n";
  cout << endl;

  // cout << "\tBodyFrame { EclipticJ2000{} }\n";
  cout << "\tBodyFrame { \n";
  cout << "\t\tBodyFixed { \n";
  cout << "\t\t\tCenter \"" << parent << "\"\n";
  cout << "\t\t\tMeanEquator {\n";
  cout << "\t\t\t\tCenter \"" << parent << "\"\n";
  cout << "\t\t\t}\n";
  cout << "\t\t}\n";
  cout << "\t}\n";
  cout << "\tUniformRotation {\n";
  if (the_planet->getResonantPeriod()) {
    if (!is_moon) {
      cout << "# Planet's rotation is in a resonant spin lock with the star"
           << endl;
      cout << "# spin resonance factor = " << (1.0 - the_planet->getE())
           << " / " << (1.0 + the_planet->getE()) << " = "
           << getSpinResonanceFactor(the_planet->getE()) << endl;
    } else {
      cout << "# Moon's rotation is in a resonant spin lock with the planet"
           << endl;
      cout << "# spin resonance factor = " << (1.0 - the_planet->getMoonE())
           << " / " << (1.0 + the_planet->getMoonE()) << " = "
           << getSpinResonanceFactor(the_planet->getMoonE()) << endl;
    }
  }

  if ((int)the_planet->getDay() == (int)(the_planet->getOrbPeriod() * 24.0) &&
      !the_planet->getResonantPeriod()) {
    if (!is_moon) {
      cout << "# Planet is tidally locked with one face to star.\n";
    } else {
      cout << "# Moon is tidally locked with one face to planet.\n";
    }
    cout << "\t\tInclination\t" << toString(local_inc) << endl;
  } else {
    cout << "\t\tPeriod\t\t" << toString(the_planet->getDay()) << " # Hours"
         << endl;
    cout << "\t\tInclination\t"
         << toString(local_inc + the_planet->getAxialTilt()) << endl;
  }
  cout << "\t\tAscendingNode\t"  << toString(the_planet->getAscendingNode())
       << endl;
  cout << "\t\tMeridianAngle\t" << toString(the_planet->getMeanLongitude())
       << endl;
  cout << "\t}\n";
  cout << endl;
  cout << "\tAlbedo            " << toString(the_planet->getAlbedo()) << endl;
  cout << "}\n";
  cout << endl;
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

  cout << "#! /bin/sh -x\n";
  cout << "# Stargen - " << stargen_revision << "; seed=" << seed << endl;
  cout << "#\n";
  cout << "# Script to generate moons for some planets in the system " <<
designation << " (" << system_name << ")\n"; cout << "#\n";

  for (the_planet = innermost_planet, counter = 1; the_planet != NULL;
the_planet = the_planet->next_planet, counter++)
  {
    mass = the_planet->getMass();
    total_moon_mass = 0;
    for (moon = the_planet->first_moon; moon != NULL; moon = moon->next_planet)
    {
      total_moon_mass += moon->getMass();
    }

    tmp = the_planet->getMinorMoons();

    num_moons += tmp;
    if (tmp > 0)
    {
      num_planets++;
      cout << "moon_orbits -a " << toString(the_planet->getA()) << " -m " <<
toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) << " -R " <<
toString(the_planet->getRadius()) << " -M " << toString(the_sun.getMass()) << "
-n p" << counter << " -N " << tmp << " -U " << toString(total_moon_mass *
SUN_MASS_IN_EARTH_MASSES) << " -s " << (seed + (counter * 1000)) << " -S " <<
designation << endl;
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
void lprint(fstream& the_file, bool& first, const string &text) {
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
  string typeString;
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
	f-=base;
	if(f<tol)
		return fraction(base,1);
	if(1-tol < f)
		return fraction(base+1,1);

	fraction lower(0,1);
	fraction upper(1,1);

	while(true) {
		fraction middle(lower.n+upper.n, lower.d+upper.d);

		if( middle.d*(f+tol) < middle.n ) {
			upper = middle;
		} else if (middle.n < middle.d*(f-tol) ) {
			lower = middle;
		} else {
			return fraction(middle.d*base + middle.n, middle.d);
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
  long double top = 1.0 - eccentricity;
  long double bottom = 1.0 + eccentricity;
  long double fraction = top / bottom;
  long double tolerance = .01;

  // this is a test to replace the hard coded check with something formulaic
  // with a tolerance of .01 the ratios shouldn't get too crazy
  // decrease tollerance to increase accuracy but you may get ratios like 1549/1322
  // for the number 0.85345349958 at a tolerance of .000001 instead of something like 
  // 7/6 for tolerances between .007 and .015
  // see https://gist.github.com/mikeando/7073d62385a34a61a6f7
  struct fraction sb = stern_brocot_search(fraction, tolerance);
  //this is setup to have numerator and denomenator flipped.
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
void mol_print(fstream& the_file, bool& first, int& count, int max,
               long double min_weight, string molecule, long double weight) {
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
  int ranCtex = random_number(1.5, 5.5);
  cout << "# Cloud cover percentage: " << (the_planet->getCloudCover() * 100.0)
       << "%\n";
  if (the_planet->getCloudCover() > 0.25) {
    cout << endl;
    cout << "\t\tCloudHeight 7\n";
    if (the_planet->getDay() > (14.0 * 24.0)) {
      cout << "\t\tCloudSpeed 300\n";
    } else if (the_planet->getDay() > (4.0 * 24.0)) {
      cout << "\t\tCloudSpeed 130\n";
    } else {
      cout << "\t\tCloudSpeed 65\n";
    }
  }

  if (the_planet->getCloudCover() > 0.75) {
    cout << "\t\tCloudMap \"t100-clouds" << ranCtex << ".*\"\n";
  } else if (the_planet->getCloudCover() > 0.25) {
    cout << "\t\tCloudMap \"t50-clouds" << ranCtex << ".*\"\n";
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
void assignDistanceColors(planet* the_planet, long double r0, long double g0,
                          long double b0) {
  long double r, g, b;

  r = r0 + ((1.0 - r0) *
            (the_planet->getA() /
             (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
                                                 SUN_MASS_IN_EARTH_MASSES))));
  g = g0 + ((1.0 - g0) *
            (the_planet->getA() /
             (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
                                                 SUN_MASS_IN_EARTH_MASSES))));
  b = b0 + ((1.0 - b0) *
            (the_planet->getA() /
             (50.0 * the_sun_clone.getREcosphere(the_planet->getMass() *
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
  cout << "\tColor [ " << toString(r) << " " << toString(g) << " "
       << toString(b) << " ]\n";
  cout << "\tBlendTexture true\n";
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
void assignTemperatureColors(planet* the_planet, long double t0, long double r0,
                             long double g0, long double b0, long double t1,
                             long double r1, long double g1, long double b1) {
  long double r, g, b, te;

  te = the_planet->getEstimatedTemp();
  // modify color proportional to temperature
  r = r0 + ((r1 - r0) * ((te - t0) / (t1 - t0)));
  g = g0 + ((g1 - g0) * ((te - t0) / (t1 - t0)));
  b = b0 + ((b1 - b0) * ((te - t0) / (t1 - t0)));

  cout << "\tColor [ " << toString(r) << " " << toString(g) << " "
       << toString(b) << " ]\n";
  cout << "\tBlendTexture true\n";
}