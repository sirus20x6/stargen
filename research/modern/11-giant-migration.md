# Giant migration: design & feasibility

*StarGen — semi-analytic Dole/Fogg generator. Status: **design proposal, not yet
implemented.** Author-facing planning doc (2026-06-17).*

---

## 1. Goal & why

Add a physically-motivated, **inward-only** giant-planet migration prescription so
StarGen produces a realistic giant-planet period/semi-major-axis distribution
instead of leaving every giant at its formation orbit.

Three payoffs:

1. **Produces hot Jupiters** — a small fraction of giants end at *a* ≈ 0.05 AU, a
   population StarGen cannot currently make in a principled way (random `-r` aside).
2. **Re-activates hot-Jupiter radius inflation.** There is no point inflating radii
   of giants that never get close to their star. Migration is the *upstream*
   prerequisite that makes the (reverted) inflation model meaningful — a follow-on
   PR, not a bundle.
3. **Fixes the giant period distribution** — the observed three-part shape
   (hot-Jupiter pile-up, period valley, cold-Jupiter peak near the snow line),
   none of which StarGen reproduces today.

**Headline target: ~1% hot-Jupiter occurrence** around Sun-like stars, with cold
Jupiters outnumbering hot Jupiters ~10-20×. This is a real multi-week effort with
genuine physics-validation risk, not a quick win. Honest framing: migration
efficiency is an empirical fudge factor even the Bern/NGPPS codes *calibrate*
rather than derive (and still call "likely too efficient"). We match observed
distributions, we do not simulate disks.

---

## 2. What StarGen does today, and why it is unphysical

Migration lives in `dist_planetary_masses()` at **`accrete.cpp:897-951`**, gated by
`allow_planet_migration` (`-r`, `fDoMigration = 0x0080`), non-seed planets only.
It runs **after `accrete_dust()` (line 890) and before `coalesce_planetesimals()`
(line 953)**. Current logic (verified in-tree):

- **All large bodies:** uniform `new_a ∈ [min_distance, a_form]`, fresh `new_e`;
  re-roll up to 1000× if perihelion < `min_distance`.
- **Jovians (≥ 0.7 M_Jup):** ~7% jump to uniform `a ∈ [min_distance, 0.2]`; ~10%
  force `e ≥ 0.1`.
- **Sub-giants (2-10 M⊕, far, dust-rich):** 25% move to uniform `a ∈ [min, a/2]`.

Why it is unphysical: uniform-random destination (no pile-up/valley/cold-peak
structure); no mass/disk dependence on migration distance; arbitrary 0.2 AU hard
wall instead of a disk inner edge/resonance trap (~0.05 AU); no tidal
circularization (can leave a 0.04 AU planet eccentric); no snow-line anchor; and a
fragile two-stream RNG mix (`active_rng()` TLS vs `random_ctx->`). Net: `-r` is
"highly experimental," effectively never run, not golden-covered.

---

## 3. Proposed model

A cheap, **parametric, citable** prescription that mimics the *outcome* of disk
migration without disk/interior ODEs — the Ida & Lin (2008a) / Bern skeleton
(analytic Type I torque with an efficiency cut + viscous Type II), minus the ODE
solver. Compute a migration *timescale*, integrate inward over an estimated
disk-clearing time, stop at a trap, circularize survivors that get close enough.

### 3.0 Where giants form (anchor)
Give the snow line a first-class role: cold-giant peak between the snow line and
the empirical Fulton turnover. `a_snow = 2.7·√(L/L⊙)·k`, `k≈0.6` → ~1.6 AU for the
Sun (Lecar 2006; Hayashi 1981 ref 2.7); outer anchor `a_break ≈ 3.6 AU`
(P_br ≈ 2800 d, Fulton 2021). Do **not** rewrite formation; bias/move giants in the
migration step.

### 3.1 Regime split
- **Gap-opening / Type II threshold:** `q_c ≈ 5×10⁻⁴` ≈ **0.5 M_Jup** at 1 M⊙
  (Crida 2006 torque balance; Lin 1993 thermal `M_p>2M_*(H/R)³`). Below → Type I.
