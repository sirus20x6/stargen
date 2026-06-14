# Stellar relations & binaries

## A1. Mass–luminosity — REPLACE with Eker et al. 2018
**StarGen now (`enviro.cpp:36-46`):** uncalibrated 4-regime Wikipedia power law
(`L=0.23·M^2.3` M<0.43; `M^4` 0.43–2; `1.4·M^3.5` 2–55; `32000·M` ≥55).

Eker et al. 2018 6-piece classical MLR (`log L = a·log M + b`), 509 eclipsing binaries,
0.179 ≤ M/M⊙ ≤ 31:
| Domain | Mass (M⊙) | log L = a·log M + b |
|---|---|---|
| Ultra low | 0.179–0.45 | 2.028·logM − 0.976 |
| Very low | 0.45–0.72 | 4.572·logM − 0.102 |
| Low | 0.72–1.05 | 5.743·logM − 0.007 |
| Intermediate | 1.05–2.40 | 4.329·logM + 0.010 |
| High | 2.4–7 | 3.967·logM + 0.093 |
| Very high | 7–31 | 2.865·logM + 1.105 |
(α peaks near solar, 5.743, not constant 3.5.) Mass–radius M≤1.5: `R=0.438M²+0.479M+0.075`.
Mass–Teff M>1.5: `log Teff=−0.170(logM)²+0.888 logM+3.671`.
**Citation:** Eker et al. 2018 MNRAS 479, 5491 (arXiv:1807.02568).
**VERDICT: REPLACE.** Drop-in, same signature; need a fallback for M<0.179 and the
very-massive/brown-dwarf tails.

## A2. Spectral-type ↔ Teff ↔ M ↔ L table — refresh to Pecaut & Mamajek
The canonical continually-updated reference is the Mamajek "modern mean dwarf sequence"
(O9V–Y0V, half-subclass steps; SpT, Teff, logL, Mbol, BCv, Mv, R, M).
**Citation:** Pecaut & Mamajek 2013 ApJS 208, 9; live table
http://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
**VERDICT: AUGMENT/REPLACE table data** (keep lookup mechanism) — data refresh.

## A3. Main-sequence lifetime — KEEP
`τ = 10 Gyr·(M/L)` (`structs.cpp:856`) is the correct form (better than M^−2.5);
improves automatically once A1 lands. Optional: mass-dependent fuel fraction.

## B4. Multiplicity — AUGMENT with Moe & Di Stefano 2017 + Raghavan 2010
- Multiplicity rises with primary mass: M-dwarf ~25%, solar ~45%, O/B >70%.
- Raghavan 2010 (solar-type): 54% single; period lognormal logP(days)≈5.0, σ≈2.3
  (a-peak ~50 AU); e flat above tidal cutoff; q roughly flat, slight peak at q≈1.
- Moe & Di Stefano: P–q interrelation — short-P (≲20 d) low e, ⟨q⟩≈0.5, twin excess
  (F_twin≈0.11); long-P power law γ=−1.6 (small q). Companion freq defined for q>0.3.
**Citations:** Raghavan et al. 2010 ApJS 190, 1 (arXiv:1007.0414); Moe & Di Stefano 2017
ApJS 230, 15 (arXiv:1606.05347).
**VERDICT: AUGMENT** — StarGen has the mechanism; drive it from these distributions.

## B5. Circumbinary / circumstellar stability — REPLACE with Holman & Wiegert 1999
**StarGen now:** constant ~3–4× separation (only the equal-mass, e=0 limit).
μ = m₂/(m₁+m₂), e = binary eccentricity, a_B = separation. Valid 0≤μ≤0.5, 0≤e≤0.7.

P-type (circumbinary) min stable a:
```
a_P/a_B = 1.60 + 5.10 e − 2.22 e² + 4.12 μ − 4.27 e μ − 5.09 μ² + 4.61 e² μ²
```
S-type (circumstellar) max stable a:
```
a_S/a_B = 0.464 − 0.380 μ − 0.631 e + 0.586 μ e + 0.150 e² − 0.198 μ e²
```
The constant 3–4× badly underestimates the unstable zone for eccentric binaries (the
+5.10 e term). Modern update: Quarles et al. 2018 AJ 155, 64 (μ to 0.01, e to 0.8).
**Citation:** Holman & Wiegert 1999 AJ 117, 621.
**VERDICT: REPLACE** the constant with the H&W polynomial a_crit(μ,e); both already
available in StarGen. Optionally Quarles 2018 coefficients.

## Priority
1. REPLACE M–L with Eker 2018 (improves L, R, Teff, lifetime at once).
2. REPLACE circumbinary factor with H&W 1999 a_crit(μ,e) (fixes a correctness bug).
3. AUGMENT multiplicity with Moe & Di Stefano + Raghavan distributions.
4. REFRESH spectral-type table to Mamajek sequence.
5. KEEP lifetime formula.
