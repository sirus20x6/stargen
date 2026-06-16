cmake_minimum_required(VERSION 3.5...4.99)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(stargen VERSION 3.0.0.0 DESCRIPTION "A program for generating solar systems")

# The output layer (PlanetRow.h) uses C++26 static reflection (P2996) + member
# annotations (P3394), which currently require GCC >= 16 with -std=c++26
# -freflection. Mainline clang does not implement reflection yet, so the clang
# and Windows CI lanes were dropped. Fail fast on an unsupported compiler.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  message(FATAL_ERROR "StarGen requires GCC >= 16 for C++26 static reflection; found "
                      "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
endif()

# Assuming GIT_VERSION is set externally (e.g., by your CI, a FindGit.cmake module, or environment variable)
# If GIT_VERSION is meant to be the primary version for your project,
# you might want to use it in the project() command directly, e.g. project(stargen VERSION ${GIT_VERSION})
# Ensure GIT_VERSION is actually populated, otherwise it will be empty.
set(my_project_VERSION ${GIT_VERSION})
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
message(STATUS "CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# The following line is redundant if the one above is set, but harmless.
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) # Explicitly enable compiler-specific extensions.

# Optional sanitizers for finding memory/UB/data-race bugs. OFF by default so
# normal builds and the pre-commit hook are unaffected. Use a separate build dir:
#   cmake -B build-asan -DSANITIZE=address && cmake --build build-asan --target stargen
#   cmake -B build-tsan -DSANITIZE=thread   && cmake --build build-tsan --target stargen
set(SANITIZE "" CACHE STRING "Enable sanitizers: address | thread | undefined (default off)")
if(SANITIZE)
  if(SANITIZE STREQUAL "address")
    set(_san_flags -fsanitize=address,undefined -fno-omit-frame-pointer -g)
  elseif(SANITIZE STREQUAL "thread")
    set(_san_flags -fsanitize=thread -fno-omit-frame-pointer -g)
  elseif(SANITIZE STREQUAL "undefined")
    set(_san_flags -fsanitize=undefined -fno-omit-frame-pointer -g)
  else()
    message(FATAL_ERROR "Unknown SANITIZE='${SANITIZE}' (use address|thread|undefined)")
  endif()
  add_compile_options(${_san_flags})
  add_link_options(${_san_flags})
  message(STATUS "Sanitizers enabled: ${SANITIZE}")
endif()

# Where the executables land. Normal builds drop them in the source dir for
# convenience (ctest, the golden test, and the pre-commit hook all run the
# source-tree ./stargen). A sanitizer build, however, must NOT overwrite that
# binary -- an ASan-instrumented ./stargen makes the normal ctest fail -- so
# sanitizer builds keep their executables inside their own build dir instead.
if(SANITIZE)
  set(STARGEN_BIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}")
else()
  set(STARGEN_BIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}")
endif()

# Tracy is a profiler, gated behind ENABLE_TRACY so production / release / CI
# builds can ship without it: it spawns profiler worker threads, opens a listen
# socket, and adds runtime overhead (README advises disabling it for production).
# Default OFF -- opt in for local profiling with -DENABLE_TRACY=ON.
#
# Tracy is ALSO forced off under any sanitizer build regardless of ENABLE_TRACY:
# its bundled rpmalloc allocator recycles memory spans across threads without
# synchronization that ThreadSanitizer observes, and its worker threads touch
# shared globals -- both flood TSan with false positives that drown out real
# data races.
#
# When Tracy is off, the fetched Tracy CMake target becomes a stub (no profiler
# threads, no rpmalloc) and we do not define TRACY_ENABLE for our code, so the
# Tracy.hpp macros (ZoneScoped, etc.) expand to nothing. TracyClient is still
# linked either way so its headers remain on the include path.
option(ENABLE_TRACY "Build with Tracy profiler instrumentation" OFF)
if(SANITIZE OR NOT ENABLE_TRACY)
  set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)   # affects the fetched Tracy build
  set(STARGEN_TRACY_DEFINES "")               # no TRACY_ENABLE for our code
else()
  set(STARGEN_TRACY_DEFINES TRACY_ENABLE)
endif()

