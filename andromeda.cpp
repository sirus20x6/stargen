#include "andromeda.h"

#include <string>

#include "catalog_loader.h"
#include "c_structs.h"
#include "planets.h"

catalog andromeda;

// Lazily loaded from data/andromeda2.tsv (see initOmegaGalaxy for rationale).
void initAndromeda() {
  static bool loaded = false;
  if (loaded) return;
  loaded = true;
  andromeda.setArg("U");
  star sol(1.0, 1.00, 5780, "G2V", 0, 0, 0, 0, 0, mercury, "Sol", true,
           "The Solar System");
  andromeda.addStar(sol);
  load_old_star_catalog("data/andromeda2.tsv", andromeda);
}
