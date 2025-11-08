# StarGen TODO & Enhancement Report

Here's my detailed report on bugs, enhancements, and preparation for multithreading, n-body simulation, and visualization.

## 📊 Executive Summary
**Codebase Status:** The project has undergone significant recent modernization (Config/SimulationContext classes, vector conversions, memory leak fixes) but still requires substantial work before implementing multithreading, n-body physics, and visualization.

**Key Metrics:**
- 214 new allocations vs 18 delete calls → Potential memory leaks
- 72+ global variables (partial migration complete)
- ~5 uses of `rand()` → Non-thread-safe RNG
- 7 files still using `using namespace std`
- Recent progress: Linked lists → vectors (planets/moons), destructor cleanup

## 🐛 Critical Bugs & Issues Found

### 1. Memory Management Issues

#### Issue 1.1: Memory Leak Imbalance
**Location:** Throughout codebase
**Severity:** HIGH
**Evidence:** 214 allocations vs 18 deallocations

```cpp
// accrete.cpp:211-220 - Multiple allocations without guaranteed cleanup
node2 = new dust();  // Line 211
node3 = new dust();  // Line 220
```

**Risk:** Long-running simulations (especially when generating thousands of systems) will leak memory.

**Recommendation:**
1. Convert remaining dust linked lists to `std::vector<dust>` (similar to planet conversion)
2. Use `std::unique_ptr<dust>` for ownership clarity
3. Add comprehensive destructors (recent work started this)

#### Issue 1.2: Dangling Pointer Risk in Moon Management
**Location:** structs.h:321-324, accrete.cpp:679-680
**Severity:** MEDIUM

```cpp
// structs.h
planet *first_moon;  // DEPRECATED: Use moons vector instead
planet *first_moon_backup;  // Used for what?
planet *reconnect_to;  // Reconnection logic unclear

// accrete.cpp:679-680
the_planet->first_moon = seed_moons;
the_planet->first_moon_backup = seed_moons;
```

**Problem:** Mixing vector-based moons with raw pointer links creates ownership confusion.

**Fix:** Complete the vector migration:
```cpp
// Should be:
std::vector<std::unique_ptr<planet>> moons;  // Clear ownership
```

### 2. Thread Safety Violations

#### Issue 2.1: Global Mutable State
**Location:** stargen.cpp:16-106, SimulationContext.h, Config.h
**Severity:** CRITICAL for multithreading

```cpp
// stargen.cpp:16-19
Config g_config;                    // Global mutable
SimulationContext g_sim_context;    // Global mutable
RandomContext g_random_context;     // Global mutable

// stargen.cpp:30-48 - Global references to Config members
int& flags_arg_clone = g_config.flags;
sun& the_sun_clone = g_sim_context.current_sun;
// ... 50+ more global references
```

**Impact:** Cannot run multiple simulations in parallel due to shared mutable state.

**Fix for Multithreading:**
```cpp
// Option 1: Thread-local storage
thread_local Config g_config;
thread_local SimulationContext g_sim_context;

// Option 2 (Better): Pass as parameters
class StarGenerator {
    Config config;
    SimulationContext context;

public:
    SolarSystem generate(const Star& star) {
        // Instance-based, thread-safe
    }
};
```

#### Issue 2.2: Non-Thread-Safe Random Number Generation
**Location:** accrete.cpp:894, utils.cpp, stargen.cpp
**Severity:** HIGH

```cpp
// accrete.cpp:894, 902 - Uses C rand()
if (((1 + rand()) % 14) == 0 && min_distance < 0.2) {
```

**Problem:** `rand()` uses global state, not thread-safe.

**Fix:**
```cpp
// Use C++11 <random> with thread-local RNG
thread_local std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<int> dist(1, 14);
if (dist(rng) == 1) { ... }
```

#### Issue 2.3: Race Conditions in Statistics Tracking
**Location:** SimulationContext.h:87-99
**Severity:** MEDIUM

```cpp
void recordEarthlike() {
    total_earthlike++;      // Non-atomic increment
    system_earthlike++;
}
```

