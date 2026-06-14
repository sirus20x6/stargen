#include "catalog_loader.h"

#include <cstdlib>      // strtold, atoi
#include <fstream>
#include <stdexcept>
#include <string>

#include "c_structs.h"  // old_star
#include "structs.h"    // catalog, star

void load_old_star_catalog(const std::string& path, catalog& cat) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("StarGen: cannot open catalog data file: " + path);
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;  // header / blank
    }

    // Split into the 12 tab-separated fields (first 11 by tab, the rest = name).
    std::string f[12];
    std::size_t start = 0;
    bool ok = true;
    for (int k = 0; k < 11; ++k) {
      std::size_t tab = line.find('\t', start);
      if (tab == std::string::npos) {
        ok = false;
        break;
      }
      f[k].assign(line, start, tab - start);
      start = tab + 1;
    }
    if (!ok) {
      continue;  // malformed row
    }
    f[11].assign(line, start, std::string::npos);

    // The c_str() pointers only need to outlive the star() construction below,
    // which copies spec_type/desig/name into std::string members.
    old_star os;
    os.luminosity  = std::strtold(f[0].c_str(), nullptr);
    os.mass        = std::strtold(f[1].c_str(), nullptr);
    os.eff_temp    = std::strtold(f[2].c_str(), nullptr);
    os.spec_type   = const_cast<char*>(f[3].c_str());
    os.m2          = std::strtold(f[4].c_str(), nullptr);
    os.e           = std::strtold(f[5].c_str(), nullptr);
    os.a           = std::strtold(f[6].c_str(), nullptr);
    os.inc         = std::strtold(f[7].c_str(), nullptr);
    os.an          = std::strtold(f[8].c_str(), nullptr);
    os.desig       = const_cast<char*>(f[9].c_str());
    os.in_celestia = std::atoi(f[10].c_str());
    os.name        = const_cast<char*>(f[11].c_str());

    star temp(os);
    cat.addStar(temp);
  }
}
