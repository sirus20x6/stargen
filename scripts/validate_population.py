#!/usr/bin/env python3
"""StarGen population-validation harness.

Generates a population of systems (one per seed, single solar-mass star), parses
the JSON output, and reports architecture statistics against modern Kepler/CKS
targets. This is a *diagnostic*, not a pass/fail physics gate: it tells you where
StarGen's generated population diverges from observed planetary systems.

IMPORTANT caveat printed in the report: StarGen emits the *intrinsic* population
(every body it forms), whereas the Kepler/CKS targets are *detection-limited*
(transiting, short-period, R >~ 1 Re). Counts and the radius distribution are
therefore also reported under a crude "detection cut" (P < 400 d, R > 1 Re) so
they are comparable; the architecture metrics (period ratios, Hill spacing,
peas-in-a-pod size correlation, eccentricity, mutual inclination) are compared
on the multi-planet sample directly.

Targets and sources are documented in research/modern/08-spin-obliquity-validation.md
and research/modern/02-eccentricity-damping.md.

Usage:
    scripts/validate_population.py [--bin PATH] [--seeds N] [--mass M] [--json]

Exit status is 0 unless --check is passed and a gross-sanity bound is violated
(used by the ctest wrapper; the bounds are deliberately loose to avoid flaking on
statistical noise).
"""
from __future__ import annotations

import argparse
import json
import math
import os
import subprocess
import sys
import tempfile

R_EARTH_KM = 6371.0
SUN_MASS_IN_EARTH = 332946.0
JUP_EARTH = 317.8  # Jupiter mass in Earth masses (const.h JUPITER_MASS); giants = M > 0.3 M_Jup