**Problem:** If multiple threads call this, counts will be incorrect.

**Fix:**
```cpp
#include <atomic>

std::atomic<int> total_earthlike{0};
std::atomic<int> total_habitable{0};
```

### 3. Potential Null Pointer Dereferences

#### Issue 3.1: Unchecked Pointer Access in Dust Traversal
**Location:** accrete.cpp:159-177
**Severity:** MEDIUM

```cpp
// accrete.cpp:159
current_dust_band = dust_head;  // Could be nullptr
while (current_dust_band != nullptr &&
       current_dust_band->getOuterEdge() < inside_range) {
    current_dust_band = current_dust_band->next_band;  // OK - checked in loop
}

// Line 169 - POTENTIAL BUG
dust_here = current_dust_band->getDustPresent();  // If current_dust_band == nullptr, CRASH!
```

**Fix:**
```cpp
if (current_dust_band == nullptr) {
    dust_here = false;
} else {
    dust_here = current_dust_band->getDustPresent();
}
```

**Note:** The code actually has this check on line 166-168, so this specific instance is OK, but the pattern is risky.

### 4. Numerical Stability Issues

#### Issue 4.1: Division by Zero Risk
**Location:** accrete.cpp:128, 141
**Severity:** MEDIUM

```cpp
// accrete.cpp:128
return a * (1.0 - e) * (1.0 - mass) / (1.0 + cloud_eccentricity);
```

**Problem:** If `cloud_eccentricity == -1.0`, division by zero.

**Mitigation:** Add assertion or check:
```cpp
assert(cloud_eccentricity > -1.0 && "Invalid eccentricity");
```

#### Issue 4.2: Potential Numerical Overflow in Temperature Calculations
**Location:** enviro.cpp:spec_type_to_eff_temp
**Severity:** LOW

Large stellar temperatures (Wolf-Rayet stars: 200,000K) combined with power laws could overflow `long double`.

**Recommendation:** Add range checks and clamps.

### 5. Code Quality Issues

#### Issue 5.1: Massive Predefined Planet Check Functions
**Location:** accrete.cpp:1054-1801 (747 lines!)
**Severity:** LOW (maintainability)

```cpp
auto accrete::is_predefined_planet(planet *the_planet) -> bool {
    if (is_in_eriEps(the_planet)) return true;
    else if (is_in_UMa47(the_planet)) return true;
    // ... 60+ more else-if chains
    return false;
}
```

**Problem:** O(n) lookup, unmaintainable, should be hash table.

**Fix:**
```cpp
// Use unordered_set for O(1) lookup
std::unordered_set<std::pair<long double, long double>> predefined_planets;

bool is_predefined_planet(const planet* p) {
    return predefined_planets.count({p->getA(), p->getE()}) > 0;
}
```

#### Issue 5.2: Namespace Pollution Remaining
**Location:** 7 files still have `using namespace std`

**Files:** radius_tables.h, elements.cpp, Planetary_Habitability_Laboratory.cpp, dole.h, andromeda.h

**Fix:** Remove and use explicit `std::` prefixes.

## 🧵 Thread Safety Assessment

**Current Thread Safety:** ZERO ❌

**Blockers for Multithreading:**
1. **Global Mutable State**
   - `g_config`, `g_sim_context`, `g_random_context` are shared
   - All 50+ legacy global references

2. **Non-Thread-Safe RNG**
   - `rand()` uses global state
   - `srand()` called at startup only

3. **Statistics Counters**
   - Non-atomic increments
   - Race conditions on totals

4. **Output File Handles**
   - Shared `std::fstream` objects
   - No synchronization

### Roadmap to Thread Safety

**Phase 1: State Isolation (2-3 weeks)**
1. Make `Config` and `SimulationContext` instance members
2. Remove all global references
3. Pass state via parameters or member variables

**Phase 2: RNG Modernization (1 week)**
1. Replace `rand()` with `<random>` library
2. Use thread-local RNG instances
3. Seed from `std::random_device`

