# Condensation sequence & composition vs orbital distance

**StarGen now:** composition assigned by 3-bin `orbital_zone()` (`enviro.cpp:237`) +
hardcoded random thresholds in `assign_composition()` (`enviro.cpp:2499`); the
mass-fraction → density machinery (imf/rmf/cmf, `acceleration()` with `fudge_factor=.6`)
already exists. Only the location→composition mapping is crude. Legacy
`empirical_density()` `mass^(1/8)·(r_eco/r)^(1/4)·5.5` at `enviro.cpp:274`.

## 1. Lodders 2003 — 50% condensation temperatures
Canonical 50% T_c table (10⁻⁴ bar, solar gas), one number per element:
| Phase | 50% T_c (K) |
|---|---|
| Corundum Al₂O₃ | ~1675 |
| Hibonite / refractories | ~1630–1550 |
| Forsterite Mg₂SiO₄ | ~1354 |
| **Iron metal** | **~1334** |
| Enstatite MgSiO₃ | ~1316 |
| Troilite FeS | ~700 |
| Water ice | ~180 |
| C/N ices (CH₄, NH₃, CO₂) | <~200 |
Improves on Grossman 1972: modern abundances (solar Z dropped 1.9%→1.4%), one self-
consistent 50% T_c per element, larger thermochemical database.
**Citation:** Lodders 2003 ApJ 591, 1220 (table reproduced arXiv:0901.1149); Wood et al.
2019 update.
**VERDICT: AUGMENT** — compact lookup converting local disk T → which species are solid;
replaces the meaning of the 3-bin orbital_zone with a continuous temperature axis.

## 2. Wang, Lineweaver & Ireland 2019 — devolatilization law (headline)
Earth/Sun abundance ratio f vs 50% condensation temperature:
```
log(f) = 3.676·log(T_c) − 11.556        (α=3.676±0.142, β=−11.556±0.436)
```
Refractories f≈1; volatiles progressively depleted. Earth devolatilization temperature
T_D=1391±15 K; CI chondrites ~550 K. Ready-to-use partitions: carbon f_ref=0.09±0.08
(91% volatile), oxygen f_ref=0.20±0.04 (80% volatile).
**Citation:** Wang et al. 2019 Icarus 328, 287 (arXiv:1810.12741).
**VERDICT: AUGMENT (single highest-value):** with Lodders, gives a continuous physical
replacement for assign_composition()'s random thresholds — disk T → T_c per element →
log(f) → ice/rock/iron fractions deterministically (+ optional scatter).

## 3. Rocky exoplanet composition (validation)
Dressing et al. 2015: small dense planets (R<1.6 R⊕) lie on a single Earth-like Fe/
silicate line (Kepler-93b: 1.478 R⊕, 4.02 M⊕, 6.88 g/cm³). Zeng 2016 CMF=0.26±0.07;
Zeng 2019 reference Earth-like = 32.5% Fe + 67.5% MgSiO₃ → iron fraction ≈0.325.
**VERDICT: AUGMENT** — anchor default rocky composition to iron fraction ≈0.33; retires
the `fudge_factor=.6` and the 5.5 constant. Optionally adopt Zeng 2016 radius.

## 4. C/O ratio → carbon planets (Bond, Lauretta & O'Brien 2010)
C/O > ~0.8 → carbon planets (graphite/SiC/TiC dominate). Sun C/O≈0.55. Mg/Si sets
silicate mineralogy. StarGen already tracks cmf but assigns it via `pow(rand,8)` — no
physical driver. Add a per-star C/O (≈solar, or from metallicity) with the C/O≈0.8
threshold. Moriarty 2014 lowers effective threshold to ~0.65.
**Citation:** Bond et al. 2010 Icarus 205, 321.
**VERDICT: AUGMENT (low priority, high leverage)** — gives the dead cmf field a real driver.

## 5. Stellar abundance → planet composition
Adibekyan et al. 2021 (Science 374, 330): planet iron fraction correlates with host-star
Fe/(Fe+Mg+Si), slope >4 (contested, Brinkman 2024/25 → closer to 1:1). Putirka & Rarick
2019 + Hinkel & Unterborn 2018: exotic mineralogy rare — a simple Fe/Mg/Si model suffices.
**VERDICT: AUGMENT (lowest priority)** — couple planet baseline iron fraction to stellar
metallicity; second-order vs formation temperature.

## Priority
1. Lodders 2003 T_c table + Wang 2019 devolatilization → continuous composition.
2. Zeng 2016/2019 calibration (iron ≈0.325; retire fudge_factor/5.5).
3. C/O → carbon fraction (give cmf a real driver).
4. Stellar Fe/Mg/Si → planet iron fraction (optional).
