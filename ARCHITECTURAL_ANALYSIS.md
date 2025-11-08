# StarGen Architectural Analysis & Modernization Report

**Date:** 2025-11-07
**Purpose:** Identify architectural improvements needed to modernize the codebase and prepare for n-body simulation with visualization
**Codebase Size:** ~467,000 lines of C/C++ code (including large star catalog files)

---

## Executive Summary

StarGen is a scientifically-grounded planetary system generator with a 45+ year lineage (1970s Fortran → 1988 C → modern C++23). While recent modernization efforts have begun (JSON export, Boost removal, C++23 adoption), the codebase still exhibits significant technical debt from its multi-decade evolution. Major architectural improvements are needed before implementing n-body physics and visualization capabilities.

**Critical Findings:**
- **50+ global variables** create tight coupling and prevent thread safety
- **Raw pointer-based linked lists** throughout (265+ occurrences) with manual memory management
- **196 raw `new` calls** vs only 17 `delete` calls (potential memory leaks)
- **Zero use of smart pointers** (std::unique_ptr/shared_ptr)
- **"using namespace std"** in 20+ files (namespace pollution)
- **Monolithic functions** mixing business logic, I/O, and presentation
- **Massive generated files** (41MB source files!) make analysis/compilation slow

---

## 1. Current Architecture Overview

### 1.1 Core Components

```
┌─────────────────────────────────────────────────────────┐
│                    main.cpp (CLI)                        │
│              - Argument parsing (550+ lines)             │
│              - Global variable initialization            │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│              stargen.cpp (Orchestrator)                  │
│              - Generation loop                           │
│              - Filtering logic                           │
│              - Heavy use of global state                 │
└────┬───────────────┬───────────────┬────────────────────┘
     │               │               │
┌────▼──────┐  ┌────▼──────┐  ┌────▼──────────────────┐
│  accrete  │  │  enviro   │  │  display              │
│  (Dust    │  │ (Planet   │  │ (Multi-format output) │
│  accretion)│  │  physics) │  │  - HTML/JSON/CSV      │
└───────────┘  └───────────┘  └───────────────────────┘
```

### 1.2 Data Flow Issues

**Current:** Tight coupling through global variables
```cpp
// stargen.h - 50+ global variables!
extern int flag_verbose;
extern bool allow_planet_migration;
extern long double compainion_mass_arg;
extern sun the_sun_clone;
extern int total_earthlike;
// ... 40+ more
```

This architecture makes it **impossible** to:
- Run multiple simulations in parallel
- Unit test components in isolation
- Reuse code as a library
- Add thread-safe n-body calculations

---

## 2. Critical Architectural Problems

### 2.1 Memory Management Crisis

**Problem:** Manual memory management with raw pointers and linked lists

**Evidence:**
```cpp
// structs.h - C-style linked lists everywhere
class planet {
    planet *first_moon;       // Raw pointer to moon list
    planet *next_planet;      // Raw pointer to sibling
    planet *reconnect_to;     // Raw pointer for reconnection
    // ...
};

class dust {
    dust *next_band;          // Raw pointer to next dust band
};

// accrete.cpp - Manual allocation without smart pointers
dust_head = new dust();
dust_head->next_band = nullptr;
```

**Impact:**
- 196 `new` allocations vs only 17 `delete` calls → **memory leaks**
- No RAII patterns
- Difficult to reason about ownership
- Crash-prone when exceptions occur

**Modern Solution:**
```cpp
// Should be:
class planet {
    std::unique_ptr<planet> first_moon;
    std::unique_ptr<planet> next_planet;
    // Or better: std::vector<std::unique_ptr<planet>> moons;
};
```

### 2.2 Global Variable Pollution

**Count:** 50+ global variables in stargen.h alone

**Categories:**
1. **Configuration:** `flag_verbose`, `decimals_arg`, `max_age`, `min_age`
2. **State:** `dust_left`, `planet_head`, `dust_head` (in accrete class, but class members as globals)
3. **Statistics:** `total_earthlike`, `total_habitable`, `total_worlds`
4. **Clones:** `the_sun_clone`, `flags_arg_clone` (why?!)

**Problems:**
- Thread safety impossible
- Hidden dependencies
- Testing requires global state setup
- Reentrancy impossible

### 2.3 Namespace Pollution

**Problem:** `using namespace std;` in 20+ files

**Example:**
```cpp
// main.cpp, stargen.h, enviro.cpp, etc.
using namespace std;
```

