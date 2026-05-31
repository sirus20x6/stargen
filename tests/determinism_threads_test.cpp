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
std::string generate_json(const fs::path& run_dir, int threads,
                          const std::string& out_name) {
  std::error_code ec;
  fs::remove_all(run_dir, ec);
  fs::create_directories(run_dir / "html");
  // Make stargen's input data resolvable relative to this cwd.
  fs::create_directory_symlink(fs::path(STARGEN_SOURCE_DIR) / "data",
                               run_dir / "data");
  // Drop the sentinel marker so stargen will write into ./html.
  { std::ofstream(run_dir / "html" / kMarkerName) << "marker\n"; }

  std::ostringstream cmd;
  // cd into the private dir so data/ resolves and output lands in ./html.
  // Fixed seed (-s) and count (-n) with a star mass (-m) so the parallel path
  // runs; JSON output (-JS); -o sets the base filename; -T# the thread count.
  cmd << "cd \"" << run_dir.string() << "\" && "
      << '"' << STARGEN_BIN << '"'
      << " -s12345 -n12 -m1.0 -JS"
      << " -o " << out_name
      << " -T" << threads
      << " > /dev/null 2>&1";

  int rc = std::system(cmd.str().c_str());
  REQUIRE(rc == 0);

  fs::path json_path = run_dir / "html" / (out_name + ".json");
  REQUIRE(fs::exists(json_path));
  std::string contents = read_file(json_path);
  REQUIRE_FALSE(contents.empty());
  return contents;
}

}  // namespace

TEST_CASE("output is identical regardless of worker-thread count",
          "[determinism][threads]") {
  const fs::path base = fs::path(STARGEN_TEST_WORKDIR) / "determinism_run";
  const std::string name = "detsys";

  std::string single = generate_json(base / "t1", 1, name);
  std::string multi = generate_json(base / "t8", 8, name);

  // Byte-for-byte identical: same seed must yield the same systems no matter
  // how many threads generated them.
  REQUIRE(single == multi);

  std::error_code ec;
  fs::remove_all(base, ec);
}
