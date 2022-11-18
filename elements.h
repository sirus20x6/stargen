#ifndef ELEMENTS_H
#define ELEMENTS_H
#include <iostream>   // for operator<<, basic_ostream, endl, cerr, ostream
#include <map>        // for map
#include <string>     // for operator<<, string, char_traits
#include <vector>     // for vector
#include "structs.h"  // for planet, sun
#include "utils.h"    // for toString, writeMap

extern ChemTable gases;

void initGases();

#endif
