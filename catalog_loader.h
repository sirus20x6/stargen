#ifndef CATALOG_LOADER_H
#define CATALOG_LOADER_H

#include <string>

class catalog;

// Load a predefined-star catalogue from a tab-separated data file into `cat`.
// Each non-comment line is 12 tab fields matching old_star:
//   luminosity  mass  eff_temp  spec_type  m2  e  a  inc  an  desig  in_celestia  name
// The numeric fields are full-precision decimal (strtold round-trips the exact
// long double), so a loaded catalogue is byte-identical to the former compiled
// old_star[] arrays. Lines beginning with '#' (the header) and blank lines are
// skipped. Throws std::runtime_error if the file cannot be opened.
void load_old_star_catalog(const std::string& path, catalog& cat);

#endif  // CATALOG_LOADER_H
