# Debugging Notes: Double-Free Crash

## Issue
Program crashes with "double free or corruption" error when running in parallel mode with multiple systems.

## Investigation Timeline

### GDB Analysis
- Backtrace showed crash in `sun::~sun()` when freeing string members
- Stack trace: `sun::~sun` ← `planet::~planet` ← `accrete::free_planet`
- Error: `free(): double free detected in tcache 2`

### Root Cause Analysis

#### Initial Hypothesis: Duplicate Copy Constructors
- Found `sun` class had TWO copy constructors:
  - `sun(sun &);` - non-const reference
  - `sun(const sun &);` - const reference
- This is unusual and can cause overload resolution issues
- **Fix Applied**: Consolidated to single `sun(const sun &) = default;`
- **Result**: Crash still occurs

####Current Understanding
The issue is deeper than the copy constructor. Possible causes:

1. **Memory Lifetime Issues**:
   - `SystemOutputData.the_sun` - sun object stored by value
   - `planet.theSun` - sun object stored by value in each planet
   - Multiple copies of sun objects with string members
   - Some copies may be sharing string storage unexpectedly

2. **Parallel Mode Specifics**:
   - Object pool reuses `StarGenerator` and `accrete` objects
   - `thread_sun` is a local variable passed to both:
     - `generate_stellar_system()` which copies it to `planet.theSun`
     - `SystemOutputData` constructor which copies it to `.the_sun`
   - Timing/sequencing of destruction may cause issues

3. **Observed Behavior**:
   - Sequential mode (`-c1`) works fine with valgrind
   - Parallel mode (`-j2 -c2`) crashes with double-free
   - Crash occurs during cleanup/destruction phase

## Code Locations

- `structs.h:74` - Sun copy constructor declaration
- `structs.cpp:878-936` - Sun copy constructor implementations (now removed)
- `stargen.cpp:762` - `thread_sun` local variable creation
- `stargen.cpp:812` - Passing `thread_sun` to SystemOutputData
- `stargen.cpp:2177` - `the_planet->setTheSun(the_sun)` copies sun to planet
- `structs.cpp:1578` - `setTheSun()` implementation uses assignment
- `stargen.cpp:169-220` - SystemOutputData struct definition

## Next Steps

1. Add explicit move/copy semantics to sun class (Rule of Five)
2. Consider using shared_ptr for sun objects to avoid copying
3. Investigate if compiler-generated assignment operator is correct
4. Check if there's hidden state/caching in sun that isn't being copied
5. Use AddressSanitizer for more detailed memory tracking
6. Add logging to track sun object creation/destruction

## Testing Commands

```bash
# Works fine:
./stargen -t -c1 -s42 -m1.0
valgrind ./stargen -t -c1 -s42 -m1.0

# Crashes with double-free:
./stargen -t -j2 -c2 -s42 -m1.0
./stargen -t -j4 -c10 -s42 -m1.0
```
