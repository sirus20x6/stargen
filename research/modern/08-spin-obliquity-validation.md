# Planetary spin / obliquity + population validation targets

## A1. Spin & obliquity from accretion
Giant impacts dominate final spin and produce:
- **Obliquity isotropic 0–180°**, prograde/retrograde equally likely; distribution
  `n(ε)dε = ½ sin ε dε` (peaks near 90°). Agnor/Canup/Levison 1999; Chambers 2001;
  Kokubo & Ida 2007.
- **Spin rate Gaussian, ~70% of breakup rate `ω_cr=(GM/R³)^½`, nearly mass-independent**
  (Kokubo & Genda 2010, realistic accretion).
StarGen's uniform 0–23.5° is both too narrow and the wrong shape (excludes Venus ~177°,
Uranus ~98°).
**Citations:** Kokubo & Ida 2007 ApJ 671, 2082; Kokubo & Genda 2010 ApJ 714, L21
(arXiv:1003.4384).
**VERDICT: REPLACE** tilt sampler with isotropic `p(ε) ∝ sin ε` on [0,180°] for impact-
dominated planets. (Optionally draw initial spin near 0.7·ω_cr.)

## A2. Tidal-locking timescale — REPLACE fixed resonance factor
Gladman et al. 1996 form:
```
τ_lock = (ω · a⁶ · C · Q) / (3 G M*² k₂ Rₚ⁵)        (C ∝ Mₚ Rₚ²)
```
Scalings τ∝a⁶, τ∝M*⁻², τ∝Q/k₂. Convenient:
`t_sync ≈ 3.5e5 yr·(Q/k₂/1e3)(ρc/6)·(M*/0.3 M⊙)^(−3/2)·(a/0.1 AU)^(9/2)`.
Regimes (Grießmeier 2005): τ≤0.1 Gyr locked; 0.1–10 maybe; ≥10 not locked. Eccentric
orbits → spin-orbit resonance (Mercury 3:2). M-dwarf HZ planets lock <1 Myr; even an
Earth at 1 AU around the Sun could lock in 4.5 Gyr if initial P≳3 d (Barnes 2017).
**Citation:** Barnes 2017 CMDA 129, 509 (arXiv:1708.02981).
**VERDICT: REPLACE** with τ_lock(a,M*,Mₚ,Rₚ,Q,k₂,age) vs system age → synchronous /
resonance / free spin. StarGen has a,M*,Mₚ,Rₚ; Q,k₂ from rocky/gas defaults.

## A3. Initial spin vs mass
Shortest period density-set: `T_break = 2π(3/4πGρ)^½ ≈ 1.5 h at ρ=5 g/cm³`. Natal spin
~Gaussian, mean ≈0.7 breakup, mass-independent. **VERDICT: AUGMENT** — replace period/2
default with a near-breakup draw, then evolve via A2.

## B4. Occurrence / radius distribution / η⊕ (validation targets)
| Quantity | Value | Source |
|---|---|---|
| Radius valley | deficit 1.5–2.0 R⊕; peaks ~1.5 & ~2.4 R⊕ | Fulton 2017 |
| FGK with close-in super-Earth/sub-Nep | ~30–50% | Petigura+/Fressin+ |
| Planets/star (FGK, bias-corrected) | ~1.4–2.0 | Hsu 2019 |
| GK 1–2 R⊕ at P=25–50 d | 7.7±1.3% | Petigura 2013 |
| η⊕ (Earth-size in HZ, Sun-like) | ~0.1–0.6 (order-of-mag uncertain) | multiple |
**Citations:** Fulton et al. 2017 AJ 154, 109; Petigura et al. 2013 PNAS 110, 19273;
Hsu et al. 2019 AJ 159, 248; Bryson et al. 2021.

## B5. Architecture (peas in a pod)
| Quantity | Value | Source |
|---|---|---|
| Adjacent size correlation | Pearson r=0.53 | Weiss 2018 |
| Outer larger | 65% of pairs | Weiss 2018 |
| Spacing | 93% pairs ≥10 R_H,m; peak ~20 R_H,m; no period ratios <1.2 | Weiss 2018 |
| Period ratios | excess just wide of 2:1 & 3:2 | Fabrycky 2014 |
| Mutual inclinations | Rayleigh σ≈1–2° | Fang & Margot 2012 |
**Citations:** Weiss et al. 2018 AJ 155, 48 (arXiv:1706.06204); Fabrycky et al. 2014
ApJ 790, 146; Fang & Margot 2012.

## B6. Population synthesis as template
Bern/NGPPS (Emsenhuber et al. 2021a/b, A&A 656 A69/A70; NGPPS VII 2025 vs HARPS/Coralie)
and Ida & Lin. Validation battery: mass–radius/mass–distance diagram; radius/mass
distribution incl. valley; occurrence vs period & radius; intra-system architecture;
direct comparison to a detection-limited survey. **ADOPT AS TEMPLATE** for StarGen's
statistical self-checks.

## Priority
1. Fix obliquity → isotropic sin ε (cheap, clearly correct).
2. Replace fixed resonance factor with real τ_lock calc.
3. Build a validation harness (B4+B5, NGPPS template) — tells you where Dole diverges.
4. Initial spin from breakup-scaled draw.
5. Radius valley is an expected miss (evaporation-driven) — document.

**Honesty note:** σ≈1–2° inclination and ~70%-of-breakup spin came from search extracts;
confirm against source PDFs before hard-coding.
