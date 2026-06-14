# Eccentricity / inclination damping

**StarGen now:** planetesimals injected with Rayleigh eccentricity (coeff 0.077);
eccentricity only changes via merger angular-momentum averaging; NO damping otherwise.

## 1. Type I gas-disk damping (the modern standard)
Wave timescale (Tanaka & Ward 2004, parameterized by Cresswell & Nelson 2008 eq. 9):
```
t_wave = (M*/M_p) · (M*/(Σ a²)) · (H/r)^4 · Ω_K^(-1)
```
Eccentricity / inclination damping (Cresswell & Nelson 2008), with ε=e/h, ι=i/h:
```
t_e = (t_wave/0.780)·(1 − 0.14 ε² + 0.06 ε³ + 0.18 ε ι²)
t_i = (t_wave/0.544)·(1 − 0.30 ι² + 0.24 ι³ + 0.14 ι ε²)
```
For small e,i: `t_e ≈ t_wave/0.78`, `t_i ≈ t_wave/0.544`. Damping is ~(H/r)² faster
than migration: `τ_e/τ_a ~ (H/r)²`. Valid for e,i ≲ H/r; weakens as `ė ∝ e^-2` beyond.

**Citations:** Tanaka & Ward 2004, ApJ 602, 388; Papaloizou & Larwood 2000, MNRAS 315,
823; Cresswell & Nelson 2008, A&A 482, 677 (arXiv:0811.4322); Cresswell et al. 2007,
A&A 473, 329. Refinement: Kanagawa et al. 2024, arXiv:2404.02247 (C&N accurate ~20%).

**VERDICT: AUGMENT (highest priority).** Apply `e ← e·exp(−Δt/t_e)` over an assumed
gas-disk lifetime (a few Myr), aspect ratio h≈0.03–0.05, calibrate to σ_e≈0.04 for
multis. Used by every modern population-synthesis code.

## 2. Gas drag on planetesimals (Adachi, Hayashi & Nakazawa 1976)
`(1/e)(de/dt) = −(1/τ_aero)·√(5/8 e² + 1/2 i² + η²)`; η≈0.5% sub-Keplerian headwind.
Acts on the planetesimal population, which StarGen doesn't track individually.
**VERDICT: KEEP/optional** — #1 is more directly applicable.

## 3. Dynamical friction (Kokubo & Ida)
Equipartition drains random energy of big bodies → low e,i, ~5–10 Hill spacing. The
*correct* reason multis are circular without gas, but intrinsically N-body.
**VERDICT: capture the outcome via #1; do NOT implement** equipartition directly.

## 4. Validation target (observed exoplanet eccentricities)
| Sample | Multi | Single |
|---|---|---|
| Xie et al. 2016 (LAMOST) | ⟨e⟩=0.04, σ≈0.03 | ⟨e⟩=0.32 |
| Van Eylen 2019 | ⟨e⟩=0.076, σ≈0.061 | ⟨e⟩=0.30 |
| Mills et al. 2019 | ⟨e⟩=0.044, σ≈0.04 | ⟨e⟩=0.21 |

Robust: eccentricity–multiplicity anticorrelation; multis σ_e≈0.03–0.06.
**Citations:** Xie et al. 2016, PNAS 113, 11431; Van Eylen et al. 2019, AJ 157, 61;
Mills et al. 2019, AJ 157, 145; He, Ford & Ragozzine 2020 (AMD), AJ 160, 276.

## Note on the old folder paper
Dormand & Woolfson 1971 suggested Goldreich circularization toward the semi-latus
rectum. Modern view: that is the WRONG global mechanism — tidal circularization
`τ_e ∝ Q'(a/R_p)^5` only acts inside ~0.1–0.2 AU. **Demote** it to a hot-orbit special
case; use C&N gas damping globally. (Goldreich & Soter 1966, Icarus 5, 375.)

## Priority
1. Add Cresswell & Nelson 2008 gas-disk damping post-step (cheap, closed-form).
2. Add a Kepler σ_e≈0.04 validation diagnostic.
3. Reframe Goldreich circularization as short-period-only.
