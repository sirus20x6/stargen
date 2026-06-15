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

    for sys_ in systems:
        planets = sorted(sys_.get("planets", []), key=lambda p: p["Distance"])
        counts.append(len(planets))
        total_mass_earth.append(sum(p["Total Mass"] for p in planets) * SUN_MASS_IN_EARTH)
        det = 0
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
        det_counts.append(det)
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