# The real-time viewer needs raylib, which pulls in GLFW + a display backend
# (X11/Wayland/GL) at *configure* time. Headless CI and the generator/test
# targets need none of that, so make the viewer opt-out. Default ON preserves
# the local developer experience; CI configures with -DSTARGEN_BUILD_VIEWER=OFF.
option(STARGEN_BUILD_VIEWER "Build the raylib-based real-time viewer" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

# The standard math library is a separate link target on Linux/BSD (libm) but is
# folded into the C runtime on Windows and macOS. Linking "m" explicitly on
# Windows makes lld-link search for a nonexistent m.lib and fail; only reference
# it on platforms where it is a real library.
if(WIN32)
  set(STARGEN_MATH_LIB "")
else()
  set(STARGEN_MATH_LIB m)
endif()

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

# Add raylib dependency for viewer (only when building the viewer; raylib's
# configure step requires a display backend that headless CI lacks).
if(STARGEN_BUILD_VIEWER)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
  )
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Don't build raylib examples
  FetchContent_MakeAvailable(raylib) # Defines raylib target
endif()

# Add Catch2 v3 dependency for unit testing
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4 # Specific released tag
)
FetchContent_MakeAvailable(Catch2) # Defines Catch2::Catch2 and Catch2::Catch2WithMain

# Enable CTest-based testing
enable_testing()

# ============================================================================
# stargen_core: STATIC library of all stargen sources except main.cpp
# ============================================================================
# The stargen executable and the test binary both link this library, so the
# generation/catalog logic is compiled once and exercised identically by tests.
add_library(stargen_core STATIC
    accrete.cpp
    andromeda.cpp
    display.cpp
    dole.cpp
    elements.cpp
    enviro.cpp
    gas_radius_helpers.cpp
    ic3094.cpp
    jimb.cpp
    omega_galaxy.cpp
    Planetary_Habitability_Laboratory.cpp
    planets.cpp
    radius_tables.cpp
    ring_universe.cpp
    solid_radius_helpers.cpp
    solstation.cpp
    StarGenerator.cpp
    stargen.cpp
    star_temps.cpp
    star_trek.cpp
    structs.cpp
    utils.cpp
    PerformanceMonitor.cpp
    OrbitalSimulator.cpp       # NEW: Orbital mechanics simulator (optional, for viewer)
    OrbitalStateManager.cpp    # NEW: Multi-system orbital state management (optional, for viewer)
    catalog_loader.cpp         # runtime loader for the data/*.tsv predefined-star catalogues
    # DO NOT add TracyClient.cpp here. It's built as part of the TracyClient library.
)

# ----------------------------------------------------------------------------
# Generated star-catalogue translation units: enormous machine-style data
# tables, not first-party logic. Compile them with -w so their machine-generated
# noise stays out of the -Wall/-Wextra first-party warning set added below.
# COMPILE_OPTIONS is a single per-source list emitted after CMAKE_CXX_FLAGS_*, so
# -w overrides the target's -Wall for just these files. (gcc compiles these huge
# dynamic initializers fine; the former clang -O0 workaround was dropped with the
# clang lane.)
set(STARGEN_CATALOG_SOURCES
    andromeda.cpp
    ic3094.cpp
    jimb.cpp
    omega_galaxy.cpp
    Planetary_Habitability_Laboratory.cpp
    ring_universe.cpp
    solstation.cpp
    star_trek.cpp
)
set_source_files_properties(
    ${STARGEN_CATALOG_SOURCES}
    PROPERTIES COMPILE_OPTIONS "-w"
)

# display.cpp is the sole consumer of C++26 static reflection (PlanetRow.h's
# reflective JSON/CSV emitters). -freflection is REQUIRED and is NOT implied by
# -std=c++26, so enable it for just this translation unit (keeping it off the
# catalogue TUs and the FetchContent dependencies).
set_source_files_properties(
    display.cpp
    PROPERTIES COMPILE_OPTIONS "-freflection"
)

target_compile_definitions(stargen_core PUBLIC ${STARGEN_TRACY_DEFINES})
target_include_directories(stargen_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tracy
    ${CMAKE_SOURCE_DIR}/third_party   # vendored Aurora "au" units library (au/au.hh)
)
target_link_libraries(stargen_core PUBLIC
    TracyClient                     # Links the Tracy library and sets up its include paths.
    nlohmann_json::nlohmann_json
    ${STARGEN_MATH_LIB}             # Standard math library (libm; empty on Windows/macOS)
    yaml-cpp
)

# First-party warning flags: -Wall/-Wextra on our own translation units. The
# generated catalogue TUs are excluded via -w above. NOT -Werror -- warnings are
# for visibility and must not break the build. PRIVATE so they don't leak into
# the test target (which compiles Catch2) or other consumers. Guarded to the
# GNU/Clang frontends (the only compilers this project builds with; the Windows
# lane uses clang++).
target_compile_options(stargen_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra>)

