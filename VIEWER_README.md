# Stellar System Viewer

Real-time interactive visualization of generated stellar systems with orbital motion.

## Features

### Phase 1: Basic Infrastructure ✅
- **OrbitalSimulator**: Kepler's equation solver for accurate orbital mechanics
- **OrbitalStateManager**: Multi-system runtime position tracking
- **Basic App Loop**: 60 FPS raylib application with delta time handling

### Phase 2: 2D Visualization ✅
- **Top-down orbital view**: View solar systems from above
- **Planet rendering**: Circles sized and colored by planet type
- **Orbit paths**: Visual elliptical orbits for each planet
- **Camera system**:
  - Pan with mouse drag
  - Zoom with mouse wheel
- **UI overlay**:
  - FPS counter
  - Simulation time display
  - Time scale control
  - Control hints
  - Pause/resume indicator

## Controls

- **SPACE**: Pause/Resume simulation
- **+/-** or **Numpad +/-**: Adjust time scale (speed up/slow down)
- **Mouse Wheel**: Zoom in/out
- **Left Mouse Drag**: Pan camera
- **ESC**: Exit viewer

## Building

### Requirements

The viewer requires X11 development libraries on Linux:

```bash
# Ubuntu/Debian
sudo apt-get install libxrandr-dev libxcursor-dev libxinerama-dev libxi-dev libgl1-mesa-dev

# Fedora/RHEL
sudo dnf install libXrandr-devel libXcursor-devel libXinerama-devel libXi-devel mesa-libGL-devel
```

### Compile

```bash
# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release .

# Build the viewer
cmake --build . --target viewer --parallel

# Run
./viewer
```

## Architecture

### System Generation
Uses existing `StarGenerator` and `accrete` classes to generate planetary systems with realistic orbital parameters (semi-major axis, eccentricity, inclination, etc.).

### Orbital Simulation
`OrbitalSimulator` calculates real-time 3D positions using:
- Kepler's equation (solved via Newton-Raphson)
- Orbital element to Cartesian conversion
- Time-stepping with configurable speed

### Rendering
raylib provides:
- Window management
- 2D rendering primitives
- Input handling
- Frame timing

## Implementation Details

### World to Screen Transformation
- World units: Astronomical Units (AU)
- Screen units: Pixels
- Zoom level: pixels per AU
- Origin: Sun at screen center

### Planet Visualization
- **Color coding**:
  - Yellow/Orange: Gas giants
  - Sky blue: Sub-gas giants
  - Blue: Potentially habitable (temp > 273K)
  - Light gray: Rocky/icy worlds
- **Size**: Not to scale (visibility enhancement)
- **Orbits**: Drawn as circles (ellipse approximation for e < 0.1)

### Time Simulation
- Base rate: 1 second = 1 day (for smooth animation)
- Adjustable via time scale (0.1x to arbitrarily fast)
- Pause functionality
- Frame-independent using delta time

## Future Enhancements (Pending)

### Phase 3: 3D Graphics
- [ ] Full 3D rendering with Vulkan or raylib 3D
- [ ] Sphere rendering for planets/star
- [ ] Basic lighting and shadows
- [ ] Skybox/space environment

### Phase 4: Visual Polish
- [ ] Planet textures (procedural or atlas-based)
- [ ] Atmosphere glow effects
- [ ] Orbital trails showing recent paths
- [ ] Improved orbit rendering (true ellipses)

### Phase 5: Advanced Features
- [ ] Background system generation (parallel loading)
- [ ] ImGui integration for advanced UI
- [ ] System library (browse multiple systems)
- [ ] Follow planet camera mode
- [ ] System information panels
- [ ] Screenshot/video export

## Code Structure

```
viewer.cpp              - Main application loop
OrbitalSimulator.h/cpp  - Orbital mechanics calculations
OrbitalStateManager.h/cpp - Multi-system management
Vector3.h               - 3D vector math utilities
```

## Notes

- The viewer is **non-invasive**: Generation and simulation are completely separate
- Generation data (planet structs) are never modified by the simulator
- Systems can be generated offline and loaded for viewing
- Simulation is deterministic and reproducible

## Troubleshooting

### "RandR headers not found" error
Install X11 development packages (see Requirements above).

### Black screen / No planets visible
- Try zooming out (mouse wheel down)
- The generated system may have widely spaced planets
- Check console output for planet count

### Performance issues
- Disable Tracy profiling for production builds
- Reduce window size in viewer.cpp
- Consider lowering target FPS

## Example Output

```
Initializing stellar system generator...
Generating stellar system (seed 42)...
Generated 8 planets
Initializing orbital simulator...
Starting viewer...
[Opens window with animated solar system]
```
