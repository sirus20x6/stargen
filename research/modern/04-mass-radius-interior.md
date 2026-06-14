# Planet mass–radius relations & interior structure

**StarGen now:** machine-generated lookup tables. Gas-giant radius = Fortney, Marley &
Barnes (2007) (cited in `gas_radius_helpers.cpp:15-16`), age-interpolated 300 Myr / 1
Gyr / 4.5 Gyr (`enviro.cpp:1657-1661`). Rocky = Seager 2007/Fortney 2007-era multi-layer
EOS tables (`solid_radius_helpers.cpp`, called `enviro.cpp:1483-1576`).

## 1. Rocky planets — Zeng et al. 2016/2019
Zeng, Sasselov & Jacobsen 2016 (note: Jacobsen, not Stewart), PREM-based closed form:
```
R/R⊕ = (1.07 − 0.21·CMF) · (M/M⊕)^(1/3.7)
```
Validity 1–8 M⊕, CMF (core mass fraction) 0.0–0.4; matches curves to ~0.01 R⊕. Earth
CMF=0.26±0.07. Exponent 1/3.7≈0.27 is the canonical Earth-composition slope.
Zeng et al. 2019 (PNAS) extends to water/ice worlds via tabulated grids; "Earth-like" =
32.5% Fe + 67.5% MgSiO₃.
**Citations:** Zeng et al. 2016 ApJ 819, 127 (arXiv:1512.08827); Zeng et al. 2019 PNAS
116, 9723 (arXiv:1906.04253).
**VERDICT: REPLACE** the rocky branch — one line replaces much of `solid_radius_helpers`;
keep Zeng 2019 tables for the icy/water branch.

## 2. Probabilistic / population relations
- Wolfgang et al. 2016: sub-Neptunes `M = 2.7·(R/R⊕)^1.3 M⊕`, scatter ~1.9 M⊕.
- Chen & Kipping 2017 "Forecaster": broken power law, Terran→Neptunian break ~2.0 M⊕
  (slope S≈0.28 Terran), Neptunian→Jovian ~130 M⊕, Jovian S≈−0.04. (Upper breakpoints
  from secondary summaries — verify against forecaster repo before hard-coding.)
- Otegi et al. 2020 two-regime, density split ρ=3.3 g/cm³: rocky `R=1.03·M^0.29`,
  volatile-rich `R=0.70·M^0.63`.
- Müller et al. 2024 "revisited" (arXiv:2311.12593): rocky slope ≈0.28, consistent.
**VERDICT:** generator doesn't need inverse-mass estimation; Otegi two-regime is the
most useful cheap analytic fallback / validation envelope.

## 3. Radius valley (Fulton gap) — known structural gap
Bimodal small-planet radii, deficit at **~1.5–2.0 R⊕**, peaks ~1.5 and ~2.4 R⊕
(Fulton 2017). Negative period slope `R∝P^(−0.09)` (Van Eylen 2018). Driven by
photoevaporation (Owen & Wu 2017) / core-powered mass loss (Gupta & Schlichting). Valley
shifts to ~1.3–1.5 R⊕ for low-mass M dwarfs.
**Citations:** Fulton et al. 2017 AJ 154, 109 (arXiv:1703.10375); Van Eylen et al. 2018
MNRAS 479, 4786 (arXiv:1710.05398); Owen & Wu 2017; Gupta & Schlichting 2019.
**VERDICT:** StarGen's accretion model won't carve the valley; a cheap post-hoc
envelope-strip step for close-in low-mass planets would help. Full modeling out of scope.

## 4. Giant-planet radii / inflation
Fortney 2007 still standard for cool/warm giants (check the 2007 erratum ApJ 668, 1267
if equations were ported — tables are fine). Missing: (a) metal enrichment
`M_z ∝ M_p^0.6` (Thorngren et al. 2016; Chachan et al. 2025 `M_z = 14.7 + 0.09(M_p−core)`
M⊕); (b) hot-Jupiter inflation (Teq≳1000 K, radii to ~2 R_Jup) — physics unsolved,
use an empirical inflation bump.
**Citations:** Fortney, Marley & Barnes 2007 ApJ 659, 1661 + erratum 668, 1267;
Thorngren et al. 2016 ApJ 831, 64; Chachan et al. 2025 arXiv:2509.20428.

## Priority
1. Adopt Zeng 2016 closed form for rocky planets (highest value, lowest effort).
2. Add radius-valley realism (envelope-retention rule) — biggest population mismatch.
3. Add empirical hot-Jupiter inflation (Teq≳1000 K); optional Thorngren metal enrichment.
4. Keep Fortney 2007 gas tables (verify erratum).
