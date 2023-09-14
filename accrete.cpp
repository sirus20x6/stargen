#include "accrete.h"
#include <cmath>      // for pow, sqrt, NAN, fabs, exp, M_PI
#include <cstdlib>    // for rand
#include <iostream>   // for operator<<, basic_ostream, char_traits, cerr
#include <string>     // for operator<<, string
#include "const.h"    // for SUN_MASS_IN_EARTH_MASSES, PROTOPLANET_MASS, K
#include "planets.h"  // for BETHyib, Bellerophon, EPSErib, EPSEric, GL849b
#include "stargen.h"  // for flag_verbose, max_distance_arg, fDoMoons, flags...
#include "structs.h"  // for planet, dust, gen, sun
#include "utils.h"    // for toString, random_eccentricity, random_number


/**
 * @brief Set Initial Conditions
 * 
 * @param inner_limit_of_dust 
 * @param outer_limit_of_dust 
 */
void accrete::set_initial_conditions(long double inner_limit_of_dust,
                            long double outer_limit_of_dust) {
  planet_head = nullptr;
  hist_head = nullptr;

  dust_head = new dust();
  dust_head->next_band = nullptr;
  dust_head->setOuterEdge(outer_limit_of_dust);
  dust_head->setInnerEdge(inner_limit_of_dust);
  dust_head->setDustPresent(true);
  dust_head->setGasPresent(true);
  dust_left = true;

  cloud_eccentricity = 0.2;
  //	cloud_eccentricity = 0.6;

  gen *hist = nullptr;
  hist = new gen();
  hist->dusts = dust_head;
  hist->planets = planet_head;
  hist->next = hist_head;
  hist_head = hist;
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
  dust *current_dust_band = nullptr;
  bool dust_here = false;

  // this loop finds the dust band whose outer edge is within our inside range.
  current_dust_band = dust_head;
  while (current_dust_band != nullptr &&
         current_dust_band->getOuterEdge() < inside_range) {
    current_dust_band = current_dust_band->next_band;
  }
  // if we have no dust band, there's no dust here; otherwise, it depends on the
  // dust record
  if (current_dust_band == nullptr) {
    dust_here = false;
  } else {
    dust_here = current_dust_band->getDustPresent();
  }
  // this loop ORs together all of the dust bands between the first one we found
  // and the dust band whose inner edge is outside our outside range.
  while (current_dust_band != nullptr &&
         current_dust_band->getInnerEdge() < outside_range) {
    dust_here = dust_here || current_dust_band->getDustPresent();
    current_dust_band = current_dust_band->next_band;
  }

  // return whether or not we found a dust band in our range that still had dust
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
  bool gas = false;
  dust *node1 = nullptr;
  dust *node2 = nullptr;
  dust *node3 = nullptr;

  dust_left = false;
  if (mass > crit_mass) {
    gas = false;
  } else {
    gas = true;
  }
  node1 = dust_head;
  while (node1 != nullptr) {
    if (node1->getInnerEdge() < min && node1->getOuterEdge() > max) {
      node2 = new dust();
      node2->setInnerEdge(min);
      node2->setOuterEdge(max);
      if (node1->getGasPresent()) {
        node2->setGasPresent(gas);
      } else {
        node2->setGasPresent(false);
      }
      node2->setDustPresent(false);
      node3 = new dust();
      node3->setInnerEdge(max);
      node3->setOuterEdge(node1->getOuterEdge());
      node3->setGasPresent(node1->getGasPresent());
      node3->setDustPresent(node1->getDustPresent());
      node3->next_band = node1->next_band;
      node1->next_band = node2;
      node2->next_band = node3;
      node1->setOuterEdge(min);
      node1 = node3->next_band;
    } else {
      if (node1->getInnerEdge() < max && node1->getOuterEdge() > max) {
        node2 = new dust();
        node2->next_band = node1->next_band;
        node2->setDustPresent(node1->getDustPresent());
        node2->setGasPresent(node1->getGasPresent());
        node2->setOuterEdge(node1->getOuterEdge());
        node2->setInnerEdge(max);
        node1->next_band = node2;
        node1->setOuterEdge(max);
        if (node1->getGasPresent()) {
          node1->setGasPresent(gas);
        } else {
          node1->setGasPresent(false);
        }
        node1->setDustPresent(false);
        node1 = node2->next_band;
      } else {
        if (node1->getInnerEdge() < min && node1->getOuterEdge() > min) {
          node2 = new dust();
          node2->next_band = node1->next_band;
          node2->setDustPresent(false);
          if (node1->getGasPresent()) {
            node2->setGasPresent(gas);
          } else {
            node2->setGasPresent(false);
          }
          node2->setOuterEdge(node1->getOuterEdge());
          node2->setInnerEdge(min);
          node1->next_band = node2;
          node1->setOuterEdge(min);
          node1 = node2->next_band;
        } else {
          if (node1->getInnerEdge() >= min && node1->getOuterEdge() <= max) {
            if (node1->getGasPresent()) {
              node1->setGasPresent(gas);
            }
            node1->setDustPresent(false);
            node1 = node1->next_band;
          } else {
            if (node1->getOuterEdge() < min || node1->getInnerEdge() > max) {
              node1 = node1->next_band;
            }
          }
        }
      }
    }
  }
  node1 = dust_head;
  while (node1 != nullptr) {
    if (node1->getDustPresent() && node1->getOuterEdge() >= body_inner_bound &&
        node1->getInnerEdge() <= body_outer_bound) {
      dust_left = true;
    }
    node2 = node1->next_band;
    if (node2 != nullptr) {
      if (node1->getDustPresent() == node2->getDustPresent() &&
          node1->getGasPresent() == node2->getGasPresent()) {
        node1->setOuterEdge(node2->getOuterEdge());
        node1->next_band = node2->next_band;
        delete node2;
      }
    }
    node1 = node1->next_band;
  }
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
                  long double crit_mass, dust *dust_band) -> long double {
  // cout << EM(last_mass) << " " << EM(new_dust) << " " << EM(new_gas) << " "
  // << a << " " << e << " " << EM(crit_mass) << endl;
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
// base case: if this is the last dust band, return 0
  if (dust_band == nullptr) {
    return 0;
  } else {
  // if we have dust, use the dust density, otherwise zero
  if (dust_band->getDustPresent()) {
    temp_density = dust_density;
  }

    // if the last mass is below the critical mass, or there's no dust in this 
    // dust band, the density is the overall accretion density;
    // otherwise, the mass density is this horrifying formula
    if (last_mass < crit_mass || !dust_band->getGasPresent()) {
      mass_density = temp_density;
    } else {
      mass_density =
          K * temp_density / (1.0 + sqrt(crit_mass / last_mass) * (K - 1.0));
      gas_density = mass_density - temp_density;
    }

    // cout << dust_band->getOuterEdge() << " " << dust_band->getInnerEdge() <<
    // endl;

    // if the outer edge exceeds the accretion inner limit or the inner edge is 
    // outside the outer limit, just collect the dust from the next band
    if (dust_band->getOuterEdge() <= r_inner ||
        dust_band->getInnerEdge() >= r_outer) {
      return collect_dust(last_mass, new_dust, new_gas, a, e, crit_mass,
                          dust_band->next_band);
    } else {
      bandwidth = r_outer - r_inner;

      temp1 = r_outer - dust_band->getOuterEdge();
      if (temp1 < 0.0) {
        temp1 = 0.0;
      }
      width = bandwidth - temp1;

      // account for the gap between the inner edge and the start of 
      // the accretion radius
      temp2 = dust_band->getInnerEdge() - r_inner;
      if (temp2 < 0.0) {
        temp2 = 0.0;
      }
      width = width - temp2;

      // calculate the area of a cross-section, and the volume
      temp = 4.0 * PI * std::pow(a, 2.0) * reduced_mass *
             (1.0 - e * (temp1 - temp2) / bandwidth);
      volume = temp * width;

      // calculate the total mass of this lane plus the mass of the next lane
      new_mass = volume * mass_density;
      new_gas = volume * gas_density;
      new_dust = new_mass - new_gas;

      next_mass = collect_dust(last_mass, next_dust, next_gas, a, e, crit_mass,
                               dust_band->next_band);

      new_gas = new_gas + next_gas;
      new_dust = new_dust + next_dust;

      return new_mass + next_mass;
    }
  }
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
    // cout << "test1\n";
    temp_mass = new_mass;
    // fixed point algorithm: accumulate more mass until the difference is less 
    // than .01% of the old mass
    new_mass =
        collect_dust(new_mass, new_dust, new_gas, a, e, crit_mass, dust_head);
    // cout << "test2\n";
    // cout << "Old mass: " << (temp_mass * SUN_MASS_IN_EARTH_MASSES) << endl;
    // cout << "New mass: " << (new_mass * SUN_MASS_IN_EARTH_MASSES) << endl;
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
  // cout << "test\n";
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
    // cout << "test2" << count << endl;
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

    if (fabs(diff) <= fabs(dist1) || fabs(diff) <= fabs(dist2)) {
      long double new_dust = 0;
      long double new_gas = 0;
      long double new_a =
          (the_planet->getMass() + mass) /
          ((the_planet->getMass() / the_planet->getA()) + (mass / a));

      temp = the_planet->getMass() * sqrt(the_planet->getA()) * sqrt(1.0 - std::pow(the_planet->getE(), 2.0));
      temp += (mass * sqrt(a) * sqrt(sqrt(1.0 - std::pow(e, 2.0))));
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
              cerr << "Moon Captured... " << toString(the_planet->getA())
                   << " AU ("
                   << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
                   << " EM) <- " << toString(mass * SUN_MASS_IN_EARTH_MASSES)
                   << " EM\n";
            }
          } else {
            if (flag_verbose & 0x0100) {
              string big_moons_message;
              if (existing_mass < (the_planet->getMass() * 0.05)) {
                big_moons_message = "";
              } else {
                big_moons_message = " (big moons)";
              }

              string moon_size_message;
              if ((mass * SUN_MASS_IN_EARTH_MASSES) >= 2.5) {
                moon_size_message = ", too big";
              } else if ((mass * SUN_MASS_IN_EARTH_MASSES) <= .0001) {
                moon_size_message = ", too small";
              } else {
                moon_size_message = "";
              }
              cerr << "Moon Escapes... " << toString(the_planet->getA())
                   << " AU ("
                   << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
                   << " EM)" << big_moons_message << " "
                   << toString(mass * SUN_MASS_IN_EARTH_MASSES) << " EM"
                   << moon_size_message << endl;
            }
          }
        }
      }

      if (!finished && !is_predefined_planet(the_planet)) {
        if (flag_verbose & 0x0100) {
          cerr << "Collision between two planetesimals! "
               << toString(the_planet->getA()) << " AU ("
               << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " EM) + " << toString(a) << " AU ("
               << toString(mass * SUN_MASS_IN_EARTH_MASSES)
               << " EM = " << toString(dust_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EMd + " << toString(gas_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EMg [" << toString(crit_mass * SUN_MASS_IN_EARTH_MASSES)
               << " EM])-> " << toString(new_a) << " AU (" << toString(e) << ")"
               << endl;
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
  if (hist_head->planets == nullptr) {
    hist_head->planets = planet_head;
  }
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
  // planet *seed_moons = NULL;
  planet *moon = nullptr;
  planet *temp_moons = nullptr;
  planet *new_moon = nullptr;
  planet *prev_moon = nullptr;
  bool is_seed = false;
  int i = 0;

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

  while (planet_outer_bound <= planet_inner_bound) {
    planet_outer_bound = farthest_planet(stell_mass_ratio);
  }

  // cout << planet_inner_bound << " " << planet_outer_bound << endl;

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
            cerr << "Injecting Moon... " << toString(a) << " AU ("
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
      cerr << "Checking " << toString(a) << " AU.\n";
    }
    // if we have dust inside the limits...
    if (dust_available(inner_effect_limit(a, e, total_mass),
                       outer_effect_limit(a, e, total_mass))) {
      if (flag_verbose & 0x0100) {
        cerr << "Injecting protoplanet at " << toString(a) << " AU.\n";
      }

      dust_density = dust_density_coeff * sqrt(stell_mass_ratio) *
                     exp(-ALPHA * std::pow(a, 1.0 / N));
      crit_mass = critical_limit(a, e, stell_luminosity_ratio);
      if (total_mass == PROTOPLANET_MASS && is_seed == false) {
        // cout << "test1\n";
        // cout << total_mass << " " << dust_mass << " " << gas_mass << " " << a
        // << " " << e << " " << crit_mass << " " << planet_inner_bound << " "
        // << planet_outer_bound << endl;
        accrete_dust(total_mass, dust_mass, gas_mass, a, e, crit_mass,
                     planet_inner_bound, planet_outer_bound);
        // cout << "test2\n";
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
          long double new_a = 0;
          long double new_e = 0;
          new_a = random_number(min_distance, a);
          new_e = random_eccentricity(ecc_coef);
          for (int i = 0;(calcPerihelion(new_a, new_e) < min_distance) && (i < 1000); i++) {
            new_a = random_number(min_distance, a);
            new_e = random_eccentricity(ecc_coef);
          }
          if (total_mass >= ((0.7 * JUPITER_MASS) / SUN_MASS_IN_EARTH_MASSES)) {
            if (((1 + rand()) % 14) == 0 && min_distance < 0.2) {
              new_a = random_number(min_distance, 0.2);
              new_e = random_eccentricity(ecc_coef);
              for (int i = 0;(calcPerihelion(new_a, new_e) < min_distance) && (i < 1000); i++) {
                new_a = random_number(min_distance, 0.2);
                new_e = random_eccentricity(ecc_coef);
              }
            }
            if (((1 + rand()) % 10) == 0 &&
                calcPerihelion(new_a, 0.1) > min_distance) {
              while (new_e < 0.1) {
                new_e = random_eccentricity(0.25);
                for (int i = 0;(calcPerihelion(new_a, new_e) < min_distance) && (i < 1000); i++) {
                  new_a = random_number(min_distance, a);
                  new_e = random_eccentricity(ecc_coef);
                }
              }
            }
          } else if (total_mass >= (2.0 / SUN_MASS_IN_EARTH_MASSES) &&
                     total_mass <= (10.0 / SUN_MASS_IN_EARTH_MASSES) &&
                     a >= (4.0 * sqrt(stell_luminosity_ratio)) &&
                     (dust_mass / total_mass) >= 0.75) {
            if (((1 + rand()) % 4) == 0) {
              new_a = random_number(min_distance, a / 2.0);
              new_e = random_eccentricity(ecc_coef);
              for (int i = 0;calcPerihelion(new_a, new_e) < planet_inner_bound && i < 1000;i++) {
                new_a = random_number(min_distance, a / 2.0);
                new_e = random_eccentricity(ecc_coef);
              }
            }
          }
          a = new_a;
          e = new_e;
          if (flag_verbose & 0x0200) {
            cerr << "Planet migrated to " << toString(a)
                 << " AU with eccentricity of " << toString(e) << endl;
          }
        }
        // cout << "test 1\n";
        coalesce_planetesimals(a, e, total_mass, crit_mass, dust_mass, gas_mass,
                               stell_luminosity_ratio, planet_inner_bound,
                               planet_outer_bound, do_moons);
        // cout << "test 2\n";
      } else if (flag_verbose & 0x0100) {
        cerr << ".. failed due to large neighbor.\n";
      }
    } else if (flag_verbose & 0x0200) {
      cerr << ".. failed.\n";
    }

    if (is_seed) {
      // Page 17 of formation of planetary systems by aggregation
      // last paragraph
      temp = total_mass / (1.0 + total_mass);
      reduced_mass = std::pow(temp, 1.0 / 4.0);
      r_inner = inner_effect_limit(a, e, reduced_mass);
      r_outer = outer_effect_limit(a, e, reduced_mass);
      crit_mass = critical_limit(a, e, stell_luminosity_ratio);
      if (planet_inner_bound > a) {
        planet_inner_bound = a;
      }
      update_dust_lanes(r_inner, r_outer, total_mass, crit_mass,
                        planet_inner_bound, planet_outer_bound);
    }
    // cout << "test 3\n";
  }
  return planet_head;
}

