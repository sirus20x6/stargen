// Unit tests for the accretion / orbital math used by the planet
// generation pipeline. These exercise the pure-ish free functions in
// enviro.cpp (orbital zone, density/radius, period, escape velocity,
// mass-luminosity relation) and the seeded RNG helpers in utils.cpp.
//
// All RNG-dependent assertions seed the global generator first so the
// results are deterministic and reproducible.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "const.h"
#include "enviro.h"
#include "utils.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// ── Mass-luminosity relation ───────────────────────────────────────────────
TEST_CASE("luminosity follows the mass-luminosity relation", "[enviro]") {
    // For mass_ratio == 1.0 the exponent is 0.5*(2-1)+4.4 = 4.9 and
    // pow(1.0, 4.9) == 1.0 exactly: a star like the Sun has L == 1.
    REQUIRE_THAT(static_cast<double>(luminosity(1.0L)),
                 WithinAbs(1.0, 1e-9));

    // Luminosity is strictly increasing in mass over the stellar range:
    // a more massive star is brighter.
    REQUIRE(luminosity(0.5L) < luminosity(1.0L));
    REQUIRE(luminosity(1.0L) < luminosity(2.0L));

    // Below one solar mass the star is dimmer than the Sun.
    REQUIRE(luminosity(0.5L) < 1.0L);
    REQUIRE(luminosity(0.5L) > 0.0L);
}

// ── Orbital zone classification ────────────────────────────────────────────
TEST_CASE("orbital_zone partitions the disk into three zones", "[enviro]") {
    // With luminosity == 1.0 the zone boundaries sit at 4*sqrt(1)=4 AU and
    // 15*sqrt(1)=15 AU.
    REQUIRE(orbital_zone(1.0L, 1.0L) == 1);   // inner, well inside 4 AU
    REQUIRE(orbital_zone(1.0L, 10.0L) == 2);  // between 4 and 15 AU
    REQUIRE(orbital_zone(1.0L, 30.0L) == 3);  // beyond 15 AU

    // Boundary behaviour: the comparison is strict "<", so a radius exactly
    // on the inner boundary falls into the *next* zone out.
    REQUIRE(orbital_zone(1.0L, 4.0L) == 2);
    REQUIRE(orbital_zone(1.0L, 15.0L) == 3);

    // A brighter star pushes the zones outward: at 10 AU around an L=16 star
    // (sqrt=4, inner boundary 16 AU) we are still in zone 1.
    REQUIRE(orbital_zone(16.0L, 10.0L) == 1);
}

// ── Volume radius <-> volume density round trip ─────────────────────────────
TEST_CASE("volume_radius and volume_density are inverses", "[enviro]") {
    const long double mass = 3.0e-6L;       // ~1 Earth mass in solar masses
    const long double density = 5.52L;      // Earth's mean density (g/cc)

    const long double radius_km = volume_radius(mass, density);
    REQUIRE(radius_km > 0.0L);

    // Feeding the computed radius back must recover the original density.
    const long double recovered = volume_density(mass, radius_km);
    REQUIRE_THAT(static_cast<double>(recovered),
                 WithinRel(static_cast<double>(density), 1e-9));

    // An Earth-mass, Earth-density body should have roughly Earth's radius
    // (~6371 km); allow a generous tolerance for the simplified model.
    REQUIRE(radius_km > 5000.0L);
    REQUIRE(radius_km < 8000.0L);

    // Denser body at the same mass => smaller radius.
    REQUIRE(volume_radius(mass, 10.0L) < volume_radius(mass, 5.0L));
}

// ── Orbital period (Kepler's third law) ─────────────────────────────────────
TEST_CASE("period reproduces an Earth-like year", "[enviro]") {
    // separation 1 AU, total mass ~= 1 solar mass => one year in days.
    const long double earth_period = period(1.0L, 3.0e-6L, 1.0L);
    REQUIRE_THAT(static_cast<double>(earth_period),
                 WithinAbs(static_cast<double>(DAYS_IN_A_YEAR), 1.0));

    // Larger separation => longer period (Kepler's third law).
    REQUIRE(period(4.0L, 0.0L, 1.0L) > period(1.0L, 0.0L, 1.0L));

    // Around a unit-mass star, doubling nothing but distance by 4x should
    // multiply the period by 4^1.5 = 8.
    const long double p1 = period(1.0L, 0.0L, 1.0L);
    const long double p4 = period(4.0L, 0.0L, 1.0L);
    REQUIRE_THAT(static_cast<double>(p4 / p1), WithinRel(8.0, 1e-9));
}

// ── Escape velocity ─────────────────────────────────────────────────────────
TEST_CASE("escape_vel is positive and scales with mass and radius", "[enviro]") {
    const long double mass = 3.0e-6L;   // ~Earth mass (solar masses)
    const long double radius = 6371.0L; // km

    const long double v = escape_vel(mass, radius);
    REQUIRE(v > 0.0L);

    // v_esc = sqrt(2 G M / R): more mass at fixed radius => faster escape.
    REQUIRE(escape_vel(2.0L * mass, radius) > v);
    // Larger radius at fixed mass => slower escape.
    REQUIRE(escape_vel(mass, 2.0L * radius) < v);

    // Quadrupling mass should multiply escape velocity by exactly 2.
    REQUIRE_THAT(static_cast<double>(escape_vel(4.0L * mass, radius) / v),
                 WithinRel(2.0, 1e-9));
}

// ── Seeded RNG helpers ──────────────────────────────────────────────────────
TEST_CASE("random_number stays within bounds and is reproducible", "[utils]") {
    seed_random(42u);
    for (int i = 0; i < 1000; ++i) {
        const long double r = random_number(-3.0L, 7.0L);
        REQUIRE(r >= -3.0L);
        REQUIRE(r <= 7.0L);
    }

    // Re-seeding with the same value reproduces the identical sequence.
    seed_random(1234u);
    const long double a0 = random_number(0.0L, 1.0L);
    const long double a1 = random_number(0.0L, 1.0L);
    seed_random(1234u);
    const long double b0 = random_number(0.0L, 1.0L);
    const long double b1 = random_number(0.0L, 1.0L);
    REQUIRE(a0 == b0);
    REQUIRE(a1 == b1);
}

TEST_CASE("about varies symmetrically around a center", "[utils]") {
    seed_random(7u);
    const long double center = 25.0L;
    const long double variation = 0.4L;
    for (int i = 0; i < 1000; ++i) {
        const long double v = about(center, variation);
        REQUIRE(v >= center - variation);
        REQUIRE(v <= center + variation);
    }
}

TEST_CASE("random_eccentricity returns a valid orbital eccentricity", "[utils]") {
    seed_random(99u);
    for (int i = 0; i < 1000; ++i) {
        const long double e = random_eccentricity();
        // Eccentricity must lie in [0, 1) for a bound (elliptical) orbit.
        REQUIRE(e >= 0.0L);
        REQUIRE(e < 1.0L);
    }
}
