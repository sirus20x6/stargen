#include "elements.h"
#include <iosfwd>	 // for std
#include <string>	 // for string
#include "const.h"	// for AVE, FREEZING_POINT_OF_WATER, AN_AR, AN_CH4
#include "structs.h"  // for ChemTable, Chemical
#include <yaml-cpp/yaml.h> // for YAML parsing
#include <fstream>
#include <stdexcept>  // for std::runtime_error

ChemTable gases;

void loadElementsFromYAML(const std::string& filename) {
    int loaded = 0;
    try {
        YAML::Node config = YAML::LoadFile(filename);

        for (const auto& node : config) {
            // A non-positive atomic weight would feed downstream divisions and
            // produce NaN/inf. (density CAN legitimately be 0.0 in the shipped
            // table for some synthetic elements, so it is not rejected here.)
            if (node["atomic_weight"].as<double>() <= 0.0) {
                throw std::runtime_error(
                    "StarGen: element '" + node["symbol"].as<std::string>() +
                    "' in '" + filename + "' has a non-positive atomic weight");
            }
            Chemical chem(
                node["atomic_number"].as<int>(),
                node["symbol"].as<std::string>(),
                node["html_symbol"].as<std::string>(),
                node["name"].as<std::string>(),
                node["atomic_weight"].as<double>(),
                node["melting_point"].as<double>(),
                node["boiling_point"].as<double>(),
                node["density"].as<double>(),
                node["p0"].as<double>(),
                node["c"].as<double>(),
                node["n"].as<double>(),
                node["abund_e"].as<double>(),
                node["abund_s"].as<double>(),
                node["reactivity"].as<int>(),
                node["max_inspired_pp"].as<double>(),
                node["min_inspired_pp"].as<double>()
            );
            gases.addChemicle(chem);
            ++loaded;
        }
    } catch (const YAML::Exception& e) {
        // Missing file, parse error, or a missing/mistyped field: surface a
        // clear, catchable error instead of calling exit() from inside a
        // loader (which would also abort any in-flight generation).
        throw std::runtime_error("StarGen: cannot load elements from '" +
                                 filename + "': " + e.what());
    }
    if (loaded == 0) {
        throw std::runtime_error("StarGen: no elements loaded from '" +
                                 filename + "' (empty or wrong file?)");
    }
}

void initGases(const std::string& path)
{
    loadElementsFromYAML(path);
}