**Risk:** Name collisions, especially problematic for:
- Future C++ standard library additions
- Third-party libraries
- Your own `distance()`, `generate()`, etc. functions

### 2.4 Monolithic Functions

**Example:** `main.cpp` argument parsing (350+ lines in one function)
```cpp
// main.cpp:109-363 - Single massive loop parsing all arguments
for (int i = 0; i < argc; i++) {
    if (compare_string_char(temp_string, 1, "PHL", 3)) {
        // ... 20 lines
    } else if (compare_string_char(temp_string, 1, "sn", 2)) {
        // ... more cases
    } // ... 40+ more else-if branches
}
```

**Should be:** Command pattern or options parsing library (e.g., CLI11, cxxopts)

### 2.5 Tight Coupling

**Problem:** Direct dependencies between all subsystems

```cpp
// accrete.cpp depends on:
#include "planets.h"    // For predefined planet data
#include "stargen.h"    // For global flags
#include "utils.h"      // For utilities

// enviro.cpp depends on:
#include "stargen.h"    // For global the_sun_clone
#include "elements.h"   // For gas table
#include "display.h"    // For breathability_phrase!?
```

No clear separation between:
- Domain logic (physics calculations)
- Data access (star catalogs)
- Presentation (output formatting)

---

## 3. Code Quality Issues

### 3.1 Mixed Programming Paradigms

The codebase is a mix of:
- **K&R C** (1978): Manual memory, C-style strings
- **ANSI C** (1989): Function prototypes
- **C++98**: Classes, some OOP
- **C++11/14**: `auto`, `nullptr`, range-based for
- **C++23**: constexpr functions, structured bindings

**Example of inconsistency:**
```cpp
// Old C-style (const.h)
#define PROTOPLANET_MASS 1.0E-15

// Modern C++ (const.h - same file!)
constexpr double EARTH_MASS_IN_GRAMS = 5.9742E27;

// C-style arrays
string breathability_phrase[4] = {"none", ...};

// Modern C++ containers
vector<Chemical> chemicals;
```

### 3.2 Data Structure Problems

**Current:** Manual linked lists everywhere
```cpp
planet *p = planet_head;
while (p != nullptr) {
    // Process planet
    p = p->next_planet;  // 265+ of these traversals!
}
```

**Should be:** STL containers
```cpp
std::vector<std::unique_ptr<Planet>> planets;
for (auto& planet : planets) {
    // Process planet - safe, fast, clear
}
```

**Benefits:**
- Automatic memory management
- Cache-friendly (contiguous memory)
- Standard algorithms (sort, find, etc.)
- Iterator safety
- Size tracking

### 3.3 Massive Generated Files

**Problem:** Star catalog files are HUGE

| File | Size | Stars | LOC |
|------|------|-------|-----|
| omega_galaxy2.cpp | 41 MB | 307,283 | ~1M+ lines |
| ring_universe.cpp | 7.9 MB | 60,001 | ~200K lines |
| ic3094_2.cpp | 719 KB | 5,001 | ~15K lines |

**Impact:**
- Slow compilation (even with parallel builds)
- Difficult to analyze with tools
- Git history bloat
- Memory consumption

**Solution:** Move to binary data files or databases
```cpp
// Current: Hardcoded in source
void initOmegaGalaxy() {
    omega_galaxy.addStar(star(0.89, 0.5, ...));
    omega_galaxy.addStar(star(0.91, 0.6, ...));
    // ... 307,283 times!
}

// Should be: Load from binary file
catalog omega_galaxy = CatalogLoader::fromBinary("omega_galaxy2.bin");
```

### 3.4 Lack of Error Handling

**Problem:** Direct use of raw pointers without null checks

```cpp
// accrete.cpp - No null checks before dereferencing
current_dust_band = dust_head;
while (current_dust_band != nullptr &&  // Only checked in loop condition
       current_dust_band->getOuterEdge() < inside_range) {
    current_dust_band = current_dust_band->next_band;
}
// What if dust_head was never initialized?
```

**Limited use of:**
- Exception handling
- Result types (std::optional)
- Assertions/preconditions
- Defensive programming

---

## 4. Modernization Opportunities

### 4.1 Object-Oriented Refactoring

**Current:** Procedural with some classes
```cpp
// accrete.h - Class is just a namespace for functions
class accrete {
public:
    auto dist_planetary_masses(...) -> planet *;
private:
    bool dust_left;        // But these are instance variables
    dust *dust_head;       // Mixed paradigm!
    planet *planet_head;
};
```

