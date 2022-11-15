project(stargen_2014)

cmake_minimum_required(VERSION 3.9)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GSL_LIBRARIES})
find_package(Boost 1.79.0) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()

add_executable(stargen accrete.cpp andromeda.cpp andromeda2.c display.cpp dole.cpp elements.cpp enviro.cpp gas_radius_helpers.cpp ic3094.cpp ic3094_2.c jimb.cpp main.cpp omega_galaxy.cpp omega_galaxy2.c Planetary_Habitability_Laboratory.cpp planets.cpp radius_tables.cpp ring_universe.cpp ring_universe2.c solid_radius_helpers.cpp solstation.cpp stargen.cpp star_temps.cpp star_trek.cpp structs.cpp utils.cpp)

target_link_libraries(stargen ${LIBS})

install(TARGETS stargen RUNTIME DESTINATION bin)
install(DIRECTORY html ref DESTINATION bin)
