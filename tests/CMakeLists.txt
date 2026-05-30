# Unit tests for stargen, built against the stargen_core static library.
add_executable(stargen_tests
    test_basic.cpp
)

target_link_libraries(stargen_tests PRIVATE
    stargen_core
    Catch2::Catch2WithMain
)

# Make Catch2's contributed CMake helpers (catch_discover_tests) available.
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(stargen_tests)