**Better:** Clear domain models
```cpp
class SolarSystem {
    Star primary_star;
    std::vector<std::unique_ptr<Planet>> planets;

public:
    void generate(const SystemParams& params);
    void exportToJson(const std::filesystem::path& path) const;
};

class Planet {
    PhysicalProperties properties;
    Orbit orbit;
    std::vector<std::unique_ptr<Moon>> moons;

public:
    void calculateAtmosphere();
    bool isHabitable() const;
};
```

### 4.2 Modern C++ Features to Adopt

| Feature | Current Use | Should Use | Benefit |
|---------|-------------|------------|---------|
| Smart pointers | 0% | 100% | Memory safety |
| std::optional | 0% | For nullable returns | Explicit optionality |
| std::variant | 0% | For type-safe unions | Type safety |
| Ranges (C++20) | 0% | For data processing | Cleaner code |
| Concepts (C++20) | 0% | For template constraints | Better errors |
| Modules (C++20) | 0% | Replace headers | Faster compilation |
| std::format (C++20) | 0% | Replace printf-style | Type safety |

### 4.3 Design Patterns to Apply

#### Dependency Injection
```cpp
// Current: Global dependencies
void generate_planets(...) {
    if (flag_verbose) { ... }  // Global!
    dust_density = ...;        // Global!
}

// Better: Inject dependencies
class PlanetGenerator {
    const Config& config;
    Logger& logger;

public:
    PlanetGenerator(const Config& cfg, Logger& log)
        : config(cfg), logger(log) {}

    std::vector<Planet> generate(const Star& star);
};
```

#### Builder Pattern
```cpp
// Better than 20+ function parameters
auto system = SolarSystemBuilder()
    .withStar(Star::fromMass(1.0))
    .withSeed(12345)
    .withDustDensity(0.002)
    .generateMoons(true)
    .filterEarthlike(true)
    .build();
```

#### Strategy Pattern
```cpp
// For output formats
class OutputFormatter {
public:
    virtual ~OutputFormatter() = default;
    virtual void format(const SolarSystem&, std::ostream&) = 0;
};

class HtmlFormatter : public OutputFormatter { ... };
class JsonFormatter : public OutputFormatter { ... };
class CsvFormatter : public OutputFormatter { ... };
```

---

## 5. N-Body Preparation Requirements

### 5.1 Current Limitations

The current architecture **cannot support** n-body physics because:

1. **No time-stepping**: Current model is static snapshot
2. **No gravitational interactions**: Planets don't interact
3. **Thread safety**: N-body requires parallelization
4. **Data structures**: Linked lists too slow for force calculations
5. **No vector math**: Missing linear algebra primitives

### 5.2 N-Body Architecture Needs

**Required Components:**

```cpp
// 1. Vector math library
class Vec3 {
    double x, y, z;
    // operators +, -, *, dot, cross, etc.
};

// 2. State representation
struct CelestialBody {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    double mass;
    double radius;
};

// 3. Force calculator
class GravitySimulator {
public:
    void computeForces(std::span<CelestialBody> bodies);
    void integrate(std::span<CelestialBody> bodies, double dt);

private:
    // Barnes-Hut tree for O(N log N) vs O(N²)
    OctreeNode* spatial_tree;
};

// 4. Time evolution
class Simulation {
public:
    void step(double dt);
    void runUntil(double end_time);

private:
    std::vector<CelestialBody> bodies;
    GravitySimulator simulator;
    IntegrationMethod method; // Euler, Verlet, RK4, etc.
};
```

### 5.3 Visualization Requirements

For real-time visualization:

1. **Separate rendering thread**: Physics calculation on one thread, rendering on another
2. **Event system**: Decouple simulation from visualization
3. **Buffer management**: Double/triple buffering for smooth display
4. **Graphics library**: OpenGL, Vulkan, or game engine (Unity/Godot)
5. **Camera system**: For navigating 3D space

**Architecture:**
```cpp
class SimulationEngine {
    Simulation sim;

public:
    void update(double dt);
    SnapshotView getState() const; // Thread-safe view
};

class Visualizer {
    SimulationEngine& engine;

public:
    void render();  // Runs on separate thread
    void handleInput();
};
```

---

## 6. Prioritized Recommendations

### Phase 1: Foundation (6-8 weeks)
**Goal:** Establish clean architecture without breaking functionality