- **Type I** (cores ≲ tens M⊕): Tanaka 2002 isothermal
  `τ_I ≈ 1.1×10⁵·(M_p/M⊕)⁻¹·(r/10AU)^(25/14) yr` (∝ M_p⁻¹, ∝ Σ_gas⁻¹). Apply the
  empirically-required **efficiency reduction `f ≈ 0.1`** (Ida & Lin 2008a: full
  rate over-migrates, cut ≥10×). `f` is the **primary calibration knob**, not a
  first-principles constant.
- **Type II** (above gap mass): viscous, ~mass-independent
  `τ_II ≈ (α(H/r)²Ω)⁻¹`, α≈1e-3-1e-2; slower than Type I but not negligible.

### 3.2 How far a giant moves
No ODE. Estimate a disk-clearing budget `t_disk` (~1-10 Myr) and move inward by the
e-folding fraction completed:
`a_final = a_form·exp(−t_disk/τ_mig)`, clamped at the trap `a_stop`.
The mass/radius dependence in τ *automatically* yields the three-part shape: the
fastest (massive, short-τ) cores reach `a_stop` (pile-up); most gap-opening giants
barely move (cold peak); intermediate cases sparsely fill the **period valley**
(0.1-1 AU) — the valley emerges as the low-probability transition region, deepest
for massive planets (matches Udry 2003).

### 3.3 Stopping (→ hot-Jupiter pile-up)
Halt at **`a_stop ≈ 0.05 AU`** — disk inner edge / magnetospheric cavity (Alfvén
radius; Lin/Bodenheimer/Richardson 1996), or the 2:1 MMR with the truncated edge
(Heller 2019), or corotation — all converge on ~0.05 AU. Produces the pile-up at
P ≈ 3 d. Treat the sharp 3-day spike as a **soft target** (selection-driven,
debated, weak for M>0.5 M_Jup).

### 3.4 Eccentricity (the hot/cold dichotomy — key signature)
- **Cold/warm (a≳0.1 AU):** broad, **⟨e⟩≈0.27**, mode ~0.3 (Kipping 2013; Weldon
  2025). Needs its own broader draw — StarGen's default `ecc_coef` is too circular.
- **Hot (perihelion < a_f≈0.05 AU):** force **e→0** (tidal circularization locus
  `a_f = 0.0393·(M_p/M_J)^−0.057·(M_*/M⊙)^0.023 AU`, Wang 2023; τ_circ sharp at
  ~0.05 AU). Yields hot ⟨e⟩≈0.06. One cut reproduces the observed dichotomy.

### 3.5-3.7 Deferred / out of scope
High-e (scattering/Kozai) channel → follow-on PR. Optional [Fe/H] occurrence
multiplier (if metallicity is tracked). **Not** doing: outward migration / corotation
traps, N-body/resonance capture, disk-structure or interior ODEs, far-out (>10 AU)
turnover fitting (Fulton α2 only ~2σ).

---

## 4. Integration points

- **Location:** replace `accrete.cpp:907-944` (the uniform-random block) with one
  call to a new pure free function `migrate_giant(...)`, keeping the **exact**
  insertion point (after `accrete_dust`, before `coalesce_planetesimals`, so
  migrated a,e feed the Hill merge as today).
- **Signature (RNG-explicit, no globals):**
  `migrate_giant(a,e,total_mass,dust_mass,gas_mass,crit_mass,stell_mass_ratio,stell_luminosity_ratio,a_snow,a_stop,min_distance,RandomContext* rng) → (a_new,e_new)`.
- **Hill-merge interaction** (`coalesce_planetesimals`, mutual-Hill `accrete.cpp:537-541`):
  migrated a,e change overlaps → migrated giants can merge with new neighbors
  (intended, but path-dependent). **Caveats to test:** prevent a coalesced giant
  from migrating twice (mark migrated bodies); neighbor scattering is *not* modeled
  (a giant sweeping inward doesn't perturb planets it passes — architecture may look
  artificially clean).
