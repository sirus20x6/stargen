cmake_minimum_required(VERSION 3.20)
project(stargen VERSION 3.0.0.0 DESCRIPTION "A program for generating solar systems")

# Assuming GIT_VERSION is set externally (e.g., by your CI, a FindGit.cmake module, or environment variable)
# If GIT_VERSION is meant to be the primary version for your project,
# you might want to use it in the project() command directly, e.g. project(stargen VERSION ${GIT_VERSION})
# Ensure GIT_VERSION is actually populated, otherwise it will be empty.
set(my_project_VERSION ${GIT_VERSION})
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}") # This will be 3.0.0.0
message(STATUS "CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}") # This will be 3.0.0.0

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# The following line is redundant if the one above is set, but harmless.
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) # Explicitly enable compiler-specific extensions.

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

cmake_host_system_information(RESULT logical QUERY NUMBER_OF_LOGICAL_CORES)
cmake_host_system_information(RESULT physical QUERY NUMBER_OF_PHYSICAL_CORES)

message(STATUS "NUMBER_OF_LOGICAL_CORES: ${logical}")
message(STATUS "NUMBER_OF_PHYSICAL_CORES: ${physical}")
# Sets the default parallelism for `cmake --build . --parallel` if no explicit number is given.
set(CMAKE_BUILD_PARALLEL_LEVEL ${logical})

include(FetchContent)

# Fetch Tracy content
# Consider using a specific GIT_TAG for reproducibility, instead of master.zip
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.10 # Example: Use the latest tag (as of late 2023) or a specific commit
    # OLD: URL https://github.com/wolfpld/tracy/archive/master.zip
)
FetchContent_MakeAvailable(tracy) # This populates tracy_SOURCE_DIR, tracy_BINARY_DIR and defines targets like TracyClient

# Fetch nlohmann json
cmake_policy(SET CMP0135 NEW) # Important for FetchContent with nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json) # Defines nlohmann_json::nlohmann_json

# Add yaml-cpp dependency
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.7.0 # Good: specific tag
)
FetchContent_MakeAvailable(yaml-cpp) # Defines yaml-cpp target

add_executable(stargen
    accrete.cpp
    andromeda.cpp
    andromeda2.cpp
    display.cpp
    dole.cpp
    elements.cpp
    enviro.cpp
    gas_radius_helpers.cpp
    ic3094.cpp
    ic3094_2.cpp
    jimb.cpp
    main.cpp
    omega_galaxy.cpp
    omega_galaxy2.cpp
    Planetary_Habitability_Laboratory.cpp
    planets.cpp
    radius_tables.cpp
    ring_universe.cpp
    solid_radius_helpers.cpp
    solstation.cpp
    stargen.cpp
    star_temps.cpp
    star_trek.cpp
    structs.cpp
    utils.cpp
    PerformanceMonitor.cpp
    # DO NOT add TracyClient.cpp here. It's built as part of the TracyClient library.
    # REMOVED: _deps/tracy-src/public/TracyClient.cpp
)

# Define Tracy enabling macro - This is correct for enabling Tracy
target_compile_definitions(stargen PRIVATE TRACY_ENABLE)

# This line includes ${CMAKE_SOURCE_DIR}/tracy.
# If you have project-specific files in a "tracy" folder at your source root, keep this.
# If this was an attempt to help find the *fetched* Tracy library, it's not the correct way and
# would be superseded by linking TracyClient. Preserving it in case it's for unrelated local headers.
target_include_directories(stargen PRIVATE ${CMAKE_SOURCE_DIR}/tracy)

# Link libraries
target_link_libraries(stargen PRIVATE
    TracyClient                     # ADDED: This links the Tracy library and correctly sets up its include paths.
    nlohmann_json::nlohmann_json
    m                               # Standard math library
    yaml-cpp
)
# REMOVED: target_include_directories(stargen PRIVATE ${tracy_SOURCE_DIR})
# Linking TracyClient handles include directories automatically and correctly.

# Set the binary output directory to the source directory
set_target_properties(stargen PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Install commands are commented out, as per your existing file.
# install(TARGETS stargen RUNTIME DESTINATION bin)
# install(DIRECTORY html ref DESTINATION bin)