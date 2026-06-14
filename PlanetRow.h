#ifndef PLANETROW_H
#define PLANETROW_H

// ---------------------------------------------------------------------------
// PlanetRow: a single, annotated source of truth for the flat per-planet field
// list shared by the JSON and CSV output formats (and consumed for overlapping
// fields by HTML/CELESTIA). The reflective emitters below (C++26 P2996 + P3394)
// iterate the struct's members so a field is declared exactly once -- its label
// (= JSON key / CSV column) lives in a member annotation, its value is read by
// splice. Adding/renaming a field is a one-line edit here instead of touching
// jsonRow + csv_row + the CSV header in lockstep.
//
// Values are stored RAW (straight from the planet getters, no unit conversion):
// JSON/CSV emit raw, while HTML/CELESTIA apply their own conversions at emit
// time. to_row() (defined in display.cpp, which has the getters/helpers) is the
// single population point.
//
// Requires GCC >= 16 with -std=c++26 -freflection (set per-source on display.cpp
// in CMakeLists.txt -- the only translation unit that includes this header).
// ---------------------------------------------------------------------------

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <meta>
#include <nlohmann/json.hpp>

namespace planetrow {

// Structural compile-time string usable as a member annotation value (a bare
// pointer/string_view is not "structural" and fails reflect_constant; a char
// array is).
template <std::size_t N>
struct fixed_string {
  char data[N]{};
  consteval fixed_string(const char (&s)[N]) {
    for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
  }
  constexpr std::string_view view() const { return std::string_view(data, N - 1); }
};
template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

// Returns the field's annotated label, or "" for members with no annotation
// (helper fields like atmosphere_json/is_moon, which the emitters skip). Keyed
// on the member as a template parameter so the member -- and thus the spliced
// annotation type -- is a constant expression (a function parameter is not).
// The result has static storage (define_static_string), so returning it does
// not dangle. Every output member carries exactly one fixed_string annotation.
template <std::meta::info M>
consteval const char* label_v() {
  constexpr auto anns = std::define_static_array(std::meta::annotations_of(M));
  if constexpr (anns.size() == 0) {
    return "";
  } else {
    constexpr std::meta::info ann = anns[0];
    return std::define_static_string(
        std::meta::extract<typename [: std::meta::type_of(ann) :]>(ann).view());
  }
}

}  // namespace planetrow

// The shared per-planet row. Member order == CSV column order. Each output field
// carries its label as an annotation; helper members (trailing, unannotated) are
// skipped by the reflective emitters.
struct PlanetRow {
  [[=planetrow::fixed_string("Planet #")]]                       std::string planet_no;
  [[=planetrow::fixed_string("Distance")]]                       long double distance;       // JSON key -> "Distance au" for moons
  [[=planetrow::fixed_string("Eccentricity")]]                   long double eccentricity;
  [[=planetrow::fixed_string("Inclination")]]                    long double inclination;
  [[=planetrow::fixed_string("Longitude of the Ascending Node")]] long double ascending_node;
  [[=planetrow::fixed_string("Longitude of the Pericenter")]]    long double longitude_of_pericenter;
  [[=planetrow::fixed_string("Mean Longitude")]]                 long double mean_longitude;
  [[=planetrow::fixed_string("Axial Tilt")]]                     long double axial_tilt;
  [[=planetrow::fixed_string("Ice Mass Fraction")]]              long double ice_mass_fraction;
  [[=planetrow::fixed_string("Rock Mass Fraction")]]             long double rock_mass_fraction;
  [[=planetrow::fixed_string("Carbon Mass Fraction")]]           long double carbon_mass_fraction;
  [[=planetrow::fixed_string("Total Mass")]]                     long double total_mass;
  [[=planetrow::fixed_string("Is Gas Giant")]]                   bool        is_gas_giant;
  [[=planetrow::fixed_string("Dust Mass")]]                      long double dust_mass;
  [[=planetrow::fixed_string("Gas Mass")]]                       long double gas_mass;
  [[=planetrow::fixed_string("Radius of Core")]]                 long double radius_of_core;
  [[=planetrow::fixed_string("Total Radius")]]                   long double total_radius;
  [[=planetrow::fixed_string("Orbit Zone")]]                     int         orbit_zone;
  [[=planetrow::fixed_string("Density")]]                        long double density;
  [[=planetrow::fixed_string("Orbit Period")]]                   long double orbit_period;
  [[=planetrow::fixed_string("Rotation Period")]]                long double rotation_period;
  [[=planetrow::fixed_string("Has Spin Orbit Resonance")]]       bool        has_spin_orbit_resonance;
  [[=planetrow::fixed_string("Escape Velocity")]]                long double escape_velocity;
  [[=planetrow::fixed_string("Surface Acceleration")]]           long double surface_acceleration;
  [[=planetrow::fixed_string("Surface Gravity")]]                long double surface_gravity;
  [[=planetrow::fixed_string("RMS Velocity")]]                   long double rms_velocity;
  [[=planetrow::fixed_string("Minimum Molecular Weight")]]       long double minimum_molecular_weight;
  [[=planetrow::fixed_string("Volatile Gas Inventory")]]         long double volatile_gas_inventory;
  // JSON key preserves the historical typo "Get Surface Pressure"; the CSV
  // header uses "Surface Pressure" (substituted in emit_csv_header).
  [[=planetrow::fixed_string("Get Surface Pressure")]]           long double surface_pressure;
  [[=planetrow::fixed_string("Greenhouse Effect")]]              bool        greenhouse_effect;
  [[=planetrow::fixed_string("Boiling Point")]]                  long double boiling_point;
  [[=planetrow::fixed_string("Albedo")]]                         long double albedo;
  [[=planetrow::fixed_string("Exospheric Temperature")]]         long double exospheric_temperature;
  [[=planetrow::fixed_string("Estimated Temperature")]]          long double estimated_temperature;
  [[=planetrow::fixed_string("Estimated Terran Temperature")]]   long double estimated_terran_temperature;
  [[=planetrow::fixed_string("Surface Temperature")]]            long double surface_temperature;
  [[=planetrow::fixed_string("Greenhouse Rise")]]                long double greenhouse_rise;
  [[=planetrow::fixed_string("High Temperature")]]               long double high_temperature;
  [[=planetrow::fixed_string("Low Temperature")]]                long double low_temperature;
  [[=planetrow::fixed_string("Maximum Temperature")]]            long double maximum_temperature;
  [[=planetrow::fixed_string("Minimum Temperature")]]            long double minimum_temperature;
  [[=planetrow::fixed_string("Hydrosphere")]]                    long double hydrosphere;
  [[=planetrow::fixed_string("Cloud Cover")]]                    long double cloud_cover;
  [[=planetrow::fixed_string("Ice Cover")]]                      long double ice_cover;
  [[=planetrow::fixed_string("Atmosphere")]]                     std::string atmosphere;   // CSV-style ({:.2f}); JSON overrides with atmosphere_json
  [[=planetrow::fixed_string("Type")]]                           std::string type;
  [[=planetrow::fixed_string("Minor Moons")]]                    int         minor_moons;

