cmake_minimum_required(VERSION 3.20)
project(stargen VERSION 3.0.0.0 DESCRIPTION "A program for generating solar systems")

set(my_project_VERSION ${GIT_VERSION})
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
message(STATUS "CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

cmake_host_system_information(RESULT logical QUERY NUMBER_OF_LOGICAL_CORES)
cmake_host_system_information(RESULT physical QUERY NUMBER_OF_PHYSICAL_CORES)

message(STATUS "NUMBER_OF_LOGICAL_CORES: ${logical}")
message(STATUS "NUMBER_OF_PHYSICAL_CORES: ${physical}")
set(CMAKE_BUILD_PARALLEL_LEVEL ${logical})

include(FetchContent)

# Fetch Tracy content
FetchContent_Declare(
    tracy
    URL https://github.com/wolfpld/tracy/archive/master.zip
)
FetchContent_MakeAvailable(tracy)

# Fetch nlohmann json
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.1/json.tar.xz)
FetchContent_MakeAvailable(json)

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
    _deps/tracy-src/public/TracyClient.cpp
)

# Include Tracy directories
target_include_directories(stargen PRIVATE ${tracy_SOURCE_DIR})

# Define Tracy enabling macro
target_compile_definitions(stargen PRIVATE TRACY_ENABLE)
target_include_directories(stargen PRIVATE ${CMAKE_SOURCE_DIR}/tracy)

# Link libraries
target_link_libraries(stargen PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(stargen PRIVATE m)

# Set the binary output directory to the source directory
set_target_properties(stargen PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Remove install commands since the binary will be in the source directory
# install(TARGETS stargen RUNTIME DESTINATION bin)
# install(DIRECTORY html ref DESTINATION bin)
