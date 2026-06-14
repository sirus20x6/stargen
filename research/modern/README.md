# StarGen modernization research (2026-06-14)

This folder records a literature-currency review of StarGen's physical models. The
parent `research/` folder holds the original 1969–1987 source papers StarGen was
built from; this `modern/` subfolder records (a) a map of what StarGen currently
implements, and (b) where modern (≈2000–2026) literature offers better data/models,
with concrete formulas, values, citations, and a KEEP / AUGMENT / REPLACE verdict
for each.

The review was produced by mining the source PDFs (see `09-old-source-papers-summary.md`)
and then doing targeted web research per subsystem. StarGen-side constants and file
locations were verified directly against the tree; modern paper values were gathered
via web search — a few (noted inline) came from search extracts rather than the source
PDFs and should be confirmed against the originals before hard-coding.

## Documents

| File | Subsystem |
|---|---|
| `00-current-implementation-map.md` | What StarGen models today, file:function level |
| `01-accretion-and-tidal-dynamics.md` | Accretion, isolation mass, Hill spacing, tidal disruption |
| `02-eccentricity-damping.md` | Orbital eccentricity / inclination damping |
| `03-disk-structure-and-snowline.md` | Nebula surface density, disk mass, snow line |
| `04-mass-radius-interior.md` | Planet mass–radius relations, radius valley |
| `05-composition-condensation.md` | Condensation sequence, composition vs distance |
| `06-habitable-zone-climate.md` | Habitable zone + greenhouse/surface temperature |
| `07-stellar-relations-binaries.md` | M–L–R–T_eff, lifetime, multiplicity, circumbinary stability |
| `08-spin-obliquity-validation.md` | Spin/obliquity, tidal locking, population validation targets |
| `09-old-source-papers-summary.md` | What the original 1969–1987 papers contain |

## Headline

StarGen is **already current** in some places and **badly dated** in others; the
modern replacements are mostly *cheaper* closed-form formulas, not heavier ones.

### Already current — keep
- **Habitable zone**: Kopparapu et al. (2013/2014) polynomial with erratum-corrected
  coefficients, verified digit-for-digit in `enviro.cpp:1754`, incl. planet-mass
  interpolation. Still the field standard.
- **Main-sequence lifetime**: `τ = 10 Gyr·(M/L)` (`structs.cpp:856`) is the correct
  physical form; improves automatically once the M–L law is fixed.
- **Gas-giant radii**: Fortney, Marley & Barnes (2007) tables — dated but defensible.
- **Roche constant**: 1.523 fluid-Roche prefactor still standard.

### Outdated — modern drop-in exists
| Feature | Now | Replace with | Verdict |
|---|---|---|---|
| Mass–luminosity | uncalibrated 4-regime power law (`enviro.cpp:36`) | Eker et al. 2018 6-piece M–L–R–T | REPLACE |
| Rocky radius | hand-rolled EOS tables | Zeng et al. 2016 `R=(1.07−0.21·CMF)·M^(1/3.7)` | REPLACE |
| Greenhouse/surface-T | Fogg-1985 grey opacity, fudged | Haqq-Misra/Kopparapu T_s(S,pCO₂) fit | REPLACE |
| Disk surface density | Dole `exp(−α·r^(1/3))` | Lynden-Bell–Pringle tapered power law | REPLACE |
| Composition vs distance | 3-bin `orbital_zone()` + RNG | Lodders 2003 T_c + Wang 2019 devolatilization | AUGMENT |
| Circumbinary stability | constant 3–4× | Holman & Wiegert 1999 a_crit(μ,e) | REPLACE |
| Axial tilt | uniform 0–23.5° | isotropic p(ε)∝sin ε (Kokubo & Ida 2007) | REPLACE |

### New cheap physics
- Eccentricity damping: Cresswell & Nelson 2008 (overrides old Goldreich suggestion).
- Hill-radius spacing + isolation mass (overrides old "tidal disruption gate" idea).
- Snow line `r_snow ≈ 2.7·(L/L⊙)^½ AU`.
- Tidal-lock timescale `τ_lock ∝ a⁶·M⋆⁻²·Q/k₂`.
- Realistic multiplicity (Moe & Di Stefano 2017 + Raghavan 2010).

### Known structural gap
- Radius valley (~1.8 R⊕, Fulton 2017) is an evaporation effect; a Dole accretion
  code won't reproduce it without a bolt-on photoevaporation step. Document, don't force.

### Highest-leverage single addition
- A **population-validation harness** comparing generated systems to Kepler/CKS targets
  (planets/star, peas-in-a-pod, spacing, inclinations, period-ratio pile-ups, η⊕),
  using the Bern/NGPPS battery (Emsenhuber 2021) as the template.
