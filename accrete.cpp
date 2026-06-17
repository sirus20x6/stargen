#include "accrete.h"
#include "AccretionRecorder.h"
#include <cassert>    // for assert
#include <cmath>      // for pow, sqrt, NAN, fabs, exp, M_PI
#include <iostream>   // for operator<<, basic_ostream, char_traits, cerr
#include <string>     // for operator<<, string
#include "const.h"    // for SUN_MASS_IN_EARTH_MASSES, PROTOPLANET_MASS, K
#include "planets.h"  // for BETHyib, Bellerophon, EPSErib, EPSEric, GL849b
#include "stargen.h"  // for flag_verbose, max_distance_arg, fDoMoons, flags...
#include "structs.h"  // for planet, dust, gen, sun
#include "utils.h"    // for toString, random_eccentricity, random_number
#include "RandomContext.h"  // for RandomContext

/**
 * @brief Destructor for accrete class - ensures all allocated memory is freed
 */
accrete::~accrete() {
  // Clean up generation history (includes dust and planets)
  if (hist_head != nullptr) {
    free_generations();
    hist_head = nullptr;
  }

  // dust_bands vector automatically cleaned up (RAII)

  // Clean up any remaining planets (in case they weren't in hist_head)
  if (planet_head != nullptr) {
    free_planet(planet_head);
    planet_head = nullptr;
  }

  // Clean up seed moons if they exist
  if (seed_moons != nullptr) {
    free_planet(seed_moons);
    seed_moons = nullptr;
  }
}

/**
 * @brief Set Initial Conditions
 *
 * @param inner_limit_of_dust
 * @param outer_limit_of_dust
 */
void accrete::set_initial_conditions(long double inner_limit_of_dust,
                            long double outer_limit_of_dust) {
  // Clean up previous allocations to prevent memory leaks
  dust_bands.clear();  // Vector automatically cleans up

  if (hist_head != nullptr) {
    free_generations();
    hist_head = nullptr;
  }

  // Reset planet_head (planets are cleaned up via hist_head)
  planet_head = nullptr;

  // Create initial dust band
  dust initial_dust;
  initial_dust.setOuterEdge(outer_limit_of_dust);
  initial_dust.setInnerEdge(inner_limit_of_dust);
  initial_dust.setDustPresent(true);
  initial_dust.setGasPresent(true);
  dust_bands.push_back(initial_dust);
  dust_left = true;

  cloud_eccentricity = 0.2;
  //	cloud_eccentricity = 0.6;

  // Create new history node
  gen *hist = nullptr;
  hist = new gen();
  hist->dusts = dust_bands;  // Copy dust vector to history
  hist->planets = planet_head;
  hist->next = hist_head;
  hist_head = hist;

  // Transfer ownership: planets are now owned by history, prevent double-free
  planet_head = nullptr;

  ZoneScoped;
}

/**
 * @brief Stellar Dust Limit
 * 
 * @param stell_mass_ratio 
 * @return long double 
 */
auto accrete::stellar_dust_limit(long double stell_mass_ratio) -> long double {
  return 200.0 * std::pow(stell_mass_ratio, (long double) 1.0 / 3.0);
}

/**
 * @brief Returns the nearest a planet can be to this star, based on the mass ratio.
 * 
 * @param stell_mass_ratio 
 * @param nearest_planet_factor 
 * @return long double 
 */
auto accrete::nearest_planet(long double stell_mass_ratio,
                    long double nearest_planet_factor) -> long double {
  return nearest_planet_factor * std::pow(stell_mass_ratio, (long double) 1.0 / 3.0);
}

/**
 * @brief Returns the farthest a planet can be from this star, based on the mass ratio.
 * 
 * @param stell_mass_ratio 
 * @return long double 
 */
auto accrete::farthest_planet(long double stell_mass_ratio) -> long double {
  return 50.0 * std::pow(stell_mass_ratio, (long double) 1.0 / 3.0);
}

/**
 * @brief inner effect limit
 * 
 * @param a 
 * @param e 
 * @param mass 
 * @return long double 
 */
auto accrete::inner_effect_limit(long double a, long double e, long double mass)
    -> long double {
  assert(cloud_eccentricity > -1.0 && "cloud_eccentricity must be > -1.0 to avoid division by zero");
  return a * (1.0 - e) * (1.0 - mass) / (1.0 + cloud_eccentricity);
}

/**
 * @brief outer effect limit
 * 
 * @param a 
 * @param e 
 * @param mass 
 * @return long double 
 */
auto accrete::outer_effect_limit(long double a, long double e, long double mass)
    -> long double {
  assert(cloud_eccentricity < 1.0 && "cloud_eccentricity must be < 1.0 to avoid division by zero");
  return a * (1.0 + e) * (1.0 + mass) / (1.0 - cloud_eccentricity);
}

/**
 * @brief Returns whether or not there is still dust between inside_range and 
 * outside_range in this current accretion process.
 * 
 * @param inside_range 
 * @param outside_range 
 * @return true 
 * @return false 
 */
auto accrete::dust_available(long double inside_range, long double outside_range)
    -> bool {
  bool dust_here = false;

  // Find the first dust band whose outer edge is within our inside range
  size_t index = 0;
  while (index < dust_bands.size() &&
         dust_bands[index].getOuterEdge() < inside_range) {
    index++;
  }

  // If we have no dust band, there's no dust here; otherwise, it depends on the dust record
  if (index >= dust_bands.size()) {
    dust_here = false;
  } else {
    dust_here = dust_bands[index].getDustPresent();
  }

  // This loop ORs together all of the dust bands between the first one we found
  // and the dust band whose inner edge is outside our outside range
  while (index < dust_bands.size() &&
         dust_bands[index].getInnerEdge() < outside_range) {
    dust_here = dust_here || dust_bands[index].getDustPresent();
    index++;
  }

  // Return whether or not we found a dust band in our range that still had dust
  ZoneScoped;
  return dust_here;
}

