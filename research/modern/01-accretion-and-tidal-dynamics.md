# Accretion dynamics, isolation mass, Hill spacing, tidal disruption

**StarGen now:** planetesimals accrete with no Roche/tidal survival check; bodies
merge on orbital overlap with angular-momentum conservation; spacing is an emergent
side-effect of overlap merging, not an explicit criterion.

## 1. Oligarchic growth — isolation mass + Hill-radius spacing (the modern core)
This is the modern replacement for "spacing emerges from overlap merging," and it is
closed-form (no N-body required).

Mutual Hill radius:
```
R_H,m = ((a_i + a_j)/2) · ((m_i + m_j)/(3 M*))^(1/3)
```
N-body sims (Kokubo & Ida 1998/2000/2002) find oligarchs settle to **~5–10 mutual
Hill radii** apart (canonical ~10), self-regulated by dynamical friction.

Isolation mass (terrestrial-disk closed form, Kokubo & Ida 2002):
```
M_iso ≈ 0.16 · (b̃/10)^(3/2) · (a/AU)^(3/4) · M⊕
```
General power-law disk (Σ∝Σ₀a^−α; Ida & Lin 2004):
```
M_iso = 5e-3 · c_iso^(3/2) · (Σ₀/10 g cm⁻²)^(3/2) · (a/AU)^(2−α) · (M*/M⊙)^(−1/2) M⊕
```

**Citations:** Kokubo & Ida 2002, ApJ 581, 666; Chambers 2001, Icarus 152, 205
(https://ui.adsabs.harvard.edu/abs/2001Icar..152..205C); Raymond et al. review
https://arxiv.org/abs/2411.03453.

**VERDICT: AUGMENT (highest-value).** Adopt isolation mass + ~10 mutual-Hill-radii
spacing as the explicit accretion/spacing criterion. Closed-form, cheap.

## 2. Collision outcomes — perfect merging vs fragmentation
Modern sims use the Leinhardt & Stewart (2012) outcome model (merge / partial
accretion / hit-and-run / erosion / catastrophic). Of giant impacts ~1/3 are partial
accretion, ~1/3 hit-and-run. BUT final planet count and mass distribution from perfect
merging remain broadly consistent (Chambers 2013) — perfect merging inflates accretion
efficiency and ignores hit-and-run/erosion but is fine for first-order statistics.

Largest-remnant law: `M_lr/M_tot = -0.5(Q_R/Q*_RD − 1) + 0.5`.
**Citations:** Leinhardt & Stewart 2012, ApJ 745, 79 (arXiv:1106.6084); Stewart &
Leinhardt 2012, ApJ 751, 32; Chambers 2013, Icarus 224, 43.

**VERDICT: KEEP perfect merging by default; optional AUGMENT** with LS12 closed-form
per-collision algebra if hit-and-run/erosion/debris ever wanted.

## 3. Roche / tidal disruption — still standard
Fluid Roche limit `r_Roche = 1.52·(M*/ρ)^(1/3) = 2.44·R_pl·(ρ_pl/ρ*)^(1/3)`
unchanged since 1975; rigid-body prefactor 1.26. Refinements <2% (oblateness 2.423).
**VERDICT: KEEP the constant.** But the *concept* of a static disruption gate as the
spacing/survival mechanism is superseded by Hill regulation (item 1). The old folder
paper (Donnison & Williams 1975) suggested a Roche gate — modern practice prefers Hill
spacing. Treat the Roche gate as an optional refinement, not the core mechanism.

## 4. Pebble accretion — out of scope for this feature
Lambrechts & Johansen 2012 — dominant for giant-planet cores/outer disk, not a
terrestrial-survival rule. Pebble isolation mass `M_iso ≈ 20·(H/r/0.05)³ M⊕` is a
giant-core-scale cutoff. **VERDICT: do NOT adopt** for survival; major paradigm change.

## Priority
1. AUGMENT with isolation mass + ~10 mutual-Hill-radii spacing (closed-form).
2. KEEP Roche constant (1.523) if a gate is retained; it's a refinement.
3. KEEP perfect merging; optional LS12 mode later.
4. Pebble accretion out of scope.
