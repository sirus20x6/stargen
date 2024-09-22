#ifndef ENVIRO_H
#define ENVIRO_H

// include "const.h"
#include "structs.h"
#include "utils.h"

extern string breathability_phrase[4];
extern map<map<long double, long double>, vector<long double> >
    polynomial_cache;

auto mass_to_luminosity(long double) -> long double;
auto luminosity_to_mass(long double) -> long double;
auto getStarType(string) -> string;
auto getSubType(string) -> int;
auto spec_type_to_eff_temp(const string&) -> long double;
auto eff_temp_to_spec_type(long double, long double) -> string;
auto orbital_zone(long double, long double) -> int;
auto volume_radius(long double, long double) -> long double;
auto empirical_density(long double, long double, long double, bool)
    -> long double;
auto volume_density(long double, long double) -> long double;
auto period(long double, long double, long double) -> long double;
auto day_length(planet *, long double, bool) -> long double;
auto inclination(long double, long double) -> long double;
auto escape_vel(long double, long double) -> long double;
auto rms_vel(long double, long double) -> long double;
auto molecule_limit(long double, long double, long double) -> long double;
auto min_molec_weight(planet *) -> long double;
auto acceleration(long double, long double) -> long double;
auto acceleration(planet *the_planet) -> long double;
auto acceleration_oblateness_refinement(planet *the_planet) -> long double;
auto gravity(long double) -> long double;
auto vol_inventory(long double, long double, long double, long double, int, bool, bool) -> long double;
auto pressure(long double, long double, long double) -> long double;
auto boiling_point(long double) -> long double;
auto hydro_fraction(long double, long double) -> long double;
auto cloud_fraction(long double, long double, long double, long double)
    -> long double;
auto ice_fraction(long double, long double) -> long double;
auto eff_temp(long double, long double, long double) -> long double;
auto est_temp(long double, long double, long double) -> long double;
auto grnhouse(long double, long double) -> bool;
auto green_rise(long double, long double, long double) -> long double;
auto planet_albedo(planet *) -> long double;
long double opacity(long double molecular_weight, long double surf_pressure, long double effective_temp);
auto gas_life(long double, planet *) -> long double;
void calculate_surface_temp(planet *, bool, long double, long double,
                            long double, long double, long double, bool);
void iterate_surface_temp(planet *, bool);
auto inspired_partial_pressure(long double, long double) -> long double;
auto breathability(planet *) -> unsigned int;
void set_temp_range(planet *);
auto getSpinResonanceFactor(long double) -> long double;
auto radius_improved(long double, long double, long double, long double, bool,
                     int, planet *) -> long double;
auto fudged_radius(long double, long double, long double, long double, bool,
                   int, planet *) -> long double;
auto gas_radius(long double, long double, long double, long double, planet *)
    -> long double;
auto round_threshold(long double) -> long double;
auto ultimateStrength(long double) -> long double;
auto habitable_zone_distance(sun &, int, long double) -> long double;
auto calcLambda(long double, long double) -> long double;
void gas_giant_temperature_albedo(planet *, long double, bool);
auto getGasGiantAlbedo(const string&, const string&, long double) -> long double;
void calculate_gases(sun &, planet *, string);
void assign_composition(planet *, sun &, bool);
auto is_gas_planet(planet *) -> bool;
auto is_earth_like(planet *) -> bool;
auto is_habitable_jovian_conservative(planet *) -> bool;
auto is_habitable_jovian(planet *) -> bool;
auto is_terrestrial(planet *) -> bool;
auto calcOblateness(planet *) -> long double;
auto calcPhlPressure(planet *) -> long double;
auto is_habitable_conservative(planet *) -> bool;
auto is_habitable_optimistic(planet *) -> bool;
auto calcHzd(planet *) -> long double;
auto calcHzc(planet *) -> long double;
auto calcHza(planet *) -> long double;
auto calcEsi(planet *) -> long double;
auto calcSph(planet *) -> long double;
auto calcEsiHelper(long double value, long double ref_value, long double weight,long double n = 4) -> long double;
auto is_potentialy_habitable_conservative(planet *) -> bool;
auto is_potentialy_habitable_optimistic(planet *) -> bool;
auto calcRelHumidity(planet *) -> long double;
auto getPlantLifeAlbedo(const string&, long double) -> long double;
auto calcFlux(long double, long double) -> long double;
auto calculate_moment_of_inertia_coeffient(planet *) -> long double;
auto calculate_moment_of_inertia(planet *) -> long double;
auto calcOblateness_improved(long double, long double, long double, long double)
    -> long double;
auto calcLuminosity(planet *) -> long double;
auto calcRadius(planet *) -> long double;
auto planet_radius_helper(long double planet_mass, long double mass1,
                          long double radius1, long double mass2,
                          long double radius2, long double mass3,
                          long double radius3, bool use_cache = true)
    -> long double;
auto planet_radius_helper2(long double, long double, long double, long double,
                           long double) -> long double;
auto planet_radius_helper3(long double, long double, long double, long double,
                           long double) -> long double;
auto convert_su_to_eu(long double) -> long double;
auto convert_au_to_km(long double) -> long double;
auto is_potentialy_habitable_extended(planet *) -> bool;
auto is_habitable_extended(planet *) -> bool;
auto is_potentialy_habitable_earth_like(planet *) -> bool;
auto is_habitable_earth_like(planet *) -> bool;
auto is_potentialy_habitable(planet *) -> bool;
auto is_habitable(planet *) -> bool;
auto convert_km_to_eu(long double) -> long double;
void makeHabitable(sun &, planet *, const string&, bool, bool);

template <typename Key, typename T>
void radiusDebug(const string &text, planet *the_planet,
                 long double calculated_radii, map<Key, T> &radii);

template <typename Key, typename T>
void radiusDebug(const string &text, planet *the_planet,
                 long double calculated_radii, map<Key, T> &radii) {
  cerr << "Age: " << toString(the_planet->getTheSun().getAge()) << endl;
  if (the_planet->getMoonA() == 0.0) {
    cerr << "Planet's distance: " << toString(the_planet->getA()) << " AU"
         << endl;
  } else {
    cerr << "Moon's parent's distance: " << toString(the_planet->getA())
         << " AU\n";
    cerr << "Moon's distance: "
         << toString(convert_au_to_km(the_planet->getMoonA())) << " km\n";
  }
  cerr << "Core composition: " << toString(the_planet->getImf() * 100.0, 4)
       << "% ice, " << toString(the_planet->getRmf() * 100.0, 4) << "% rock ("
       << toString(the_planet->getCmf() * 100.0, 4) << "% carbon)\n";
  cerr << "Mass: " << toString(convert_su_to_eu(the_planet->getMass())) << " EU"
       << endl;
  if (the_planet->getGasGiant()) {
    cerr << "Dust mass: "
         << toString(convert_su_to_eu(the_planet->getDustMass())) << " EU"
         << endl;
    cerr << "Temperature: " << toString(the_planet->getEstimatedTemp()) << " K"
         << endl;
  }
  cerr << "Calculated radius: " << toString(calculated_radii) << endl;
  cerr << text << endl;
  writeMap(radii);
}
#endif