/**
 * @brief update dust lanes
 * 
 * @param min 
 * @param max 
 * @param mass 
 * @param crit_mass 
 * @param body_inner_bound 
 * @param body_outer_bound 
 */
void accrete::update_dust_lanes(long double min, long double max, long double mass,
                       long double crit_mass, long double body_inner_bound,
                       long double body_outer_bound) {
  bool gas = (mass <= crit_mass);
  dust_left = false;

  // Process each dust band - iterate carefully to handle insertions
  size_t i = 0;
  while (i < dust_bands.size()) {
    dust& band = dust_bands[i];

    if (band.getInnerEdge() < min && band.getOuterEdge() > max) {
      // Case 1: Band spans entire range - split into 3 bands
      // [inner...min] [min...max] [max...outer]
      dust middle_band;
      middle_band.setInnerEdge(min);
      middle_band.setOuterEdge(max);
      middle_band.setGasPresent(band.getGasPresent() ? gas : false);
      middle_band.setDustPresent(false);

      dust right_band;
      right_band.setInnerEdge(max);
      right_band.setOuterEdge(band.getOuterEdge());
      right_band.setGasPresent(band.getGasPresent());
      right_band.setDustPresent(band.getDustPresent());

      // Modify current band to be left part
      band.setOuterEdge(min);

      // Insert middle and right bands after current
      dust_bands.insert(dust_bands.begin() + i + 1, right_band);
      dust_bands.insert(dust_bands.begin() + i + 1, middle_band);

      // Skip past the newly inserted bands
      i += 3;
    } else if (band.getInnerEdge() < max && band.getOuterEdge() > max) {
      // Case 2: Band overlaps max boundary - split into 2
      // [inner...max] [max...outer]
      dust right_band;
      right_band.setInnerEdge(max);
      right_band.setOuterEdge(band.getOuterEdge());
      right_band.setDustPresent(band.getDustPresent());
      right_band.setGasPresent(band.getGasPresent());

      // Modify current band
      band.setOuterEdge(max);
      band.setGasPresent(band.getGasPresent() ? gas : false);
      band.setDustPresent(false);

      // Insert right band after current
      dust_bands.insert(dust_bands.begin() + i + 1, right_band);
      i += 2;
    } else if (band.getInnerEdge() < min && band.getOuterEdge() > min) {
      // Case 3: Band overlaps min boundary - split into 2
      // [inner...min] [min...outer]
      dust right_band;
      right_band.setInnerEdge(min);
      right_band.setOuterEdge(band.getOuterEdge());
      right_band.setDustPresent(false);
      right_band.setGasPresent(band.getGasPresent() ? gas : false);

      // Modify current band
      band.setOuterEdge(min);

      // Insert right band after current
      dust_bands.insert(dust_bands.begin() + i + 1, right_band);
      i += 2;
    } else if (band.getInnerEdge() >= min && band.getOuterEdge() <= max) {
      // Case 4: Band completely within range - modify in place
      if (band.getGasPresent()) {
        band.setGasPresent(gas);
      }
      band.setDustPresent(false);
      i++;
    } else {
      // Case 5: Band outside range - skip
      i++;
    }
  }

  // Check if there's dust left in the affected region
  for (const auto& band : dust_bands) {
    if (band.getDustPresent() &&
        band.getOuterEdge() >= body_inner_bound &&
        band.getInnerEdge() <= body_outer_bound) {
      dust_left = true;
      break;
    }
  }

  // Merge adjacent bands with identical properties
  size_t j = 0;
  while (j < dust_bands.size() - 1) {
    if (dust_bands[j].getDustPresent() == dust_bands[j + 1].getDustPresent() &&
        dust_bands[j].getGasPresent() == dust_bands[j + 1].getGasPresent()) {
      // Merge j+1 into j
      dust_bands[j].setOuterEdge(dust_bands[j + 1].getOuterEdge());
      dust_bands.erase(dust_bands.begin() + j + 1);
      // Don't increment j - check if we can merge with the next band too
    } else {
      j++;
    }
  }

  ZoneScoped;
}

/**
 * @brief collect dust
 * 
 * @param last_mass 
 * @param new_dust 
 * @param new_gas 
 * @param a 
 * @param e 
 * @param crit_mass 
 * @param dust_band 
 * @return long double 
 */
