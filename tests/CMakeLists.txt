# Unit tests for stargen, built against the stargen_core static library.
add_executable(stargen_tests
    test_basic.cpp
    enviro_test.cpp
    determinism_threads_test.cpp
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