/**
 * @brief frree dust
 * 
 * @param head 
 */

void accrete::free_dust(dust *head) {
  dust *node = nullptr;
  dust *next = nullptr;

  for (node = head; node != nullptr; node = next) {
    next = node->next_band;
    delete node;
    // cout << "Deleted Dust\n";
  }
}

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
    // cout << "Deleted World\n";
  }
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

    if (node->dusts != nullptr) {
      free_dust(node->dusts);
    }

    if (node->planets != nullptr) {
      free_planet(node->planets);
    }

    delete node;
    // cout << "Deleted Generation\n";
  }
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
  if (is_in_eriEps(the_planet)) {
    return true;
  } else if (is_in_UMa47(the_planet)) {
    return true;
  } else if (is_in_horIot(the_planet)) {
    return true;
  } else if (is_in_xiumab(the_planet)) {
    return true;
  } else if (is_in_51peg(the_planet)) {
    return true;
  } else if (is_in_55can(the_planet)) {
    return true;
  } else if (is_in_UPSAndA(the_planet)) {
    return true;
  } else if (is_in_hd10180(the_planet)) {
    return true;
  } else if (is_in_gliese581(the_planet)) {
    return true;
  } else if (is_in_hd10647(the_planet)) {
    return true;
  } else if (is_in_83leoB(the_planet)) {
    return true;
  } else if (is_in_muari(the_planet)) {
    return true;
  } else if (is_in_hd28185(the_planet)) {
    return true;
  } else if (is_in_hd40307(the_planet)) {
    return true;
  } else if (is_in_kepler22(the_planet)) {
    return true;
  } else if (is_in_taucet(the_planet)) {
    return true;
  } else if (is_in_alfcentb(the_planet)) {
    return true;
  } else if (is_in_EPSEri(the_planet)) {
    return true;
  } else if (is_cyteen(the_planet)) {
    return true;
  } else if (is_in_GL849(the_planet)) {
    return true;
  } else if (is_in_ILAqr(the_planet)) {
    return true;
  } else if (is_in_HD20794(the_planet)) {
    return true;
  } else if (is_in_BETHyi(the_planet)) {
    return true;
  } else if (is_in_hd208527(the_planet)) {
    return true;
  } else if (is_in_kepler11(the_planet)) {
    return true;
  } else if (is_in_bajor(the_planet)) {
    return true;
  } else if (is_in_gliese667C(the_planet)) {
    return true;
  } else if (is_in_kepler283(the_planet)) {
    return true;
  } else if (is_in_kepler62(the_planet)) {
    return true;
  } else if (is_in_kepler296(the_planet)) {
    return true;
  } else if (is_in_gliese180(the_planet)) {
    return true;
  } else if (is_in_gliese163(the_planet)) {
    return true;
  } else if (is_in_kepler61(the_planet)) {
    return true;
  } else if (is_in_gliese422(the_planet)) {
    return true;
  } else if (is_in_kepler298(the_planet)) {
    return true;
  } else if (is_in_kepler174(the_planet)) {
    return true;
  } else if (is_in_gliese682(the_planet)) {
    return true;
  } else if (is_in_hd38529(the_planet)) {
    return true;
  } else if (is_in_hd202206(the_planet)) {
    return true;
  } else if (is_in_hd8673(the_planet)) {
    return true;
  } else if (is_in_hd22781(the_planet)) {
    return true;
  } else if (is_in_hd217786(the_planet)) {
    return true;
  } else if (is_in_hd106270(the_planet)) {
    return true;
  } else if (is_in_hd38801(the_planet)) {
    return true;
  } else if (is_in_hd39091(the_planet)) {
    return true;
  } else if (is_in_hd141937(the_planet)) {
    return true;
  } else if (is_in_hd33564(the_planet)) {
    return true;
  } else if (is_in_hd23596(the_planet)) {
    return true;
  } else if (is_in_hd222582(the_planet)) {
    return true;
  } else if (is_in_hd86264(the_planet)) {
    return true;
  } else if (is_in_hd196067(the_planet)) {
    return true;
  } else if (is_in_hd10697(the_planet)) {
    return true;
  } else if (is_in_hd132406(the_planet)) {
    return true;
  } else if (is_in_hd13908(the_planet)) {
    return true;
  } else if (is_in_hd2039(the_planet)) {
    return true;
  } else if (is_in_hd82943(the_planet)) {
    return true;
  } else if (is_in_moa2011blg293l(the_planet)) {
    return true;
  } else if (is_in_hd213240(the_planet)) {
    return true;
  }
  return false;
}