# stargen executable = main.cpp + stargen_core
add_executable(stargen
    main.cpp
)

# Define Tracy enabling macro (empty in sanitizer builds; see STARGEN_TRACY_DEFINES)
target_compile_definitions(stargen PRIVATE ${STARGEN_TRACY_DEFINES})

# This line includes ${CMAKE_SOURCE_DIR}/tracy.
target_include_directories(stargen PRIVATE ${CMAKE_SOURCE_DIR}/tracy)

# Link libraries
target_link_libraries(stargen PRIVATE
    stargen_core
)

# First-party warnings for main.cpp too (see stargen_core note above).
target_compile_options(stargen PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra>)

# Set the binary output directory to the source directory
set_target_properties(stargen PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${STARGEN_BIN_OUTPUT_DIR}"
)

# Unit tests
add_subdirectory(tests)

# Install commands are commented out, as per your existing file.
# install(TARGETS stargen RUNTIME DESTINATION bin)
# install(DIRECTORY html ref DESTINATION bin)

# ============================================================================
# Optional: Orbital Simulator Demo
# ============================================================================
# This demo shows OrbitalSimulator working alongside existing generation code
# without modifying it. Build with: cmake --build . --target orbital_demo

add_executable(orbital_demo
    orbital_simulator_demo.cpp
    OrbitalSimulator.cpp
    OrbitalStateManager.cpp
    StarGenerator.cpp
    catalog_loader.cpp
    stargen.cpp
    accrete.cpp
    display.cpp
    dole.cpp
    elements.cpp
    enviro.cpp
    gas_radius_helpers.cpp
    planets.cpp
    radius_tables.cpp
    solid_radius_helpers.cpp
    star_temps.cpp
    structs.cpp
    utils.cpp
    PerformanceMonitor.cpp
    # Include catalog files needed by generation
    andromeda.cpp
    ic3094.cpp
    jimb.cpp
    omega_galaxy.cpp
    Planetary_Habitability_Laboratory.cpp
    ring_universe.cpp
    solstation.cpp
    star_trek.cpp
)

target_compile_definitions(orbital_demo PRIVATE ${STARGEN_TRACY_DEFINES})
target_include_directories(orbital_demo PRIVATE ${CMAKE_SOURCE_DIR}/tracy)
target_link_libraries(orbital_demo PRIVATE
    TracyClient
    nlohmann_json::nlohmann_json
    ${STARGEN_MATH_LIB}
    yaml-cpp
)

set_target_properties(orbital_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${STARGEN_BIN_OUTPUT_DIR}"
)

# ============================================================================
# Real-time Stellar System Viewer
# ============================================================================
# Interactive viewer for visualizing generated stellar systems in real-time
# with orbital motion. Build with: cmake --build . --target viewer
# Gated on STARGEN_BUILD_VIEWER so headless builds need no raylib/display deps.

if(STARGEN_BUILD_VIEWER)
add_executable(viewer
    viewer.cpp
    OrbitalSimulator.cpp
    OrbitalStateManager.cpp
    StarGenerator.cpp
    catalog_loader.cpp
    stargen.cpp
    accrete.cpp
    display.cpp
    dole.cpp
    elements.cpp
    enviro.cpp
    gas_radius_helpers.cpp
    planets.cpp
    radius_tables.cpp
    solid_radius_helpers.cpp
    star_temps.cpp
    structs.cpp
    utils.cpp
    PerformanceMonitor.cpp
    # Include catalog files needed by generation
    andromeda.cpp
    ic3094.cpp
    jimb.cpp
    omega_galaxy.cpp
    Planetary_Habitability_Laboratory.cpp
    ring_universe.cpp
    solstation.cpp
    star_trek.cpp
)

target_compile_definitions(viewer PRIVATE ${STARGEN_TRACY_DEFINES})
target_include_directories(viewer PRIVATE ${CMAKE_SOURCE_DIR}/tracy)
target_link_libraries(viewer PRIVATE
    TracyClient
    nlohmann_json::nlohmann_json
    ${STARGEN_MATH_LIB}
    yaml-cpp
    raylib
)

set_target_properties(viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${STARGEN_BIN_OUTPUT_DIR}"
)
endif()  # STARGEN_BUILD_VIEWER
