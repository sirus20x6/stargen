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

## Parallel-generation determinism (resolved)

Parallel generation (`-T>1`) is now **deterministic** — single-thread and parallel runs are
byte-identical and the output does not depend on the worker-thread count. The previously
shared-mutable globals on the generation path (the gas-radius `polynomial_cache`, the
`the_sun_clone`/`g_sim_context.current_sun` fallback, the `utils.cpp` RNG context, the jovian
counter, and the Celestia texture RNG) were migrated to per-thread / `thread_local` state.
Relevant commits:
- `9aa0a3c` — make parallel generation deterministic (per-thread sun + gas tables)
- `9f2a639` — eliminate all ThreadSanitizer data races + fix a latent `min>max` abort
- `2607829` — fix jovian-counter data race + nondeterministic Celestia texture RNG

Guarded against regression:
- the ctest case **"output is identical regardless of worker-thread count"** sweeps several
  worker-thread counts and repeats the parallel run, byte-comparing each against the
  single-threaded baseline. It runs on **every** CI lane — gcc, clang, and the Windows
  (clang/MSVC) lane — not just locally.
- `scripts/verify.sh --determinism` adds a 10x `-T8` plus `-T1/-T2/-T4` byte-compare locally.

`flags_arg_clone`, `allow_planet_migration`, and `max_distance_arg` are read-only-after-init
and were correctly left shared.