auto accrete::collect_dust(long double last_mass, long double &new_dust,
                  long double &new_gas, long double a, long double e,
                  long double crit_mass, size_t band_index) -> long double {
  // std::cout << EM(last_mass) << " " << EM(new_dust) << " " << EM(new_gas) << " "
  // << a << " " << e << " " << EM(crit_mass) << std::endl;
  long double mass_density = NAN;
  long double temp1 = NAN;
  long double temp2 = NAN;
  long double temp = NAN;
  long double temp_density = 0.0;
  long double bandwidth = NAN;
  long double width = NAN;
  long double volume = NAN;
  long double gas_density = 0.0;
  long double new_mass = NAN;
  long double next_mass = NAN;
  long double next_dust = 0;
  long double next_gas = 0;

  temp = last_mass / (1.0 + last_mass);
  reduced_mass = std::pow(temp, (long double) 0.25);
  r_inner = inner_effect_limit(a, e, reduced_mass);
  r_outer = outer_effect_limit(a, e, reduced_mass);

  if (r_inner < 0.0) {
    r_inner = 0.0;
  }

  // Base case: if we've gone past the last dust band, return 0
  if (band_index >= dust_bands.size()) {
    return 0;
  }

  const dust& dust_band = dust_bands[band_index];

  // If we have dust, use the dust density, otherwise zero
  if (dust_band.getDustPresent()) {
    temp_density = dust_density;
  }

  // If the last mass is below the critical mass, or there's no dust in this
  // dust band, the density is the overall accretion density;
  // otherwise, the mass density is this horrifying formula
  if (last_mass < crit_mass || !dust_band.getGasPresent()) {
    mass_density = temp_density;
  } else {
    mass_density =
        K * temp_density / (1.0 + sqrt(crit_mass / last_mass) * (K - 1.0));
    gas_density = mass_density - temp_density;
  }

  // std::cout << dust_band.getOuterEdge() << " " << dust_band.getInnerEdge() <<
  // std::endl;

  // If the outer edge exceeds the accretion inner limit or the inner edge is
  // outside the outer limit, just collect the dust from the next band
  if (dust_band.getOuterEdge() <= r_inner ||
      dust_band.getInnerEdge() >= r_outer) {
    return collect_dust(last_mass, new_dust, new_gas, a, e, crit_mass,
                        band_index + 1);
  } else {
    bandwidth = r_outer - r_inner;

    temp1 = r_outer - dust_band.getOuterEdge();
    if (temp1 < 0.0) {
      temp1 = 0.0;
    }
    width = bandwidth - temp1;

    // Account for the gap between the inner edge and the start of
    // the accretion radius
    temp2 = dust_band.getInnerEdge() - r_inner;
    if (temp2 < 0.0) {
      temp2 = 0.0;
    }
    width = width - temp2;

    // Calculate the area of a cross-section, and the volume
    temp = 4.0 * PI * std::pow(a, 2.0) * reduced_mass *
           (1.0 - e * (temp1 - temp2) / bandwidth);
    volume = temp * width;

    // Calculate the total mass of this lane plus the mass of the next lane
    new_mass = volume * mass_density;
    new_gas = volume * gas_density;
    new_dust = new_mass - new_gas;

    next_mass = collect_dust(last_mass, next_dust, next_gas, a, e, crit_mass,
                             band_index + 1);

    new_gas = new_gas + next_gas;
    new_dust = new_dust + next_dust;

    return new_mass + next_mass;
  }
  ZoneScoped;
}



/**
 * @brief Orbital radius is in AU, eccentricity is unitless, and the stellar 
 * luminosity ratio is with respect to the sun.  The value returned is the
 * mass at which the planet begins to accrete gas as well as dust, and is
 * in units of solar masses. 
 * 
 * @param orb_radius 
 * @param eccentricity 
 * @param stell_luminosity_ratio 
 * @return long double 
 */
auto accrete::critical_limit(long double orb_radius, long double eccentricity,
                    long double stell_luminosity_ratio) -> long double {
    long double temp = NAN;
  long double perihelion_dist = NAN;

  perihelion_dist = orb_radius - (orb_radius * eccentricity);
  temp = perihelion_dist * sqrt(stell_luminosity_ratio);
  ZoneScoped;
  return B * std::pow(temp, -0.75);
}

/**
 * @brief ACCRETE, the algorithm. Accrete some dust for the current process, using the 
 * supplied mass, the planetoid properties given, and the inner and outer bounds
 * of the new body.
 * 
 * @param seed_mass 
 * @param new_dust 
 * @param new_gas 
 * @param a 
 * @param e 
 * @param crit_mass 
 * @param body_inner_bound 
 * @param body_outer_bound 
 */

void accrete::accrete_dust(long double &seed_mass, long double &new_dust,
                  long double &new_gas, long double a, long double e,
                  long double crit_mass, long double body_inner_bound,
                  long double body_outer_bound) {
    long double new_mass = seed_mass;
  long double temp_mass = NAN, temp_mass2 = NAN;

  do {
    temp_mass = new_mass;
    // std::fixed point algorithm: accumulate more mass until the difference is less
    // than .01% of the old mass
    new_mass =
        collect_dust(new_mass, new_dust, new_gas, a, e, crit_mass, 0);
    if (new_mass < temp_mass) {
      new_mass = temp_mass;
      break;
    }
  } while (!((new_mass - temp_mass) < (0.0001 * temp_mass)));

  // add the new mass to the seed mass
  seed_mass += new_mass;
  // update the dust lanes with the new seed mass
  update_dust_lanes(r_inner, r_outer, seed_mass, crit_mass, body_inner_bound,
                    body_outer_bound);
  ZoneScoped;
}

/**
 * @brief coalesce planetesimals
 * 
 * @param a 
 * @param e 
 * @param mass 
 * @param crit_mass 
 * @param dust_mass 
 * @param gas_mass 
 * @param stell_luminosity_ratio 
 * @param body_inner_bound 
 * @param body_outer_bound 
 * @param do_moons 
 */