| Priority | Task | Effort | Impact | Risk |
|----------|------|--------|--------|------|
| 🔴 CRITICAL | Remove global variables | High | Very High | Medium |
| 🔴 CRITICAL | Replace raw pointers with smart pointers | High | Very High | Medium |
| 🔴 CRITICAL | Fix memory leaks (add missing deletes) | Low | High | Low |
| 🟡 HIGH | Convert linked lists to std::vector | Medium | High | Low |
| 🟡 HIGH | Remove "using namespace std" | Low | Medium | Low |
| 🟢 MEDIUM | Extract command-line parsing | Low | Medium | Low |

**Deliverables:**
- [ ] Create `Config` class to hold all configuration
- [ ] Create `SimulationContext` to hold state
- [ ] Convert `planet::first_moon` to `std::vector<std::unique_ptr<Planet>>`
- [ ] Convert `dust::next_band` to `std::vector<DustBand>`
- [ ] Run Valgrind/AddressSanitizer to find all leaks

### Phase 2: Modularization (4-6 weeks)
**Goal:** Clear separation of concerns

| Priority | Task | Effort | Impact |
|----------|------|--------|--------|
| 🔴 CRITICAL | Separate domain model from I/O | High | Very High |
| 🟡 HIGH | Extract output formatters | Medium | High |
| 🟡 HIGH | Create proper data access layer | Medium | High |
| 🟢 MEDIUM | Add unit tests | High | High |
| 🟢 MEDIUM | Move catalogs to data files | Medium | Medium |

**Deliverables:**
- [ ] `core/` - Domain models (Planet, Star, System)
- [ ] `physics/` - Calculations (accrete, enviro)
- [ ] `io/` - Input/output (formatters, loaders)
- [ ] `data/` - Star catalogs as binary files
- [ ] `tests/` - Unit test suite

### Phase 3: Modern C++ (3-4 weeks)
**Goal:** Leverage C++20/23 features

| Priority | Task | Effort | Impact |
|----------|------|--------|--------|
| 🟡 HIGH | Add std::optional for nullable returns | Medium | Medium |
| 🟡 HIGH | Use std::variant for planet types | Medium | Medium |
| 🟢 MEDIUM | Apply ranges for data processing | Medium | Medium |
| 🟢 MEDIUM | Use std::format instead of sprintf | Low | Low |

### Phase 4: N-Body Foundation (8-10 weeks)
**Goal:** Add time-evolution capabilities

| Priority | Task | Effort | Impact |
|----------|------|--------|--------|
| 🔴 CRITICAL | Add vector math library | Medium | Very High |
| 🔴 CRITICAL | Implement basic integrator (Verlet) | High | Very High |
| 🔴 CRITICAL | Add gravitational force calculation | Medium | Very High |
| 🟡 HIGH | Implement Barnes-Hut for O(N log N) | High | High |
| 🟡 HIGH | Add parallelization (OpenMP/TBB) | Medium | High |

### Phase 5: Visualization (10-12 weeks)
**Goal:** Real-time 3D rendering

| Priority | Task | Effort | Impact |
|----------|------|--------|--------|
| 🔴 CRITICAL | Choose graphics framework | Low | Very High |
| 🔴 CRITICAL | Implement camera system | Medium | Very High |
| 🟡 HIGH | Add orbital trail rendering | Medium | High |
| 🟡 HIGH | Implement time controls | Low | Medium |
| 🟢 MEDIUM | Add UI overlay | Medium | Medium |

---

## 7. Quick Wins (Start Here!)

These can be done independently and provide immediate value:

### 7.1 Fix Memory Leaks (1 day)
```bash
# Run Valgrind to find all leaks
valgrind --leak-check=full --show-leak-kinds=all ./stargen -D1
```

Add missing destructors and delete calls.

### 7.2 Remove "using namespace std" (2 hours)
```bash
# Find all uses
grep -r "using namespace std" *.cpp *.h

# Replace with specific using declarations or std:: prefix
```

### 7.3 Add .clang-tidy Configuration (1 hour)
```yaml
# .clang-tidy
Checks: >
  modernize-*,
  readability-*,
  performance-*,
  bugprone-*,
  -modernize-use-trailing-return-type

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
```

### 7.4 Convert One Subsystem (1 week)
Pick the smallest, most isolated component (e.g., `utils.cpp`) and fully modernize it:
- Remove global dependencies
- Add unit tests
- Use modern C++ idioms
- Document with Doxygen

Use this as a **template** for other subsystems.

