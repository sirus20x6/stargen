# StarGen current implementation map

What StarGen models today, with file:function references and a sophistication tag.
Generated tables (`gas_radius_helpers.cpp`, `solid_radius_helpers.cpp`,
`radius_tables.cpp`) are machine-generated lookup tables.

## Nebula / dust-gas distribution — PARTIAL
- Dust density power-law, `accrete.cpp:872`: `DUST_DENSITY_COEFF * sqrt(M*) * L^0.25`,
  Dole exponential `A·exp(−α·r^(1/3))`, `A=2.0E-3`, `α=5`, `n=3` (`const.h:125-127`).
- Cloud eccentricity fixed 0.2 (`accrete.cpp:66`).
- Protoplanet seed mass `1.0E-15` Msun (`const.h:10`).

## Planetesimal injection & dynamics — PARTIAL
- Injected eccentricity Rayleigh, coeff `ECCENTRICITY_COEFF=0.077` (`const.h:9`).
- Gravitational reach reduced-mass `μ^(1/4)` (`accrete.cpp:332`).
- Orbital period Kepler (`enviro.cpp:318`).
- Coalescence on orbital overlap with angular-momentum conservation
  (`accrete.cpp:514-546`): `a₃=(m₁+m₂)/(m₁/a₁+m₂/a₂)`, e from L conservation.
- NO N-body, resonances, migration physics (flag only), tidal evolution.

## Accretion / critical mass — FULL (Dole)
- Critical mass `m_c = B·(perihelion·√L)^(−3/4)`, `B=1.2E-5` (`accrete.cpp:423`).
- Gas-phase density ramp `ρ = K·ρ_dust/(1+√(m_c/m)·(K−1))`, `K=50` (`accrete.cpp:358`).

## Stellar properties — PARTIAL
- Mass→luminosity 4-regime power law (`enviro.cpp:36-46`) — uncalibrated.
- Effective temperature from spectral-type lookup tables (`star_temps.cpp`).
- Main-sequence lifetime `τ = 1.0E10·(M/L)` (`structs.cpp:856`).
- Binary: secondary properties + circumbinary; min stable distance ~3–4×
  separation (`structs.h:106`).
- Habitable zone: Kopparapu polynomial, erratum coeffs (`enviro.cpp:1745-1905`).

## Planet mass / composition — FULL data model
- Mass = dust + gas; ice/rock/carbon mass fractions imf/rmf/cmf (`structs.h:120-122`).
- `assign_composition()` (`enviro.cpp:2499`) sets fractions by `orbital_zone()`
  (3 bins, `enviro.cpp:237`) + random thresholds.
- `acceleration()`/density from per-component densities with a `fudge_factor=.6`
  (`enviro.cpp:548-580`).
- Legacy `empirical_density()` `mass^(1/8)·(r_eco/r)^(1/4)·5.5` (`enviro.cpp:274`).

## Planet radius / density — PARTIAL (tables)
- Rocky: `solid_radius_helpers.cpp` (Seager/Fortney-2007 era EOS tables).
- Gas: `gas_radius_helpers.cpp` Fortney, Marley & Barnes (2007), age-interpolated
  300 Myr / 1 Gyr / 4.5 Gyr (`enviro.cpp:1645-1661`).

## Rotation / tilt — PARTIAL
- Day length default period/2; tidal-lock via spin-orbit resonance factor
  (`enviro.cpp:356-411`).
- Axial tilt random 0–23.5°.

## Surface / atmosphere / habitability — FULL heuristics
- Albedo lookup by type (`const.h:68-86`), cloud/ice/water coverage iterated.
- Surface T radiative equilibrium + greenhouse `green_rise`/`grnhouse`
  (`enviro.cpp:865-897`, Fogg-1985 grey opacity), iterated.
- Volatile inventory, Jeans retention by escape velocity, 13-gas composition.
- Habitable zone, ESI, breathability, Earth-like classification.

## Moons / rings / binary
- Moons: capture by mass, basic Keplerian orbit. Rings: NOT modeled.
- Binary: static circumbinary snapshot.

## NOT modeled at all
N-body perturbations, resonances, migration physics, tidal evolution, dynamical
stability, photoevaporation / radius valley, pebble accretion, fragmentation
(perfect merging only), differentiation/EOS, radiogenic heating, photochemistry,
magnetic fields, stellar mass loss / post-MS evolution.