**Phase 3: Atomic Statistics (1 week)**
1. Convert counters to `std::atomic<>`
2. Use mutex for min/max tracking
3. Implement thread-safe output

**Phase 4: Parallel Generation (2 weeks)**
1. Use thread pool for system generation
2. Implement work queue
3. Merge results safely

**Example Thread-Safe Architecture:**
```cpp
class ThreadSafeStarGen {
    // Each thread gets its own generator
    static StarSystem generateSystem(const Config& cfg, long seed) {
        SimulationContext ctx;  // Thread-local
        thread_local RandomContext rng;  // Thread-local

        // Generate without touching globals
        return StarSystem{...};
    }

public:
    std::vector<StarSystem> generateMultiple(const Config& cfg, int count) {
        std::vector<std::future<StarSystem>> futures;

        for (int i = 0; i < count; ++i) {
            futures.push_back(std::async(std::launch::async,
                generateSystem, std::ref(cfg), cfg.random_seed + i));
        }

        std::vector<StarSystem> results;
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        return results;
    }
};
```

## 🌌 N-Body Simulation Integration

### Current Limitations
The codebase currently generates **STATIC snapshots**:
- No time evolution
- No gravitational interactions between planets
- No orbital mechanics beyond Kepler's laws
- No perturbations, resonances, or instabilities

### Architecture Requirements for N-Body

#### 1. Time-Stepping Infrastructure
Need to add:
```cpp
class NBodySimulator {
    std::vector<CelestialBody> bodies;
    long double time = 0.0;
    long double dt = 0.001;  // Timestep in years

    void step() {
        computeForces();
        integrate();
        time += dt;
    }

    void simulate(long double duration) {
        while (time < duration) {
            step();
        }
    }
};
```

#### 2. Vector Math Library
Currently missing linear algebra. Need:
```cpp
struct Vec3 {
    long double x, y, z;

    Vec3 operator+(const Vec3& other) const;
    Vec3 operator-(const Vec3& other) const;
    Vec3 operator*(long double scalar) const;
    long double magnitude() const;
    long double dot(const Vec3& other) const;
    Vec3 cross(const Vec3& other) const;
};

struct CelestialBody {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    long double mass;
};
```

**Recommendation:** Use Eigen library for production:
```cpp
#include <Eigen/Dense>

using Vec3 = Eigen::Vector3d;
```

#### 3. Integrator Selection

| Method | Accuracy | Speed | Recommendation |
|--------|----------|-------|----------------|
| Euler | Low | Fast | ❌ Too inaccurate |
| Verlet | Medium | Fast | ✅ Good starting point |
| RK4 | High | Medium | ✅ Good balance |
| Leapfrog | High | Fast | ✅ Best for orbits |
| Symplectic | Highest | Slow | 🟡 For long-term stability |

**Recommended: Leapfrog (Velocity Verlet)**
```cpp
void leapfrogStep(CelestialBody& body, long double dt) {
    // Kick
    body.velocity += body.acceleration * (dt / 2.0);

    // Drift
    body.position += body.velocity * dt;

    // Compute new acceleration
    computeAcceleration(body);

    // Kick
    body.velocity += body.acceleration * (dt / 2.0);
}
```

#### 4. Force Calculation
```cpp
Vec3 computeGravitationalForce(const CelestialBody& a, const CelestialBody& b) {
    Vec3 r = b.position - a.position;
    long double dist = r.magnitude();

    if (dist < softening_length) {
        dist = softening_length;  // Avoid singularity
    }

    long double force_mag = G * a.mass * b.mass / (dist * dist);
    return r * (force_mag / dist);  // Normalized direction
}

void computeForces(std::vector<CelestialBody>& bodies) {
    // O(N²) all-pairs
    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].acceleration = Vec3{0, 0, 0};

        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;

            Vec3 force = computeGravitationalForce(bodies[i], bodies[j]);
            bodies[i].acceleration += force / bodies[i].mass;
        }
    }
}
```

