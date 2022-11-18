project(stargen_2014 DESCRIPTION "A program for generating solar systems")

cmake_minimum_required(VERSION 3.11)
project(stargen VERSION 3.11.0)
#set(CMAKE_PROJECT_VERSION = 3.4.5)
#set(CMAKE_PROJECT_VERSION_MAJOR 3)
#    CMAKE_PROJECT_VERSION_MINOR
#    CMAKE_PROJECT_VERSION_PATCH
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GSL_LIBRARIES})
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

include(FetchContent)

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
    utils.cpp)

target_link_libraries(stargen PRIVATE ${LIBS})
target_link_libraries(stargen PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(stargen PRIVATE m)

install(TARGETS stargen RUNTIME DESTINATION bin)
install(DIRECTORY html ref DESTINATION bin)