---

## 8. Specific Code Examples

### 8.1 Before/After: Global State Elimination

**Before:**
```cpp
// stargen.h
extern int flag_verbose;
extern long double max_age;

// stargen.cpp
int flag_verbose = 0;
long double max_age = 6.0E9;

// enviro.cpp
void some_function() {
    if (flag_verbose & 0x0001) {
        cout << "Age: " << max_age << endl;
    }
}
```

**After:**
```cpp
// Config.h
struct Config {
    int verbose_level = 0;
    long double max_age = 6.0E9;
    // ... all config in one place
};

// enviro.cpp
void some_function(const Config& config, Logger& log) {
    if (config.verbose_level & 0x0001) {
        log.info("Age: {}", config.max_age);
    }
}
```

### 8.2 Before/After: Memory Management

**Before:**
```cpp
planet* create_planet() {
    planet* p = new planet();
    p->next_planet = nullptr;
    return p;
}

void process_planets(planet* head) {
    planet* p = head;
    while (p != nullptr) {
        // Process...
        p = p->next_planet;
    }
    // Memory leak! Never deleted
}
```

**After:**
```cpp
std::unique_ptr<Planet> create_planet() {
    return std::make_unique<Planet>();
}

void process_planets(std::vector<std::unique_ptr<Planet>>& planets) {
    for (auto& planet : planets) {
        // Process...
    }
    // Automatic cleanup when vector goes out of scope
}
```

### 8.3 Before/After: Data Structures

**Before:**
```cpp
class dust {
private:
    long double innerEdge;
    long double outerEdge;
    bool dustPresent;
    bool gasPresent;
public:
    dust *next_band;  // Public raw pointer!
};

dust* dust_head = nullptr;  // Global!
```

**After:**
```cpp
struct DustBand {
    long double inner_edge;
    long double outer_edge;
    bool dust_present = true;
    bool gas_present = true;
};

class DustDisk {
    std::vector<DustBand> bands;

public:
    void add_band(DustBand band) { bands.push_back(std::move(band)); }
    bool has_dust_in_range(double inner, double outer) const;
    void update_after_accretion(double min, double max, bool remove_gas);
};
```

---

## 9. Testing Strategy

### 9.1 Current State
- **Zero unit tests**
- **No test framework**
- **Manual testing only**

### 9.2 Recommended Approach

**Framework:** Catch2 or Google Test

```cpp
// test_accrete.cpp
#include <catch2/catch_test_macros.hpp>
#include "accrete.h"

TEST_CASE("Dust disk initialization") {
    DustDisk disk(0.3, 200.0);

    REQUIRE(disk.band_count() == 1);
    REQUIRE(disk.has_dust_in_range(1.0, 2.0));
}

TEST_CASE("Accretion removes dust") {
    DustDisk disk(0.3, 200.0);
    disk.update_after_accretion(1.0, 1.5, false);

    REQUIRE_FALSE(disk.has_dust_in_range(1.0, 1.5));
    REQUIRE(disk.has_dust_in_range(2.0, 3.0));
}
```

**Coverage Goals:**
- Phase 1: 30% code coverage
- Phase 2: 50% code coverage
- Phase 3: 70% code coverage

---

## 10. Long-Term Vision

### 10.1 Ideal Architecture (Post-Refactor)

```
stargen/
├── core/
│   ├── models/          # Domain objects
│   │   ├── Star.h/cpp
│   │   ├── Planet.h/cpp
│   │   ├── Orbit.h/cpp
│   │   └── SolarSystem.h/cpp
│   └── interfaces/      # Abstract interfaces
│       └── ISimulator.h
├── physics/
│   ├── accretion/       # Dust disk accretion
│   ├── atmosphere/      # Atmospheric calculations
│   ├── gravity/         # N-body simulation
│   └── integrators/     # Numerical integration
├── data/
│   ├── loaders/         # Catalog loaders
│   ├── catalogs/        # Binary star catalogs
│   └── elements/        # Chemical element data
├── io/
│   ├── formatters/      # Output formatters
│   │   ├── HtmlFormatter.h/cpp
│   │   ├── JsonFormatter.h/cpp
│   │   └── CsvFormatter.h/cpp
│   └── exporters/
├── visualization/       # (Future)
│   ├── renderer/
│   ├── camera/
│   └── ui/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── benchmarks/
└── app/
    └── main.cpp         # CLI entry point
```

### 10.2 API Design (Future Library Use)