- **Known, accepted staleness (document in-code):** dust bands computed from
  formation orbits (phantom swept zones); composition (ice/rock/gas) baked at
  formation a — a giant moved to 0.05 AU keeps "icy" composition though T≫1000 K.
  Bound this with validation metric (h); recomputing volatile loss is a follow-on
  if the metric is alarming.

---

## 5. Determinism plan

Must preserve byte-identical serial==parallel (per-thread `RandomContext`,
verify.sh `--determinism`).

1. **Single RNG stream** — `migrate_giant` uses only `rng->`; eliminate the
   `active_rng()`/`random_ctx->` mixing.
2. **Fixed, unconditional draw order** — draw a fixed sequence per giant regardless
   of branch; derive the outcome arithmetically. **Replace rejection loops** (the
   up-to-1000× re-rolls) with an **analytic perihelion clamp** (zero extra draws,
   threshold-robust). *This is the single most important determinism change* —
   rejection loops consume RNG non-uniformly and shift the stream for all downstream
   planets on any later tweak.
3. Document the exact draw sequence in `migrate_giant` and warn in `RandomContext.h`.
4. **Determinism CI:** add a `-r` lane to `verify.sh --determinism`.
5. **Goldens:** unchanged while migration is opt-in (off by default). A golden
   re-baseline happens *only* if/when migration becomes default (§7) — a deliberate
   "physics: enable giant migration by default" change. Statistical regressions are
   caught by `validate_population.py` over 100+ seeds, not the 3-seed suite.

---

## 6. Validation plan

`scripts/validate_population.py` already reports peas-radius, outer-larger,
hill_median/ge10, period ratios, sigma_e, mutual incl, detection counts, Otegi M-R,
and **mean_total_mass_earth**. Migration changes orbits → validate the new
distribution **and** re-confirm mass/architecture.

### 6a. New migration metrics (targets are BANDS)
| Metric | Target | Source |
|---|---|---|
| Hot-Jupiter fraction (a<0.1 AU, M>0.3 M_Jup) | ~0.5-1% (accept 0.4-1.2%) | Howard12/Fressin13/Mayor11 |
| Cold-Jupiter fraction (a≈1-10 AU) | ~10-20% | Cumming08; Zhu22 |
| Cold:hot ratio | ~10-20× | derived |
| log-a histogram: HJ peak ~0.05 AU; valley 0.1-1 AU (deepest for M>2 M_Jup); cold peak near a_snow, turnover ~3.6 AU | qualitative + slopes α1≈+0.4, α2≈−1.5 | Fulton21; Dawson&Johnson18 |
| e by a-bin: hot ⟨e⟩~0.06; cold ⟨e⟩~0.27 (mode 0.3) | dichotomy | Weldon25; Kipping13 |
| Hot-composition consistency: frac of hot (a<0.2 AU, T>1000 K) with ice_frac>0.01 | ~0% | composition-staleness guard |

### 6b. Mandatory mass & architecture checks (the disk-misadventure lesson)
The LBP disk change passed every *architecture* metric while inflating mean mass
~30× into brown-dwarf territory — caught only after `mean_total_mass_earth` was
added. **Migration must not be merged on orbital metrics alone.** Every run:
mean total mass within Ansdell ~M*^1.8; per-mass-bin 10/50/90th percentiles
(catch inflation a mean hides); density sanity (>95% with 0.1<ρ<14 g/cc);
un-migrated architecture intact (peas r≈0.5, hill_median ~20, sigma_e_multi
0.03-0.06, mutual incl 1-2°); detection counts 1.4-2.0.

### 6c. Merge checklist
`-r` byte-identical across threads; HJ fraction in band; cold:hot ~10-20×;
e-by-a dichotomy reproduced; period histogram shows pile-up+valley+cold peak;
**mass not inflated**; architecture in range; density/composition sane. Run
`--check` on **both** modes.

---

## 7. Phased rollout

0. **Instrument first** — add §6a/§6b metrics + capture migration-OFF baseline
   *before* writing the model.
