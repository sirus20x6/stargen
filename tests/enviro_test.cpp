// Unit tests for the environmental / atmospheric calculations in enviro.cpp.
//
// These exercise the pure free functions that model a planet's surface
// environment: temperature (effective + greenhouse), atmospheric pressure,
// bulk density, the boiling point of water as a function of pressure, the
// molecular-weight retention threshold, and the greenhouse trigger.
//
// Where possible the assertions are calibrated against Earth-ish reference
// values. To stay robust against the exact numeric constants in const.h the
// references are derived from the project's own constants (e.g. the boiling
// point at one bar, the surface pressure produced by an Earth-radius/normal
// gravity body) rather than hard-coded magic numbers; physical scaling
// relationships are checked with tight relative tolerances.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "const.h"
#include "enviro.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// ── Atmospheric pressure ────────────────────────────────────────────────────
// pressure(inventory, equat_radius_km, gravity) =
//     inventory * gravity * (EARTH_SURF_PRES_IN_MILLIBARS / 1000) /
//     (KM_EARTH_RADIUS / equat_radius_km)^2
TEST_CASE("pressure recovers Earth surface pressure for Earth-like inputs",
          "[enviro][pressure]") {
    // With an Earth-radius body at 1 g and a volatile inventory of 1000, the
    // (KM_EARTH_RADIUS / equat_radius) term is 1 and the result collapses to
    // exactly EARTH_SURF_PRES_IN_MILLIBARS.
    const long double p =
        pressure(1000.0L, static_cast<long double>(KM_EARTH_RADIUS), 1.0L);
    REQUIRE_THAT(static_cast<double>(p),
                 WithinRel(static_cast<double>(EARTH_SURF_PRES_IN_MILLIBARS),
                           1e-9));

    // Earth's actual surface pressure (~1013 mb) lies in a sane range.
    REQUIRE(p > 900.0L);
    REQUIRE(p < 1100.0L);

    // Pressure is linear in both the volatile inventory and surface gravity.
    REQUIRE_THAT(
        static_cast<double>(
            pressure(2000.0L, static_cast<long double>(KM_EARTH_RADIUS), 1.0L)),
        WithinRel(static_cast<double>(2.0L * p), 1e-9));
    REQUIRE_THAT(
        static_cast<double>(
            pressure(1000.0L, static_cast<long double>(KM_EARTH_RADIUS), 2.0L)),
        WithinRel(static_cast<double>(2.0L * p), 1e-9));

    // The radius enters as 1/(KM_EARTH_RADIUS/equat_radius)^2 = (equat_radius)^2
    // scaled, so a *larger* equatorial radius yields a *higher* pressure for a
    // fixed inventory and gravity: doubling the radius quadruples the pressure.
    const long double big =
        pressure(1000.0L, 2.0L * static_cast<long double>(KM_EARTH_RADIUS),
                 1.0L);
    REQUIRE_THAT(static_cast<double>(big),
                 WithinRel(static_cast<double>(4.0L * p), 1e-9));

    // Edge case: a body with no volatile inventory has no atmosphere.
    REQUIRE_THAT(static_cast<double>(pressure(
                     0.0L, static_cast<long double>(KM_EARTH_RADIUS), 1.0L)),
                 WithinAbs(0.0, 1e-12));
}

// ── Boiling point of water ──────────────────────────────────────────────────
// boiling_point(surf_pressure_mb) =
//     1 / (log(surf_pressure_mb / MILLIBARS_PER_BAR) / -5050.5 + 1/373)
TEST_CASE("boiling_point gives 373 K at one bar and tracks pressure",
          "[enviro][boiling_point]") {
    // At exactly one bar the log term vanishes, leaving 373 K (100 C) — the
    // boiling point of water at sea level on Earth.
    const long double bp_one_bar =
        boiling_point(static_cast<long double>(MILLIBARS_PER_BAR));
    REQUIRE_THAT(static_cast<double>(bp_one_bar), WithinAbs(373.0, 1e-6));

    // Higher pressure raises the boiling point; lower pressure lowers it
    // (water boils at a lower temperature up a mountain / in a near-vacuum).
    const long double bp_high =
        boiling_point(2.0L * static_cast<long double>(MILLIBARS_PER_BAR));
    const long double bp_low =
        boiling_point(0.5L * static_cast<long double>(MILLIBARS_PER_BAR));
    REQUIRE(bp_high > bp_one_bar);
    REQUIRE(bp_low < bp_one_bar);
}

