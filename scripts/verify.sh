#!/usr/bin/env bash
# Canonical verification for StarGen. This is the SINGLE source of truth for
# "does the tree build and pass tests". The pre-commit hook calls it to block
# red commits; humans and agents should call it instead of ad-hoc commands so
# that reported results always come from the same place.
#
# Usage:
#   scripts/verify.sh            # build + ctest
#   scripts/verify.sh --determinism   # also run the 10x parallel determinism check
#
# Exit status: 0 only if every step passed. Prints a single PASS/FAIL summary
# block at the end with the REAL numbers — copy that, do not paraphrase it.

set -uo pipefail
cd "$(dirname "$0")/.." || exit 2
REPO="$PWD"

# Keep compiler/temp files off the quota'd /tmp tmpfs (a maxed quota silently
# corrupted builds and test output earlier in this project's history).
export TMPDIR="$REPO/build/_tmp"
mkdir -p "$TMPDIR"

LOG="$TMPDIR/verify.log"
: > "$LOG"
status=0

run() { # <label> <cmd...>
  echo "### $1" >> "$LOG"
  if "${@:2}" >> "$LOG" 2>&1; then
    echo "  $1: ok"
  else
    echo "  $1: FAILED (see $LOG)"
    status=1
  fi
}

echo "verify: building + testing (TMPDIR=$TMPDIR)"

run "build stargen"        cmake --build build --target stargen -j8
run "build stargen_tests"  cmake --build build --target stargen_tests -j8

# Run ctest and capture the real summary line.
ctest_line="(ctest did not run)"
if [ "$status" -eq 0 ]; then
  echo "### ctest" >> "$LOG"
  if ctest --test-dir build --output-on-failure >> "$LOG" 2>&1; then
    :
  else
    status=1
  fi
  ctest_line="$(grep -E 'tests passed|failed out of' "$LOG" | tail -1)"
  [ -z "$ctest_line" ] && ctest_line="(no ctest summary found)"
fi

# Optional: parallel determinism check (10x -T8 + cross thread-count byte compare).
det_line="(skipped; pass --determinism to run)"
if [ "${1:-}" = "--determinism" ] && [ "$status" -eq 0 ]; then
  BIN="$REPO/stargen"
  W="$TMPDIR/_det"; rm -rf "$W"
  gen() { # <tag> <threads>
    local d="$W/$1"; mkdir -p "$d/html"
    : > "$d/html/This Folder Must Exist for Stargen to work!.txt"
    ln -sfn "$REPO/data" "$d/data"
    ( cd "$d" && "$BIN" -s12345 -n12 -m1.0 -JS -o sys -T"$2" >/dev/null 2>&1 )
  }
  det_fail=0
  gen ref 8
  for i in $(seq 1 10); do gen "r$i" 8; cmp -s "$W/ref/html/sys.json" "$W/r$i/html/sys.json" || det_fail=1; done
  for t in 1 2 4; do gen "tc$t" "$t"; cmp -s "$W/ref/html/sys.json" "$W/tc$t/html/sys.json" || det_fail=1; done
  refbytes="$(stat -c%s "$W/ref/html/sys.json" 2>/dev/null || echo 0)"
  if [ "$det_fail" -eq 0 ]; then
    det_line="PASS (10x -T8 + -T1/-T2/-T4 all byte-identical; ref=${refbytes}B)"
  else
    det_line="FAIL (parallel output is non-deterministic across runs/thread-counts)"
    status=1
  fi
fi

echo
echo "================= VERIFY SUMMARY ================="
echo "  ctest:       $ctest_line"
echo "  determinism: $det_line"
if [ "$status" -eq 0 ]; then
  echo "  RESULT:      PASS"
else
  echo "  RESULT:      FAIL  (full log: $LOG)"
fi
echo "================================================="
exit "$status"
