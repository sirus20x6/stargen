#include "ic3094.h"

#include "catalog_loader.h"
#include "const.h"
#include "planets.h"

catalog ic3094;

// Lazily loaded from data/ic3094_2.tsv (see initOmegaGalaxy for rationale).
void initIC3094() {
  static bool loaded = false;
  if (loaded) return;
  loaded = true;
  ic3094.setArg("R");
  star sol(1.0, 1.00, 5780, "G2V", 0, 0, 0, 0, 0, mercury, "Sol", true,
           "The Solar System");
  ic3094.addStar(sol);
  load_old_star_catalog("data/ic3094_2.tsv", ic3094);
}
