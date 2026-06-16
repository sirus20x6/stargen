#ifndef DISPLAY_H
#define DISPLAY_H
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <nlohmann/json.hpp>

#include "structs.h"

struct fraction {
	int n;
	int d;
	fraction(int n, int d) {
		this->n = n;
		this->d = d;
	}
	float asFloat() {
		return float((double)n/(double)d);
	}
};

void text_describe_system(planet *, bool, long, bool);
void csv_describe_system(std::fstream &, planet *, bool, long, bool);
void jsonDescribeSystem(std::fstream& the_file, planet* innermost_planet,
                         bool do_gases, long int seed, bool do_moons);
void csv_row(std::fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             const std::string& id, std::stringstream& ss) ;
auto jsonRow(planet* the_planet, bool do_gases, bool is_moon,
             std::string id, std::stringstream& ss) -> nlohmann::json;
auto type_string(planet *) -> std::string;
auto cloud_type_string(planet *) -> std::string;
void create_svg_file(planet *, std::string, std::string, std::string, std::string, bool);
void openCVSorJson(std::string, std::string, std::fstream &);
void refresh_file_stream(std::fstream &, const std::string&, const std::string&, const std::string&);
void open_html_file(const std::string&, long, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&,
                    std::fstream &);
void close_html_file(std::fstream &);
void print_description(std::fstream &, const std::string&, planet *, const std::string&);
void list_molecules(std::fstream &, long double);
void html_star_details_helper(std::fstream& the_file, const std::string& header,
                              long double mass, long double luminosity,
                              long double temperature, long double age,
                              long double life, const std::string &spec_type);
void html_thumbnails(planet *, std::fstream &, const std::string&, const std::string&, const std::string&, const std::string&,
                     const std::string&, bool, bool, bool, bool, int, bool, bool orbit_map = false);
void html_thumbnail_totals(std::fstream &);
void html_describe_planet(planet *, int, int, bool, const std::string&, std::fstream &);
void html_describe_system(planet *, bool, bool, const std::string&, std::fstream &);
void celestia_describe_system(planet *, std::string, std::string, long, long double,
                              long double, bool);
void celestia_describe_world(planet *, const std::string&, const std::string&, long, long double,
                             long double, int, sun &, bool, int);
void celestia_describe_world(const planet* the_planet, std::string_view designation,
                             std::string_view system_name, long int seed, long double inc,
                             long double an, int counter, const sun& the_sun,
                             bool is_moon, int planet_num);
void handle_gas_planet_texture(planet* the_planet);
void handle_rocky_planet_texture(planet* the_planet);
void handle_ice_planet_texture(planet* the_planet);
void handle_water_planet_texture(planet* the_planet);
void handle_rocky_texture(planet* the_planet);
void handle_1face_planet_texture(planet* the_planet);
void handle_venusian_texture(planet* the_planet);
void handle_martian_texture(planet* the_planet);
void handle_terrestrial_texture(planet* the_planet);
void handle_asteroid_texture(planet* the_planet);
void print_planet_details(planet* the_planet);
void print_atmosphere_composition(planet* the_planet);
int find_gas_index(int gas_num);
std::string generate_atmosphere_string(planet* the_planet, bool do_gases);
int random_numberInt(int min, int max);
void assignTemperatureColors(planet* the_planet, double temp1, double red1, double green1, double blue1,
                             double temp2, double red2, double green2, double blue2);
void assignDistanceColors(planet* the_planet, double red, double green, double blue);
void lprint(std::fstream &, bool &, const std::string&);
auto image_type_string(planet *) -> std::string;
auto printSpinResonanceFactor(long double) -> std::string;
void mol_print(std::fstream &, bool &, int &, int, long double, std::string, long double);
auto texture_name(planet_type) -> std::string;
void display_clouds(planet *);
void assignDistanceColors(planet *, long double, long double, long double);
fraction stern_brocot_search(long double f, long double tol);

#endif
