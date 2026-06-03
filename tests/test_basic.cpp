// Basic smoke test verifying the test harness and that stargen_core's
// headers compile and link against the test binary.
#include <catch2/catch_test_macros.hpp>

#include "const.h"

TEST_CASE("PI constant from const.h is approximately 3.14159", "[const]") {
    // const.h (inherited from the accrete/starform lineage) defines PI.
    REQUIRE(PI > 3.14159);
    REQUIRE(PI < 3.14160);
}