```cpp
#include <stargen/core.h>
#include <stargen/physics.h>

int main() {
    // Create solar system
    auto system = stargen::SolarSystemBuilder()
        .withStar(stargen::Star::fromSpectralType("G2V"))
        .withSeed(12345)
        .withHabitableZoneFilter(true)
        .build();

    // Export to various formats
    system.exportToJson("output.json");
    system.exportToHtml("output.html");

    // Run n-body simulation
    stargen::Simulation sim(system);
    sim.run_for_years(1000000);  // 1 million years

    // Visualize
    stargen::Visualizer viz(sim);
    viz.show();  // Opens 3D window

    return 0;
}
```

---

## 11. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Breaking existing functionality | High | High | Comprehensive regression tests before changes |
| Performance degradation | Medium | Medium | Benchmark before/after, profile hotspots |
| Incompatible with old data | Low | Medium | Maintain backward compatibility layer |
| Scope creep | High | High | Stick to phased approach, freeze features |
| Team resistance to changes | Medium | Medium | Show quick wins first, document benefits |

---

## 12. Success Metrics

### 12.1 Code Quality
- [ ] Zero global variables
- [ ] 100% use of smart pointers
- [ ] 70%+ test coverage
- [ ] Zero Valgrind errors
- [ ] Zero clang-tidy warnings

### 12.2 Performance
- [ ] Compilation time < 2 minutes (currently 5-10 min with 41MB files)
- [ ] System generation < 100ms (current: varies)
- [ ] Memory usage predictable and bounded

### 12.3 Maintainability
- [ ] Clear module boundaries
- [ ] New contributor can understand architecture in < 1 day
- [ ] Adding new output format takes < 2 hours
- [ ] Adding new star catalog takes < 30 minutes

---

## 13. Conclusion

StarGen has strong scientific foundations and interesting features, but its architecture reflects its 45-year evolution from Fortran through multiple C/C++ paradigms. To achieve the goal of n-body simulation with visualization, **significant architectural refactoring is essential**.

**Recommended Start:**
1. **Week 1:** Fix memory leaks, remove namespace pollution (quick wins)
2. **Week 2-3:** Create Config/Context classes, start removing globals
3. **Week 4-6:** Convert one subsystem (utils or display) as proof-of-concept
4. **Week 7+:** Apply learnings to rest of codebase

**Key Success Factor:** Incremental refactoring with continuous testing. Don't attempt a complete rewrite—refactor piece by piece while maintaining a working system.

The path is clear, but it requires discipline and commitment to modern software engineering practices. The payoff will be a maintainable, extensible, thread-safe codebase ready for advanced features like n-body physics and real-time visualization.

---

## Appendices

### A. Tools Recommended

**Static Analysis:**
- clang-tidy (already has .clang-format)
- cppcheck
- PVS-Studio (commercial)

**Dynamic Analysis:**
- Valgrind (memory leaks)
- AddressSanitizer (buffer overflows)
- ThreadSanitizer (when adding threads)

**Profiling:**
- Tracy (already integrated!)
- perf/gprof
- Hotspot (GUI for perf)

**Testing:**
- Catch2 or Google Test
- Benchmark (Google Benchmark)

**Build:**
- CMake (already using)
- ccache (speed up rebuilds)
- Ninja (faster than Make)

### B. Learning Resources

**Modern C++:**
- "Effective Modern C++" - Scott Meyers
- "C++17: The Complete Guide" - Nicolai Josuttis
- "C++ Best Practices" - Jason Turner

**N-Body Simulation:**
- "Computer Simulation of Liquids" - Allen & Tildesley
- "The Art of Molecular Dynamics Simulation" - Rapaport
- Barnes-Hut algorithm paper (1986)

**Software Architecture:**
- "Clean Architecture" - Robert Martin
- "Refactoring" - Martin Fowler
- "Working Effectively with Legacy Code" - Michael Feathers

### C. Contact & Next Steps

For questions or discussion:
- Review this document with the team
- Prioritize Phase 1 tasks
- Set up project tracking (GitHub Projects/Issues)
- Schedule regular architecture review meetings

**Immediate Action Items:**
1. [ ] Run Valgrind to document current memory leaks
2. [ ] Set up clang-tidy in CI/CD
3. [ ] Create GitHub issues for Phase 1 tasks
4. [ ] Start with one "quick win" this week

---

*Report prepared by: Claude (Anthropic AI Assistant)*
*For: StarGen Modernization Project*