#### 5. Performance Optimization: Barnes-Hut Tree
For systems with many bodies (>100), use spatial partitioning:
```cpp
struct OctreeNode {
    Vec3 center_of_mass;
    long double total_mass;
    Vec3 bounds_min, bounds_max;
    std::array<std::unique_ptr<OctreeNode>, 8> children;
    CelestialBody* body = nullptr;  // Leaf node
};

class BarnesHutTree {
    std::unique_ptr<OctreeNode> root;
    long double theta = 0.5;  // Opening angle

public:
    void build(const std::vector<CelestialBody>& bodies);
    Vec3 computeForce(const CelestialBody& body);
};
```

**Complexity:** O(N²) → O(N log N)

#### 6. Data Structure Changes Needed

**Current:**
```cpp
class planet {
    long double a;  // Semi-major axis
    long double e;  // Eccentricity
    // Keplerian elements only
};
```

**For N-Body:**
```cpp
class planet {
    // Keep Keplerian (for initial conditions)
    long double a, e, inclination, ascending_node;

    // Add Cartesian (for n-body)
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;

    // Convert between representations
    void keplerianToCartesian();
    void cartesianToKeplerian();
};
```

#### 7. Integration with Existing Code
**Strategy:** Hybrid approach

1. Use current accrete.cpp to generate initial conditions
2. Convert Keplerian elements to Cartesian coordinates
3. Run n-body simulation for stability check
4. Convert back to Keplerian for output

```cpp
SolarSystem generateStableSystem(const Config& cfg) {
    // Phase 1: Generate using accretion
    planet* planets = accrete_object.dist_planetary_masses(...);

    // Phase 2: Convert to n-body
    NBodySimulator sim;
    for (planet* p = planets; p != nullptr; p = p->next_planet) {
        sim.addBody(keplerianToCartesian(*p));
    }

    // Phase 3: Stability check (simulate 1 million years)
    sim.simulate(1e6);

    // Phase 4: Check for ejections/collisions
    if (sim.isStable()) {
        return convertToSolarSystem(sim.getBodies());
    } else {
        return generateStableSystem(cfg);  // Retry
    }
}
```

### Recommended N-Body Libraries
Rather than implementing from scratch:

1. **Rebound** (C library with C++ bindings)
   - Production-ready n-body integrator
   - Multiple integrators (WHFast, IAS15, etc.)
   - Collision detection
   - https://github.com/hannorein/rebound

2. **AMUSE** (Python/C++)
   - Astrophysical Multipurpose Software Environment
   - Stellar evolution + n-body
   - Modular architecture

3. **Custom Lightweight Implementation**
   - If you want full control
   - ~500-1000 LOC for basic Leapfrog
   - ~2000 LOC for Barnes-Hut

**Recommendation:** Start with Rebound for rapid prototyping, then optimize if needed.

## 🎨 Visualization Integration (SDL/Raylib)

### Current Limitations
No real-time visualization:
- Outputs static HTML, JSON, CSV
- No interactive exploration
- No orbital motion display

### Architecture for Visualization

#### Option 1: SDL2 (Lower-level, more control)

**Pros:**
- Mature, widely used
- Cross-platform
- 2D and OpenGL 3D support
- Direct pixel manipulation

**Cons:**
- More boilerplate
- Must implement UI widgets yourself

**Basic Integration:**
```cpp
#include <SDL2/SDL.h>

class SolarSystemVisualizer {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SolarSystem system;

public:
    SolarSystemVisualizer(const SolarSystem& sys) : system(sys) {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("StarGen Visualizer",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 720, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED);
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw star
        drawCircle(640, 360, 20, {255, 255, 0, 255});

        // Draw planets
        for (const auto& planet : system.planets) {
            long double x = 640 + planet.a * 50 * cos(planet.angle);
            long double y = 360 + planet.a * 50 * sin(planet.angle);
            drawCircle(x, y, planet.radius / 1000, planetColor(planet));
        }

        SDL_RenderPresent(renderer);
    }

    void mainLoop() {
        bool running = true;
        SDL_Event event;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
            }

            update(1.0 / 60.0);  // 60 FPS
            render();
            SDL_Delay(16);
        }
    }
};
```

