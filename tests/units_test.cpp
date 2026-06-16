// Foundation test for the `au`-based unit vocabulary (units.hh).
//
// These do not touch generation output. They (1) prove the vendored au library
// and StarGen's unit vocabulary compile and link under the project toolchain,
// and (2) pin the bit-identity relationships the migration relies on: integer/
// power-of-ten conversions are byte-identical to StarGen's literals, while the
// rational mmHg factor differs by ~1 ULP (a documented, deliberate rebaseline
// site). If a future au upgrade changes these, the test fails loudly.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "const.h"
#include "units.hh"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// AU -> cm is an exact integer factor (149,597,870,700 m), so au reproduces
// StarGen's CM_PER_AU = 1.495978707e13 bit-for-bit.
TEST_CASE("au AU->cm equals CM_PER_AU bit-for-bit", "[units]") {
    const long double au_cm = au::astronomical_units(1.0L).in(sgu::centimeters);
    REQUIRE(au_cm == static_cast<long double>(CM_PER_AU));
}

// mmHg -> mbar uses the exact rational 101325/76000; StarGen multiplies by the
// rounded factor (1013.25/760). They agree to ~1 ULP -- this is the documented
// rebaseline site, asserted within a tight relative tolerance (not exact).
TEST_CASE("au mmHg->mbar matches MMHG_TO_MILLIBARS within ~1 ULP", "[units]") {
    const double au_mb = static_cast<double>(sgu::mm_hg(1.0L).in(sgu::millibars));
    REQUIRE_THAT(au_mb,
                 WithinRel(static_cast<double>(MMHG_TO_MILLIBARS), 1e-15));
    // 760 mmHg == 1 standard atmosphere == 1013.25 mbar.
    REQUIRE_THAT(static_cast<double>(sgu::mm_hg(760.0L).in(sgu::millibars)),
                 WithinAbs(1013.25, 1e-9));
}

// Smoke test: the custom exact units extract the expected raw long double.
TEST_CASE("au custom units extract expected raw values", "[units]") {
    REQUIRE(sgu::atmospheres(1.0L).in(au::pascals) == 101325.0L);
    // 1e5 dyne == 1 N exactly.
    REQUIRE(sgu::dynes(1.0e5L).in(au::newtons) == 1.0L);
    // EARTH_SURF_PRES_IN_MILLIBARS is 1013.25 mb; express it back in atmospheres.
    REQUIRE_THAT(static_cast<double>(
                     sgu::millibars(static_cast<long double>(
                                        EARTH_SURF_PRES_IN_MILLIBARS))
                         .in(sgu::atmospheres)),
                 WithinAbs(1.0, 1e-9));
}
