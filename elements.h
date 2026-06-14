#ifndef ELEMENTS_H
#define ELEMENTS_H
#include <iostream>   // for operator<<, basic_ostream, std::endl, std::cerr, ostream
#include <map>        // for std::map
#include <string>     // for operator<<, string, char_traits
#include <vector>     // for std::vector
#include "structs.h"  // for planet, sun
#include "utils.h"    // for toString, writeMap

extern ChemTable gases;

// Load the element/gas table. `path` defaults to the repo-relative location but
// can be overridden to make the data location configurable. Throws
// std::runtime_error (not exit()) if the file is missing, malformed, or empty.
void initGases(const std::string& path = "data/elements.yaml");

#endif
