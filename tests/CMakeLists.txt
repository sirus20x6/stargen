# Unit tests for stargen, built against the stargen_core static library.
add_executable(stargen_tests
    test_basic.cpp
    enviro_test.cpp
    determinism_threads_test.cpp
    elements_error_test.cpp
    units_test.cpp
)

target_link_libraries(stargen_tests PRIVATE
    stargen_core
    Catch2::Catch2WithMain
)

# The determinism test shells out to the built stargen binary; give it the path.
# It also depends on the binary existing at test time.
target_compile_definitions(stargen_tests PRIVATE
    STARGEN_BIN="$<TARGET_FILE:stargen>"
    STARGEN_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    STARGEN_TEST_WORKDIR="${CMAKE_CURRENT_BINARY_DIR}"
)
add_dependencies(stargen_tests stargen)

# Make Catch2's contributed CMake helpers (catch_discover_tests) available.
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(stargen_tests)

# ------------------------------------------------------------------
# Golden-output regression harness.
#
# Runs the stargen binary for a few fixed seeds, normalizes volatile
# fields (timestamps, absolute paths, version strings, CRLF), and diffs
# the result against committed goldens in tests/golden/. Regenerate the
# goldens after an intentional output change by invoking the driver
# directly with the update flag (ctest does not reliably forward env vars
# to a test's child process):
#   cmake -DSTARGEN_UPDATE_GOLDENS=1 \
#         -DSTARGEN_BIN=$PWD/stargen \
#         -DGOLDEN_DIR=$PWD/tests/golden \
#         -DSEEDS="12345;42;7" \
#         -P tests/golden_regression.cmake
# ------------------------------------------------------------------
set(STARGEN_GOLDEN_SEEDS "12345;42;7")

add_test(
    NAME golden_regression
    COMMAND ${CMAKE_COMMAND}
        -DSTARGEN_BIN=$<TARGET_FILE:stargen>
        -DGOLDEN_DIR=${CMAKE_CURRENT_SOURCE_DIR}/golden
        -DSEEDS=${STARGEN_GOLDEN_SEEDS}
        -DWORKING_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/golden_regression.cmake
)

# golden_regression byte-compares ~20-significant-digit stargen output against
# goldens baked with one specific toolchain. Floating-point codegen differs
# across compiler / glibc / -march, so the goldens are NOT portable to CI
# runners (they fail there even in Release). Label it "local-only" so CI can
# skip it with `ctest --label-exclude local-only`, while verify.sh and the
# pre-commit hook still run it under the canonical local toolchain.
set_tests_properties(golden_regression PROPERTIES LABELS "local-only")

# ------------------------------------------------------------------
# Population-validation harness (diagnostic).
#
# scripts/validate_population.py generates a population of systems and reports
# StarGen's architecture statistics against modern Kepler/CKS targets (peas-in-a-
# pod size correlation, Hill spacing, period ratios, eccentricity, mutual
# inclination, occurrence). It is a *diagnostic*, so the ctest only enforces
# loose gross-sanity bounds (--check); statistical drift is expected and is for
# humans to read, not for CI to fail on. Labeled local-only: needs python3 and is
# not a portable pass/fail physics test. Run the full report with:
#   python3 scripts/validate_population.py --seeds 120
# ------------------------------------------------------------------
find_program(STARGEN_PYTHON3 NAMES python3)
if(STARGEN_PYTHON3)
    add_test(
        NAME population_validation
        COMMAND ${STARGEN_PYTHON3}
            ${CMAKE_SOURCE_DIR}/scripts/validate_population.py
            --bin $<TARGET_FILE:stargen>
            --seeds 40
            --check
    )
    set_tests_properties(population_validation PROPERTIES LABELS "local-only")
endif()