  // --- helper fields: no annotation, skipped by the reflective emitters ---
  std::string atmosphere_json;  // JSON-style atmosphere (toString formatting)
  bool        is_moon = false;  // drives the JSON "Distance" -> "Distance au" rename
};

// Populated in display.cpp (needs the planet getters + type_string/atmosphere
// helpers). Single source of per-planet output values.
auto to_row(planet* the_planet, bool is_moon, const std::string& id, bool do_gases) -> PlanetRow;

// --- Reflective emitters (instantiated only in display.cpp, which has -freflection) ---

// JSON object for one planet/moon, matching the legacy jsonRow byte-for-byte.
inline auto emit_planet_json(const PlanetRow& r) -> nlohmann::json {
  nlohmann::json body;
  template for (constexpr std::meta::info m :
      std::define_static_array(std::meta::nonstatic_data_members_of(
          ^^PlanetRow, std::meta::access_context::current()))) {
    constexpr const char* lbl = planetrow::label_v<m>();
    if constexpr (std::string_view(lbl).size() > 0) {
      body[lbl] = r.[:m:];
    }
  }
  // JSON uses its own atmosphere formatting (see to_row), and renames the
  // distance key for moons.
  body["Atmosphere"] = r.atmosphere_json;
  if (r.is_moon) {
    body["Distance au"] = body["Distance"];
    body.erase("Distance");
  }
  return body;
}

// CSV header row: 'col', 'col', ... matching the legacy hardcoded header.
inline auto emit_csv_header() -> std::string {
  std::string out;
  bool first = true;
  template for (constexpr std::meta::info m :
      std::define_static_array(std::meta::nonstatic_data_members_of(
          ^^PlanetRow, std::meta::access_context::current()))) {
    constexpr const char* lbl = planetrow::label_v<m>();
    if constexpr (std::string_view(lbl).size() > 0) {
      if (!first) out += ", ";
      first = false;
      // CSV column uses the corrected name; JSON keeps the historical typo.
      std::string_view col = lbl;
      if (col == "Get Surface Pressure") col = "Surface Pressure";
      out += '\'';
      out += col;
      out += '\'';
    }
  }
  out += '\n';
  return out;
}

// Defined in utils.cpp. Escapes a value placed inside a single-quoted CSV
// field (' -> '', CR/LF -> space); a no-op on clean strings, so default output
// is byte-identical. The JSON path stays raw (nlohmann escapes there).
auto escapeCsvField(const std::string&) -> std::string;

// CSV data row for one planet/moon, matching the legacy csv_row formatting:
// strings 'quoted', floats {:.6f}, ints/bools default. Comma+space separated.
inline auto emit_csv_row(const PlanetRow& r) -> std::string {
  std::string out;
  bool first = true;
  template for (constexpr std::meta::info m :
      std::define_static_array(std::meta::nonstatic_data_members_of(
          ^^PlanetRow, std::meta::access_context::current()))) {
    constexpr const char* lbl = planetrow::label_v<m>();
    if constexpr (std::string_view(lbl).size() > 0) {
      if (!first) out += ", ";
      first = false;
      const auto& v = r.[:m:];
      using T = std::remove_cvref_t<decltype(v)>;
      if constexpr (std::is_same_v<T, std::string>) {
        out += std::format("'{}'", escapeCsvField(v));
      } else if constexpr (std::is_floating_point_v<T>) {
        out += std::format("{:.6f}", v);
      } else {
        out += std::format("{}", v);
      }
    }
  }
  out += '\n';
  return out;
}

#endif  // PLANETROW_H