void accrete::coalesce_planetesimals(long double a, long double e, long double mass,
                            long double crit_mass, long double dust_mass,
                            long double gas_mass,
                            long double stell_luminosity_ratio,
                            long double body_inner_bound,
                            long double body_outer_bound, bool do_moons) {
    planet *the_planet = nullptr;
  planet *next_planet = nullptr;
  planet *prev_planet = nullptr;
  bool finished = false;
  long double temp = NAN;
  long double diff = NAN;
  long double dist1 = NAN;
  long double dist2 = NAN;

  do_moons = (flags_arg_clone & fDoMoons) != 0;

  finished = false;
  prev_planet = nullptr;
  next_planet = nullptr;

  int count = 0;

  // First we try to find an existing planet with an over-lapping orbit.
  for (the_planet = planet_head, count = 1; the_planet != nullptr;
       the_planet = the_planet->next_planet, count++) {
    // std::cout << "test2" << count << std::endl;
    diff = the_planet->getA() - a;

    if ((diff > 0.0)) {
      dist1 = (a * (1.0 + e) * (1.0 + reduced_mass)) - a;
      /* x aphelion */
      reduced_mass = std::pow(the_planet->getMass() / (1.0 + the_planet->getMass()), 0.25);
      dist2 = the_planet->getA() - (the_planet->getA() * (1.0 - the_planet->getE()) * (1.0 - reduced_mass));
    } else {
      dist1 = a - (a * (1.0 - e) * (1.0 - reduced_mass));
      /* x perihelion */
      reduced_mass = std::pow((the_planet->getMass() / (1.0 + the_planet->getMass())), 0.25);
      dist2 = (the_planet->getA() * (1.0 + the_planet->getE()) * (1.0 + reduced_mass)) - the_planet->getA();
    }

    // Dynamical-stability (oligarchic) merge: bodies separated by fewer than
    // K_STABLE_HILL mutual Hill radii are unstable and collide, on top of the
    // Dole feeding-zone overlap test. Masses are in solar units with the star
    // implicitly ~1 Msun (matching reduced_mass above), exact for a 1-Msun star.
    long double mutual_hill =
        ((the_planet->getA() + a) / 2.0) * std::cbrt((the_planet->getMass() + mass) / 3.0);

    if (fabs(diff) <= fabs(dist1) || fabs(diff) <= fabs(dist2) ||
        (K_STABLE_HILL > 0.0 && fabs(diff) <= K_STABLE_HILL * mutual_hill)) {
      long double new_dust = 0;
      long double new_gas = 0;
      long double new_a =
          (the_planet->getMass() + mass) /
          ((the_planet->getMass() / the_planet->getA()) + (mass / a));

      temp = the_planet->getMass() * sqrt(the_planet->getA()) * sqrt(1.0 - std::pow(the_planet->getE(), 2.0));
      temp += (mass * sqrt(a) * sqrt(1.0 - std::pow(e, 2.0)));
      temp /= ((the_planet->getMass() + mass) * sqrt(new_a));
      temp = 1.0 - std::pow(temp, 2.0);
      if (temp < 0.0 || temp >= 1.0) {
        temp = 0.0;
      }
      e = sqrt(temp);
      if (do_moons || is_predefined_planet(the_planet)) {
        long double existing_mass = 0;

        if (the_planet->first_moon != nullptr) {
          planet *m = nullptr;
          for (m = the_planet->first_moon; m != nullptr; m = m->next_planet) {
            existing_mass += m->getMass();
          }
        }

        if (mass < crit_mass) {
          if (((mass * SUN_MASS_IN_EARTH_MASSES) < 2.5 &&
               (mass * SUN_MASS_IN_EARTH_MASSES) > .0001 &&
               existing_mass < (the_planet->getMass() * 0.05)) ||
              is_predefined_planet(the_planet)) {
            planet *the_moon = nullptr;
            the_moon = new planet();

            the_moon->setDustMass(dust_mass);
            the_moon->setGasMass(gas_mass);

            if (the_moon->getMass() > the_planet->getMass()) {
              long double temp_dust = the_planet->getDustMass();
              long double temp_gas = the_planet->getGasMass();

              the_planet->setDustMass(the_moon->getDustMass());
              the_planet->setGasMass(the_moon->getGasMass());

              the_moon->setDustMass(temp_dust);
              the_moon->setGasMass(temp_gas);
            }

            if (the_planet->first_moon == nullptr) {
              the_planet->first_moon = the_moon;
            } else {
              the_moon->next_planet = the_planet->first_moon;
              the_planet->first_moon = the_moon;
            }
            // the_planet->addMoon(the_moon);

            finished = true;

            if (flag_verbose & 0x0100) {
              std::cerr << "Moon Captured... " << toString(the_planet->getA())
                   << " AU ("
                   << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
                   << " EM) <- " << toString(mass * SUN_MASS_IN_EARTH_MASSES)
                   << " EM\n";
            }
          } else {
            if (flag_verbose & 0x0100) {
              std::string big_moons_message;
              if (existing_mass < (the_planet->getMass() * 0.05)) {
                big_moons_message = "";
              } else {
                big_moons_message = " (big moons)";
              }

              std::string moon_size_message;
              if ((mass * SUN_MASS_IN_EARTH_MASSES) >= 2.5) {
                moon_size_message = ", too big";
              } else if ((mass * SUN_MASS_IN_EARTH_MASSES) <= .0001) {
                moon_size_message = ", too small";
              } else {
                moon_size_message = "";
              }
              std::cerr << "Moon Escapes... " << toString(the_planet->getA())
                   << " AU ("
                   << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
                   << " EM)" << big_moons_message << " "
                   << toString(mass * SUN_MASS_IN_EARTH_MASSES) << " EM"
                   << moon_size_message << std::endl;
            }
          }
        }
      }

      if (!finished && !is_predefined_planet(the_planet)) {
        if (flag_verbose & 0x0100) {
          std::cerr << "Collision between two planetesimals! "
               << toString(the_planet->getA()) << " AU ("
               << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " EM) + " << toString(a) << " AU ("
               << toString(mass * SUN_MASS_IN_EARTH_MASSES)
               << " EM = " << toString(dust_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EMd + " << toString(gas_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EMg [" << toString(crit_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EM])-> " << toString(new_a) << " AU (" << toString(e) << ")"
               << std::endl;
        }

        temp = the_planet->getMass() + mass;
        accrete_dust(temp, new_dust, new_gas, new_a, e, stell_luminosity_ratio,
                     body_inner_bound, body_outer_bound);

        the_planet->setA(new_a);
        the_planet->setE(e);
        the_planet->setDustMass(the_planet->getDustMass() + dust_mass +
                                new_dust);
        the_planet->setGasMass(the_planet->getGasMass() + gas_mass + new_gas);
        if (temp >= crit_mass) {
          the_planet->setGasGiant(true);
        }

        while (the_planet->next_planet != nullptr &&
               the_planet->next_planet->getA() < new_a) {
          next_planet = the_planet->next_planet;

          if (the_planet == planet_head) {
            planet_head = next_planet;
          } else {
            if (prev_planet) {
              prev_planet->next_planet = next_planet;
            }
          }

          the_planet->next_planet = next_planet->next_planet;
          next_planet->next_planet = the_planet;
          prev_planet = next_planet;
        }
      }

      finished = true;
      break;
    } else {
      prev_planet = the_planet;
    }
  }

  if (!finished)  // Planetesimals didn't collide. Make it a planet.
  {
    the_planet = new planet();

    the_planet->setA(a);
    the_planet->setE(e);
    the_planet->setDustMass(dust_mass);
    the_planet->setGasMass(gas_mass);
    the_planet->first_moon = seed_moons;
    the_planet->first_moon_backup = seed_moons;
    
    if (mass >= crit_mass) {
      the_planet->setGasGiant(true);
    } else {
      the_planet->setGasGiant(false);
    }

    if (planet_head == nullptr) {
      planet_head = the_planet;
      the_planet->next_planet = nullptr;
    } else if (a < planet_head->getA()) {
      the_planet->next_planet = planet_head;
      planet_head = the_planet;
    } else if (planet_head->next_planet == nullptr) {
      planet_head->next_planet = the_planet;
      the_planet->next_planet = nullptr;
    } else {
      next_planet = planet_head;
      while (next_planet != nullptr && next_planet->getA() < a) {
        prev_planet = next_planet;
        next_planet = next_planet->next_planet;
      }
      the_planet->next_planet = next_planet;
      if (prev_planet) {
        prev_planet->next_planet = the_planet;
      }
    }
  }
  // Pin history's chain pointer to the CURRENT head every pass. Planetesimals
  // are PREPENDED as the new head (see the `a < planet_head->getA()` case above),
  // so a one-time snapshot (the old `if (... == nullptr)`) left hist_head->planets
  // pointing at an interior node: free_generations() then freed only the suffix
  // and every planet prepended afterwards leaked (ASan: the chain returned by
  // dist_planetary_masses and stored as innermost_planet was never fully freed).
  // hist_head->planets and the returned chain are the SAME nodes, and only the
  // history path frees them, so re-pinning frees the whole chain exactly once.
  hist_head->planets = planet_head;
  ZoneScoped;
}