1. **Implement behind `-r`, default OFF** — replace `accrete.cpp:907-944`; goldens
   unchanged; document draw order; add `-r` determinism lane.
2. **Calibrate `f`** (~0.05-0.3) and secondaries (a_stop, σ_e, a_snow) against §6a
   on ≥1000 seeds.
3. **Physics doc** — record assumptions/formulas/citations/calibrated params +
   achieved-vs-target table.
4. **Consider default ON (separate PR)** — gated on all §6 metrics in band, a
   deliberate golden re-baseline, release notes. Defer; do not bundle.
5. **Deferred follow-ons** — hot-Jupiter radius inflation, then high-e scattering
   channel, then [Fe/H] multiplier — each its own validated PR.

### Kill-switch / negative-result criterion
Stays opt-in (or reverts) if, after honest `f` calibration: HJ fraction can't reach
~0.4-1.2% without breaking the cold:hot ratio or cold-Jupiter fraction; **or** the
e-by-a dichotomy can't be reproduced; **or** any mass/architecture regression
appears; **or** determinism can't stay byte-identical. "Opt-in `-r` with a
defensible giant distribution" is an acceptable landing zone; "default ON" is
strictly gated. A clean revert (single-function swap) must stay trivial.

---

## 8. Risks & open questions

**Physics:** `f` is a calibrated fudge (state it); inward-only misses
outward-migration features; the 3-day spike is contested (validate loosely);
transit-vs-RV HJ rates differ 2-3× (accept a band); snow line uncertain/evolving
(anchor outer peak on Fulton break, keep a_snow tunable); far-out turnover ~2σ
(don't tune to it).

**Code:** RNG fragility is the top risk — single stream + fixed draw order +
analytic clamp is non-negotiable; confirm a coalesced giant can't migrate twice;
neighbor scattering unmodeled (bounded by stability/period-ratio check);
composition/dust staleness accepted+bounded; `-r` likely never run for
M-dwarfs/binaries/circumbinary — test those (`min_distance` edge cases).

**Open questions:** single exponential step vs 2-3 substeps to carve the valley
(decide empirically); is [Fe/H] threaded to the giant-formation path?; does `a_stop`
need to scale with stellar mass (start fixed 0.05 AU); cheapest place to compute
a_snow/a_stop once per system (`accrete.cpp:777-814`); `t_disk` fixed vs drawn (one
draw in the documented order).

**Effort (honest):** ~1-2 weeks for Phases 0-3 if the determinism rework is clean;
calibration (Phase 2) is the unpredictable part (valley/pile-up may need τ-form
iteration). Default-ON and the inflation/scattering follow-ons are separate.

---

### Key citations
Tanaka 2002 (Type I τ); Crida 2006 (gap, q_c≈5e-4); Lin 1993 (thermal gap); Ida &
Lin 2008a + review arXiv:2410.00093 (efficiency f); Emsenhuber 2021 (NGPPS/Bern);
Fulton 2021 (CLS II, P_br≈2800 d, arXiv:2105.11584); Dawson & Johnson 2018
(three-part distribution, arXiv:1801.06117); Howard 2012 / Fressin 2013 / Mayor
2011 (HJ rates); Cumming 2008 / Zhu 2022 (cold-Jupiter rate); Lin/Bodenheimer/
Richardson 1996 (magnetospheric cavity); Heller 2019 (2:1 truncation); Wang 2023
(a_f circularization locus, arXiv:2308.06324); Weldon 2025 / Kipping 2013 (⟨e⟩);
Lecar 2006 / Hayashi 1981 (snow line).

### Code anchors
migration block `accrete.cpp:897-951`; insertion between `accrete_dust`
(`accrete.cpp:890`) and `coalesce_planetesimals` (`accrete.cpp:953`); Hill-merge
`accrete.cpp:492-555` (mutual-Hill 537-541); inner/outer bound init
`accrete.cpp:777-814`; flag `stargen.cpp:602`, `stargen.h:33`, `main.cpp:330`; RNG
`RandomContext.h`, `utils.cpp`; validation `scripts/validate_population.py`;
determinism `scripts/verify.sh --determinism`.
