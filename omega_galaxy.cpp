#include "omega_galaxy.h"

#include "catalog_loader.h"
#include "const.h"
#include "planets.h"

catalog omega_galaxy;

/**
 * @brief Lazily initialize the Omega Galaxy catalogue from data/omega_galaxy2.tsv.
 *
 * Loaded on first use (catalogue selection or the -l/usage banner) rather than
 * eagerly at startup, so ordinary generation runs never pay the ~300k-row parse.
 * The guard makes repeated calls a no-op (and avoids double-appending stars).
 */
void initOmegaGalaxy() {
  static bool loaded = false;
  if (loaded) return;
  loaded = true;
  star sol(1.0, 1.00, 5780, "G2V", 0, 0, 0, 0, 0, mercury, "Sol", true,
           "The Solar System");
  omega_galaxy.addStar(sol);
  load_old_star_catalog("data/omega_galaxy2.tsv", omega_galaxy);
}