/**
 * @brief this appears to be the entry point into the entire module - DKL
 * 
 * @param the_sun 
 * @param inner_dust 
 * @param outer_dust 
 * @param outer_planet_limit 
 * @param dust_density_coeff 
 * @param ecc_coef 
 * @param nearest_planet_factor 
 * @param seed_system 
 * @param do_moons 
 * @return planet* 
 */

// --- Giant migration (research/modern/11-giant-migration.md; -r only) ----------
// Parametric inward disk migration of a giant / giant-core. Deterministic: a FIXED
// number of draws (two) in a FIXED order from the per-system RandomContext, and an
// ANALYTIC perihelion clamp (no rejection loops), so serial == parallel and the
// stream is robust to threshold tweaks. Modifies a and e in place. Runs only under
// allow_planet_migration (-r), so the default golden path never executes it.
static void migrate_giant(long double &a, long double &e,
                          long double total_mass_solar, long double stell_mass_ratio,
                          long double min_distance, RandomContext *rng) {
  const long double a_form = a;
  const long double m_p_earth = total_mass_solar * SUN_MASS_IN_EARTH_MASSES;

  // Fixed draw order (always two): available-time fraction, then ecc quantile.
  const long double u_t = rng->randDouble(0.0L, 1.0L);
  const long double u_e = rng->randDouble(0.0L, 1.0L);

  // Inner trap: disk inner edge / magnetospheric cavity, never inside min_distance.
  long double a_stop = MIGRATION_INNER_EDGE_AU;
  if (a_stop < min_distance) {
    a_stop = min_distance;
  }

  // Migration timescale (yr): viscous Type II above the gap-opening mass, faster
  // Tanaka Type I below it; efficiency folds in the empirical rate reduction.
  const long double gap_mass_earth =
      MIGRATION_GAP_MASS_MJUP * JUPITER_MASS * stell_mass_ratio;
  long double tau_yr;
  if (m_p_earth >= gap_mass_earth) {
    tau_yr = MIGRATION_TYPE2_NORM_YR * std::pow(a_form, 1.5L) / sqrt(stell_mass_ratio);
  } else {
    tau_yr = MIGRATION_TYPE1_NORM_YR / m_p_earth * std::pow(a_form / 10.0L, 25.0L / 14.0L);
  }
  tau_yr /= MIGRATION_EFFICIENCY;
  if (tau_yr <= 0.0L) {
    return;
  }

  // Available migration time = fraction of disk lifetime (early formers get the
  // full budget). The spread carves the hot/warm/cold distribution.
  const long double dt = u_t * DISK_LIFETIME_YEARS;
  long double a_new = a_form * std::exp(-dt / tau_yr);
  if (a_new < a_stop) {
    a_new = a_stop;
  }
  if (a_new > a_form) {
    a_new = a_form;  // inward only
  }

  // Eccentricity: broad Rayleigh for cold/warm giants; tidal circularization for
  // hot orbits whose perihelion crosses the circularization locus.
  long double e_broad = GIANT_ECC_SIGMA * sqrt(-2.0L * std::log(1.0L - u_e));
  if (e_broad > GIANT_ECC_MAX) {
    e_broad = GIANT_ECC_MAX;
  }
  long double e_new = e_broad;
  if (a_new * (1.0L - e_broad) < MIGRATION_CIRC_AU) {
    e_new = 0.0L;  // tides circularize hot Jupiters
  }
  // Analytic clamp: keep perihelion >= min_distance without a rejection loop.
  if (a_new > 0.0L && a_new * (1.0L - e_new) < min_distance) {
    e_new = 1.0L - (min_distance / a_new);
    if (e_new < 0.0L) {
      e_new = 0.0L;
    }
  }

  a = a_new;
  e = e_new;
}