/**
 * @brief is in eriEps
 * 
 * @param the_planet 
 * @return true 
 * @return false 
 */
auto accrete::is_in_eriEps(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, eriEpsI)) {
    return true;
  }
  return false;
}

auto accrete::is_in_UMa47(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, UMa47III)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, UMa47II)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, UMa47I)) {
    return true;
  }
  return false;
}

auto accrete::is_in_horIot(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, horIotI)) {
    return true;
  }
  return false;
}

auto accrete::is_in_xiumab(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, xiumabb)) {
    return true;
  }
  return false;
}

auto accrete::is_in_51peg(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, Bellerophon)) {
    return true;
  }
  return false;
}

auto accrete::is_in_55can(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, can55d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, can55f)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, can55c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, can55b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, can55e)) {
    return true;
  }
  return false;
}

auto accrete::is_in_UPSAndA(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, UPSAndAe)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, UPSAndAd)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, UPSAndAc)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, UPSAndAb)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd10180(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd10180h)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180g)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180f)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180j)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180e)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180i)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd10180b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese581(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese581f)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese581d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese581g)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese581c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese581b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese581e)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd10647(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd10647b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_83leoB(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, leo83Bb)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, leo83Ba)) {
    return true;
  }
  return false;
}

auto accrete::is_in_muari(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, muarie)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, muarib)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, muarid)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, muaric)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd28185(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd28185b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd40307(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd40307g)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd40307f)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd40307e)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd40307d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd40307c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd40307b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler22(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler22b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_taucet(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, taucetf)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, taucete)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, taucetd)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, taucetc)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, taucetb)) {
    return true;
  }
  return false;
}

