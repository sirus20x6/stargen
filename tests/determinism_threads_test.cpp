// Determinism across worker-thread counts.
//
// StarGen's parallel generation path (stargen.cpp) assigns every system a
// deterministic per-index seed (seed_arg + index * increment) and emits the
// collected results in a fixed order during a single-threaded output phase.
// Output is therefore expected to be byte-identical regardless of how many
// worker threads (-T#) ran the generation.
//
// This test shells out to the built `stargen` binary (path injected via the
// STARGEN_BIN compile definition) twice with the SAME seed but DIFFERENT thread
// counts (-T1 vs -T8) and asserts the produced JSON is byte-identical. JSON is
// used because it is a stable text format that embeds no timestamps or absolute
// paths, so an exact byte compare is meaningful.
//
// StarGen resolves its input data (data/*.yaml) relative to the current working
// directory, and with the default output path it writes JSON to <cwd>/html/.
// So each invocation runs in its own private working directory that contains a
// `data` symlink back to the source tree and an `html/` output dir holding the
// sentinel marker StarGen requires. Working dirs live under the build tree
// (STARGEN_TEST_WORKDIR), never /tmp, to avoid tmpfs user quotas.

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef STARGEN_BIN
#error "STARGEN_BIN must be defined (path to the stargen executable)"
#endif
#ifndef STARGEN_SOURCE_DIR
#error "STARGEN_SOURCE_DIR must be defined (repo root holding data/)"
#endif
#ifndef STARGEN_TEST_WORKDIR
#error "STARGEN_TEST_WORKDIR must be defined (scratch dir under the build tree)"
#endif

namespace fs = std::filesystem;

namespace {

// StarGen refuses to write into an output directory that lacks this sentinel.
constexpr const char* kMarkerName =
    "This Folder Must Exist for Stargen to work!.txt";

std::string read_file(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Run stargen with the given thread count inside a private working directory
// and return the generated JSON. With the default output path stargen writes
// <cwd>/html/<out_name>.json.
std::string generate_json(const fs::path& run_dir, int threads, long seed,
                          const std::string& out_name) {
  std::error_code ec;
  fs::remove_all(run_dir, ec);
  fs::create_directories(run_dir / "html");
  // Make stargen's input data resolvable relative to this cwd. On POSIX a
  // symlink is cheapest, but on Windows creating one needs a privilege the CI
  // runner lacks (the call would throw), so copy the small data tree instead.
#ifdef _WIN32
  fs::copy(fs::path(STARGEN_SOURCE_DIR) / "data", run_dir / "data",
           fs::copy_options::recursive);
#else
  fs::create_directory_symlink(fs::path(STARGEN_SOURCE_DIR) / "data",
                               run_dir / "data");
#endif
  // Drop the sentinel marker so stargen will write into ./html.
  { std::ofstream(run_dir / "html" / kMarkerName) << "marker\n"; }

  // Shell differences: cmd.exe needs the null device spelled NUL and a
  // drive-aware `cd /d`; POSIX shells use /dev/null and a plain cd. Both honor
  // `&&`, quoting, and `2>&1`.
#ifdef _WIN32
  const char* kCd = "cd /d ";
  const char* kNull = "NUL";
#else
  const char* kCd = "cd ";
  const char* kNull = "/dev/null";
#endif
  std::ostringstream cmd;
  // cd into the private dir so data/ resolves and output lands in ./html.
  // Fixed seed (-s) and count (-n) with a star mass (-m) so the parallel path
  // runs; JSON output (-JS); -o sets the base filename; -T# the thread count.
  cmd << kCd << '"' << run_dir.string() << "\" && "
      << '"' << STARGEN_BIN << '"'
      << " -s" << seed << " -n12 -m1.0 -JS"
      << " -o " << out_name
      << " -T" << threads
      << " > " << kNull << " 2>&1";

  int rc = std::system(cmd.str().c_str());
  REQUIRE(rc == 0);

  fs::path json_path = run_dir / "html" / (out_name + ".json");
  REQUIRE(fs::exists(json_path));
  std::string contents = read_file(json_path);
  REQUIRE_FALSE(contents.empty());
  return contents;
}

}  // namespace

// The determinism bug this guards against (per-thread RNG / sun / gas-radius
// table state) was INTERMITTENT: a single -T1-vs--T8 comparison passed by luck
// most of the time. To reliably catch a regression we (a) repeat the parallel
// run several times to expose run-to-run jitter at a fixed thread count, and
// (b) sweep several worker-thread counts, all byte-compared against the
// single-threaded baseline for the same seed. A few seeds are checked because
// some seeds produce more gas giants (the former jitter source) than others.
TEST_CASE("output is identical regardless of worker-thread count",
          "[determinism][threads]") {
  const fs::path base = fs::path(STARGEN_TEST_WORKDIR) / "determinism_run";
  const std::string name = "detsys";

  const long seeds[] = {12345, 777, 31337};
  const int thread_counts[] = {2, 4, 8, 16};
  constexpr int kParallelRepeats = 6;  // catch run-to-run nondeterminism at -T8

  for (long seed : seeds) {
    // Single-threaded output is the deterministic reference for this seed.
    const std::string reference = generate_json(base / "ref", 1, seed, name);

    // Repeat the same parallel thread count: a data race shows up as run-to-run
    // variation even when the thread count is held fixed.
    for (int rep = 0; rep < kParallelRepeats; ++rep) {
      std::string repeated = generate_json(base / "rep", 8, seed, name);
      INFO("seed=" << seed << " repeated -T8 run #" << rep
                   << " differs from single-threaded reference");
      REQUIRE(repeated == reference);
    }

    // Sweep thread counts: output must not depend on how many workers ran.
    for (int t : thread_counts) {
      std::string multi = generate_json(base / "t", t, seed, name);
      INFO("seed=" << seed << " -T" << t
                   << " differs from single-threaded reference");
      REQUIRE(multi == reference);
    }
  }

  std::error_code ec;
  fs::remove_all(base, ec);
}