auto accrete::dist_planetary_masses(sun &the_sun, long double inner_dust,
                           long double outer_dust,
                           long double outer_planet_limit,
                           long double dust_density_coeff, long double ecc_coef,
                           long double nearest_planet_factor,
                           planet *seed_system, bool do_moons) -> planet * {
    long double stell_mass_ratio = 0;
  long double stell_luminosity_ratio = 0;
  long double a = NAN;
  long double e = NAN;
  long double total_mass = NAN;
  long double dust_mass = NAN;
  long double gas_mass = NAN;
  long double crit_mass = NAN;
  long double planet_inner_bound = NAN;
  long double planet_outer_bound = NAN;
  long double temp = NAN;

  do_moons = (flags_arg_clone & fDoMoons) != 0;
  planet *seeds = seed_system;
  // planet *seed_moons = nullptr;
  planet *moon = nullptr;
  planet *temp_moons = nullptr;
  planet *new_moon = nullptr;
  planet *prev_moon = nullptr;
  bool is_seed = false;

  if (!the_sun.getIsCircumbinary()) {
    stell_mass_ratio = the_sun.getMass();
    stell_luminosity_ratio = the_sun.getLuminosity();
    planet_inner_bound = nearest_planet(stell_mass_ratio, nearest_planet_factor);
  } else {
    stell_mass_ratio = the_sun.getCombinedMass();
    stell_luminosity_ratio = the_sun.getCombinedLuminosity();
    planet_inner_bound = the_sun.getMinStableDistance();
  }

  set_initial_conditions(inner_dust, outer_dust);
  if (recorder != nullptr) { recorder->capture(dust_bands, planet_head); }  // initial dust disk

  if (the_sun.getIsCircumbinary()) {
    planet_outer_bound = farthest_planet(stell_mass_ratio);
  } else if (is_close(outer_planet_limit, (long double)0) ||
             outer_planet_limit <= (long double)0.0 ||
             outer_planet_limit <= planet_inner_bound) {
    planet_outer_bound = farthest_planet(stell_mass_ratio);
  } else {
    planet_outer_bound = outer_planet_limit;
  }

  if (max_distance_arg < planet_outer_bound &&
      !is_close(max_distance_arg, (long double)0)) {
    planet_outer_bound = max_distance_arg;
  }

  // Safety check: if stellar mass is zero, we can't compute planetary bounds
  if (stell_mass_ratio <= 0) {
    std::cerr << "ERROR: Stellar mass is zero or negative. Cannot generate planets." << std::endl;
    std::cerr << "Please specify star mass (-m) or luminosity (-y) or use a predefined star catalog." << std::endl;
    return nullptr;
  }

  while (planet_outer_bound <= planet_inner_bound) {
    planet_outer_bound = farthest_planet(stell_mass_ratio);
  }

  // std::cout << planet_inner_bound << " " << planet_outer_bound << std::endl;

  // --- Metallicity-dependent giant-formation gate (default-on; see const.h) -----
  // Decide ONCE whether this star can form gas giants, from a per-star [Fe/H] draw
  // and the observed giant-frequency-metallicity relation. Three fixed draws in a
  // fixed order (Box-Muller [Fe/H] = 2 draws, then the acceptance draw) -> no
  // rejection loops, serial == parallel byte-identical. If not giant-capable, gas
  // runaway is suppressed below (crit_mass made unreachable) so bodies stay cores.
  bool giant_capable = true;
  {
    const long double u1 = random_ctx->randDouble(1.0e-12L, 1.0L);  // avoid log(0)
    const long double u2 = random_ctx->randDouble(0.0L, 1.0L);
    const long double feh =
        GIANT_METALLICITY_MEAN +
        GIANT_METALLICITY_SIGMA * sqrt(-2.0L * log(u1)) * cos(2.0L * PI * u2);
    the_sun.setMetallicity(feh);  // expose the per-star [Fe/H] in output
    // Carbon-rich (C/O>1) systems: a deterministic per-system flag hashed from
    // the seed (NO RNG draw -> stream unperturbed) that makes rocky bodies
    // carbon-dominated (-> tCarbon). See assign_composition() / const.h.
    the_sun.setCarbonRich(carbon_rich_from_seed(random_ctx->getSeed()));
    long double p_giant = GIANT_FORMATION_NORM * stell_mass_ratio *
                          std::pow(10.0L, GIANT_FORMATION_FEH_SLOPE * feh);
    if (stell_mass_ratio > GIANT_FORMATION_MASS_TURNOVER_MSUN) {  // steep drop for A-stars
      p_giant *= GIANT_FORMATION_MASS_TURNOVER_MSUN / stell_mass_ratio;
    }
    if (p_giant > GIANT_FORMATION_PROB_CAP) {
      p_giant = GIANT_FORMATION_PROB_CAP;
    }
    const long double u_giant = random_ctx->randDouble(0.0L, 1.0L);
    giant_capable = (u_giant < p_giant);
  }

  // while there's still dust left...
  while (dust_left) {
    if (seeds != nullptr) {
      // give us a random proto planet within the inner and outer bounds
      a = seeds->getA();
      e = seeds->getE();
      dust_mass = seeds->getDustMass();
      gas_mass = seeds->getGasMass();
      total_mass = seeds->getMass();
      if (seeds->first_moon != nullptr && do_moons) {
        prev_moon = nullptr;
        temp_moons = new planet;
        seed_moons = temp_moons;
        for (moon = seeds->first_moon; moon != nullptr;
             moon = moon->next_planet, temp_moons = temp_moons->next_planet) {
          if (temp_moons == nullptr) {
            temp_moons = new planet;
          }
          // temp_moons->setMoonA(moon->getMoonA());
          // temp_moons->setMoonE(moon->getMoonE());
          if (flag_verbose & 0x0100) {
            std::cerr << "Injecting Moon... " << toString(a) << " AU ("
                 << toString(total_mass * SUN_MASS_IN_EARTH_MASSES)
                 << " EM) <- "
                 << toString(temp_moons->getMass() * SUN_MASS_IN_EARTH_MASSES)
                 << " EM\n";
          }
          temp_moons->setDustMass(moon->getDustMass());
          temp_moons->setGasMass(moon->getGasMass());
          if (prev_moon != nullptr) {
            prev_moon->next_planet = temp_moons;
            prev_moon->reconnect_to = temp_moons;
          }
          prev_moon = temp_moons;
        }
      } else {
        seed_moons = nullptr;
      }
      seeds = seeds->next_planet;
      is_seed = true;
    } else {
      a = random_number(planet_inner_bound, planet_outer_bound);
      e = random_eccentricity(ecc_coef);
      for (int i = 0;(calcPerihelion(a, e) < planet_inner_bound) && (i < 1000);i++) {
        a = random_number(planet_inner_bound, planet_outer_bound);
        e = random_eccentricity(ecc_coef);
      }
      total_mass = PROTOPLANET_MASS;
      dust_mass = 0;
      gas_mass = 0;
      is_seed = false;
      seed_moons = nullptr;
    }

    if (flag_verbose & 0x0200) {
      std::cerr << "Checking " << toString(a) << " AU.\n";
    }
    // if we have dust inside the limits...
    if (dust_available(inner_effect_limit(a, e, total_mass),
                       outer_effect_limit(a, e, total_mass))) {
      if (flag_verbose & 0x0100) {
        std::cerr << "Injecting protoplanet at " << toString(a) << " AU.\n";
      }

      dust_density = dust_density_coeff * sqrt(stell_mass_ratio) *
                     exp(-ALPHA * std::pow(a, 1.0 / NDENSITY));
      crit_mass = critical_limit(a, e, stell_luminosity_ratio);
      if (!giant_capable) {
        crit_mass = INCREDIBLY_LARGE_NUMBER;  // metallicity gate: suppress gas runaway -> rocky/ice core
      }
      if (total_mass == PROTOPLANET_MASS && is_seed == false) {
        // std::cout << "test1\n";
        // std::cout << total_mass << " " << dust_mass << " " << gas_mass << " " << a
        // << " " << e << " " << crit_mass << " " << planet_inner_bound << " "
        // << planet_outer_bound << std::endl;
        accrete_dust(total_mass, dust_mass, gas_mass, a, e, crit_mass,
                     planet_inner_bound, planet_outer_bound);
        // std::cout << "test2\n";
        dust_mass += PROTOPLANET_MASS;
      }

      if (total_mass > PROTOPLANET_MASS) {
        if (allow_planet_migration && is_seed == false) {
          long double min_distance = 0.0;
          if (the_sun.getIsCircumbinary() || (nearest_planet_factor != 0.3)) {
            min_distance = planet_inner_bound;
          }
          else {
            min_distance = 0.015;
          }
          long double new_a = a;
          long double new_e = e;
          // Only giants / giant-cores migrate; terrestrials stay at their
          // formation orbit (protects the inner peas-in-a-pod architecture).
          // See research/modern/11-giant-migration.md and migrate_giant() above.
          if ((total_mass * SUN_MASS_IN_EARTH_MASSES) >=
              (MIGRATION_MIN_MASS_MJUP * JUPITER_MASS)) {
            migrate_giant(new_a, new_e, total_mass, stell_mass_ratio, min_distance,
                          random_ctx);
          }
          a = new_a;
          e = new_e;
          if (flag_verbose & 0x0200) {
            std::cerr << "Planet migrated to " << toString(a)
                 << " AU with eccentricity of " << toString(e) << std::endl;
          }
        }
        // std::cout << "test 1\n";
        coalesce_planetesimals(a, e, total_mass, crit_mass, dust_mass, gas_mass,
                               stell_luminosity_ratio, planet_inner_bound,
                               planet_outer_bound, do_moons);
        // std::cout << "test 2\n";
      } else if (flag_verbose & 0x0100) {
        std::cerr << ".. failed due to large neighbor.\n";
      }
    } else if (flag_verbose & 0x0200) {
      std::cerr << ".. failed.\n";
    }

    if (is_seed) {
      // Page 17 of formation of planetary systems by aggregation
      // last paragraph
      temp = total_mass / (1.0 + total_mass);
      reduced_mass = std::pow(temp, 1.0 / 4.0);
      r_inner = inner_effect_limit(a, e, reduced_mass);
      r_outer = outer_effect_limit(a, e, reduced_mass);
      crit_mass = critical_limit(a, e, stell_luminosity_ratio);
      if (!giant_capable) {
        crit_mass = INCREDIBLY_LARGE_NUMBER;  // metallicity gate: suppress gas runaway -> rocky/ice core
      }
      if (planet_inner_bound > a) {
        planet_inner_bound = a;
      }
      update_dust_lanes(r_inner, r_outer, total_mass, crit_mass,
                        planet_inner_bound, planet_outer_bound);
    }
    // std::cout << "test 3\n";
    if (recorder != nullptr) {  // one frame per accretion step (formation viewer)
      recorder->capture(dust_bands, planet_head);
    }
  }
  ZoneScoped;

  // Transfer ownership to caller - prevent double-free in destructor
  planet *result = planet_head;
  planet_head = nullptr;
  return result;
}