#### Option 2: Raylib (Easier, game-oriented)

**Pros:**
- Very simple API
- Built-in 3D camera
- UI widgets included
- Great for prototyping

**Cons:**
- Less mature than SDL
- Smaller community

**Basic Integration:**
```cpp
#include "raylib.h"

class RaylibVisualizer {
    SolarSystem system;
    Camera3D camera;

public:
    RaylibVisualizer(const SolarSystem& sys) : system(sys) {
        InitWindow(1280, 720, "StarGen 3D");
        SetTargetFPS(60);

        camera.position = {100.0f, 100.0f, 100.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    void mainLoop() {
        while (!WindowShouldClose()) {
            UpdateCamera(&camera, CAMERA_ORBITAL);

            BeginDrawing();
            ClearBackground(BLACK);

            BeginMode3D(camera);

            // Draw star
            DrawSphere({0, 0, 0}, 5.0f, YELLOW);

            // Draw planets with orbits
            for (const auto& planet : system.planets) {
                DrawCircle3D({0, 0, 0}, planet.a * 10,
                    {1, 0, 0}, 90.0f, GRAY);

                Vector3 pos = {
                    planet.a * 10 * cos(planet.angle),
                    0,
                    planet.a * 10 * sin(planet.angle)
                };
                DrawSphere(pos, planet.radius / 10000, BLUE);
            }

            EndMode3D();

            DrawFPS(10, 10);
            EndDrawing();
        }
    }
};
```

### Recommended Visualization Architecture
**Hybrid Approach: Separate Visualization from Simulation**

```
┌──────────────────────────────────┐
│   StarGen Core (Current Code)   │
│   - Planet generation            │
│   - Physics calculations         │
│   - Data export                  │
└────────────┬─────────────────────┘
             │ JSON/Binary
             ▼
┌──────────────────────────────────┐
│   Visualization Layer            │
│   - Raylib/SDL renderer          │
│   - Camera controls              │
│   - UI overlays                  │
│   - Time stepping (visual only)  │
└──────────────────────────────────┘
```

**Benefits:**
- Decoupling allows independent development
- Can run simulations headless on servers
- Multiple visualization frontends possible
- Testing easier

### Integration Points

#### 1. Data Export for Visualization
```cpp
// Add to SolarSystem class
struct VisualizationData {
    struct Body {
        std::string name;
        Vec3 position;
        long double radius;
        Color color;
        std::vector<Vec3> orbit_path;  // Precomputed orbit
    };

    std::vector<Body> bodies;

    void saveToFile(const std::string& filename) {
        // JSON or binary format
    }
};

VisualizationData system.exportForVisualization();
```

#### 2. Real-Time Mode
```cpp
class LiveSimulation {
    NBodySimulator simulator;
    RaylibVisualizer visualizer;

public:
    void run() {
        while (visualizer.isRunning()) {
            // Update physics
            for (int i = 0; i < 100; ++i) {  // 100 physics steps per frame
                simulator.step();
            }

            // Update visualization
            visualizer.updateBodies(simulator.getBodies());
            visualizer.render();
        }
    }
};
```

#### 3. UI Features to Add
- [ ] Planet selection (click to highlight)
- [ ] Orbital trail rendering
- [ ] Time control (pause, speed up, slow down)
- [ ] Camera modes (follow planet, free flight, top-down)
- [ ] Stats overlay (semi-major axis, eccentricity, etc.)
- [ ] Habitable zone visualization (green ring)
- [ ] Planetary atmosphere visualization (glow effect)

### Recommended Choice: Raylib

**Reasoning:**
- Simpler to integrate
- Built-in 3D camera
- Good documentation
- Active development
- Easy to add ImGui for advanced UI