def repo_root() -> str:
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def generate(bin_path: str, work: str, seed: int, mass: float) -> dict | None:
    """Run stargen for one seed and return the parsed system, or None on failure."""
    html = os.path.join(work, "html")
    os.makedirs(html, exist_ok=True)
    open(os.path.join(html, "This Folder Must Exist for Stargen to work!.txt"), "a").close()
    data_link = os.path.join(work, "data")
    if not os.path.exists(data_link):
        os.symlink(os.path.join(repo_root(), "data"), data_link)
    out = os.path.join(html, "sys.json")
    try:
        os.remove(out)
    except FileNotFoundError:
        pass
    rc = subprocess.run(
        [bin_path, f"-s{seed}", f"-m{mass}", "-JS", "-o", "sys"],
        cwd=work, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    if rc.returncode != 0 or not os.path.exists(out):
        return None
    with open(out) as fh:
        return json.load(fh)


def pearson(xs: list[float], ys: list[float]) -> float:
    n = len(xs)
    if n < 2:
        return float("nan")
    mx, my = sum(xs) / n, sum(ys) / n
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    sxx = sum((x - mx) ** 2 for x in xs)
    syy = sum((y - my) ** 2 for y in ys)
    if sxx == 0 or syy == 0:
        return float("nan")
    return sxy / math.sqrt(sxx * syy)


def percentile(vals: list[float], q: float) -> float:
    if not vals:
        return float("nan")
    s = sorted(vals)
    i = max(0, min(len(s) - 1, int(round(q * (len(s) - 1)))))
    return s[i]


def otegi_radius(mass_earth: float, rocky: bool) -> float:
    """Observed mass-radius relation, Otegi, Bouchy & Helled 2020 (A&A 634, A43,
    arXiv:1911.04745): rocky M = 0.9 R^3.45, volatile-rich M = 1.74 R^1.58
    (M in Earth masses, R in Earth radii). Inverted here to R(M). Valid below
    ~120 M_earth."""
    if rocky:
        return (mass_earth / 0.9) ** (1.0 / 3.45)
    return (mass_earth / 1.74) ** (1.0 / 1.58)


def analyze(systems: list[dict], star_mass: float) -> dict:
    all_e: list[float] = []
    multi_e: list[float] = []
    all_incl: list[float] = []
    counts: list[int] = []
    det_counts: list[int] = []
    radii_earth: list[float] = []          # all small planets (R < 4 Re), intrinsic
    det_radii_earth: list[float] = []      # detection cut (P<400 d, R>1 Re)
    adj_r_inner: list[float] = []
    adj_r_outer: list[float] = []
    outer_larger = 0
    adj_pairs = 0
    hill_spacings: list[float] = []
    period_ratios: list[float] = []
    total_mass_earth: list[float] = []
    otegi_resid: list[float] = []  # (R - R_Otegi)/R_Otegi for non-giant planets < 120 M_earth
    # --- giant-migration baseline (Phase 0 of research/modern/11-giant-migration.md) ---
    giant_a: list[float] = []       # semi-major axis (AU) of giants (M > 0.3 M_Jup)
    giant_e_hot: list[float] = []   # eccentricity of hot giants (a < 0.1 AU)
    giant_e_cold: list[float] = []  # eccentricity of warm/cold giants (a >= 0.1 AU)
    all_mass_e: list[float] = []    # every planet mass (M_earth), for percentiles
    densities: list[float] = []     # bulk density g/cc, for sanity
    n_hot_giant_sys = 0             # systems with >= 1 hot Jupiter (per-star occurrence)
    n_cold_giant_sys = 0            # systems with >= 1 cold Jupiter
    hot_icy = 0                     # hot bodies (a<0.2 AU, Teq>1000 K) still carrying ice
    hot_bodies = 0                  # denominator for hot_icy (composition-staleness guard)

    for sys_ in systems:
        planets = sorted(sys_.get("planets", []), key=lambda p: p["Distance"])
        counts.append(len(planets))
        total_mass_earth.append(sum(p["Total Mass"] for p in planets) * SUN_MASS_IN_EARTH)
        det = 0
        sys_hot_giant = False
        sys_cold_giant = False
        for p in planets:
            e = p["Eccentricity"]
            all_e.append(e)
            all_incl.append(abs(p["Inclination"]))
            r_e = p["Total Radius"] / R_EARTH_KM
            if r_e < 4.0:
                radii_earth.append(r_e)
            if p["Orbit Period"] < 400.0 and r_e > 1.0:
                det += 1
                if r_e < 4.0:
                    det_radii_earth.append(r_e)
            # Observed mass-radius envelope (Otegi 2020): does the generated
            # radius track real exoplanets at this mass? Restrict to Otegi's
            # domain (non-giant, 0.5-120 M_earth); pick the rocky vs volatile
            # branch by bulk density (>= 3.3 g/cc ~ the rocky/water divide).
            m_e = p["Total Mass"] * SUN_MASS_IN_EARTH
            if not p.get("Is Gas Giant", False) and 0.5 <= m_e <= 120.0 and r_e > 0.0:
                r_pred = otegi_radius(m_e, p["Density"] >= 3.3)
                if r_pred > 0.0:
                    otegi_resid.append((r_e - r_pred) / r_pred)
            # giant-migration baseline: classify giants (M > 0.3 M_Jup) by orbit
            a_au = p["Distance"]
            all_mass_e.append(m_e)
            densities.append(p["Density"])
            if m_e > 0.3 * JUP_EARTH:
                giant_a.append(a_au)
                if a_au < 0.1:
                    sys_hot_giant = True
                    giant_e_hot.append(e)
                else:
                    giant_e_cold.append(e)
                if 1.0 <= a_au <= 10.0:
                    sys_cold_giant = True
            # composition-staleness guard: hot bodies (a<0.2 AU, Teq>1000 K) that
            # still carry ice. With migration OFF this should be ~0 (no hot giants).
            if a_au < 0.2 and p.get("Estimated Temperature", 0.0) > 1000.0:
                hot_bodies += 1
                if p.get("Ice Mass Fraction", 0.0) > 0.01:
                    hot_icy += 1
        det_counts.append(det)
        n_hot_giant_sys += 1 if sys_hot_giant else 0
        n_cold_giant_sys += 1 if sys_cold_giant else 0
        if len(planets) >= 2:
            for p in planets:
                multi_e.append(p["Eccentricity"])
            for a, b in zip(planets, planets[1:]):
                adj_pairs += 1
                ra = a["Total Radius"] / R_EARTH_KM
                rb = b["Total Radius"] / R_EARTH_KM
                adj_r_inner.append(ra)
                adj_r_outer.append(rb)
                if rb > ra:
                    outer_larger += 1
                # mutual Hill radius (masses and star mass both in solar units)
                ma = a["Total Mass"]
                mb = b["Total Mass"]
                aa = a["Distance"]
                ab = b["Distance"]
                rh = 0.5 * (aa + ab) * ((ma + mb) / (3.0 * star_mass)) ** (1.0 / 3.0)
                if rh > 0:
                    hill_spacings.append((ab - aa) / rh)
                pa = a["Orbit Period"]
                pb = b["Orbit Period"]
                if pa > 0:
                    period_ratios.append(pb / pa)

    def rms(v: list[float]) -> float:
        return math.sqrt(sum(x * x for x in v) / len(v)) if v else float("nan")

    return {
        "n_systems": len(systems),
        "mean_total_mass_earth": sum(total_mass_earth) / len(total_mass_earth) if total_mass_earth else float("nan"),
        "mean_planets": sum(counts) / len(counts) if counts else float("nan"),
        "mean_planets_detcut": sum(det_counts) / len(det_counts) if det_counts else float("nan"),
        "sigma_e_all": rms(all_e),
        "sigma_e_multi": rms(multi_e),
        "sigma_incl_deg": rms(all_incl),
        "n_planets": len(all_e),
        "peas_radius_corr": pearson(adj_r_inner, adj_r_outer),
        "outer_larger_frac": outer_larger / adj_pairs if adj_pairs else float("nan"),
        "hill_median": percentile(hill_spacings, 0.5),
        "hill_ge10_frac": sum(1 for d in hill_spacings if d >= 10) / len(hill_spacings) if hill_spacings else float("nan"),
        "pr_lt_1p2_frac": sum(1 for r in period_ratios if r < 1.2) / len(period_ratios) if period_ratios else float("nan"),
        "pr_min": min(period_ratios) if period_ratios else float("nan"),
        "radius_valley_count": sum(1 for r in radii_earth if 1.5 <= r <= 2.0),
        "radius_small_count": sum(1 for r in radii_earth if 1.0 <= r < 1.5),
        "radius_subnep_count": sum(1 for r in radii_earth if 2.0 < r <= 3.0),
        "otegi_n": len(otegi_resid),
        "otegi_median_abs_resid": percentile([abs(x) for x in otegi_resid], 0.5),
        "otegi_within25_frac": (sum(1 for x in otegi_resid if abs(x) <= 0.25) / len(otegi_resid)) if otegi_resid else float("nan"),
        "hj_occurrence": n_hot_giant_sys / len(systems) if systems else float("nan"),
        "cj_occurrence": n_cold_giant_sys / len(systems) if systems else float("nan"),
        "cold_hot_ratio": (n_cold_giant_sys / n_hot_giant_sys) if n_hot_giant_sys else float("inf"),
        "n_giants": len(giant_a),
        "giant_a_bins": [
            sum(1 for a in giant_a if a < 0.1),
            sum(1 for a in giant_a if 0.1 <= a < 1.0),
            sum(1 for a in giant_a if 1.0 <= a < 3.0),
            sum(1 for a in giant_a if 3.0 <= a < 10.0),
            sum(1 for a in giant_a if a >= 10.0),
        ],
        "giant_e_hot_mean": (sum(giant_e_hot) / len(giant_e_hot)) if giant_e_hot else float("nan"),
        "giant_e_cold_mean": (sum(giant_e_cold) / len(giant_e_cold)) if giant_e_cold else float("nan"),
        "mass_p10": percentile(all_mass_e, 0.10),
        "mass_p50": percentile(all_mass_e, 0.50),
        "mass_p90": percentile(all_mass_e, 0.90),
        "density_sane_frac": (sum(1 for d in densities if 0.1 < d < 14.0) / len(densities)) if densities else float("nan"),
        "hot_icy_frac": (hot_icy / hot_bodies) if hot_bodies else float("nan"),
        "hot_bodies_n": hot_bodies,
    }


def report(m: dict) -> None:
    def line(name, val, target, fmt="{:.3f}"):
        v = fmt.format(val) if val == val else "  nan"
        print(f"  {name:<34} {v:>10}    target: {target}")

    print("=" * 70)
    print(f"StarGen population validation  ({m['n_systems']} systems, {m['n_planets']} planets)")
    print("=" * 70)
    print("ARCHITECTURE (multi-planet sample; directly comparable to Kepler/CKS):")
    line("peas-in-a-pod radius corr (r)", m["peas_radius_corr"], "~0.5  (Weiss 2018)")
    line("outer-planet-larger fraction", m["outer_larger_frac"], "~0.65 (Weiss 2018)")
    line("median spacing (mutual Hill R)", m["hill_median"], "~20   (Weiss 2018)")
    line("fraction spaced >= 10 R_H", m["hill_ge10_frac"], "~0.93 (Weiss 2018)")
    line("fraction period-ratio < 1.2", m["pr_lt_1p2_frac"], "~0.0  (Weiss 2018)")
    line("min adjacent period ratio", m["pr_min"], ">1.0  (dynamical packing)")
    line("sigma_e (multi-planet)", m["sigma_e_multi"], "0.03-0.06 (Xie16/VanEylen19)")
    line("sigma_e (all)", m["sigma_e_all"], "higher (mixes singles)")
    line("mutual inclination RMS (deg)", m["sigma_incl_deg"], "~1-2  (Fang&Margot12)")
    print()
    print("COUNTS / RADII (intrinsic vs detection-limited; see caveat):")
    line("mean planets / star (intrinsic)", m["mean_planets"], "(StarGen emits ALL bodies)")
    line("mean total planet mass (Earths)", m["mean_total_mass_earth"], "scales ~M*^1.8 (Ansdell 2016)")
    line("mean planets / star (P<400d,R>1Re)", m["mean_planets_detcut"], "~1.4-2.0 (Hsu 2019)")
    print(f"  small-planet radius bins (Re):  1.0-1.5: {m['radius_small_count']:<5}"
          f" 1.5-2.0(valley): {m['radius_valley_count']:<5} 2.0-3.0: {m['radius_subnep_count']}")
    print("      target: a DEFICIT at 1.5-2.0 Re (Fulton 2017). StarGen has no")
    print("      photoevaporation, so it will NOT show the valley -- expected miss.")
    print()
    print("MASS-RADIUS ENVELOPE (generated radii vs observed exoplanets):")
    line("median |R - R_Otegi| / R_Otegi", m["otegi_median_abs_resid"],
         "small (Otegi 2020 obs M-R)")
    line("fraction within +/-25% of Otegi", m["otegi_within25_frac"],
         f">~0.8 desirable  (n={m['otegi_n']})")
    print()
    print("GIANT MIGRATION BASELINE (giants = M > 0.3 M_Jup; targets are POST-migration):")
    line("hot-Jupiter occurrence (a<0.1 AU)", m["hj_occurrence"],
         "~0.005-0.01 (Howard12/Mayor11)", fmt="{:.4f}")
    line("cold-Jupiter occurrence (1-10 AU)", m["cj_occurrence"],
         "~0.10-0.20 (Zhu 2022)")
    line("cold:hot ratio", m["cold_hot_ratio"], "~10-20x", fmt="{:.1f}")
    b = m["giant_a_bins"]
    print(f"  giant a-bins (AU):  <0.1: {b[0]:<4} 0.1-1: {b[1]:<4} 1-3: {b[2]:<4}"
          f" 3-10: {b[3]:<4} >10: {b[4]:<4} (n={m['n_giants']})")
    print("      target shape: HJ pile-up (<0.1) + period valley (0.1-1) + cold peak (1-10)")
    line("giant <e> hot (a<0.1 AU)", m["giant_e_hot_mean"], "~0.06 circularized (Kipping13)")
    line("giant <e> warm/cold (a>=0.1 AU)", m["giant_e_cold_mean"], "~0.27 broad (Kipping13)")
    print()
    print("MASS / DENSITY SANITY (disk-misadventure guard -- must NOT regress):")
    print(f"  planet mass M_earth  p10={m['mass_p10']:.3f}  p50={m['mass_p50']:.3f}"
          f"  p90={m['mass_p90']:.2f}    (watch for inflation)")
    line("fraction 0.1 < rho < 14 g/cc", m["density_sane_frac"], ">0.95")
    line("hot bodies w/ ice (a<0.2,T>1000K)", m["hot_icy_frac"],
         f"~0 (n_hot={m['hot_bodies_n']})")
    print("=" * 70)
    print("CAVEAT: StarGen generates the intrinsic population; Kepler targets are")
    print("detection-limited. Compare architecture metrics directly; compare counts")
    print("and radii only under the detection cut. Sources: research/modern/.")
    print("=" * 70)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--bin", default=os.path.join(repo_root(), "stargen"))
    ap.add_argument("--seeds", type=int, default=120)
    ap.add_argument("--seed-start", type=int, default=1)
    ap.add_argument("--mass", type=float, default=1.0)
    ap.add_argument("--json", action="store_true", help="emit metrics as JSON")
    ap.add_argument("--check", action="store_true", help="exit nonzero on gross-sanity violation")
    args = ap.parse_args()

    if not os.path.exists(args.bin):
        print(f"stargen binary not found: {args.bin}", file=sys.stderr)
        return 2

    os.environ.setdefault("TMPDIR", os.path.join(repo_root(), "build", "_tmp"))
    os.makedirs(os.environ["TMPDIR"], exist_ok=True)

    systems = []
    with tempfile.TemporaryDirectory(dir=os.environ["TMPDIR"]) as work:
        for i in range(args.seeds):
            s = generate(args.bin, work, args.seed_start + i, args.mass)
            if s is not None:
                systems.append(s)

    if not systems:
        print("no systems generated", file=sys.stderr)
        return 2

    m = analyze(systems, args.mass)
    if args.json:
        print(json.dumps(m, indent=2))
    else:
        report(m)

    if args.check:
        # Deliberately loose gross-sanity bounds (guard against catastrophic
        # regressions, not statistical drift).
        bad = []
        if not (0.5 <= m["mean_planets"] <= 40):
            bad.append(f"mean_planets={m['mean_planets']:.2f}")
        if not (0.0 <= m["sigma_e_all"] <= 0.5):
            bad.append(f"sigma_e_all={m['sigma_e_all']:.3f}")
        if m["pr_min"] == m["pr_min"] and m["pr_min"] < 1.0:
            bad.append(f"pr_min={m['pr_min']:.3f} (<1.0: overlapping orbits)")
        if bad:
            print("GROSS-SANITY VIOLATION: " + "; ".join(bad), file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