/**
 * @brief frree dust
 * 
 * @param head 
 */

// REMOVED: free_dust - No longer needed with std::vector

/**
 * @brief free planet
 * 
 * @param head 
 */

void accrete::free_planet(planet *head) {
    planet *node = nullptr;
  planet *next = nullptr;

  for (node = head; node != nullptr; node = next) {
    next = node->next_planet;
    delete node;
    // std::cout << "Deleted World\n";
  }
  ZoneScoped;
}

/**
 * @brief free generations
 * 
 */

void accrete::free_generations() {
  gen *node = nullptr;
  gen *next = nullptr;

  for (node = hist_head; node != nullptr; node = next) {
    next = node->next;

    // node->dusts is now a std::vector - automatically cleaned up

    if (node->planets != nullptr) {
      free_planet(node->planets);
    }

    delete node;
    // std::cout << "Deleted Generation\n";
  }

  // CRITICAL: Nullify hist_head to prevent double-free in parallel mode
  // Without this, if free_generations() is called twice (e.g., from SystemOutputData
  // destructor and then from reset()), it will try to traverse already-freed memory
  hist_head = nullptr;

  ZoneScoped;
}

/**
 * @brief Reset accrete state for object pool reuse
 *
 * Clears all internal state so the object can be safely reused.
 */
