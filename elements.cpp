#include "elements.h"
#include <iosfwd>	 // for std
#include <string>	 // for string
#include "const.h"	// for AVE, FREEZING_POINT_OF_WATER, AN_AR, AN_CH4
#include "structs.h"  // for ChemTable, Chemical
#include <yaml-cpp/yaml.h> // for YAML parsing
#include <fstream>

using namespace std;

ChemTable gases;

void loadElementsFromYAML(const string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);
        
        for (const auto& node : config) {
            Chemical chem(
                node["atomic_number"].as<int>(),
                node["symbol"].as<string>(),
                node["html_symbol"].as<string>(),
                node["name"].as<string>(),
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
        }
    } catch (const YAML::Exception& e) {
        cerr << "Error loading elements from YAML: " << e.what() << endl;
        exit(1);
    }
}

void initGases()
{
    loadElementsFromYAML("data/elements.yaml");
}