**Code Structure:**
```
stargen/
├── src/
│   ├── core/          # Existing physics code
│   ├── visualization/ # New visualization layer
│   │   ├── renderer.cpp
│   │   ├── camera_controller.cpp
│   │   ├── ui_overlay.cpp
│   │   └── shaders/
│   └── main_vis.cpp   # Visualization entry point
├── CMakeLists.txt     # Add Raylib as dependency
└── README_VIS.md      # Visualization documentation
```

**CMakeLists.txt Addition:**
```cmake
# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

# Visualization executable
add_executable(stargen_vis
    src/visualization/renderer.cpp
    src/visualization/camera_controller.cpp
    src/visualization/ui_overlay.cpp
    src/main_vis.cpp
)
target_link_libraries(stargen_vis PRIVATE raylib)
```

## 📋 Prioritized Enhancement Roadmap

### Phase 1: Critical Fixes (2-3 weeks)
**Priority: CRITICAL**

1. **Complete Memory Leak Fixes**
   - [ ] Convert dust linked lists to vectors
   - [ ] Add smart pointers for planet ownership
   - [ ] Comprehensive destructor audit
   - Files: accrete.cpp, structs.h

2. **Thread Safety Foundation**
   - [ ] Remove global references
   - [ ] Make Config/SimulationContext instance members
   - [ ] Replace `rand()` with `<random>`
   - Files: stargen.cpp, stargen.h, accrete.cpp, utils.cpp

3. **Null Pointer Safety**
   - [ ] Add `std::optional` or `nullptr` checks for nullable returns
   - [ ] Audit all pointer dereferences
   - Files: accrete.cpp, enviro.cpp

### Phase 2: Architecture Refactoring (4-6 weeks)
**Priority: HIGH**

1. **Dependency Injection**
   - [ ] Create `StarGenerator` class
   - [ ] Pass Config/Context as parameters
   - [ ] Remove last global variables
   - New files: star_generator.h/cpp

2. **Data Structure Modernization**
   - [ ] Complete std::vector migration
   - [ ] Remove all raw pointer linked lists
   - [ ] Use std::unique_ptr for ownership
   - Files: All

3. **Namespace Cleanup**
   - [ ] Remove `using namespace std`
   - [ ] Use explicit `std::` prefixes
   - Files: radius_tables.h, elements.cpp, etc.

### Phase 3: Multithreading Support (3-4 weeks)
**Priority: HIGH (for your goals)**

1. **Atomic Statistics**
   - [ ] Convert counters to `std::atomic<>`
   - [ ] Add mutex for min/max tracking
   - Files: SimulationContext.h

2. **Thread Pool Implementation**
   - [ ] Implement work queue
   - [ ] Add thread-safe result collection
   - New files: thread_pool.h/cpp

3. **Parallel Generation**
   - [ ] Multi-threaded system generation
   - [ ] Benchmark performance
   - Files: stargen.cpp

### Phase 4: N-Body Foundation (6-8 weeks)
**Priority: MEDIUM (for your goals)**

1. **Vector Math Library**
   - [ ] Integrate Eigen library
   - [ ] Add Vec3 wrapper for celestial mechanics
   - New files: vec3.h, celestial_body.h

2. **Coordinate System Conversion**
   - [ ] Keplerian → Cartesian
   - [ ] Cartesian → Keplerian
   - New files: coordinate_conversion.h/cpp

3. **N-Body Integrator**
   - [ ] Implement Leapfrog integrator
   - [ ] Add force calculation
   - [ ] Integrate Rebound library (optional)
   - New files: nbody_simulator.h/cpp

4. **Stability Analysis**
   - [ ] Run simulations for 1M years
   - [ ] Detect ejections/collisions
   - [ ] Reject unstable systems
   - Files: stargen.cpp, new stability check

### Phase 5: Visualization (4-6 weeks)
**Priority: MEDIUM (for your goals)**

1. **Raylib Integration**
   - [ ] Add Raylib to CMake
   - [ ] Create basic 3D renderer
   - New files: visualization/renderer.cpp

