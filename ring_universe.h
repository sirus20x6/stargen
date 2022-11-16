#ifndef RING_UNIVERSE_H
#define RING_UNIVERSE_H
#include "structs.h"
#include "c_structs.h"

using namespace std;

extern catalog ring_universe;

void initRingUniverse();

extern old_star ring_universe_stars[];
extern int total_ring_universe_stars;

#endif