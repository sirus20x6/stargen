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
// counts (-T1 vs -T8) and asserts the produced JSON file is byte-identical.
// JSON is used because it is a stable text format that embeds no timestamps or
// absolute paths, so an exact byte compare is meaningful.

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef STARGEN_BIN
#error "STARGEN_BIN must be defined (path to the stargen executable)"
#endif

namespace fs = std::filesystem;

namespace {

// StarGen refuses to write into an output directory that lacks its sentinel
// marker file (see html/"This Folder Must Exist for Stargen to work!.txt").
constexpr const char* kMarkerName =
    "This Folder Must Exist for Stargen to work!.txt";

std::string read_file(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Run stargen with the given thread count into a fresh output directory and
// return the contents of the generated JSON file. out_name is passed via -o and
// determines the output filename (<out_name>.json).
std::string generate_json(const fs::path& out_dir, int threads,
                          const std::string& out_name) {
  fs::remove_all(out_dir);
  fs::create_directories(out_dir);
  // Drop the sentinel marker so stargen will write here.
  { std::ofstream(out_dir / kMarkerName) << "marker\n"; }

  std::ostringstream cmd;
  // Fixed seed (-s) and count (-n) with a fixed star mass (-m) so the parallel
  // path is taken; JSON output (-JS); -o sets the output base filename; -T#
  // selects the worker-thread count; -path <dir> selects the output directory.
  cmd << '"' << STARGEN_BIN << '"'
      << " -s12345 -n12 -m1.0 -JS"
      << " -o " << out_name
      << " -T" << threads
      << " -path \"" << out_dir.string() << "\""
      << " > /dev/null 2>&1";

  int rc = std::system(cmd.str().c_str());
  REQUIRE(rc == 0);

  fs::path json_path = out_dir / (out_name + ".json");
  REQUIRE(fs::exists(json_path));
  std::string contents = read_file(json_path);
  REQUIRE_FALSE(contents.empty());
  return contents;
}

}  // namespace

TEST_CASE("output is identical regardless of worker-thread count",
          "[determinism][threads]") {
  const fs::path base = fs::temp_directory_path() / "stargen_det_threads";
  const std::string name = "detsys";

  std::string single = generate_json(base / "t1", 1, name);
  std::string multi = generate_json(base / "t8", 8, name);

  // Byte-for-byte identical: same seed must yield the same systems no matter
  // how many threads generated them.
  REQUIRE(single == multi);

  // Cleanup.
  std::error_code ec;
  fs::remove_all(base, ec);
}
