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
)
add_dependencies(stargen_tests stargen)

# Make Catch2's contributed CMake helpers (catch_discover_tests) available.
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(stargen_tests)
