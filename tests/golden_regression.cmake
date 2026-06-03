# Golden-output regression driver, invoked via `cmake -P`.
#
# Required -D variables:
#   STARGEN_BIN  - absolute path to the stargen executable
#   GOLDEN_DIR   - directory holding the committed golden files
#   SEEDS        - ';'-separated list of seeds to exercise
#   WORKING_DIR  - directory to run stargen from (it loads data/*.yaml via a
#                  path relative to its cwd, so this must be the repo root)
#
# For each seed this runs `${STARGEN_BIN} -t -m1.0 -s<seed>` (deterministic
# text output to stdout for a 1-solar-mass star), normalizes volatile fields,
# and diffs the result against tests/golden/stargen_seed_<seed>.txt.
#
# A star mass (-m1.0) is required: with no mass/luminosity stargen prints an
# error and emits no system, which would make every golden empty and identical.
#
# To (re)write the goldens instead of comparing, set STARGEN_UPDATE_GOLDENS=1
# either as an environment variable or as a -D cache argument. The most
# reliable way to regenerate is to invoke the driver directly (ctest does not
# forward arbitrary environment variables to a test's child process on every
# platform):
#
#   cmake -DSTARGEN_UPDATE_GOLDENS=1 \
#         -DSTARGEN_BIN=<path/to/stargen> \
#         -DGOLDEN_DIR=<repo>/tests/golden \
#         -DSEEDS="12345;42;7" \
#         -P <repo>/tests/golden_regression.cmake
#
# The env-var form also works when the variable is exported to the cmake
# process:  STARGEN_UPDATE_GOLDENS=1 cmake ... -P golden_regression.cmake

if(NOT DEFINED STARGEN_BIN)
    message(FATAL_ERROR "STARGEN_BIN not set")
endif()
if(NOT DEFINED GOLDEN_DIR)
    message(FATAL_ERROR "GOLDEN_DIR not set")
endif()
if(NOT DEFINED SEEDS)
    message(FATAL_ERROR "SEEDS not set")
endif()
if(NOT DEFINED WORKING_DIR)
    message(FATAL_ERROR "WORKING_DIR not set")
endif()

# Replace volatile substrings in `content`; result is returned in `out_var`.
function(normalize_output content out_var)
    set(text "${content}")

    # Normalize line endings (CRLF / lone CR -> LF) so goldens are stable
    # regardless of the platform that produced them.
    string(REGEX REPLACE "\r\n" "\n" text "${text}")
    string(REGEX REPLACE "\r" "\n" text "${text}")

    # Scrub ISO-8601-ish timestamps: 2026-05-30T13:34:00 / 2026-05-30 13:34
    string(REGEX REPLACE
        "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]([T ][0-9][0-9]:[0-9][0-9](:[0-9][0-9])?)?"
        "<TIMESTAMP>" text "${text}")

    # Scrub bare clock times: 13:34:00
    string(REGEX REPLACE
        "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]"
        "<TIME>" text "${text}")

    # Scrub absolute build/source paths so goldens are machine independent.
    string(REGEX REPLACE "/[^ \t\n\":,]*/stargen[^ \t\n\":,]*" "<PATH>" text "${text}")

    # Scrub semantic version strings (e.g. the StarGen revision banner).
    string(REGEX REPLACE "v?[0-9]+\\.[0-9]+\\.[0-9]+" "<VERSION>" text "${text}")

    # Collapse a run of trailing newlines to a single one so writer/reader
    # round-trips stay stable.
    string(REGEX REPLACE "\n+$" "\n" text "${text}")

    set(${out_var} "${text}" PARENT_SCOPE)
endfunction()

set(failures "")

foreach(seed IN LISTS SEEDS)
    execute_process(
        COMMAND "${STARGEN_BIN}" -t -m1.0 "-s${seed}"
        WORKING_DIRECTORY "${WORKING_DIR}"
        OUTPUT_VARIABLE raw_output
        RESULT_VARIABLE run_rc
    )
    if(NOT run_rc EQUAL 0)
        list(APPEND failures "seed ${seed}: stargen exited with ${run_rc}")
        continue()
    endif()

    normalize_output("${raw_output}" actual)

    # Guard against a regression where stargen emits nothing (e.g. a missing
    # star mass): an empty golden would silently "pass" against empty output
    # and defeats the purpose of the harness.
    string(STRIP "${actual}" actual_stripped)
    if(actual_stripped STREQUAL "")
        list(APPEND failures "seed ${seed}: stargen produced empty output")
        continue()
    endif()

    set(golden_file "${GOLDEN_DIR}/stargen_seed_${seed}.txt")

    # Update mode is selected by either a -D cache arg or an environment
    # variable, with any value other than the empty string or "0".
    set(update_goldens FALSE)
    if(DEFINED STARGEN_UPDATE_GOLDENS AND NOT "${STARGEN_UPDATE_GOLDENS}" STREQUAL "0" AND NOT "${STARGEN_UPDATE_GOLDENS}" STREQUAL "")
        set(update_goldens TRUE)
    endif()
    if(DEFINED ENV{STARGEN_UPDATE_GOLDENS} AND NOT "$ENV{STARGEN_UPDATE_GOLDENS}" STREQUAL "0" AND NOT "$ENV{STARGEN_UPDATE_GOLDENS}" STREQUAL "")
        set(update_goldens TRUE)
    endif()

    if(update_goldens)
        file(WRITE "${golden_file}" "${actual}")
        message(STATUS "Updated golden: ${golden_file}")
        continue()
    endif()

    if(NOT EXISTS "${golden_file}")
        list(APPEND failures "seed ${seed}: missing golden ${golden_file} (regenerate with STARGEN_UPDATE_GOLDENS=1)")
        continue()
    endif()

    file(READ "${golden_file}" expected_raw)
    normalize_output("${expected_raw}" expected)

    if(NOT actual STREQUAL expected)
        list(APPEND failures "seed ${seed}: output differs from ${golden_file}")
    endif()
endforeach()

list(LENGTH failures n_fail)
if(n_fail GREATER 0)
    string(REPLACE ";" "\n  " failure_text "${failures}")
    message(FATAL_ERROR "Golden regression failed:\n  ${failure_text}")
endif()

message(STATUS "Golden regression passed for seeds: ${SEEDS}")
