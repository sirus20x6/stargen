# Habitable zone & greenhouse / climate

The two halves are at opposite ends of currency.

## 1. Habitable zone — ALREADY CURRENT (keep)
StarGen uses the actual Kopparapu et al. (2014) parametric polynomial with
erratum-corrected coefficients, verified digit-for-digit in code:
`calc_stellar_flux` (`enviro.cpp:1745`): `S_eff = seff + a·t + b·t² + c·t³ + d·t⁴`,
`t = T_eff − 5780`; coefficients in `habitable_zone_distance_helper` (`enviro.cpp:1754`),
plus the planet-mass interpolation (`enviro.cpp:1777-1793`).

Erratum coefficients (1 M⊕), matching StarGen:
| Boundary | S_eff☉ | a | b | c | d |
|---|---|---|---|---|---|
| Recent Venus | 1.776 | 2.136e-4 | 2.533e-8 | −1.332e-11 | −3.097e-15 |
| Runaway GH | 1.107 | 1.332e-4 | 1.58e-8 | −8.308e-12 | −1.931e-15 |
| Max GH | 0.356 | 6.171e-5 | 1.698e-9 | −3.198e-12 | −5.575e-16 |
| Early Mars | 0.32 | 5.547e-5 | 1.526e-9 | −2.874e-12 | −5.011e-16 |

No newer model supersedes it (2020–2025 reviews confirm). **VERDICT: KEEP.** Only TODO:
verify provenance of non-Kopparapu modes `FIRST_CO2_CONDENSATION_LIMIT`,
`TWO_AU_CLOUD_LIMIT` (const.h:321-322).
**Citations:** Kopparapu et al. 2013 ApJ 765, 131 (arXiv:1301.6674) + erratum ApJ 770,
82; Kopparapu et al. 2014 ApJL 787, L29 (arXiv:1404.5292).

## 2. Greenhouse / surface temperature — BADLY DATED (replace)
`green_rise` (`enviro.cpp:884`) = `(pow1_4(1+0.75τ)−1)·T_eff·convection_factor`,
`grnhouse` (`enviro.cpp:865`) a binary trigger — Fogg 1985 grey opacity hand-tuned
(`pow(x,.4)`) to hit Venus; no real CO₂ dependence.

**Drop-in replacement:** Haqq-Misra/Kopparapu-lineage closed-form `T_s(S, pCO₂)`: a
~25-term 4th-order bivariate polynomial in X=ln(pCO₂ [bar]) and Y=S/S⊕ fit to a 1D
radiative-convective grid (flux 0.35–1.05 S⊕, pCO₂ 1e-6–10 bar). Evaluates instantly.
**Citation:** "Carbonate-Silicate Cycle Predictions … Testing the HZ Concept,"
arXiv:2012.00819. Alt: ESTM (Vladilo et al. 2015, arXiv:1604.08864) if latitude wanted.
**VERDICT: REPLACE** the greenhouse solver with the T_s(S,pCO₂) polynomial — same call
site, physical CO₂/insolation dependence, GCM-anchored. NOTE: this breaks unit test #6
(grnhouse) and possibly #7 (eff_temp) — update tests + goldens together.

## 3. Inner edge: 3D vs 1D
3D GCMs move the runaway inner edge inward of Kopparapu 1D: Leconte 2013 (1.10 S₀),
Wolf & Toon 2015 (1.21 S₀), Yang et al. 2013 (~2× flux for tidally-locked, cloud
thermostat). 1D is conservative. **VERDICT: KEEP 1D default; AUGMENT** with a
rotation-regime flag + optimistic inner-edge bump for slow/locked rotators.
**Citations:** Leconte et al. 2013 Nature 504, 268; Yang et al. 2013 ApJL 771, L45;
Wolf & Toon 2015.

## 4. Outer edge / CO₂ collapse
Outer edge still max-greenhouse (S_eff≈0.356). Turbet et al. 2017: CO₂ can condense at
cold poles before warming a Snowball, biting from ~1.27 AU (S_eff≈0.62); CO₂-ice clouds
push effective limit to ~1.40 AU. **VERDICT: KEEP boundary; AUGMENT** with a CO₂-collapse-
risk flag. **Citation:** Turbet et al. 2017 EPSL 476, 11.

## 5. M-dwarf habitability flags (cheap, high realism)
PMS runaway + water loss → abiotic O₂ "mirage Earths" (Luger & Barnes 2015); tidal
locking; XUV/flare escape; magnetic suppression. Computable from existing fields
(T_eff, distance, mass, gravity). **VERDICT: AUGMENT** — add flags `tidally_locked`,
`pms_desiccation_risk`, `abiotic_O2_candidate`, `high_XUV_escape_risk`.
**Citation:** Luger & Barnes 2015 Astrobiology 15, 119 (arXiv:1411.7412).

## Priority
1. REPLACE greenhouse solver with T_s(S,pCO₂) polynomial (the stale component).
2. Add M-dwarf habitability flags (cheap, big realism win).
3. Add CO₂-collapse / deglaciation-risk flag.
4. Optional rotation-regime tag + inner-edge bump.
5. HZ boundaries: leave as-is (verify the two non-Kopparapu modes' provenance).