2. **Camera System**
   - [ ] Orbital camera
   - [ ] Free-flight camera
   - [ ] Follow-planet mode
   - New files: visualization/camera_controller.cpp

3. **UI Overlay**
   - [ ] Planet stats panel
   - [ ] Time controls
   - [ ] Filtering options
   - New files: visualization/ui_overlay.cpp

4. **Advanced Rendering**
   - [ ] Orbital trails
   - [ ] Habitable zone visualization
   - [ ] Atmospheric glow effects
   - New files: visualization/shaders/

## 🔧 Specific Recommendations

### Immediate Actions (This Week)

1. **Fix Memory Leaks:**
```bash
# Convert dust to vector
git checkout -b fix/dust-vector-migration
# Edit accrete.cpp, accrete.h
git commit -m "Convert dust linked lists to std::vector"
```

2. **Replace rand():**
```cpp
// utils.cpp - Create thread-safe RNG
thread_local std::mt19937 rng(std::random_device{}());

long double random_number(long double min, long double max) {
    std::uniform_real_distribution<long double> dist(min, max);
    return dist(rng);
}
```

3. **Add Assertions:**
```cpp
// accrete.cpp
#include <cassert>

auto accrete::inner_effect_limit(...) -> long double {
    assert(cloud_eccentricity > -1.0 && "Invalid eccentricity");
    return a * (1.0 - e) * (1.0 - mass) / (1.0 + cloud_eccentricity);
}
```

### Short-Term (Next Month)

1. **Create StarGenerator Class:**
```cpp
// star_generator.h
class StarGenerator {
    Config config;
    SimulationContext context;

public:
    StarGenerator(const Config& cfg) : config(cfg) {}

    SolarSystem generate(const Star& star, long seed);
    std::vector<SolarSystem> generateMultiple(int count);
};
```

2. **Add Unit Tests:**
```cpp
// tests/test_accretion.cpp
#include <gtest/gtest.h>

TEST(AccretionTest, DustAvailable) {
    accrete acc;
    acc.set_initial_conditions(0.5, 50.0);
    EXPECT_TRUE(acc.dust_available(1.0, 2.0));
}
```

### Medium-Term (3-6 Months)

1. **N-Body Integration:**
```bash
# Add Rebound library
git submodule add https://github.com/hannorein/rebound external/rebound
```

2. **Visualization Prototype:**
```bash
# Create visualization module
mkdir -p src/visualization
# Add Raylib dependency
# Create basic 3D renderer
```

### Long-Term (6-12 Months)

1. **Full Refactoring:**
   - Modern C++23 architecture
   - Complete unit test coverage
   - Benchmark suite
   - Documentation generation

2. **Advanced Features:**
   - Machine learning for habitability prediction
   - Galactic-scale simulation
   - WebAssembly port for browser visualization

## 📚 Recommended Tools & Libraries

### For Development
- **Static Analysis:**
  - clang-tidy (already configured)
  - cppcheck
  - valgrind for memory leak detection

- **Profiling:**
  - Tracy (already integrated!)
  - perf on Linux
  - Instruments on macOS

- **Testing:**
  - Google Test
  - Catch2
  - Benchmark library

### For Features
- **N-Body:**
  - Rebound - https://github.com/hannorein/rebound
  - Eigen (linear algebra)

- **Visualization:**
  - Raylib - https://www.raylib.com/
  - ImGui (for UI overlays)

- **Parallelization:**
  - C++20 std::jthread
  - Threading Building Blocks (TBB)
  - OpenMP (for simple parallelism)

## 🎯 Summary & Next Steps

### Critical Path to Your Goals
To achieve multithreading + n-body + visualization:

1. **Month 1-2:** Thread safety (remove globals, add atomic counters)
2. **Month 3-4:** N-body foundation (vector math, coordinate conversion)
3. **Month 5-6:** Basic visualization (Raylib integration, 3D rendering)
4. **Month 7-8:** Integration (thread-safe n-body with live visualization)
5. **Month 9-12:** Polish (optimization, advanced features, documentation)