// ── Bulk (volume) density ───────────────────────────────────────────────────
// volume_density is the inverse of volume_radius; an Earth-mass, Earth-density
// body should round-trip back to Earth's mean density.
TEST_CASE("volume_density reproduces Earth's mean density",
          "[enviro][density]") {
    const long double earth_mass = 3.0e-6L;  // ~1 Earth mass in solar masses
    const long double earth_density = 5.52L;  // Earth mean density (g/cc)

    const long double radius_km = volume_radius(earth_mass, earth_density);
    const long double recovered = volume_density(earth_mass, radius_km);
    REQUIRE_THAT(static_cast<double>(recovered),
                 WithinRel(static_cast<double>(earth_density), 1e-9));

    // At fixed mass, a larger radius means a less dense body (density ~ 1/r^3).
    REQUIRE(volume_density(earth_mass, 2.0L * radius_km) <
            volume_density(earth_mass, radius_km));
    // Doubling the radius reduces density by a factor of 8.
    REQUIRE_THAT(
        static_cast<double>(volume_density(earth_mass, 2.0L * radius_km)),
        WithinRel(static_cast<double>(recovered / 8.0L), 1e-9));
}

// ── Molecular-weight retention ──────────────────────────────────────────────
// molecule_limit returns the smallest molecular weight a planet can retain
// against thermal (Jeans) escape, given its mass, radius and exospheric temp.
TEST_CASE("molecule_limit gives a sensible retention threshold for Earth",
          "[enviro][molec_weight]") {
    const long double earth_mass = 3.0e-6L;    // solar masses
    const long double earth_radius = 6371.0L;  // km
    const long double exo_temp = 1273.0L;      // K, Earth-like exosphere

    const long double limit =
        molecule_limit(earth_mass, earth_radius, exo_temp);

    // The retained-weight threshold must be a positive, finite value.
    REQUIRE(limit > 0.0L);
    REQUIRE(std::isfinite(static_cast<double>(limit)));

    // Earth retains heavy gases (N2 ~ 28, O2 ~ 32) but its threshold is well
    // below those; a generous sanity window keeps this stable across constant
    // tweaks while still catching gross regressions.
    REQUIRE(limit < 28.0L);

    // A more massive planet (deeper gravity well) retains *lighter* molecules,
    // so its molecule_limit is lower.
    REQUIRE(molecule_limit(4.0L * earth_mass, earth_radius, exo_temp) < limit);

    // A hotter exosphere drives faster thermal escape, raising the minimum
    // weight that can be retained.
    REQUIRE(molecule_limit(earth_mass, earth_radius, 2.0L * exo_temp) > limit);
}

// ── Greenhouse trigger ──────────────────────────────────────────────────────
// grnhouse(r_ecosphere, orb_radius) decides whether a runaway greenhouse can
// occur: true when the orbit is inside the greenhouse boundary.
TEST_CASE("grnhouse triggers only inside the greenhouse distance",
          "[enviro][greenhouse]") {
    const long double r_ecosphere = 1.0L;  // AU, Sun-like ecosphere radius

    // Close to the star: greenhouse effect is possible.
    REQUIRE(grnhouse(r_ecosphere, 0.5L) == true);
    // Far from the star: too cold for a runaway greenhouse.
    REQUIRE(grnhouse(r_ecosphere, 5.0L) == false);
}

// ── Effective temperature ───────────────────────────────────────────────────
// eff_temp(ecosphere_radius, orb_radius, albedo) is the blackbody-style
// equilibrium temperature scaled by distance and albedo.
TEST_CASE("eff_temp scales with distance and albedo as expected",
          "[enviro][temperature]") {
    const long double r_ecosphere = 1.0L;
    const long double albedo = 0.3L;  // ~Earth's Bond albedo

    // With orb_radius == ecosphere_radius and albedo == EARTH_ALBEDO the
    // formula collapses to EARTH_EFFECTIVE_TEMP exactly.
    const long double t_earth = eff_temp(r_ecosphere, 1.0L, albedo);
    REQUIRE_THAT(static_cast<double>(t_earth),
                 WithinRel(static_cast<double>(EARTH_EFFECTIVE_TEMP), 1e-9));

    // The equilibrium temperature of an Earth-like world is a couple hundred
    // kelvin — comfortably above absolute zero and below the boiling point of
    // water at 1 bar.
    REQUIRE(t_earth > 200.0L);
    REQUIRE(t_earth < 373.0L);

    // Moving the planet outward cools it (T ~ 1/sqrt(orbital radius)).
    REQUIRE(eff_temp(r_ecosphere, 2.0L, albedo) < t_earth);
    // Quadrupling the orbital distance halves the effective temperature.
    REQUIRE_THAT(static_cast<double>(eff_temp(r_ecosphere, 4.0L, albedo)),
                 WithinRel(static_cast<double>(t_earth / 2.0L), 1e-9));

    // A more reflective (higher-albedo) planet absorbs less and is cooler.
    REQUIRE(eff_temp(r_ecosphere, 1.0L, 0.6L) < t_earth);
}

