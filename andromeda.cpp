#include "andromeda.h"
#include <string>        // for string
#include "andromeda2.h"  // for andromeda_stars, total_andromeda_stars
#include "c_structs.h"   // for old_star
#include "planets.h"     // for mercury

catalog andromeda;

void initAndromeda() {
  andromeda.setArg("U");
  star sol(1.0, 1.00, 5780, "G2V", 0, 0, 0, 0, 0, mercury, "Sol", true,
           "The Solar System");
  andromeda.addStar(sol);
  for (int i = 0; i < total_andromeda_stars; i++) {
    star temp(andromeda_stars[i]);
    andromeda.addStar(temp);
  }
}
