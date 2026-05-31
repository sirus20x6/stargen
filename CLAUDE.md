# StarGen — project instructions

## Verification & honesty (read this first)

This project has a hard rule, enforced by a git hook, because "I verified it / tests pass"
claims have outrun reality here before:

- **Never state a result, or write a commit message / status claiming success, in the same
  turn as the commands that would justify it.** Run the verification, wait for the output,
  read it, and *then* state the result in a separate step.
- **Only quote numbers (byte counts, test ratios, planet counts) that appear verbatim in a
  tool result currently in view.** If you cannot point to the exact line, do not write the
  number. Do not reuse a number from earlier in the session — re-measure.
- **Prefer pasting raw verification output** over characterizing it. Let the reader see it.
- The canonical check is `scripts/verify.sh` (build + ctest; `--determinism` for the 10x
  parallel byte-compare). Use it instead of ad-hoc command sequences so results are
  reproducible and comparable.
- A `pre-commit` hook runs `scripts/verify.sh` and **refuses commits on a red tree**. Enable
  it once per clone: `git config core.hooksPath hooks`. Override only for deliberate
  non-code commits with `SKIP_VERIFY=1 git commit ...` and say so.

## Build

- Build system: CMake + Unix Makefiles into `./build` (already configured; deps fetched).
- **Always `export TMPDIR=$PWD/build/_tmp` (mkdir -p first) before building or running.**
  `/tmp` is a quota'd tmpfs; a maxed quota silently corrupts compiler temp files and program
  output (this caused phantom build failures and bogus test results earlier).
- **Build target-scoped only:** `cmake --build build --target stargen -j8` and
  `cmake --build build --target stargen_tests -j8`.
  Do **not** run a bare `cmake --build build` — the `viewer` target is currently broken
  (raylib `Vector3` clash) and unrelated; a full build will fail through no fault of yours.
- Tests: `ctest --test-dir build --output-on-failure`.
- Sanitizer builds (`-DSANITIZE=address|thread|undefined`) use a SEPARATE build dir
  (`cmake -B build-asan -DSANITIZE=address`); their executables land in that build dir,
  not the source tree, so they no longer overwrite the normal `./stargen`.

## Shell gotcha (zsh)

The default Bash tool runs under **zsh**, which expands globs differently from bash:
- `grep --include=*.cpp` fails with `no matches found: --include=*.cpp` because zsh tries to
  glob `*.cpp` itself. Quote the pattern (`grep '--include=*.cpp'`), use `git grep`, or run
  the command via the `delegator` `execute` MCP tool (plain `sh`), which is also unaffected by
  the `/tmp` quota. Several "0 results" greps during the determinism work were false negatives
  from this — verify a surprising empty grep with `git grep` before acting on it.

## Running the generator (for manual checks)

`stargen` loads `data/*.yaml` relative to the current directory and, with the default output
path, writes to `<cwd>/html/<name>.json`. To run it standalone, use a scratch dir under
`build/` containing an `html/` subdir with the sentinel file
`This Folder Must Exist for Stargen to work!.txt` and a `data` symlink to the repo's `data/`,
then `cd` into it. Useful flags: `-s<seed>`, `-n<count>`, `-m<mass>`, `-JS` (JSON),
`-o <name>`, `-T<threads>`, `-t` (text to stdout).

## Known open issue: parallel-generation determinism

Parallel generation (`-T>1`) is **not** deterministic and the single-thread vs parallel
paths diverge. A read-only investigation mapped ~76 shared-mutable-global access sites on the
generation path; the confirmed culprits are:
- `polynomial_cache` — a **file-scope global** `std::map` at `enviro.cpp:25`, written without
  synchronization during radius calculation (the gas-giant radius/density FP jitter source).
- the free RNG functions in `utils.cpp` (`random_number`/`about`/`random_eccentricity`/
  `gaussian`/`exponential`/`randf`/`srandf`) draw from the global `g_random_context`.
- `the_sun_clone` (= `g_sim_context.current_sun`) fallback reads/writes in `enviro.cpp`
  habitability + gas-radius helpers (`gas_radius_helpers.cpp` too).

The correct fix is a per-thread/`thread_local` active-context migration of those globals.
`flags_arg_clone`, `allow_planet_migration`, `max_distance_arg` are read-only-after-init and
do NOT need migration. Track this on the "Determinism test" kanban card.