// ── Radiative equilibrium temperature ───────────────────────────────────────
// equilibrium_temp(luminosity, orb_radius, albedo) is the standard
// T_eq = T_EQ_BASE * (1-A)^(1/4) * (L/Lsun)^(1/4) / sqrt(a) — the physically
// correct equilibrium temperature used for gas planets (no Earth-surface bias).
TEST_CASE("equilibrium_temp matches the textbook radiative-equilibrium temperature",
          "[enviro][temperature]") {
    // Earth: L=1 Lsun, a=1 AU, A=0.3 -> ~255 K. (Earth's 288 K mean surface is
    // this equilibrium temperature plus the ~33 K greenhouse effect.)
    const long double t_earth = equilibrium_temp(1.0L, 1.0L, 0.3L);
    REQUIRE_THAT(static_cast<double>(t_earth),
                 WithinRel(static_cast<double>(T_EQ_BASE) * std::pow(0.7, 0.25), 1e-9));
    REQUIRE(t_earth > 250.0L);
    REQUIRE(t_earth < 260.0L);

    // Farther out is cooler; quadrupling the distance halves it (T ~ 1/sqrt(a)).
    REQUIRE(equilibrium_temp(1.0L, 2.0L, 0.3L) < t_earth);
    REQUIRE_THAT(static_cast<double>(equilibrium_temp(1.0L, 4.0L, 0.3L)),
                 WithinRel(static_cast<double>(t_earth / 2.0L), 1e-9));

    // A more luminous star warms the planet; T ~ L^(1/4): 16x luminosity doubles T.
    REQUIRE_THAT(static_cast<double>(equilibrium_temp(16.0L, 1.0L, 0.3L)),
                 WithinRel(static_cast<double>(t_earth * 2.0L), 1e-9));

    // A more reflective (higher-albedo) planet is cooler.
    REQUIRE(equilibrium_temp(1.0L, 1.0L, 0.6L) < t_earth);

    // A realistic gas-dwarf albedo at a habitable-zone distance sits below
    // freezing — the honest equilibrium temperature (greenhouse not included).
    REQUIRE(equilibrium_temp(1.126L, 1.16L, SUB_NEPTUNE_ALBEDO) < FREEZING_POINT_OF_WATER);
}

// ── Lehmer 2020 1-D climate polynomial ───────────────────────────────────────
// lehmer_surface_temp(S, pCO2_bar) returns the full surface temperature (albedo
// + CO2/H2O greenhouse implicit) for rocky worlds in S in [0.35,1.05] S_earth,
// pCO2 in [1e-6,10] bar. It now DRIVES surf_temp for in-domain rocky planets.
TEST_CASE("lehmer_surface_temp is Earth-like and monotone in CO2 and insolation",
          "[enviro][temperature][climate]") {
    // Earth-ish: S=1 S_earth, pCO2 ~ 4e-4 bar (~400 ppm) -> temperate, liquid water.
    const long double t_earth = lehmer_surface_temp(1.0L, 4.0e-4L);
    REQUIRE(t_earth > 270.0L);
    REQUIRE(t_earth < 310.0L);

    // More CO2 -> stronger greenhouse -> warmer (within the valid domain).
    REQUIRE(lehmer_surface_temp(1.0L, 4.0e-3L) > t_earth);

    // More / less insolation -> warmer / cooler.
    REQUIRE(lehmer_surface_temp(1.05L, 4.0e-4L) > t_earth);
    REQUIRE(lehmer_surface_temp(0.40L, 4.0e-4L) < t_earth);
}
