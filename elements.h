#ifndef ELEMENTS_H
#define ELEMENTS_H
#include <iostream>   // for operator<<, basic_ostream, std::endl, std::cerr, ostream
#include <map>        // for std::map
#include <string>     // for operator<<, string, char_traits
#include <vector>     // for std::vector
#include "structs.h"  // for planet, sun
#include "utils.h"    // for toString, writeMap

extern ChemTable gases;

void initGases();

#endif
