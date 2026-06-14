// Error-path contract for the elements YAML loader.
//
// loadElementsFromYAML()/initGases() were changed from calling exit() deep in
// the call stack to THROWING std::runtime_error, so the caller can report and
// exit cleanly (main.cpp wraps initData() in a try/catch). These tests lock that
// contract: a future refactor that reverts to exit() — or swallows the throw —
// will fail here. Each case throws WITHOUT populating the global `gases` table
// (the throw happens before any addChemicle), so it does not perturb other
// tests in this binary.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include "elements.h"

namespace fs = std::filesystem;

TEST_CASE("initGases throws (not exit) on a missing elements file",
          "[elements][error]") {
  REQUIRE_THROWS_AS(initGases("stargen_definitely_missing_elements.yaml"),
                    std::runtime_error);
}

TEST_CASE("initGases throws on an empty elements file", "[elements][error]") {
  const fs::path p = fs::path(STARGEN_TEST_WORKDIR) / "empty_elements.yaml";
  { std::ofstream(p) << ""; }
  REQUIRE_THROWS_AS(initGases(p.string()), std::runtime_error);
  std::error_code ec;
  fs::remove(p, ec);
}

TEST_CASE("initGases throws on a non-positive atomic weight",
          "[elements][error]") {
  const fs::path p = fs::path(STARGEN_TEST_WORKDIR) / "badweight_elements.yaml";
  {
    std::ofstream f(p);
    f << "- atomic_number: 1\n"
      << "  symbol: XX\n"
      << "  atomic_weight: 0.0\n";
  }
  REQUIRE_THROWS_AS(initGases(p.string()), std::runtime_error);
  std::error_code ec;
  fs::remove(p, ec);
}
