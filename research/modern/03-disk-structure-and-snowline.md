# Disk surface density, disk mass, snow line

**StarGen now:** dust density `A·exp(−α·r^(1/3))`, A=2.0E-3 Msun/AU³, α=5, scaled by
√(M*) and L^0.25 — a 3D volume density with no characteristic radius / outer truncation,
predating all observational constraints below.

## 1. MMSN slope/normalization
Hayashi 1981: `Σ = 1700·(r/AU)^(−3/2) g/cm²`, ~0.01–0.02 Msun out to ~30 AU,
`T = 280·(r/AU)^(−1/2) K`. Weidenschilling 1977 Σ₀≈3200. Modern: Desch 2007 steep/
massive (Σ≈50500·r^(−2.168)) is dynamically fatal (Crida 2009). ALMA + α-disk favor
**shallower slopes p≈0.9–1.0**, disk masses higher than the literal minimum.
**Citations:** Hayashi 1981 PTPS 70, 35; Weidenschilling 1977 Ap&SS 51, 153; Desch
2007 ApJ 671, 878; Crida 2009 ApJ 698, 606 (arXiv:0903.3008); Andrews et al. 2009 ApJ
700, 1502.

## 2. Lynden-Bell & Pringle self-similar tapered power law (modern standard shape)
```
Σ(R) = ((2−γ)·M_disk/(2π R_c²)) · (R/R_c)^(−γ) · exp(−(R/R_c)^(2−γ))
```
Inner disk ∝ R^(−γ); exponential taper gives a physical outer truncation (no arbitrary
cutoff). Fit by essentially every modern ALMA disk paper. γ≈1.0.
**Citations:** Lynden-Bell & Pringle 1974 MNRAS 168, 603; Hartmann et al. 1998 ApJ 495,
385; Hughes et al. 2008 ApJ 678, 1119; Andrews et al. 2011 ApJ 732, 42; Manara et al.
2023 PPVII (arXiv:2203.09930).
**VERDICT: REPLACE.** Single highest-value disk change; only marginally more complex
than Dole's exponential, gives slope + truncation + mass in one formula.

## 3. ALMA-observed γ, R_c, M_disk∝M_star
- Slope: continuum median p≈0.9; defensible single value γ≈1.0.
- R_c = 14–198 AU (Ophiuchus); size–luminosity `R_eff ∝ L_mm^0.5`; M_d ∝ R_c^1.6.
- **M_disk ∝ M_star^(~1.8)** (Ansdell 2016, Lupus), super-linear, with ~0.8–0.9 dex
  intrinsic scatter at fixed M_star; steepens with age.
**Citations:** Andrews et al. 2010 ApJ 723, 1241; Tripathi et al. 2017 ApJ 845, 44;
Andrews et al. 2018b ApJL 865, L13; Ansdell et al. 2016 ApJ 828, 46; Pascucci et al.
2016 ApJ 831, 125; Hendler et al. 2020 ApJ 895, 126.
**VERDICT: AUGMENT** — replace √(M*) with M_disk drawn from a log-normal centered on
M_star^1.8 with ~0.8 dex scatter (scatter = realistic diversity for a generator).

## 4. Snow line
Position ~2.7 AU in MMSN (T≈150–170 K); ×4 solid-density jump outside. Irradiation
scaling `r_snow ∝ L^(1/2)`; on the MS `r_snow ≈ 2.7·(M*/M⊙) AU`. Viscous heating pushes
it inward early (`r_snow ∝ Ṁ^(2/9)`); Martin & Livio 2012 dead-zone keeps Solar snow
line ~3.1 AU. 1σ range ≈ 2–5 AU (solar), ~0.8 AU (M dwarf).
**Citations:** Hayashi 1981; Martin & Livio 2012 MNRAS 425, L6 & 2013 MNRAS 434, 633;
Mulders et al. 2015 ApJ 807, 9; Garaud & Lin 2007 ApJ 654, 606.
**VERDICT: AUGMENT (easy, high-value):** explicit `r_snow = 2.7·(L*/L⊙)^(1/2) AU` with
×4 ice boost outside.

## Priority
1. REPLACE radial profile with Lynden-Bell–Pringle tapered power law (γ≈1, R_c).
2. AUGMENT disk mass: M_disk ∝ M_star^1.8 log-normal, ~0.8 dex scatter; R_c ∝ via M_d∝R_c^1.6.
3. AUGMENT explicit luminosity-scaled snow line with ×4 ice boost.
4. If a pure power law fallback is kept, use p≈1.0, not 3/2; avoid the Desch nebula.
