# Global Variable Refactoring Guide

## Overview

This guide explains how to migrate from global variables to the new Config, SimulationContext, and RandomContext classes.

## New Architecture

### Config Class (`Config.h`)
Holds all **configuration parameters** that are set at the start of a simulation and don't change during execution:
- Star properties (mass, luminosity, temperature, spectral type)
- Companion star properties (for binary systems)
- Simulation options (do_gases, do_moons, do_migration)
- Filtering options (filter_earthlike, filter_habitable)
- Output configuration (format, paths, filenames)
- Random seed and system count
- Accretion parameters

### SimulationContext Class (`SimulationContext.h`)
Holds all **runtime state** that changes during simulation execution:
- Current sun being simulated
- Statistics counters (total_earthlike, total_habitable, etc.)
- Min/max statistics for breathable planets
- Min/max statistics for potentially habitable planets

### RandomContext Class (`RandomContext.h`)
Holds **random number generator state**:
- seed, jseed, ifrst, nextn
- Each instance provides an independent RNG stream

## Migration Strategy

### Phase 1: Coexistence (CURRENT)
- New classes exist alongside old globals
- Old code continues to work unchanged
- New code can optionally use the classes

### Phase 2: Gradual Migration (FUTURE)
Migrate functions one subsystem at a time:

1. **Pick a subsystem** (e.g., `enviro.cpp`)
2. **Update function signatures** to accept Config/Context parameters
3. **Update function bodies** to use the class members instead of globals
4. **Update callers** to pass the Config/Context objects

### Phase 3: Global Removal (FUTURE)
Once all code uses the classes:
1. Remove global variable declarations from `stargen.h`
2. Remove global variable definitions from implementation files
3. Verify compilation succeeds

## Example Migration

### Before (using globals):
```cpp
// stargen.h
extern int flag_verbose;
extern long double max_age;

// somefile.cpp
void some_function() {
    if (flag_verbose & 0x0001) {
        std::cout << "Max age: " << max_age << std::endl;
    }
}
```

### After (using Config):
```cpp
// somefile.cpp
void some_function(const Config& config) {
    if (config.isVerbose(0x0001)) {
        std::cout << "Max age: " << config.max_age << std::endl;
    }
}
```

## Benefits of New Architecture

### 1. Thread Safety
Each thread can have its own Config/Context/RandomContext:
```cpp
// Thread-safe parallel generation
void generate_system_thread(int thread_id, Config config, RandomContext rng) {
    rng.setSeed(config.random_seed + thread_id);
    SimulationContext context;
    // Generate system using thread-local objects
}
```

### 2. Testability
Easy to create test configurations:
```cpp
void test_earthlike_detection() {
    Config config;
    config.filter_earthlike = true;
    config.stellar_mass = 1.0;

    SimulationContext context;
    // Run test with known configuration
}
```

### 3. Multiple Simulations
Run multiple simulations simultaneously:
```cpp
Config config1, config2;
config1.stellar_mass = 0.5;  // Red dwarf
config2.stellar_mass = 2.0;  // Blue giant

SimulationContext ctx1, ctx2;
// Generate both systems without interference
```

### 4. Clear Dependencies
Function signatures document what they need:
```cpp
// Old way - hidden dependencies on globals
void calculate_orbit(planet* p);

// New way - explicit dependencies
void calculate_orbit(planet* p, const Config& config, SimulationContext& ctx);
```

## Migration Priorities

### High Priority (Do First)
Functions that are called frequently and modify state:
- `generate_planet()` - should accept Config and SimulationContext
- `calculate_gases()` - should accept Config
- `check_planet()` - should accept Config and SimulationContext

### Medium Priority (Do Next)
Subsystem entry points:
- `accrete::dist_planetary_masses()` - should accept Config
- `generate_stellar_system()` - should accept Config and SimulationContext
- Display functions - should accept Config for formatting options

### Low Priority (Do Last)
Simple utility functions with few dependencies:
- Math utilities in `enviro.cpp`
- Radius calculation functions
- Type checking predicates

## Function Signature Patterns

### Read-Only Configuration
```cpp
// Before
auto calculate_value() -> long double;

// After
auto calculate_value(const Config& config) -> long double;
```

### Modifying Statistics
```cpp
// Before
void record_planet(planet* p);

// After
void record_planet(planet* p, const Config& config, SimulationContext& ctx);
```

### Using RNG
```cpp
// Before
auto generate_random() -> long double;

// After
auto generate_random(RandomContext& rng) -> long double;
```

### Full Context
```cpp
// Before
void generate_system();

// After
void generate_system(const Config& config, SimulationContext& ctx, RandomContext& rng);
```

## Backward Compatibility During Migration

To maintain compilation during migration, we can use global instances:

```cpp
// Temporary global instances (to be removed later)
Config g_config;
SimulationContext g_context;
RandomContext g_rng;

// Old function (unchanged)
void old_function() {
    if (flag_verbose) {  // Still uses old global
        // ...
    }
}

// Migrated function (uses classes)
void new_function(const Config& config) {
    if (config.isVerbose()) {  // Uses new class
        // ...
    }
}

// Wrapper for gradual migration
void old_caller() {
    new_function(g_config);  // Pass global instance
}
```

## Testing Strategy

### Unit Tests
```cpp
TEST(ConfigTest, DefaultValues) {
    Config config;
    EXPECT_EQ(config.stellar_mass, 0.0);
    EXPECT_FALSE(config.do_moons);
}

TEST(SimulationContextTest, Statistics) {
    SimulationContext ctx;
    ctx.recordEarthlike();
    EXPECT_EQ(ctx.total_earthlike, 1);
}
```

### Integration Tests
```cpp
TEST(SystemGenerationTest, WithConfig) {
    Config config;
    config.stellar_mass = 1.0;
    config.do_gases = true;

    SimulationContext ctx;
    RandomContext rng(12345);

    auto planets = generate_system(config, ctx, rng);
    EXPECT_GT(planets.size(), 0);
}
```

## Next Steps

1. **Create global instances** (g_config, g_context, g_rng) in stargen.cpp
2. **Migrate main.cpp** to populate Config from command-line arguments
3. **Migrate one subsystem** (suggest: enviro.cpp) as proof-of-concept
4. **Update callers** to pass Config/Context objects
5. **Repeat** for other subsystems
6. **Remove globals** once all code migrated

## Questions?

See also:
- `ARCHITECTURAL_ANALYSIS.md` - Overall architecture recommendations
- `Config.h`, `SimulationContext.h`, `RandomContext.h` - Class definitions
- `stargen.h` - Current global variables (to be removed)
