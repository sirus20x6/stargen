# The original 1969–1987 source papers (research/ folder)

What the PDFs StarGen was built from contain, and which ideas survived the modern review.

## Dole 1969 — "Formation of Planetary Systems by Aggregation" (+ markdown OCR)
StarGen's accrete core IS this paper. Verified faithful: ECCENTRICITY_COEFF=0.077,
B=1.2E-5, cloud_eccentricity=0.2, μ^(1/4) reach, angular-momentum coalescence.
- Canonical dust coeff A=1.5E-3 (StarGen uses 2.0E-3 — a deliberate accrete-lineage
  tweak; the "4 M⊕ critical at 1 AU" and "92% of mass in 0.3–50 AU" lines are free
  regression checks).
- Binary via density coefficient A: reclassify a protoplanet exceeding ~0.02 M⊙ as a
  companion star. (Modern multiplicity stats, doc 07, are the better driver.)

## Magni & Paolicchi 1979 — "A Simple Synthetical Approach"
- Power-law surface density σ∝r^(−X), X∈[0.8,1.9]; outer-region mass-depletion taper
  (1 − q·r/R), q≈1, to fix "outer planets too massive." (Superseded by the Lynden-
  Bell–Pringle tapered power law, doc 03.)
- Generalized capture ring x/r = θ(m/M)^β; physical Hill exponent β=1/3. (Superseded by
  the Hill-radius / isolation-mass treatment, doc 01.)

## Brown 1987 — "Possible Mass Distributions in Other Nebulae"
- Supernova-fragment shear parameter `a` selecting system archetypes (all-giant /
  Sol-like / asteroid-only / retrograde-mixed). Retrograde outer planets are a unique
  exotic feature. (Niche; not pursued — Lynden-Bell–Pringle covers normal disks.)

## Donnison & Williams 1975 — tidal forces on a protoplanet
- Roche survival gate `R_c = 1.523·(M*/ρ)^(1/3)`, disrupt if q/R_c<~1, dispersal
  instantaneous. Constant still valid (doc 01) but the gate-as-spacing-mechanism is
  superseded by Hill regulation.

## Dormand & Woolfson 1971 — Capture Theory & condensation
- Goldreich circularization toward the semi-latus rectum r=a(1−e²). Modern view (doc
  02): wrong global mechanism — only acts inside ~0.1–0.2 AU; use Cresswell-Nelson gas
  damping instead.

## Isaacman & Sagan 1977 — sensitivity to initial conditions
Directly about StarGen's algorithm. Free regression/validation targets:
- Output should be invariant to seed mass (PROTOPLANET_MASS).
- N(ε) ~linear for ε<0.3; gas giants only for nebula mass 0.02–0.2 M⊙.
- β (NDENSITY=3.0 → β=1/3) is dangerously sensitive — a worthwhile sensitivity test.
- Dermott spacing σ realism score.

## Hoyle 1979 + NASA Geology reports
- Hoyle condensation T(r)=3500·√(R/r); planet-by-planet condensation temps. (Superseded
  by Lodders 2003 + Wang 2019, doc 05.)
- Goettel terrestrial bulk compositions (Mercury→Mars Fe-core 65%→12%); Pollack
  composition-dependent greenhouse (CH₄/NH₃ ~10× CO₂); Solomon thermal-history →
  volcanism; Fanale regolith pressure buffering. (Greenhouse superseded by the
  T_s(S,pCO₂) polynomial, doc 06; composition by doc 05.)

## Net
The old papers established the architecture and several still-valid constants. Every
*model* they propose for the enhancement areas now has a better modern replacement
(documented in 01–08), and the modern replacements are generally cheaper closed forms.
