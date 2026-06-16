# Vendored Aurora `au` units library (single-header)

`au.hh` is a single-header amalgamation of Aurora's [`au`](https://github.com/aurora-opensource/au)
compile-time units library, generated with only the units and constants StarGen
uses (to keep compile time down). It is consumed via `#include "au/au.hh"` — the
repo's `third_party/` directory is on the include path — and is wrapped by the
project's `units.hh`, which adds StarGen-specific units (mmHg, atmospheres,
dynes) and aliases.

`au` provides zero-runtime-overhead, `long double`-friendly dimensional analysis.
It compiles under the project toolchain (gcc-16, `-std=c++26`, incl. `-freflection`).

## Regenerating

Pinned upstream commit: `d1b883626f6a7359b6ed55cd3d642b55905b9be4`

```sh
git clone https://github.com/aurora-opensource/au
cd au && git checkout d1b883626f6a7359b6ed55cd3d642b55905b9be4
python3 tools/bin/make-single-file --noio \
  --units meters seconds grams kelvins newtons pascals bars joules watts \
          radians degrees hours days astronomical_units \
  --constants speed_of_light boltzmann_constant avogadro_constant \
              planck_constant standard_gravity \
  --version-id "au-single-header @ <sha> (StarGen vendored)" \
  > ../stargen/third_party/au/au.hh
```

When a subsystem needs a unit/constant not yet present, add it to the `--units`
/ `--constants` list and regenerate, then expose it via `units.hh`. Measured
constants (solar mass, Earth mass, G, Stefan-Boltzmann σ) are not exact units —
define them as `au` *quantities* in `units.hh`, not as `--constants` here.

License: Apache-2.0 (retained in `au.hh`'s header).