void accrete::reset() {
  // Free any existing data
  if (hist_head != nullptr) {
    free_generations();
    hist_head = nullptr;
  }

  if (planet_head != nullptr) {
    // Planet head should be nullptr after update_hist transfer
    // But check just in case
    planet_head = nullptr;
  }

  if (seed_moons != nullptr) {
    free_planet(seed_moons);
    seed_moons = nullptr;
  }

  // Clear dust bands and preallocate capacity
  dust_bands.clear();
  dust_bands.reserve(32);  // Preallocate for typical accretion (prevents reallocations)

  // Reset state variables to defaults
  dust_left = false;
  r_inner = 0.0;
  r_outer = 0.0;
  reduced_mass = 0.0;
  dust_density = 0.0;
  cloud_eccentricity = 0.2;
}

/**
 * @brief is predefined planet helper
 * 
 * @param the_planet 
 * @param predined_planet 
 * @return true 
 * @return false 
 */

auto accrete::is_predefined_planet_helper(planet *the_planet, planet *predined_planet)
    -> bool {
        // if (the_planet->getA() == predined_planet->getA() && the_planet->getE() ==
  // predined_planet->getE())
  ZoneScoped;
  if (is_close(the_planet->getA(), predined_planet->getA()) &&
      is_close(the_planet->getE(), predined_planet->getE())) {
    return true;
  }
  return false;
}

/**
 * @brief is predefined planet
 * 
 * @param the_planet 
 * @return true 
 * @return false 
 */
auto accrete::is_predefined_planet(planet *the_planet) -> bool {
  ZoneScoped;
  // Catalogue of known exoplanets / predefined bodies. This was previously a
  // ~750-line if/else cascade dispatching to 58 one-line per-system wrapper
  // functions (is_in_eriEps, is_in_hd10180, ...), each of which compared
  // the_planet against one or more of these globals via
  // is_predefined_planet_helper. The cascade returned true on the FIRST match,
  // so the result is order-independent: true iff the_planet's (a, e) is within
  // is_close tolerance of ANY catalogue entry. A single flat scan is therefore
  // behaviourally identical to the old cascade, with far less code.
  //
  // NOTE: a hash map is not applicable here -- matching is tolerance-based
  // (is_close on a and e), not exact-key equality. The globals are populated by
  // initPlanets() (main.cpp) before any generation runs, so this function-local
  // static (thread-safe initialisation since C++11) captures valid pointers
  // once and is never mutated afterwards.
  static const std::vector<planet *> predefined = {
      eriEpsI, UMa47III, UMa47II, UMa47I, horIotI, xiumabb,
      Bellerophon, can55d, can55f, can55c, can55b, can55e,
      UPSAndAe, UPSAndAd, UPSAndAc, UPSAndAb, hd10180h, hd10180g,
      hd10180f, hd10180j, hd10180e, hd10180d, hd10180i, hd10180c,
      hd10180b, gliese581f, gliese581d, gliese581g, gliese581c, gliese581b,
      gliese581e, hd10647b, leo83Bb, leo83Ba, muarie, muarib,
      muarid, muaric, hd28185b, hd40307g, hd40307f, hd40307e,
      hd40307d, hd40307c, hd40307b, kepler22b, taucetf, taucete,
      taucetd, taucetc, taucetb, alfcentbb, EPSEric, EPSErib,
      cyteen, GL849b, ILAqrb, ILAqrc, ILAqrd, HD20794d,
      HD20794c, HD20794b, BETHyib, hd208527b, kepler11g, kepler11f,
      kepler11e, kepler11d, kepler11c, kepler11b, bajorI, bajorII,
      bajorIII, bajorIV, bajorV, bajorVI, bajorVII, bajorVIII,
      bajorIX, bajorX, bajorXI, bajorXII, bajorXIII, bajorXIV,
      gliese667Cb, gliese667Cc, gliese667Cd, gliese667Ce, gliese667Cf, gliese667Cg,
      gliese667Ch, kepler283b, kepler283c, kepler62b, kepler62c, kepler62d,
      kepler62e, kepler62f, kepler296b, kepler296c, kepler296d, kepler296e,
      kepler296f, gliese180b, gliese180c, gliese163b, gliese163c, gliese163d,
      kepler61b, gliese422b, kepler298b, kepler298c, kepler298d, kepler174b,
      kepler174c, kepler174d, gliese682b, gliese682c, hd38529b, hd38529c,
      hd202206b, hd202206c, hd8673b, hd22781b, hd217786b, hd106270b,
      hd38801b, hd39091b, hd141937b, hd33564b, hd23596b, hd222582b,
      hd86264b, hd196067b, hd10697b, hd132406b, hd13908b, hd13908c,
      hd2039b, hd82943b, hd82943c, hd82943d, moa2011blg293lb, hd213240b
  };
  for (planet *candidate : predefined) {
    if (is_predefined_planet_helper(the_planet, candidate)) {
      return true;
    }
  }
  return false;
}

auto accrete::calcPerihelion(long double a, long double e) -> long double {
  return a * (1.0 - e);
}