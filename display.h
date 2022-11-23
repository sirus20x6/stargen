#ifndef DISPLAY_H
#define DISPLAY_H
#include <fstream>
#include <sstream>
#include <string>

#include "structs.h"

using namespace std;

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
void csv_describe_system(fstream &, planet *, bool, long, bool);
void jsonDescribeSystem(fstream& the_file, planet* innermost_planet,
                         bool do_gases, long int seed, bool do_moons);
void csv_row(fstream &, planet *, bool, bool, string, stringstream &);
void jsonRow(fstream& the_file, planet* the_planet, bool do_gases, bool is_moon,
             string id, stringstream& ss);
auto type_string(planet *) -> string;
auto cloud_type_string(planet *) -> string;
void create_svg_file(planet *, string, string, string, string, bool);
void openCVSorJson(string, string, fstream &);
void refresh_file_stream(fstream &, const string&, const string&, const string&);
void open_html_file(const string&, long, const string&, const string&, const string&, const string&, const string&,
                    fstream &);
void close_html_file(fstream &);
void print_description(fstream &, const string&, planet *, const string&);
void list_molecules(fstream &, long double);
void html_star_details_helper(fstream& the_file, const string& header,
                              long double mass, long double luminosity,
                              long double temperature, long double age,
                              long double life, const string &spec_type);
void html_thumbnails(planet *, fstream &, const string&, const string&, const string&, const string&,
                     const string&, bool, bool, bool, bool, int, bool);
void html_thumbnail_totals(fstream &);
void html_describe_planet(planet *, int, int, bool, const string&, fstream &);
void html_describe_system(planet *, bool, bool, const string&, fstream &);
void celestia_describe_system(planet *, string, string, long, long double,
                              long double, bool);
void celestia_describe_world(planet *, const string&, const string&, long, long double,
                             long double, int, sun &, bool, int);
void moongen_describe_system(planet *, const string&, const string&, long);
void lprint(fstream &, bool &, const string&);
auto image_type_string(planet *) -> string;
auto printSpinResonanceFactor(long double) -> string;
void mol_print(fstream &, bool &, int &, int, long double, string, long double);
auto texture_name(planet_type) -> string;
void display_clouds(planet *);
void assignDistanceColors(planet *, long double, long double, long double);
void assignTemperatureColors(planet *, long double, long double, long double,
                             long double, long double, long double, long double,
                             long double);
fraction stern_brocot_search(long double f, long double tol);

#endif