auto accrete::is_in_alfcentb(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, alfcentbb)) {
    return true;
  }
  return false;
}

auto accrete::is_in_EPSEri(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, EPSEric)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, EPSErib)) {
    return true;
  }
  return false;
}

auto accrete::is_cyteen(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, cyteen)) {
    return true;
  }
  return false;
}

auto accrete::is_in_GL849(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, GL849b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_ILAqr(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, ILAqrb)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, ILAqrc)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, ILAqrd)) {
    return true;
  }
  return false;
}

auto accrete::is_in_HD20794(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, HD20794d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, HD20794c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, HD20794b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_BETHyi(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, BETHyib)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd208527(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd208527b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler11(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler11g)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler11f)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler11e)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler11d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler11c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler11b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_bajor(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, bajorI)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorIII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorIV)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorV)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorVI)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorVII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorVIII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorIX)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorX)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorXI)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorXII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorXIII)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, bajorXIV)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese667C(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese667Cb)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Cc)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Cd)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Ce)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Cf)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Cg)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese667Ch)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler283(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler283b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler283c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler62(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler62b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler62c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler62d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler62e)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler62f)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler296(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler296b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler296c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler296d)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler296e)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler296f)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese180(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese180b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese180c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese163(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese163b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese163c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese163d)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler61(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler61b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese422(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese422b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler298(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler298b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler298c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler298d)) {
    return true;
  }
  return false;
}

auto accrete::is_in_kepler174(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, kepler174b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler174c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, kepler174d)) {
    return true;
  }
  return false;
}

auto accrete::is_in_gliese682(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, gliese682b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, gliese682c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd38529(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd38529b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd38529c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd202206(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd202206b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd202206c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd8673(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd8673b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd22781(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd22781b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd217786(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd217786b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd106270(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd106270b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd38801(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd38801b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd39091(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd39091b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd141937(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd141937b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd33564(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd33564b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd23596(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd23596b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd222582(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd222582b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd86264(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd86264b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd196067(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd196067b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd10697(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd10697b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd132406(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd132406b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd13908(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd13908b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd13908c)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd2039(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd2039b)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd82943(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd82943b)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd82943c)) {
    return true;
  } else if (is_predefined_planet_helper(the_planet, hd82943d)) {
    return true;
  }
  return false;
}

auto accrete::is_in_moa2011blg293l(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, moa2011blg293lb)) {
    return true;
  }
  return false;
}

auto accrete::is_in_hd213240(planet *the_planet) -> bool {
  if (is_predefined_planet_helper(the_planet, hd213240b)) {
    return true;
  }
  return false;
}

auto accrete::calcPerihelion(long double a, long double e) -> long double {
  return a * (1.0 - e);
}