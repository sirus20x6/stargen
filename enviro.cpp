#include "enviro.h"
#include <ctype.h>                 // for isdigit
#include <cmath>                   // for pow, NAN, sqrt, fabs, exp, log, floor
#include <cstdlib>                 // for exit, EXIT_FAILURE, atoi
#include <cstring>                 // for strstr, strcmp
#include <iostream>                // for operator<<, basic_ostream, endl
#include <map>                     // for std::map, std::map<>::mapped_type
#include <string>                  // for std::string, operator<<, operator==
#include <vector>                  // for vector
#include <sstream>                 // for std::stringstream (not guaranteed via <iostream>)
#include <numbers>                 // for std::numbers::pi (portable replacement for M_PI)
#include <functional>
#include "const.h"                 // for SUN_MASS_IN_EARTH_MASSES, AVE, pow2
#include "elements.h"              // for gases
#include "gas_radius_helpers.h"    // for mini_neptune_radius, gas_dwarf_radius
#include "radius_tables.h"         // for solid_rock, solid_half_rock_half_iron
#include "solid_radius_helpers.h"  // for rock_radius, half_rock_half_iron_r...
#include "star_temps.h"            // for tempA, tempB, tempE, tempF, tempG
#include "stargen.h"               // for the_sun_clone, flag_verbose, assig...
#include "utils.h"                 // for toString, random_number, about



std::string breathability_phrase[4] = {"none", "breathable", "unbreathable",
                                  "poisonous"};

thread_local std::map<std::map<long double, long double>, std::vector<long double> > polynomial_cache;

/*
 * Main-sequence mass <-> luminosity.
 *
 * Eker et al. 2018, "Interrelated Main-Sequence Mass-Luminosity, Mass-Radius and
 * Mass-Effective Temperature Relations" (MNRAS 479, 5491; arXiv:1807.02568): a
 * six-piece classical relation log10(L/Lsun) = a*log10(M/Msun) + b, calibrated to
 * 509 detached double-lined eclipsing-binary main-sequence stars over
 * 0.179 <= M/Msun <= 31. Replaces the older uncalibrated Wikipedia power law.
 *
 * Below 0.179 Msun and above 31 Msun the nearest piece is extrapolated (the
 * generator must still produce something for brown-dwarf and very-massive tails).
 * See research/modern/07-stellar-relations-binaries.md.
 *
 * Eker's independent per-piece fits are not perfectly continuous at the regime
 * boundaries; the small (<~20%) jumps there are inherent to the published
 * relation and were present in the previous piecewise model too.
 */
namespace {
// {upper mass bound (Msun), slope a, intercept b}. The last piece's bound is the
// validity ceiling; masses above it reuse that piece.
struct EkerPiece { long double m_hi, a, b; };
constexpr EkerPiece kEkerML[] = {
    {0.45L, 2.028L, -0.976L},
    {0.72L, 4.572L, -0.102L},
    {1.05L, 5.743L, -0.007L},
    {2.40L, 4.329L,  0.010L},
    {7.00L, 3.967L,  0.093L},
    {31.0L, 2.865L,  1.105L},
};
}  // namespace

/**
 * @brief main-sequence mass -> luminosity (Eker et al. 2018)
 * @param mass stellar mass in solar masses
 * @return luminosity in solar luminosities
 */
long double mass_to_luminosity(long double mass) {
    for (const auto& p : kEkerML) {
        if (mass < p.m_hi) {
            return std::pow(10.0L, p.a * std::log10(mass) + p.b);
        }
    }
    const auto& top = kEkerML[(sizeof kEkerML / sizeof *kEkerML) - 1];
    return std::pow(10.0L, top.a * std::log10(mass) + top.b);
}

/**
 * @brief main-sequence luminosity -> mass (inverse of Eker et al. 2018)
 *
 * Thresholds are the luminosity at each piece's upper mass bound, so the inverse
 * selects the same regime the forward relation would.
 * @param luminosity luminosity in solar luminosities
 * @return stellar mass in solar masses
 */
auto luminosity_to_mass(long double luminosity) -> long double {
    const long double log_l = std::log10(luminosity);
    for (const auto& p : kEkerML) {
        const long double l_hi = std::pow(10.0L, p.a * std::log10(p.m_hi) + p.b);
        if (luminosity < l_hi) {
            return std::pow(10.0L, (log_l - p.b) / p.a);
        }
    }
    const auto& top = kEkerML[(sizeof kEkerML / sizeof *kEkerML) - 1];
    return std::pow(10.0L, (log_l - top.b) / top.a);
}

/**
 * @brief Main-sequence effective temperature from stellar mass + luminosity.
 *
 * Used when a star is specified by mass (or luminosity) with no spectral type, so
 * its effective temperature would otherwise default to a placeholder. Above 1.5
 * Msun uses the Eker et al. 2018 mass-Teff relation
 * (log Teff = -0.170 (log M)^2 + 0.888 log M + 3.671); at or below 1.5 Msun
 * derives Teff from luminosity and the Eker mass-radius relation
 * (R = 0.438 M^2 + 0.479 M + 0.075) via Stefan-Boltzmann, anchored so the Sun
 * (1 Msun) -> ~5778 K. See research/modern/07-stellar-relations-binaries.md.
 *
 * @param mass stellar mass in solar masses
 * @param luminosity luminosity in solar luminosities
 * @return effective temperature in Kelvin
 */
auto mass_to_eff_temp(long double mass, long double luminosity) -> long double {
    if (mass > 1.5L) {
        const long double lm = std::log10(mass);
        return std::pow(10.0L, -0.170L * lm * lm + 0.888L * lm + 3.671L);
    }
    long double radius = 0.438L * mass * mass + 0.479L * mass + 0.075L;  // Rsun
    if (radius <= 0.0L) {
        radius = 1.0L;
    }
    return 5778.0L * std::pow(luminosity / (radius * radius), 0.25L);
}

/**
 * @brief Get the Lum Index
 * 
 * @param spec_type 
 * @return int 
 */
int getLumIndex(const std::string& spec_type) {
    if (spec_type.find("Ia0") != std::string::npos || 
        spec_type.find("Ia") != std::string::npos || 
        spec_type.find("Ib") != std::string::npos || 
        spec_type.find("II") != std::string::npos) {
        return 2;
    }
    if (spec_type.find("III") != std::string::npos || 
        spec_type.find("IV") != std::string::npos) {
        return 1;
    }
    return 0;
}

/**
 * @brief Get the Star Type
 * 
 * @param spec_type 
 * @return std::string 
 */
std::string getStarType(std::string spec_type) {
    spec_type = my_strtoupper(spec_type);

    static const std::vector<std::pair<std::string, std::string>> starTypes = {
        {"DA", "WD"}, {"DB", "WD"}, {"DC", "WD"}, {"DO", "WD"}, {"DQ", "WD"}, {"DZ", "WD"},
        {"WN", "WN"}, {"WC", "WC"},
        {"O", "O"}, {"B", "B"}, {"A", "A"}, {"F", "F"}, {"G", "G"}, {"K", "K"},
        {"M", "M"}, {"L", "L"}, {"T", "T"}, {"Y", "Y"}, {"H", "H"}, {"E", "E"}, {"I", "I"},
        {"R", "K"}, {"S", "M"}, {"N", "M"}, {"C", "M"}
    };
    
    for (const auto& [prefix, type] : starTypes) {
        if (spec_type.find(prefix) != std::string::npos) {
            return type;
        }
    }
    
    std::cerr << "Unsupported star type: " << spec_type << std::endl;
    exit(EXIT_FAILURE);
}

/**
 * @brief Get the star Sub Type
 * 
 * @param spec_type 
 * @return int 
 */
auto getSubType(std::string spec_type) -> int {
  int total_chars = 0;
  // total_chars = spec_type.size();
  std::string buffer;

  for (std::string::iterator it = spec_type.begin(); it < spec_type.end(); it++) {
    if (isdigit(static_cast<unsigned char>(*it)) || (*it == '.' && !buffer.empty())) {
      buffer += *it;
    } else if (!buffer.empty()) {
      // Stop at the first non-numeric character after the numeric subtype began.
      break;
    }
  }
  if (buffer.empty()) {
    return 0;
  }
  return static_cast<int>(strtod(buffer.c_str(), nullptr));
}

/**
 * @brief spec type to eff temp
 * 
 * @param spec_type 
 * @return long double 
 */
long double spec_type_to_eff_temp(const std::string& spec_type) {
    if (spec_type.empty()) {
        return 0;
    }

    std::string star_type = getStarType(spec_type);
    int sub_type = getSubType(spec_type);
    int lumIndex = getLumIndex(spec_type);

    switch (star_type[0]) {
        case 'W':
            if (star_type == "WD") return tempWD[sub_type];
            if (star_type == "WN") return tempWN[sub_type];
            if (star_type == "WC") return tempWC[sub_type];
            break;
        case 'O': return tempO[lumIndex][sub_type];
        case 'B': return tempB[lumIndex][sub_type];
        case 'A': return tempA[lumIndex][sub_type];
        case 'F': return tempF[lumIndex][sub_type];
        case 'G': return tempG[lumIndex][sub_type];
        case 'K': return tempK[lumIndex][sub_type];
        case 'M': return tempM[lumIndex][sub_type];
        case 'L': return tempL[sub_type];
        case 'T': return tempT[sub_type];
        case 'Y': return tempY[sub_type];
        case 'H': return tempH[sub_type];
        case 'I': return tempI[sub_type];
        case 'E': return tempE[sub_type];
    }

    std::cerr << "Unsupported star type: " << star_type << std::endl;
    exit(EXIT_FAILURE);
}

/**
 * @brief eff temp to spec type
 * 
 * @param eff_temp 
 * @param luminosity 
 * @return std::string 
 */
std::string eff_temp_to_spec_type(long double eff_temp, long double luminosity) {
    // Lower temperature bound of each spectral class, on the standard
    // Morgan-Keenan / Pecaut & Mamajek (2013) scale. The previous boundaries
    // were miscalibrated (e.g. G set at 6000 K, so a 5778 K Sun fell into K, and
    // O was never reachable), which produced wrong spectral types for stars whose
    // type is derived from effective temperature.
    static const std::vector<std::pair<long double, std::string>> temp_classes = {
        {30000, "O"}, {10000, "B"}, {7300, "A"}, {6000, "F"},
        {5300, "G"}, {3900, "K"}, {2300, "M"}, {1300, "L"},
        {600, "T"}, {0, "Y"}
    };

    auto get_spec_class = [&]() {
        for (size_t i = 0; i < temp_classes.size(); ++i) {
            const auto& [temp, class_name] = temp_classes[i];
            if (eff_temp > temp) {
                long double prev_temp = (i > 0) ? temp_classes[i-1].first : 200000; // Assuming 200000 as max temp
                int subclass = floor(10.0 - 10.0 * (eff_temp - temp) / (prev_temp - temp));
                return class_name + std::to_string(std::max(0, std::min(9, subclass)));
            }
        }
        return std::string("Y9");
    };

    std::string spec_class = eff_temp > 52000 ? "WN" + std::to_string(std::min(9, std::max(0, int(10 - 10 * (eff_temp - 52000) / 148000))))
                                         : get_spec_class();

    // Stars on the mass/luminosity-derived path are main-sequence (mass->L is the
    // main-sequence relation; age is capped at the MS lifetime; post-MS evolution
    // is not modelled), so the luminosity class is V. The previous absolute-
    // magnitude heuristic mislabelled intrinsically-bright MS stars as IV/III
    // (e.g. an 8 Msun B-dwarf as a giant). Catalog stars set their spectral type
    // directly (setSpecType) and keep any given giant/subgiant class.
    (void)luminosity;
    return spec_class + "V";
}

/**
 * @brief This function, given the orbital radius of a planet in AU, returns
 * the orbital 'zone' of the particle.
 * 
 * @param ecosphere_radius 
 * @param orb_radius 
 * @return int 
 */
auto orbital_zone(long double ecosphere_radius, long double orb_radius) -> int {
  if (orb_radius < (4.0 * ecosphere_radius)) {
    return 1;
  } else if (orb_radius < (15.0 * ecosphere_radius)) {
    return 2;
  } else {
    return 3;
  }
}

/**
 * @brief The mass is in units of solar masses, and the density is in units
 * of grams/cc.  The radius returned is in units of km.
 * 
 * @param mass 
 * @param density 
 * @return long double 
 */

auto volume_radius(long double mass, long double density) -> long double {
  long double volume = NAN;

  mass = mass * SOLAR_MASS_IN_GRAMS;
  volume = mass / density;
  return std::pow((3.0 * volume) / (4.0 * PI), (1.0 / 3.0)) / CM_PER_KM;
}

/**
 * @brief The mass passed in is in units of solar masses, and the orbital radius
 * is in units of AU. The density is returned in units of grams/cc.
 * 
 * @param mass 
 * @param orb_radius 
 * @param r_ecosphere 
 * @param gas_giant 
 * @return long double 
 */
auto empirical_density(long double mass, long double orb_radius,
                              long double r_ecosphere, bool gas_giant) -> long double {
  long double temp = NAN;

  temp = std::pow(mass * SUN_MASS_IN_EARTH_MASSES, 1.0 / 8.0);
  temp = temp * pow1_4(r_ecosphere / orb_radius);
  if (gas_giant) {
    return temp * 1.2;
  } else {
    return temp * 5.5;
  }
}

/**
 * @brief The mass passed in is in units of solar masses, and the equatorial
 * radius is in km.  The density is returned in units of grams/cc.
 * 
 * @param mass 
 * @param equat_radius 
 * @return long double 
 */
auto volume_density(long double mass, long double equat_radius) -> long double {
  long double volume = NAN;

  mass = mass * SOLAR_MASS_IN_GRAMS;
  equat_radius = equat_radius * CM_PER_KM;
  volume = (4.0 * PI * pow3(equat_radius)) / 3.0;
  return mass / volume;
}

/**
 * @brief The separation is in units of AU, and both masses are in units of solar
 * masses. The period returned is in terms of Earth days.
 * 
 * @param separation 
 * @param small_mass 
 * @param large_mass 
 * @return long double 
 */
auto period(long double separation, long double small_mass,
                   long double large_mass) -> long double {
  long double period_in_years = NAN;

  period_in_years = sqrt(pow3(separation) / (small_mass + large_mass));
  return period_in_years * DAYS_IN_A_YEAR;
}




/**
 * @brief Fogg's information for this routine came from Dole "Habitable Planets
 * for Man", Blaisdell Publishing Company, NY, 1964.  From this, he came
 * up with his eq.12, which is the equation for the 'base_angular_velocity'
 * below.  He then used an equation for the change in angular velocity per
 * time (dw/dt) from P. Goldreich and S. Soter's paper "Q in the Solar
 * System" in Icarus, vol 5, pp.375-389 (1966).	 Using as a comparison the
 * change in angular velocity for the Earth, Fogg has come up with an
 * approximation for our new planet (his eq.13) and take that into account
 * This is used to find 'change_in_angular_velocity' below.
 * Input parameters are mass (in solar masses), radius (in Km), orbital
 * period (in days), orbital radius (in AU), density (in g/cc),
 * eccentricity, and whether it is a gas giant or not.
 * The length of the day is returned in units of hours.
 * 
 * @param the_planet 
 * @param parent_mass 
 * @param is_moon 
 * @return long double 
 */
/*--------------------------------------------------------------------------*/
/* Tidal synchronization ("despinning") timescale, Gladman et al. 1996       */
/* (Icarus 122, 166, eq. 1) / Peale 1977 / Murray & Dermott 1999. Pure       */
/* numeric core in CGS, returned in YEARS so it can be compared directly with */
/* the system age. The moment of inertia is written explicitly,              */
/* C = moi_factor * M_p * R_p^2, so the net radius dependence is R_p^-3:      */
/*   t_sync = (4/9)(Q/k2) * a^6 * w * C / (G * M_star^2 * R_p^5)   [seconds]   */
/* (The 4/9 prefactor is order-unity and convention-dependent; the lock       */
/* decision spans many orders of magnitude in t_sync so it is insensitive to  */
/* it. Sanity: Earth/Sun ~13 Gyr -> unlocked; 1 M_earth at 0.1 AU around a    */
/* 0.3 Msun M dwarf ~1.5e5 yr -> locked.)                                     */
/*--------------------------------------------------------------------------*/
auto tidal_sync_time_years(long double a_au, long double m_star_solar, long double r_p_km,
                           long double m_p_solar, long double spin_rad_s, long double moi_factor,
                           long double q, long double k2) -> long double {
  long double a_cm = a_au * CM_PER_AU;
  long double m_star_g = m_star_solar * SOLAR_MASS_IN_GRAMS;
  long double r_p_cm = r_p_km * CM_PER_KM;
  long double m_p_g = m_p_solar * SOLAR_MASS_IN_GRAMS;
  if (a_cm <= 0.0 || m_star_g <= 0.0 || r_p_cm <= 0.0 || m_p_g <= 0.0 || spin_rad_s <= 0.0 ||
      k2 <= 0.0) {
    return INCREDIBLY_LARGE_NUMBER;  // degenerate inputs cannot lock
  }
  long double moment_of_inertia = moi_factor * m_p_g * (r_p_cm * r_p_cm);
  long double a6 = std::pow(a_cm, 6.0L);
  long double r5 = std::pow(r_p_cm, 5.0L);
  long double t_sync_sec = (4.0L / 9.0L) * (q / k2) *
                           (a6 * spin_rad_s * moment_of_inertia) /
                           (GRAV_CONSTANT * (m_star_g * m_star_g) * r5);
  return t_sync_sec / SECONDS_PER_YEAR;
}

/**
 * @brief Tidal-lock timescale (years) for a planet/moon, selecting Q/k2 by type.
 * Wrapper over tidal_sync_time_years; pure, no globals/RNG -> determinism-safe.
 * parent_mass is in SOLAR masses (the star for planets, the planet for moons),
 * matching day_length's convention.
 */
auto tidal_lock_timescale_years(planet *the_planet, long double parent_mass, bool is_moon,
                                long double spin_rad_s, long double moi_factor) -> long double {
  long double a_au = is_moon ? the_planet->getMoonA() : the_planet->getA();
  long double q = is_gas_planet(the_planet) ? TIDAL_Q_GAS : TIDAL_Q_ROCKY;
  long double k2 = is_gas_planet(the_planet) ? TIDAL_LOVE_K2_GAS : TIDAL_LOVE_K2_ROCKY;
  return tidal_sync_time_years(a_au, parent_mass, the_planet->getRadius(), the_planet->getMass(),
                               spin_rad_s, moi_factor, q, k2);
}

/*--------------------------------------------------------------------------*/
/* Post-accretion gas-disk eccentricity damping (Cresswell & Nelson 2008,    */
/* A&A 482, 677; parameterizing Tanaka & Ward 2004). All in solar/AU/year     */
/* units, so Omega_K and the disk lifetime are dimensionless-consistent and   */
/* no new unit constants are needed. Returns the damped eccentricity. Pure    */
/* (no RNG, no globals) -> determinism-safe. Inclination is intentionally NOT */
/* damped here: it is assigned later in generate_planet, so it is zero at this */
/* stage (the iota terms vanish).                                             */
/*--------------------------------------------------------------------------*/
auto gas_disk_damped_eccentricity(long double e, long double a_au, long double m_p_solar,
                                  long double m_star_solar) -> long double {
  if (e <= 0.0 || a_au <= 0.0 || m_p_solar <= 0.0 || m_star_solar <= 0.0) {
    return e;
  }
  long double h = DISK_ASPECT_RATIO;
  long double sigma = DISK_SIGMA0_SOLAR_PER_AU2 * std::pow(a_au, -1.5L);  // solar / AU^2
  long double p_yr = std::sqrt((a_au * a_au * a_au) / (m_star_solar + m_p_solar));  // Kepler III
  long double omega_k = (2.0L * PI) / p_yr;                                          // rad/yr
  long double t_wave =
      (m_star_solar / m_p_solar) * (m_star_solar / (sigma * a_au * a_au)) *
      (h * h * h * h) / omega_k;  // Tanaka & Ward wave time, years
  long double eps = e / h;
  long double bracket = 1.0L - 0.14L * (eps * eps) + 0.06L * (eps * eps * eps);
  if (bracket < DISK_DAMP_BRACKET_FLOOR) {
    bracket = DISK_DAMP_BRACKET_FLOOR;  // never anti-damp at e >> h
  }
  long double t_e = (t_wave / 0.780L) * bracket;
  if (t_e <= 0.0) {
    return e;
  }
  return e * std::exp(-DISK_LIFETIME_YEARS / t_e);
}

/**
 * @brief Apply one post-accretion gas-disk eccentricity-damping pass over the
 * whole planet list (Cresswell & Nelson 2008). Called once after accretion,
 * before per-planet environment generation. Pure arithmetic, adds no RNG draws,
 * so it leaves the rest of the population (masses, distances, compositions)
 * byte-identical and preserves parallel/serial determinism.
 */
void apply_gas_disk_damping(sun &the_sun, planet *innermost) {
  long double m_star = the_sun.getIsCircumbinary() ? the_sun.getCombinedMass() : the_sun.getMass();
  for (planet *p = innermost; p != nullptr; p = p->next_planet) {
    p->setE(gas_disk_damped_eccentricity(p->getE(), p->getA(), p->getMass(), m_star));
  }
}

auto day_length(planet *the_planet, long double parent_mass,
                       bool is_moon) -> long double {
  long double planetary_mass_in_grams =
      the_planet->getMass() * SOLAR_MASS_IN_GRAMS;
  long double equatorial_radius_in_cm = the_planet->getRadius() * CM_PER_KM;
  long double year_in_hours = the_planet->getOrbPeriod() * 24.0;
  // bool giant; maybe this used to be used
  //bool giant = the_planet->getType() == tGasGiant || the_planet->getType() == tBrownDwarf || the_planet->getType() == tSubGasGiant || the_planet->getType() == tSubSubGasGiant;
  long double k2 = NAN;
  long double base_angular_velocity = NAN;
  long double change_in_angular_velocity = NAN;
  long double ang_velocity = NAN;
  long double spin_resonance_factor = NAN;
  long double day_in_hours = NAN;

  bool stopped = false;

  the_planet->setResonantPeriod(false);

  /*if (giant)
  {
    k2 = 0.24;
  }
  else
  {
    k2 = 0.33;
  }*/
  k2 = calculate_moment_of_inertia_coeffient(the_planet);

  // Calculate the base angular velocity
  base_angular_velocity = sqrt(2.0 * J * (planetary_mass_in_grams) /
                               (k2 * pow2(equatorial_radius_in_cm)));

  // This next calculation determines how much the planet's rotation is
  // slowed by the presence of the parent body.
  if (!is_moon) {
    change_in_angular_velocity =
        CHANGE_IN_EARTH_ANG_VEL * (the_planet->getDensity() / EARTH_DENSITY) *
        (equatorial_radius_in_cm / EARTH_RADIUS) *
        (EARTH_MASS_IN_GRAMS / planetary_mass_in_grams) *
        std::pow(parent_mass, 2.0) * (1.0 / std::pow(the_planet->getA(), 6.0));
  } else {
    change_in_angular_velocity =
        CHANGE_IN_EARTH_ANG_VEL * (the_planet->getDensity() / EARTH_DENSITY) *
        (equatorial_radius_in_cm / EARTH_RADIUS) *
        (EARTH_MASS_IN_GRAMS / planetary_mass_in_grams) *
        std::pow(parent_mass, 2.0) * (1.0 / std::pow(the_planet->getMoonA(), 6.0));
  }
  ang_velocity = base_angular_velocity + (change_in_angular_velocity *
                                          the_planet->getTheSun().getAge());

  if (ang_velocity <= 0.0) {
    stopped = true;
    day_in_hours = INCREDIBLY_LARGE_NUMBER;
  } else {
    day_in_hours = RADIANS_PER_ROTATION / (SECONDS_PER_HOUR * ang_velocity);
  }

  // Physically-derived tidal lock: synchronized when the Gladman et al. 1996
  // despinning timescale is short compared with the host system age (replaces the
  // old kinematic day>=year test). NB k2 here is the moment-of-inertia factor
  // computed at line ~467, NOT a tidal Love number.
  long double t_sync_years =
      tidal_lock_timescale_years(the_planet, parent_mass, is_moon, base_angular_velocity, k2);

  if (stopped || (t_sync_years <= the_planet->getTheSun().getAge())) {
    if ((the_planet->getE() > 0.1 && !is_moon) ||
        (the_planet->getMoonE() > 0.1 && is_moon)) {
      if (!is_moon) {
        spin_resonance_factor = getSpinResonanceFactor(the_planet->getE());
      } else {
        spin_resonance_factor = getSpinResonanceFactor(the_planet->getMoonE());
      }
      the_planet->setResonantPeriod(true);
      return spin_resonance_factor * year_in_hours;
    } else {
      the_planet->setAxialTilt(0);
      return year_in_hours;
    }
  }

  return day_in_hours;
}

/**
 * @brief The orbital radius is expected in units of Astronomical Units (AU).
 * Inclination is returned in units of degrees. (seb: real)
 * 
 * @param orb_radius 
 * @param parent_mass 
 * @return long double 
 */
auto inclination(long double orb_radius, long double parent_mass) -> long double {
  // Primordial obliquity is set by the last few stochastic giant impacts, which
  // arrive from random directions in a geometrically thick embryo swarm. The
  // result is an *isotropic* obliquity: cos(eps) is uniform on [-1, 1], i.e.
  // p(eps) = 0.5*sin(eps) on [0, 180] degrees. This admits retrograde spins
  // (cf. Venus ~177 deg, Uranus ~98 deg), unlike a folded Gaussian peaked at 0.
  //   Kokubo & Ida 2007, ApJ 671, 2082; Kokubo & Genda 2010, ApJ 714, L21.
  //   See research/modern/08-spin-obliquity-validation.md.
  long double cos_obliquity = 1.0L - 2.0L * random_number(0.0L, 1.0L);
  long double obliquity = std::acos(cos_obliquity) * 180.0L / PI;  // degrees, [0,180]

  // Tides erode obliquity for planets close to the star (Heller et al. 2011,
  // arXiv:1101.2156); damp linearly toward zero inside the tidal-influence
  // distance. That distance scales with stellar mass as M^(1/3) -- the same
  // dependence as the tidal-locking critical radius -- rather than the old
  // dimensionally-inconsistent comparison of an AU distance against a solar mass.
  // OBLIQUITY_TIDAL_REACH_AU = 1.0 makes this identical to the prior behavior at
  // one solar mass.
  long double tidal_reach = OBLIQUITY_TIDAL_REACH_AU * std::cbrt(parent_mass);
  if (tidal_reach > 0.0 && orb_radius < tidal_reach) {
    obliquity *= (orb_radius / tidal_reach);
  }

  return obliquity;
}

/**
 * @brief This function implements the escape velocity calculation. Note that
 * it appears that Fogg's eq.15 is incorrect.
 * The mass is in units of solar mass, the radius in kilometers, and the
 * velocity returned is in cm/sec.
 * 
 * @param mass 
 * @param radius 
 * @return long double 
 */
auto escape_vel(long double mass, long double radius) -> long double {
  long double mass_in_grams = NAN, radius_in_cm = NAN;

  mass_in_grams = mass * SOLAR_MASS_IN_GRAMS;
  radius_in_cm = radius * CM_PER_KM;
  return sqrt(2.0 * GRAV_CONSTANT * mass_in_grams / radius_in_cm);
}

/*------------------------------------------------------------------------*/
/* This is Fogg's eq.16.  The molecular weight (usually assumed to be N2) */
/* is used as the basis of the Root Mean Square (RMS) velocity of the     */
/* molecule or atom.  The velocity returned is in cm/sec.                 */
/* Orbital radius is in A.U.(ie: in units of the earth's orbital radius). */
/*------------------------------------------------------------------------*/

auto rms_vel(long double molecular_weight, long double exospheric_temp) -> long double {
  return sqrt((3.0 * MOLAR_GAS_CONST * exospheric_temp) / molecular_weight) *
         CM_PER_METER;
}

auto min_molec_weight(planet *the_planet) -> long double {
  long double mass = the_planet->getMass();
  long double radius = the_planet->getRadius();
  long double temp = the_planet->getExosphericTemp();
  long double target = 5.0E9;

  long double guess_1 = molecule_limit(mass, radius, temp);
  long double guess_2 = guess_1;

  long double life = gas_life(guess_1, the_planet);

  int loops = 0;

  target = the_planet->getTheSun().getAge();

  if (life > target) {
    while (life > target && loops++ < 25) {
      guess_1 = guess_1 / 2.0;
      life = gas_life(guess_1, the_planet);
    }
  } else {
    while (life < target && loops++ < 25) {
      guess_2 = guess_2 * 2.0;
      life = gas_life(guess_2, the_planet);
    }
  }

  loops = 0;

  while ((guess_2 - guess_1) > 0.1 && loops++ < 25) {
    long double guess_3 = (guess_1 + guess_2) / 2.0;
    life = gas_life(guess_3, the_planet);

    if (life < target) {
      guess_1 = guess_3;
    } else {
      guess_2 = guess_3;
    }
  }

  // life = gas_life(guess_2, the_planet);

  return guess_2;
}

/*------------------------------------------------------------------------*/
/* This function returns the smallest molecular weight retained by the    */
/* body, which is useful for determining the atmosphere composition.      */
/* Mass is in units of solar masses, and equatorial radius is in units of */
/* kilometers.                                                            */
/*------------------------------------------------------------------------*/

auto molecule_limit(long double mass, long double equat_radius,
                           long double exospheric_temp) -> long double {
  long double esc_velocity = escape_vel(mass, equat_radius);

  return (3.0 * MOLAR_GAS_CONST * exospheric_temp) /
         (pow2((esc_velocity / GAS_RETENTION_THRESHOLD) / CM_PER_METER));
}

/*----------------------------------------------------------------------*/
/* This function calculates the surface acceleration of a planet. The   */
/* mass is in units of solar masses, the radius in terms of km, and the */
/* acceleration is returned in units of cm/sec2.                        */
/*----------------------------------------------------------------------*/

auto acceleration(long double mass, long double radius) -> long double {
  return GRAV_CONSTANT * (mass * SOLAR_MASS_IN_GRAMS) /
         pow2(radius * CM_PER_KM);
}

auto acceleration(planet *the_planet) -> long double {
    const long double SOLAR_MASS_KG = SOLAR_MASS_IN_KG_APPROX;  // kg
    const long double EARTH_MASS_KG = EARTH_MASS_IN_KG_APPROX;  // kg
    const long double EARTH_RADIUS_KM = MEAN_EARTH_RADIUS_KM;   // km
    const long double EARTH_GRAVITY_CM_S2 = EARTH_ACCELERATION; // cm/s^2 (980.665)

    // Convert planet mass to Earth masses
    long double mass_earth_masses = the_planet->getMass() * (SOLAR_MASS_KG / EARTH_MASS_KG);
    
    // Get planet radius in Earth radii
    long double radius_earth_radii = the_planet->getRadius() / EARTH_RADIUS_KM;

    // Get composition fractions
    long double ice_fraction = the_planet->getImf();
    long double rock_fraction = the_planet->getRmf();
    long double carbon_fraction = rock_fraction * the_planet->getCmf();
    long double silicate_fraction = rock_fraction * (1.0 - the_planet->getCmf());
    long double iron_fraction = 1.0 - ice_fraction - rock_fraction;

    // Density factors (relative to Earth's average density)
    const long double ICE_DENSITY_FACTOR = 0.26;
    const long double SILICATE_DENSITY_FACTOR = 0.94;
    const long double CARBON_DENSITY_FACTOR = 1.0;
    const long double IRON_DENSITY_FACTOR = 2.25;

    // Calculate average density factor
    long double avg_density_factor = (ice_fraction * ICE_DENSITY_FACTOR +
                                      silicate_fraction * SILICATE_DENSITY_FACTOR +
                                      carbon_fraction * CARBON_DENSITY_FACTOR +
                                      iron_fraction * IRON_DENSITY_FACTOR);

    // Calculate surface gravity relative to Earth
    long double fudge_factor = .6; // densities are too high, probably not considering enough elements
    long double relative_gravity = (mass_earth_masses / std::pow(radius_earth_radii, 2)) * avg_density_factor * fudge_factor;

    // Convert to cm/s^2
    long double calculated_gravity_cm_s2 = relative_gravity * EARTH_GRAVITY_CM_S2;

    // Log the results
    // std::cout << "Planet mass (Earth masses): " << mass_earth_masses << "\n";
    // std::cout << "Planet radius (Earth radii): " << radius_earth_radii << "\n";
    // std::cout << "Calculated gravity (Earth g): " << relative_gravity << "\n";
    // std::cout << "Calculated gravity (cm/s^2): " << calculated_gravity_cm_s2 << "\n";
    // std::cout << "Ice fraction: " << ice_fraction << "\n";
    // std::cout << "Rock fraction: " << rock_fraction << "\n";
    // std::cout << "Carbon fraction: " << carbon_fraction << "\n";
    // std::cout << "Silicate fraction: " << silicate_fraction << "\n";
    // std::cout << "Iron fraction: " << iron_fraction << "\n";
    // std::cout << "Average density factor: " << avg_density_factor << "\n";

    return calculated_gravity_cm_s2;
}

auto acceleration_oblateness_refinement(planet *the_planet) -> long double {
  const long double G              = GRAV_CONSTANT_SI;        // m^3 kg^-1 s^-2
  const long double EARTH_MASS     = EARTH_MASS_IN_KG_APPROX; // kg
  const long double EARTH_RADIUS_M = MEAN_EARTH_RADIUS_M;     // meters (mean)
  const long double EARTH_GRAVITY  = EARTH_GRAVITY_M_S2;      // m/s^2

  // Constants for transition masses
  const long double ROCKY_TRANSITION_MASS_KG     = 10.0 * EARTH_MASS;   // 10 Earth masses
  const long double GAS_GIANT_TRANSITION_MASS_KG = 100.0 * EARTH_MASS;  // 100 Earth masses

  // Convert planet mass to kilograms
  long double mass_kg = (the_planet->getDustMass() + the_planet->getGasMass()) *
                        SOLAR_MASS_IN_KG_APPROX;    // Convert solar masses to kg
  long double radius_km = the_planet->getRadius();  // Equatorial radius in km

  // Check if radius and mass are valid
  if (radius_km == 0 || mass_kg == 0) {
    return 0.0;  // Avoid divide by zero
  }

  // Calculate the radius based on mass regimes
  long double radius_m;
  if (mass_kg < ROCKY_TRANSITION_MASS_KG) {
    // Rocky planets (mass < 10 Earth masses), R ∝ M^0.25
    radius_m = EARTH_RADIUS_M * std::pow(mass_kg / EARTH_MASS, 0.25);
  } else if (mass_kg < GAS_GIANT_TRANSITION_MASS_KG) {
    // Transition zone (10 to 100 Earth masses), R ∝ M^0.5
    radius_m = EARTH_RADIUS_M * std::pow(mass_kg / EARTH_MASS, 0.5);
  } else {
    // Gas giants (mass ≥ 100 Earth masses), radius approximately constant (around Jupiter's radius)
    radius_m = 71.5e6;  // Set to approximately Jupiter's radius (71,500 km)
  }

  // Adjust for oblateness (if oblateness > 0)
  long double oblateness_factor = the_planet->getOblateness();
  long double polar_radius_m =
      radius_m * (1.0 - oblateness_factor);  // Polar radius is reduced by oblateness

  // Angular velocity (omega) based on day length (convert hours to seconds)
  long double day_seconds      = the_planet->getDay() * 3600;
  long double angular_velocity = (day_seconds > 0) ? (2 * std::numbers::pi / day_seconds) : 0.0;

  // Adjust gravity for centrifugal force at the equator
  long double centrifugal_force =
      angular_velocity * angular_velocity * radius_m;  // Centrifugal force at equator
  long double gravity_equator = G * mass_kg / std::pow(radius_m, 2) - centrifugal_force;

  // Gravity at the poles (no centrifugal force)
  long double gravity_pole = G * mass_kg / std::pow(polar_radius_m, 2);

  // Average gravity (you could use a weighted average if necessary)
  long double average_gravity = (gravity_equator + gravity_pole) / 2.0;

  // Handle different mass regimes for the final gravity calculation
  long double mass_earth_masses = mass_kg / EARTH_MASS;

  if (mass_earth_masses < 1.0) {
    // Rocky bodies below Earth mass: gs ~ M^(1/2)
    average_gravity = EARTH_GRAVITY * std::pow(mass_earth_masses, 0.5);
  } else if (mass_earth_masses < 100.0) {
    // Transition zone: gravity remains roughly constant
    average_gravity = std::max(0.8 * EARTH_GRAVITY, std::min(average_gravity, 1.2 * EARTH_GRAVITY));
  }
  // For gas giants (mass > 100 Earth masses), we'll use the calculated gravity directly

  return average_gravity;
}

/*---------------------------------------------------------------------*/
/* This function calculates the surface gravity of a planet. The       */
/* acceleration is in units of cm/sec2, and the gravity is returned in */
/* units of Earth gravities.                                           */
/*---------------------------------------------------------------------*/

auto gravity(long double acceleration) -> long double {
  return (acceleration / EARTH_ACCELERATION);
}

/*---------------------------------------------------------------------------*/
/*	This implements Fogg's eq.17.  The 'inventory' returned is unitless. */
/*---------------------------------------------------------------------------*/

auto vol_inventory(long double mass, long double escape_vel,
                          long double rms_vel, long double stellar_mass,
                          int zone, bool greenhouse_effect, bool accreted_gas) -> long double {
  long double velocity_ratio = NAN, proportion_const = NAN, temp1 = NAN, temp2 = NAN, earth_units = NAN;

  velocity_ratio = escape_vel / rms_vel;
  if (velocity_ratio >= GAS_RETENTION_THRESHOLD || accreted_gas) {
    switch (zone) {
      case 1:
        proportion_const = 140000.0; /* 100 -> 140 JLB */
        break;
      case 2:
        proportion_const = 75000.0;
        break;
      case 3:
        proportion_const = 250.0;
        break;
      default:

        std::cout << "Error: orbital zone not initialized correctly!\n";
        exit(EXIT_FAILURE);
        return EXIT_FAILURE;
        break;
    }
    earth_units = mass * SUN_MASS_IN_EARTH_MASSES;
    temp1 = (proportion_const * earth_units) / stellar_mass;
    temp2 = temp1;
    if (greenhouse_effect || accreted_gas) {
      return temp2;
    } else {
      return temp2 / 140.0;
    }
  } else {
    return 0.0;
  }
}

/*------------------------------------------------------------------------------*/
/*	This implements Fogg's eq.18.  The pressure returned is in units of */
/*	millibars (mb).	 The gravity is in units of Earth gravities, the radius
 */
/*	in units of kilometers. */
/*                                                                              */
/*  JLB: Aparently this assumed that earth pressure = 1000mb. I've added a
 */
/*	fudge factor (EARTH_SURF_PRES_IN_MILLIBARS / 1000.) to correct for that
 */
/*------------------------------------------------------------------------------*/

auto pressure(long double volatile_gas_inventory,
                     long double equat_radius, long double gravity) -> long double {
  equat_radius = KM_EARTH_RADIUS / equat_radius;
  return volatile_gas_inventory * gravity *
         (EARTH_SURF_PRES_IN_MILLIBARS / 1000.0) / pow2(equat_radius);
}

/*---------------------------------------------------------------------------*/
/* This function returns the boiling point of water in an atmosphere of      */
/* pressure 'surf_pressure', given in millibars.	The boiling point is */
/* returned in units of Kelvin.  This is Fogg's eq.21.                       */
/*---------------------------------------------------------------------------*/

auto boiling_point(long double surf_pressure) -> long double {
  long double surface_pressure_in_bars = NAN;

  surface_pressure_in_bars = surf_pressure / MILLIBARS_PER_BAR;
  return 1.0 / ((log(surface_pressure_in_bars) / -5050.5) + (1.0 / 373.0));
}

/*----------------------------------------------------------------------------*/
/* This function is Fogg's eq.22.	 Given the volatile gas inventory and */
/* planetary radius of a planet (in Km), this function returns the            */
/* fraction of the planet covered with water.                                 */
/* I have changed the function very slightly:	 the fraction of Earth's      */
/* surface covered by water is 71%, not 75% as Fogg used.                     */
/*----------------------------------------------------------------------------*/

auto hydro_fraction(long double volatile_gas_inventory,
                           long double planet_radius) -> long double {
  long double temp = NAN;

  temp = (0.71 * volatile_gas_inventory / 1000.0) *
         pow2(KM_EARTH_RADIUS / planet_radius);
  if (temp >= 1.0) {
    return 1.0;
  } else {
    return temp;
  }
}

/*-----------------------------------------------------------------------*/
/* Given the surface temperature of a planet (in Kelvin), this function  */
/* returns the fraction of cloud cover available. This is Fogg's eq.23.  */
/* See Hart in "Icarus" (vol 33, pp23 - 39, 1978) for an explanation.    */
/* This equation is Hart's eq.3.                                         */
/* I have modified it slightly using constants and relationships from    */
/* Glass's book "Introduction to Planetary Geology", p.46.               */
/* The 'CLOUD_COVERAGE_FACTOR' is the amount of surface area on Earth    */
/* covered by one Kg. of cloud.                                          */
/*-----------------------------------------------------------------------*/

auto cloud_fraction(long double surf_temp,
                           long double smallest_MW_retained,
                           long double equat_radius,
                           long double hydro_fraction) -> long double {
  long double water_vapor_in_kg = NAN, fraction = NAN, surf_area = NAN, hydro_mass = NAN;

  if (smallest_MW_retained > WATER_VAPOR) {
    return 0.0;
  } else {
    surf_area = 4.0 * PI * pow2(equat_radius);
    hydro_mass = hydro_fraction * surf_area * EARTH_WATER_MASS_PER_AREA;
    water_vapor_in_kg = (0.00000001 * hydro_mass) *
                        exp(Q2_36 * (surf_temp - EARTH_AVERAGE_KELVIN));
    fraction = CLOUD_COVERAGE_FACTOR * water_vapor_in_kg / surf_area;
    if (fraction >= 1.0) {
      return 1.0;
    } else {
      return fraction;
    }
  }
}

/*------------------------------------------------------------------------------*/
/* Given the surface temperature of a planet (in Kelvin), this function */
/* returns the fraction of the planet's surface covered by ice.  This is */
/* Fogg's eq.24.	See Hart[24] in Icarus vol.33, p.28 for an explanation.
 */
/* I have changed a constant from 70 to 90 in order to bring it more in */
/* line with the fraction of the Earth's surface covered with ice, which */
/* is approximatly .016 (=1.6%). */
/*------------------------------------------------------------------------------*/

auto ice_fraction(long double hydro_fraction, long double surf_temp) -> long double {
  long double temp = NAN;

  if (surf_temp > 328.0) {
    surf_temp = 328.0;
  }
  temp = std::pow(((328.0 - surf_temp) / 90.0), 5.0);
  if (temp > (1.5 * hydro_fraction)) {
    temp = 1.5 * hydro_fraction;
  }
  if (temp >= 1.0) {
    return 1.0;
  } else {
    return temp;
  }
}

/*--------------------------------------------------------------------------*/
/* This is Fogg's eq.19.  The ecosphere radius is given in AU, the orbital  */
/* radius in AU, and the temperature returned is in Kelvin.                 */
/*--------------------------------------------------------------------------*/
auto eff_temp(long double ecosphere_radius, long double orb_radius,
                     long double albedo) -> long double {
  return sqrt(ecosphere_radius / orb_radius) *
         pow1_4((1.0 - albedo) / (1.0 - EARTH_ALBEDO)) * EARTH_EFFECTIVE_TEMP;
}

auto est_temp(long double ecosphere_radius, long double orb_radius,
                     long double albedo) -> long double {
  return sqrt(ecosphere_radius / orb_radius) *
         pow1_4((1.0 - albedo) / (1.0 - EARTH_ALBEDO)) * EARTH_AVERAGE_KELVIN;
}

/*--------------------------------------------------------------------------*/
/* Standard radiative equilibrium temperature (global average, full heat     */
/* redistribution): T_eq = T_EQ_BASE * (1-A)^(1/4) * (L/Lsun)^(1/4) / sqrt(a)*/
/* Uses the star's actual luminosity (in solar units) and the orbital radius */
/* in AU, rather than est_temp's r_ecosphere proxy and Earth-surface (288 K) */
/* normalization. Earth (L=1, a=1, A=0.3) -> 254.8 K, the true equilibrium   */
/* temperature (Earth's 288 K mean surface is that plus a ~33 K greenhouse).  */
/*--------------------------------------------------------------------------*/
auto equilibrium_temp(long double luminosity, long double orb_radius,
                      long double albedo) -> long double {
  return T_EQ_BASE * pow1_4(1.0 - albedo) * pow1_4(luminosity) / sqrt(orb_radius);
}

/*-------------------------------------------------------------------------*/
/* Old grnhouse:                                                           */
/* Note that if the orbital radius of the planet is greater than or equal  */
/* to R_inner, 99% of it's volatiles are assumed to have been deposited in */
/* surface reservoirs (otherwise, it suffers from the greenhouse effect).  */
/*-------------------------------------------------------------------------*/
/*	if ((orb_radius < r_greenhouse) && (zone == 1)) */

/*------------------------------------------------------------------------*/
/* The new definition is based on the inital surface temperature and what */
/* state water is in. If it's too hot, the water will never condense out  */
/* of the atmosphere, rain down and form an ocean. The albedo used here   */
/* was chosen so that the boundary is about the same as the old method    */
/* Neither zone, nor r_greenhouse are used in this version. JLB           */
/*------------------------------------------------------------------------*/

auto grnhouse(long double r_ecosphere, long double orb_radius) -> bool {
  long double temp =
      eff_temp(r_ecosphere, orb_radius, GREENHOUSE_TRIGGER_ALBEDO);

  if (temp > FREEZING_POINT_OF_WATER) {
    return true;
  } else {
    return false;
  }
}

/*-------------------------------------------------------------------------*/
/* This is Fogg's eq.20, and is also Hart's eq.20 in his "Evolution of     */
/* Earth's Atmosphere" article.  The effective temperature given is in     */
/* units of Kelvin, as is the rise in temperature produced by the          */
/* greenhouse effect, which is returned.                                   */
/* I tuned this by changing a std::pow(x,.25) to std::pow(x,.4) to match Venus - JLB */
/*-------------------------------------------------------------------------*/

auto green_rise(long double optical_depth, long double effective_temp,
                       long double surf_pressure) -> long double {
  long double convection_factor =
      EARTH_CONVECTION_FACTOR *
      std::pow(surf_pressure / EARTH_SURF_PRES_IN_MILLIBARS, 0.4);
  long double rise = (pow1_4(1.0 + 0.75 * optical_depth) - 1.0) *
                     effective_temp * convection_factor;

  if (rise < 0.0) {
    rise = 0.0;
  }

  return rise;
}

/*--------------------------------------------------------------------*/
/* The surface temperature passed in is in units of Kelvin.           */
/* The cloud adjustment is the fraction of cloud cover obscuring each */
/* of the three major components of albedo that lie below the clouds. */
/*--------------------------------------------------------------------*/

auto planet_albedo(planet *the_planet) -> long double {
  long double water_fraction = NAN, cloud_fraction = NAN, ice_fraction = NAN, surf_pressure = NAN,
      rock_fraction = NAN, cloud_adjustment = NAN, components = NAN, cloud_part = NAN, rock_part = NAN,
      water_part = NAN, ice_part = NAN;

  sun the_sun = the_planet->getTheSun();
  water_fraction = the_planet->getHydrosphere();
  cloud_fraction = the_planet->getCloudCover();
  ice_fraction = the_planet->getIceCover();
  surf_pressure = the_planet->getSurfPressure();

  rock_fraction = 1.0 - water_fraction - ice_fraction;
  components = 0.0;
  if (water_fraction > 0.0) {
    components += 1.0;
  }
  if (ice_fraction > 0.0) {
    components += 1.0;
  }
  if (rock_fraction > 0.0) {
    components += 1.0;
  }

  cloud_adjustment = (components > 0.0) ? (cloud_fraction / components) : 0.0;

  if (rock_fraction >= cloud_adjustment) {
    rock_fraction -= cloud_adjustment;
  } else {
    rock_fraction = 0.0;
  }

  if (water_fraction > cloud_adjustment) {
    water_fraction -= cloud_adjustment;
  } else {
    water_fraction = 0.0;
  }

  if (ice_fraction > cloud_adjustment) {
    ice_fraction -= cloud_adjustment;
  } else {
    ice_fraction = 0.0;
  }

  cloud_part = cloud_fraction * CLOUD_ALBEDO; /* about(...,0.2); */

  if (surf_pressure == 0.0) {
    rock_part = rock_fraction * ROCKY_AIRLESS_ALBEDO; /* about(...,0.3); */
    ice_part = ice_fraction * AIRLESS_ICE_ALBEDO;     /* about(...,0.4); */
    water_part = 0;
  } else {
    if (the_planet->getSph() > 0.0 &&
        (is_habitable(the_planet) ||
         (is_potentialy_habitable(the_planet) &&
          (surf_pressure / EARTH_SURF_PRES_IN_MILLIBARS) > 0.25))) {
      rock_part = rock_fraction * getPlantLifeAlbedo(the_sun.getSpecType(),
                                                     the_sun.getLuminosity());
    } else {
      rock_part = rock_fraction * ROCKY_ALBEDO; /* about(..., 0.1); */
    }
    water_part = water_fraction * WATER_ALBEDO; /* about(...,0.2); */
    ice_part = ice_fraction * ICE_ALBEDO;       /* about(...,0.1); */
  }

  return cloud_part + rock_part + water_part + ice_part;
}

/*---------------------------------------------------------------------*/
/* This function returns the dimensionless quantity of optical depth,  */
/* which is useful in determining the amount of greenhouse effect on a */
/* planet.                                                             */
/*---------------------------------------------------------------------*/

#include <cmath>

long double opacity(long double molecular_weight, long double surf_pressure, long double effective_temp) {
    // Base opacity calculation
    long double base_opacity = 0.0;

    if (molecular_weight < 10.0) {
        base_opacity = 3.0 - 0.066 * (10.0 - molecular_weight);
    } else if (molecular_weight < 20.0) {
        base_opacity = 2.34 - 0.134 * (20.0 - molecular_weight);
    } else if (molecular_weight < 30.0) {
        base_opacity = 1.0 - 0.085 * (30.0 - molecular_weight);
    } else if (molecular_weight < 45.0) {
        base_opacity = 0.15 - 0.057 * (45.0 - molecular_weight);
    } else if (molecular_weight < 100.0) {
        base_opacity = 0.05 - 0.0009 * (100.0 - molecular_weight);
    }

    // Pressure contribution
    long double pressure_in_earth = surf_pressure / EARTH_SURF_PRES_IN_MILLIBARS;
    long double pressure_factor = 1.0;
    
    if (pressure_in_earth >= 70.0) {
        pressure_factor = 8.333;
    } else if (pressure_in_earth >= 50.0) {
        pressure_factor = 6.666 + (8.333 - 6.666) * (pressure_in_earth - 50.0) / 20.0;
    } else if (pressure_in_earth >= 30.0) {
        pressure_factor = 3.333 + (6.666 - 3.333) * (pressure_in_earth - 30.0) / 20.0;
    } else if (pressure_in_earth >= 10.0) {
        pressure_factor = 2.0 + (3.333 - 2.0) * (pressure_in_earth - 10.0) / 20.0;
    } else if (pressure_in_earth >= 5.0) {
        pressure_factor = 1.5 + (2.0 - 1.5) * (pressure_in_earth - 5.0) / 5.0;
    } else {
        pressure_factor = 1.0 + (1.5 - 1.0) * (pressure_in_earth - 0.0) / 5.0;
    }

    // Temperature dependence
    // Assuming opacity increases with temperature (simplified model)
    long double temp_factor = 1.0L + 0.01L * (effective_temp - 273.15L); // 273.15 is 0°C in Kelvin
    // Clamp to >= 0 so very cold worlds (below ~173K) don't produce a
    // negative optical depth feeding into the greenhouse math.
    if (temp_factor < 0.0L) {
        temp_factor = 0.0L;
    }

    // Combine all factors
    long double optical_depth = base_opacity * pressure_factor * temp_factor;

    return optical_depth;
}

/*
 *	calculates the number of years it takes for 1/e of a gas to escape
 *	from a planet's atmosphere.
 *	Taken from Dole p. 34. He cites Jeans (1916) & Jones (1923)
 */
auto gas_life(long double molecular_weight, planet *the_planet) -> long double {
  long double v = rms_vel(molecular_weight, the_planet->getExosphericTemp());
  long double g = the_planet->getSurfGrav() * EARTH_ACCELERATION;
  long double r = the_planet->getRadius() * CM_PER_KM;
  long double t =
      (pow3(v) / (2.0 * pow2(g) * r)) * exp((3.0 * g * r) / pow2(v));
  long double years = t / (SECONDS_PER_HOUR * 24.0 * DAYS_IN_A_YEAR);

  if (years > 2.0E10) {
    years = INCREDIBLY_LARGE_NUMBER;
  }

  return years;
}

/*----------------------------------------------------------------*/
/* The temperature calculated is in degrees Kelvin.               */
/* Quantities already known which are used in these calculations: */
/* planet->molec_weight                                           */
/* planet->surf_pressure                                          */
/* R_ecosphere                                                    */
/* planet->a                                                      */
/* planet->volatile_gas_inventory                                 */
/* planet->radius                                                 */
/* planet->boil_point                                             */
/*----------------------------------------------------------------*/

void calculate_surface_temp(planet *the_planet, bool first,
                            long double last_water, long double last_clouds,
                            long double last_ice, long double last_temp,
                            long double last_albedo, bool do_gasses) {
  long double effective_temp = NAN;
  long double water_raw = NAN;
  long double clouds_raw = NAN;
  long double greenhouse_temp = NAN;
  bool boil_off = false;
  sun the_sun = the_planet->getTheSun();
  long double the_fudged_radius = fudged_radius(
      the_planet->getMass(), the_planet->getImf(), the_planet->getRmf(),
      the_planet->getCmf(), the_planet->getGasGiant(),
      the_planet->getOrbitZone(), the_planet);

  do_gasses = (flags_arg_clone & fDoGases) != 0;

  if (first) {
    if (!is_gas_planet(the_planet)) {
      the_planet->setAlbedo(EARTH_ALBEDO);
    }

    effective_temp =
        eff_temp(the_planet->getTheSun().getREcosphere(
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
                 the_planet->getA(), the_planet->getAlbedo());
    greenhouse_temp = green_rise(
        opacity(the_planet->getMolecWeight(), the_planet->getSurfPressure(), effective_temp),
        effective_temp, the_planet->getSurfPressure());
    the_planet->setSurfTemp(effective_temp + greenhouse_temp);
    set_temp_range(the_planet);
  }

  if (the_planet->getGreenhouseEffect() &&
      the_planet->getMaxTemp() < the_planet->getBoilPoint()) {
    if (flag_verbose & 0x0010) {
      std::cerr << "Deluge: " << the_planet->getTheSun().getName() << " "
           << the_planet->getPlanetNo() << " max ("
           << toString(the_planet->getMaxTemp()) << ") < boil ("
           << toString(the_planet->getBoilPoint()) << ")\n";
    }

    the_planet->setGreenhouseEffect(false);

    // the_planet->setVolatileGasInventory(vol_inventory(the_planet->getMass(),
    // the_planet->getEscVelocity(), the_planet->getRmsVelocity(),
    // the_planet->getTheSun().getMass(), the_planet->getOrbitZone(),
    // the_planet->getGreenhouseEffect(), the_planet->getGasMass() > 0.0));
    long double fudged_escape_velocity =
        escape_vel(the_planet->getMass(), the_fudged_radius);
    the_planet->setVolatileGasInventory(vol_inventory(
        the_planet->getMass(), fudged_escape_velocity,
        the_planet->getRmsVelocity(), the_planet->getTheSun().getMass(),
        the_planet->getOrbitZone(), the_planet->getGreenhouseEffect(),
        the_planet->getGasMass() > 0.0));
    the_planet->setSurfPressure(pressure(the_planet->getVolatileGasInventory(),
                                         the_fudged_radius,
                                         the_planet->getSurfGrav()));
    // the_planet->setSurfPressure(pressure(the_planet->getVolatileGasInventory(),
    // the_planet->getRadius(), the_planet->getSurfGrav()));

    the_planet->setBoilPoint(boiling_point(the_planet->getBoilPoint()));
  }

  // water_raw = hydro_fraction(the_planet->getVolatileGasInventory(),
  // the_planet->getRadius());
  water_raw =
      hydro_fraction(the_planet->getVolatileGasInventory(), the_fudged_radius);
  the_planet->setHydrosphere(water_raw);
  // clouds_raw = cloud_fraction(the_planet->getSurfTemp(),
  // the_planet->getMolecWeight(), the_planet->getRadius(),
  // the_planet->getHydrosphere());
  clouds_raw =
      cloud_fraction(the_planet->getSurfTemp(), the_planet->getMolecWeight(),
                     the_fudged_radius, the_planet->getHydrosphere());
  the_planet->setCloudCover(clouds_raw);
  the_planet->setIceCover(
      ice_fraction(the_planet->getHydrosphere(), the_planet->getSurfTemp()));
  if (the_planet->getImf() > 0.1 &&
      the_planet->getRadius() >= round_threshold(the_planet->getDensity()) &&
      the_planet->getA() >=
          habitable_zone_distance(
              the_sun, MAXIMUM_GREENHOUSE,
              the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) {
    the_planet->setIceCover(1.0);
  } else if (the_planet->getImf() &&
             the_planet->getRadius() <
                 round_threshold(the_planet->getDensity()) &&
             the_planet->getA() >=
                 habitable_zone_distance(
                     the_sun, MAXIMUM_GREENHOUSE,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) {
    the_planet->setIceCover(the_planet->getIceCover() + the_planet->getImf());
  }

  if (the_planet->getGreenhouseEffect() &&
      the_planet->getSurfPressure() > 0.0) {
    the_planet->setCloudCover(1.0);
  }

  if ((the_planet->getHighTemp() >= the_planet->getBoilPoint() && !first &&
       !((int)the_planet->getDay() ==
             (int)(the_planet->getOrbPeriod() * 24.0) ||
         the_planet->getResonantPeriod())) ||
      (the_planet->getMinTemp() >= the_planet->getBoilPoint() && !first)) {
    the_planet->setHydrosphere(0.0);
    boil_off = true;

    if (the_planet->getMolecWeight() > WATER_VAPOR) {
      the_planet->setCloudCover(0.0);
    } else {
      the_planet->setCloudCover(1.0);
    }
  }

  if (the_planet->getSurfTemp() < (FREEZING_POINT_OF_WATER - 3.0)) {
    the_planet->setHydrosphere(0.0);
  }

  the_planet->setHzc(calcHzc(the_planet));
  the_planet->setHza(calcHza(the_planet));
  the_planet->setEsi(calcEsi(the_planet));
  the_planet->setSph(calcSph(the_planet));

  if (!is_gas_planet(the_planet)) {
    if (do_gasses) {
      calculate_gases(the_sun, the_planet, "");
    }
    the_planet->setAlbedo(planet_albedo(the_planet));
  }
  effective_temp =
      eff_temp(the_planet->getTheSun().getREcosphere(the_planet->getMass() *
                                                     SUN_MASS_IN_EARTH_MASSES),
               the_planet->getA(), the_planet->getAlbedo());
      greenhouse_temp = green_rise(
        opacity(the_planet->getMolecWeight(), the_planet->getSurfPressure(), effective_temp),
        effective_temp, the_planet->getSurfPressure());
  the_planet->setSurfTemp(effective_temp + greenhouse_temp);

  if (!first) {
    if (!boil_off) {
      the_planet->setHydrosphere(
          (the_planet->getHydrosphere() + (last_water * 2.0)) / 3.0);
    }
    the_planet->setCloudCover(
        (the_planet->getCloudCover() + (last_clouds * 2.0)) / 3.0);
    the_planet->setIceCover((the_planet->getIceCover() + (last_ice * 2.0)) /
                            3.0);
    the_planet->setAlbedo((the_planet->getAlbedo() + (last_albedo * 2.0)) /
                          3.0);
    the_planet->setSurfTemp((the_planet->getSurfTemp() + (last_temp * 2.0)) /
                            3);
  }

  set_temp_range(the_planet);

  if (flag_verbose & 0x0020) {
    std::string greenhouse_string = "";
    if (the_planet->getGreenhouseEffect()) {
      greenhouse_string = "*";
    }
    std::cerr << toString(the_planet->getA()) << " AU: "
         << toString(the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER)
         << " = " << toString(effective_temp - FREEZING_POINT_OF_WATER)
         << " ef + " << toString(greenhouse_temp) << " gh" << greenhouse_string
         << " (W: " << toString(the_planet->getHydrosphere()) << " ("
         << toString(water_raw)
         << ") C: " << toString(the_planet->getCloudCover()) << " ("
         << toString(clouds_raw)
         << ") I: " << toString(the_planet->getIceCover()) << " A: ("
         << toString(the_planet->getAlbedo()) << "))\n";
  }
}

void iterate_surface_temp(planet *the_planet, bool do_gasses) {
  int count = 0;
  long double initial_temp =
      est_temp(the_planet->getTheSun().getREcosphere(the_planet->getMass() *
                                                     SUN_MASS_IN_EARTH_MASSES),
               the_planet->getA(), the_planet->getAlbedo());

  long double h2_life = gas_life(MOL_HYDROGEN, the_planet);
  long double h2o_life = gas_life(WATER_VAPOR, the_planet);
  long double n2_life = gas_life(MOL_NITROGEN, the_planet);
  long double n_life = gas_life(ATOMIC_NITROGEN, the_planet);

  do_gasses = (flags_arg_clone & fDoGases) != 0;

  if (flag_verbose & 0x20000) {
    std::cerr << the_planet->getPlanetNo() << ":                     "
         << toString(initial_temp) << " it ["
         << toString(the_planet->getTheSun().getREcosphere(
                the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES))
         << " re " << toString(the_planet->getA()) << " a "
         << toString(the_planet->getAlbedo()) << " alb]\n";
  }

  if (flag_verbose & 0x0040) {
    std::cerr << std::endl
         << "Gas lifetimes: H2 - " << toString(h2_life) << ", H2O - "
         << toString(h2o_life) << ", N - " << toString(n_life) << ", N2 - "
         << toString(n2_life) << std::endl;
  }

  calculate_surface_temp(the_planet, true, 0, 0, 0, 0, 0, do_gasses);

  for (count = 0; count <= 25; count++) {
    long double last_water = the_planet->getHydrosphere();
    long double last_clouds = the_planet->getCloudCover();
    long double last_ice = the_planet->getIceCover();
    long double last_temp = the_planet->getSurfTemp();
    long double last_albedo = the_planet->getAlbedo();

    calculate_surface_temp(the_planet, false, last_water, last_clouds, last_ice,
                           last_temp, last_albedo, do_gasses);

    if (fabs(the_planet->getSurfTemp() - last_temp) < 0.25) {
      break;
    }
  }

  the_planet->setGreenhsRise(the_planet->getSurfTemp() - initial_temp);

  if (flag_verbose & 0x20000) {
    std::cerr << the_planet->getPlanetNo() << ": "
         << toString(the_planet->getGreenhsRise())
         << " gh = " << toString(the_planet->getSurfTemp()) << " ("
         << toString(the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER)
         << " C) st - " << toString(initial_temp) << " it ["
         << toString(the_planet->getTheSun().getREcosphere(
                the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES))
         << " re " << toString(the_planet->getA()) << " a "
         << toString(the_planet->getAlbedo()) << " alb]\n";
  }
}

/*----------------------------------------------------------------------*/
/* Inspired partial pressure, taking into account humidification of the */
/* air in the nasal passage and throat This formula is on Dole's p. 14  */
/*----------------------------------------------------------------------*/

auto inspired_partial_pressure(long double surf_pressure,
                                      long double gas_pressure) -> long double {
  long double pH2O = (H20_ASSUMED_PRESSURE);
  long double fraction = gas_pressure / surf_pressure;

  return (surf_pressure - pH2O) * fraction;
}

/*------------------------------------------------------------------------*/
/* This function uses figures on the maximum inspired partial pressures   */
/* of Oxygen, other atmospheric and traces gases as laid out on pages 15, */
/* 16 and 18 of Dole's Habitable Planets for Man to derive breathability  */
/* of the planet's atmosphere. JLB                                        */
/*------------------------------------------------------------------------*/

auto breathability(planet *the_planet) -> unsigned int {
  // Breathability gates on OXYGEN only. Any gas above its maximum inspired
  // partial pressure makes the air POISONOUS (handled in the loop); oxygen must
  // additionally sit at/above its physiological floor. CO2 and N2 are NOT
  // required to be present: there is no minimum inhaled CO2 or N2 for
  // respiration (the body holds alveolar pCO2 ~40 mmHg regardless of ambient
  // CO2), so a clean, CO2-poor / N2-poor O2 atmosphere is perfectly breathable.
  // Their only hazard is in EXCESS, which the POISONOUS check already covers.
  // The previous code ANDed nitrogen_ok && co2_ok into BREATHABLE, which wrongly
  // demanded a *minimum* CO2 (>=0.05 mmHg) and N2 (>=10 mmHg) and so flagged
  // otherwise-respirable atmospheres UNBREATHABLE; classic StarGen gated on
  // oxygen alone.
  bool oxygen_ok = false;
  bool methane_excess = false;

  if (the_planet->getNumGases() == 0) {
    return NONE;
  }

  for (int i = 0; i < the_planet->getNumGases(); i++) {
    int gas_no = 0;

    long double ipp = inspired_partial_pressure(
        the_planet->getSurfPressure(), the_planet->getGas(i).getSurfPressure());
    for (int j = 0; j < gases.count(); j++) {
      if (gases[j].getNum() == the_planet->getGas(i).getNum()) {
        gas_no = j;
        break;
      }
    }

    if (ipp > gases[gas_no].getMaxIpp()) {
      // Methane's limit is its 5% lower explosive limit, not a toxicity
      // threshold -- it is a simple asphyxiant/flammable gas with no PEL. Excess
      // methane is UNBREATHABLE, not POISONOUS. Defer the verdict so a genuinely
      // toxic gas over its own limit still takes POISONOUS precedence.
      if (the_planet->getGas(i).getNum() == AN_CH4) {
        methane_excess = true;
        continue;
      }
      return POISONOUS;
    }

    if (the_planet->getGas(i).getNum() == AN_O) {
      oxygen_ok =
          ipp >= gases[gas_no].getMinIpp() && ipp <= gases[gas_no].getMaxIpp();
    }
  }

  if (methane_excess) {
    return UNBREATHABLE;
  }
  if (oxygen_ok) {
    return BREATHABLE;
  }
  return UNBREATHABLE;
}

void set_temp_range(planet *the_planet) {
  long double pressmod =
      1 / sqrt(1 + 20 * the_planet->getSurfPressure() / 1000.0);
  long double ppmod = 1 / sqrt(10 + 5 * the_planet->getSurfPressure() / 1000.0);
  long double tiltmod = fabs(cos(the_planet->getAxialTilt() * PI / 180) *
                             std::pow(1 + the_planet->getE(), 2));
  long double daymod = 1 / (200 / the_planet->getDay() + 1);
  long double mh = std::pow(1 + daymod, pressmod);
  long double ml = std::pow(1 - daymod, pressmod);
  long double hi = mh * the_planet->getSurfTemp();
  long double lo = ml * the_planet->getSurfTemp();
  long double sh = hi + std::pow((100 + hi) * tiltmod, sqrt(ppmod));
  long double wl = lo - std::pow((150 + lo) * tiltmod, sqrt(ppmod));
  long double max =
      the_planet->getSurfTemp() + sqrt(the_planet->getSurfTemp()) * 10;
  long double min = the_planet->getSurfTemp() / sqrt(the_planet->getDay() + 24);

  if (lo < min) {
    lo = min;
  }
  if (wl < 0) {
    wl = 0;
  }

  the_planet->setHighTemp(soft(hi, max, min));
  the_planet->setLowTemp(soft(lo, max, min));
  the_planet->setMaxTemp(soft(sh, max, min));
  the_planet->setMinTemp(soft(wl, max, min));
}

/**
 * @brief Spin-orbit resonance ratio for a tidally-evolved eccentric body.
 *
 * On an eccentric orbit the tidal equilibrium spin is not 1:1 synchronous but a
 * higher spin-orbit resonance (e.g. Mercury's 3:2), because the tidal torque is
 * dominated by the fast pericenter passage. This snaps the body to the nearest
 * low-order resonance using (1-e)/(1+e) -- the ratio of the orbital angular
 * velocity at apocenter to that at pericenter -- as the selector, and returns the
 * resonant ratio = (rotation period)/(orbital period), i.e. the day/year factor
 * the caller multiplies by the orbital period. Returns 1 (synchronous) at e=0.
 *   Goldreich & Peale 1966, AJ 71, 425; Murray & Dermott 1999, Solar System
 *   Dynamics, ch. 5. (Discretized lookup; valid for the low-e terrestrial regime.)
 */
auto getSpinResonanceFactor(long double eccentricity) -> long double {
  long double top = 1.0 - eccentricity;
  long double bottom = 1.0 + eccentricity;
  long double fraction = top / bottom;
  if (fraction <= AVE(1.0 / 10.0, 1.0 / 9.0)) {
    return 1.0 / 10.0;
  } else if (fraction <= AVE(1.0 / 9.0, 1.0 / 8.0)) {
    return 1.0 / 9.0;
  } else if (fraction <= AVE(1.0 / 8.0, 1.0 / 7.0)) {
    return 1.0 / 8.0;
  } else if (fraction <= AVE(1.0 / 7.0, 1.0 / 6.0)) {
    return 1.0 / 7.0;
  } else if (fraction <= AVE(1.0 / 6.0, 1.0 / 5.0)) {
    return 1.0 / 6.0;
  } else if (fraction <= AVE(1.0 / 5.0, 2.0 / 9.0)) {
    return 1.0 / 5.0;
  } else if (fraction <= AVE(2.0 / 9.0, 1.0 / 4.0)) {
    return 2.0 / 9.0;
  } else if (fraction <= AVE(1.0 / 4.0, 2.0 / 7.0)) {
    return 1.0 / 4.0;
  } else if (fraction <= AVE(2.0 / 7.0, 3.0 / 10.0)) {
    return 2.0 / 7.0;
  } else if (fraction <= AVE(3.0 / 10.0, 1.0 / 3.0)) {
    return 3.0 / 10.0;
  } else if (fraction <= AVE(1.0 / 3.0, 3.0 / 8.0)) {
    return 1.0 / 3.0;
  } else if (fraction <= AVE(3.0 / 8.0, 2.0 / 5.0)) {
    return 3.0 / 8.0;
  } else if (fraction <= AVE(2.0 / 5.0, 3.0 / 7.0)) {
    return 2.0 / 5.0;
  } else if (fraction <= AVE(3.0 / 7.0, 4.0 / 9.0)) {
    return 3.0 / 7.0;
  } else if (fraction <= AVE(4.0 / 9.0, 1.0 / 2.0)) {
    return 4.0 / 9.0;
  } else if (fraction <= AVE(1.0 / 2.0, 5.0 / 9.0)) {
    return 1.0 / 2.0;
  } else if (fraction <= AVE(5.0 / 9.0, 4.0 / 7.0)) {
    return 5.0 / 9.0;
  } else if (fraction <= AVE(4.0 / 7.0, 3.0 / 5.0)) {
    return 4.0 / 7.0;
  } else if (fraction <= AVE(3.0 / 5.0, 5.0 / 8.0)) {
    return 3.0 / 5.0;
  } else if (fraction <= AVE(5.0 / 8.0, 2.0 / 3.0)) {
    return 5.0 / 8.0;
  } else if (fraction <= AVE(2.0 / 3.0, 7.0 / 10.0)) {
    return 2.0 / 3.0;
  } else if (fraction <= AVE(7.0 / 10.0, 5.0 / 7.0)) {
    return 7.0 / 10.0;
  } else if (fraction <= AVE(5.0 / 7.0, 3.0 / 4.0)) {
    return 5.0 / 7.0;
  } else if (fraction <= AVE(3.0 / 4.0, 7.0 / 9.0)) {
    return 3.0 / 4.0;
  } else if (fraction <= AVE(7.0 / 9.0, 4.0 / 5.0)) {
    return 7.0 / 9.0;
  } else if (fraction <= AVE(4.0 / 5.0, 5.0 / 6.0)) {
    return 4.0 / 5.0;
  } else if (fraction <= AVE(5.0 / 6.0, 6.0 / 7.0)) {
    return 5.0 / 6.0;
  } else if (fraction <= AVE(6.0 / 7.0, 7.0 / 8.0)) {
    return 6.0 / 7.0;
  } else if (fraction <= AVE(7.0 / 8.0, 8.0 / 9.0)) {
    return 7.0 / 8.0;
  } else if (fraction <= AVE(8.0 / 9.0, 9.0 / 10.0)) {
    return 8.0 / 9.0;
  } else {
    return 9.0 / 10.0;
  }
}

/**
 * @brief Rocky (ice-free) planet radius from Zeng, Sasselov & Jacobsen 2016.
 *
 * R/Re = (1.07 - 0.21 * CMF) * (M/Me)^(1/3.7), a PREM-based closed form for a
 * two-layer Fe-core + MgSiO3-mantle planet (ApJ 819, 127; arXiv:1512.08827).
 * CMF is the core (iron) mass fraction. Calibrated for 1-8 Me, CMF 0-0.4; it is
 * monotonic and well-behaved when extrapolated outside that range, which a
 * generator needs. Earth (CMF ~= 0.33, M = 1 Me) -> ~1.0 Re.
 * See research/modern/04-mass-radius-interior.md.
 *
 * @param mass_earth planet mass in Earth masses
 * @param core_mass_fraction iron/core mass fraction (clamped to [0,1])
 * @return radius in Earth radii
 */
static auto zeng_rock_radius(long double mass_earth,
                             long double core_mass_fraction) -> long double {
  long double cmf = core_mass_fraction;
  if (cmf < 0.0L) {
    cmf = 0.0L;
  } else if (cmf > 1.0L) {
    cmf = 1.0L;
  }
  return (1.07L - 0.21L * cmf) * std::pow(mass_earth, 1.0L / 3.7L);
}

auto radius_improved(long double mass, long double imf, long double rmf,
                            long double cmf, bool giant, int zone,
                            planet *the_planet) -> long double {
  long double non_ice_rock_radius = 0.0;
  ;
  long double ice_rock_radius = 0.0;
  long double ice_iron_radius = 0.0;
  long double radius = 0.0;
  long double radius1 = 0.0;
  long double radius2 = 0.0;
  std::map<long double, long double> non_ice_rock_radii;
  std::map<long double, long double> ice_rock_radii;
  std::map<long double, long double> ice_iron_radii;
  long double range = NAN, upper_fraction = NAN, lower_fraction = NAN;
  long double half_mass_factor = 0.0;
  long double iron_ratio = 0.0;
  if (imf < 1.0) {
    iron_ratio = (1.0 - imf - rmf) / (1.0 - imf);
  }
  long double average = 0.0;
  mass *= SUN_MASS_IN_EARTH_MASSES;
  if (mass == 0.0) {
    mass = PROTOPLANET_MASS;
  }
  non_ice_rock_radii[0.0] = iron_radius(mass, the_planet, solid_iron);
  non_ice_rock_radii[0.5] = half_rock_half_iron_radius(
      mass, cmf, the_planet, solid_half_rock_half_iron);
  non_ice_rock_radii[1.0] = rock_radius(mass, cmf, the_planet, solid_rock);
  if (imf > 0.0) {
    ice_rock_radii[0.0] = rock_radius(mass, cmf, the_planet, solid_rock);
    ice_rock_radii[0.5] = half_rock_half_water_radius(
        mass, cmf, the_planet, solid_half_rock_half_water);
    ice_rock_radii[0.75] = one_quater_rock_three_fourths_water_radius(
        mass, cmf, the_planet, solid_one_quater_rock_three_fourths_water);
    ice_rock_radii[1.0] = water_radius(mass, the_planet, solid_water);
    if (imf < 0.5) {
      ice_rock_radius = planet_radius_helper(imf, 0.0, ice_rock_radii[0.0], 0.5,
                                             ice_rock_radii[0.5], 0.75,
                                             ice_rock_radii[0.75], false);
    } else if (imf < 0.75) {
      // std::cout << "test1\n";
      // std::cout << toString(mass) << std::endl;
      radius1 = planet_radius_helper(imf, 0.0, ice_rock_radii[0.0], 0.5,
                                     ice_rock_radii[0.5], 0.75,
                                     ice_rock_radii[0.75], false);
      radius2 = planet_radius_helper(imf, 0.5, ice_rock_radii[0.5], 0.75,
                                     ice_rock_radii[0.75], 1.0,
                                     ice_rock_radii[1.0], false);
      ice_rock_radius = rangeAdjust(imf, radius1, radius2, 0.5, 0.75);
    } else {
      // std::cout << "test2\n";
      // std::cout << toString(mass) << std::endl;
      radius1 = planet_radius_helper(imf, 0.5, ice_rock_radii[0.5], 0.75,
                                     ice_rock_radii[0.75], 1.0,
                                     ice_rock_radii[1.0], false);
      radius2 = planet_radius_helper2(imf, 0.75, ice_rock_radii[0.75], 1.0,
                                      ice_rock_radii[1.0]);
      ice_rock_radius = rangeAdjust(imf, radius1, radius2, 0.75, 1.0);
    }
    ice_iron_radii[0.0] = iron_radius(mass, the_planet, solid_iron);
    ice_iron_radii[0.047] = solid_0point953_iron_0point047_water_radius(
        mass, the_planet, solid_0point953_iron_0point047_water);
    ice_iron_radii[0.49] = solid_0point51_iron_0point49_water_radius(
        mass, the_planet, solid_0point51_iron_0point49_water);
    ice_iron_radii[0.736] = solid_0point264_iron_0point736_water_radius(
        mass, the_planet, solid_0point264_iron_0point736_water);
    ice_iron_radii[1.0] = ice_rock_radii[1.0];
    if (imf < 0.047) {
      ice_iron_radius = planet_radius_helper(imf, 0.0, ice_iron_radii[0.0],
                                             0.047, ice_iron_radii[0.047], 0.49,
                                             ice_iron_radii[0.49], false);
    } else if (imf < 0.49) {
      radius1 = planet_radius_helper(imf, 0.0, ice_iron_radii[0.0], 0.047,
                                     ice_iron_radii[0.047], 0.49,
                                     ice_iron_radii[0.49], false);
      radius2 = planet_radius_helper(imf, 0.047, ice_iron_radii[0.047], 0.49,
                                     ice_iron_radii[0.49], 0.736,
                                     ice_iron_radii[0.736], false);
      ice_iron_radius = rangeAdjust(imf, radius1, radius2, 0.047, 0.49);
    } else if (imf < 0.736) {
      radius1 = planet_radius_helper(imf, 0.047, ice_iron_radii[0.047], 0.49,
                                     ice_iron_radii[0.49], 0.736,
                                     ice_iron_radii[0.736], false);
      radius2 = planet_radius_helper(imf, 0.49, ice_iron_radii[0.49], 0.736,
                                     ice_iron_radii[0.736], 1.0,
                                     ice_iron_radii[1.0], false);
      ice_iron_radius = rangeAdjust(imf, radius1, radius2, 0.49, 0.736);
    } else {
      radius1 = planet_radius_helper(imf, 0.49, ice_iron_radii[0.49], 0.736,
                                     ice_iron_radii[0.736], 1.0,
                                     ice_iron_radii[1.0], false);
      radius2 = planet_radius_helper2(imf, 0.736, ice_iron_radii[0.736], 1.0,
                                      ice_iron_radii[1.0]);
      ice_iron_radius = rangeAdjust(imf, radius1, radius2, 0.736, 1.0);
    }
    half_mass_factor = 1.0;
    if (mass > 0.0) {
      average = AVE(non_ice_rock_radii[0.0], non_ice_rock_radii[1.0]);
      half_mass_factor = non_ice_rock_radii[0.5] / average;
    }
    radius = planet_radius_helper(
        iron_ratio, 0.0, ice_rock_radius, 0.5,
        AVE(ice_rock_radius, ice_iron_radius) * half_mass_factor, 1.0,
        ice_iron_radius, false);
  } else {
    // Ice-free rocky planet: Zeng et al. 2016 closed form. With no ice the core
    // (iron) mass fraction is simply 1 - rmf. Replaces the iron/rock EOS-table
    // blend for the dominant terrestrial case (the ice branches above still use
    // the water EOS tables, which Zeng 2016 does not cover).
    non_ice_rock_radius = zeng_rock_radius(mass, 1.0 - rmf);
    radius = non_ice_rock_radius;
  }
  radius *= KM_EARTH_RADIUS;
  return radius;
}

auto fudged_radius(long double mass, long double imf, long double rmf,
                          long double cmf, bool giant, int zone,
                          planet *the_planet) -> long double {
  long double range = NAN, upper_fraction = NAN, lower_fraction = NAN, non_ice_rock_radius = NAN,
      ice_rock_radius = NAN, radius = NAN;
  mass *= SUN_MASS_IN_EARTH_MASSES;
  // Rock+iron (ice-free) component via Zeng et al. 2016. Within the non-ice
  // portion the core (iron) mass fraction is iron/(rock+iron) = (1-imf-rmf)/(1-imf).
  long double non_ice_cmf = (imf < 1.0) ? (1.0 - imf - rmf) / (1.0 - imf) : 0.0;
  non_ice_rock_radius = zeng_rock_radius(mass, non_ice_cmf);

  if (imf <= 0.5) {
    range = 0.5 - 0.0;
    upper_fraction = imf / range;
    lower_fraction = 1.0 - upper_fraction;
    ice_rock_radius =
        (upper_fraction *
         half_rock_half_water_radius(mass, cmf, the_planet,
                                     solid_half_rock_half_water)) +
        (lower_fraction * rock_radius(mass, cmf, the_planet, solid_rock));
  } else if (imf <= 0.75) {
    range = 0.75 - 0.5;
    upper_fraction = (imf - 0.5) / range;
    lower_fraction = 1.0 - upper_fraction;
    ice_rock_radius =
        (upper_fraction * one_quater_rock_three_fourths_water_radius(
                              mass, cmf, the_planet,
                              solid_one_quater_rock_three_fourths_water)) +
        (lower_fraction *
         half_rock_half_water_radius(mass, cmf, the_planet,
                                     solid_half_rock_half_water));
  } else {
    range = 1.0 - 0.75;
    upper_fraction = (imf - 0.75) / range;
    lower_fraction = 1.0 - upper_fraction;
    ice_rock_radius =
        (upper_fraction * water_radius(mass, the_planet, solid_water)) +
        (lower_fraction *
         one_quater_rock_three_fourths_water_radius(
             mass, cmf, the_planet, solid_one_quater_rock_three_fourths_water));
  }
  radius = (ice_rock_radius * imf) + (non_ice_rock_radius * (1.0 - imf));
  radius *= KM_EARTH_RADIUS;
  return radius;
}

auto gas_radius(long double temperature, long double core_mass,
                       long double total_mass, long double star_age,
                       planet *the_planet) -> long double {
  long double jupiter_radii = 0.0;
  long double jupiter_radii1 = 0.0;
  long double jupiter_radii2 = 0.0;
  long double core_earth_masses = core_mass * SUN_MASS_IN_EARTH_MASSES;
  long double total_earth_masses = total_mass * SUN_MASS_IN_EARTH_MASSES;
  long double lower_fraction = 0.0;
  long double upper_fraction = 0.0;
  long double range = 0.0;
  long double radius = NAN;
  std::map<double, long double> age_radii;
  age_radii[300.0E6] = gas_radius_300Myr(temperature, core_earth_masses,
                                         total_earth_masses, the_planet);
  age_radii[1.0E9] = gas_radius_1Gyr(temperature, core_earth_masses,
                                     total_earth_masses, the_planet);
  age_radii[4.5E9] = gas_radius_4point5Gyr(temperature, core_earth_masses,
                                           total_earth_masses, the_planet);
  /*if (star_age < 300.0E6)
  {
    upper_fraction = 300.0E6 / star_age;
    jupiter_radii = gas_radius_300Myr(temperature, core_earth_masses,
  total_earth_masses, the_planet) * sqrt(upper_fraction);
  }
  else if (star_age < 1.0E9)
  {
    range = 1.0E9 - 300.0E6;
    upper_fraction = (star_age - 300.0E6) / range;
    lower_fraction = 1.0 - upper_fraction;
    jupiter_radii = (lower_fraction * gas_radius_300Myr(temperature,
  core_earth_masses, total_earth_masses, the_planet)) + (upper_fraction *
  gas_radius_1Gyr(temperature, core_earth_masses, total_earth_masses,
  the_planet));
  }
  else if (star_age < 4.5E9)
  {
    range = 4.5E9 - 1.0E9;
    upper_fraction = (star_age - 4.5E9) / range;
    lower_fraction = 1.0 - upper_fraction;
    jupiter_radii = (lower_fraction * gas_radius_1Gyr(temperature,
  core_earth_masses, total_earth_masses, the_planet)) + (upper_fraction *
  gas_radius_4point5Gyr(temperature, core_earth_masses, total_earth_masses,
  the_planet));
  }
  else
  {
    jupiter_radii = gas_radius_4point5Gyr(temperature, core_earth_masses,
  total_earth_masses, the_planet);
  }*/
  /*if (star_age < 300.0E6)
  {
    jupiter_radii = planet_radius_helper2(star_age, 300.0E6,
  age_radii[300.0E6], 1.0E9, age_radii[1.0E9]);
  }
  else */
  if (star_age < 1.0E9) {
    // jupiter_radii1 = planet_radius_helper2(star_age, 300.0E6,
    // age_radii[300.0E6], 1.0E9, age_radii[1.0E9]); jupiter_radii2 =
    // planet_radius_helper(star_age, 300.0E6, age_radii[300.0E6], 1.0E9,
    // age_radii[1.0E9], 4.5E9, age_radii[4.5E9], false); jupiter_radii =
    // rangeAdjust(star_age, jupiter_radii1, jupiter_radii2, 300.0E6, 1.0E9);
    jupiter_radii =
        planet_radius_helper(star_age, 300.0E6, age_radii[300.0E6], 1.0E9,
                             age_radii[1.0E9], 4.5E9, age_radii[4.5E9], false);
  } else if (star_age < 4.5E9) {
    jupiter_radii1 =
        planet_radius_helper(star_age, 300.0E6, age_radii[300.0E6], 1.0E9,
                             age_radii[1.0E9], 4.5E9, age_radii[4.5E9], false);
    jupiter_radii2 = planet_radius_helper2(star_age, 1.0E9, age_radii[1.0E9],
                                           4.5E9, age_radii[4.5E9]);
    jupiter_radii =
        rangeAdjust(star_age, jupiter_radii1, jupiter_radii2, 1.0E9, 4.5E9);
  } else {
    jupiter_radii = planet_radius_helper2(star_age, 1.0E9, age_radii[1.0E9],
                                          4.5E9, age_radii[4.5E9]);
  }
  radius = jupiter_radii * KM_JUPITER_RADIUS;

  if (total_earth_masses < 10) {
    range = 10.0 - 0.0;
    upper_fraction = pow2((10.0 - total_earth_masses) / range);
    lower_fraction = 1.0 - upper_fraction;
    radius = (lower_fraction * radius) +
             (upper_fraction * gas_dwarf_radius(the_planet));
  }
  return radius;
}

auto round_threshold(long double density) -> long double {
  return (170.0 * sqrt(ultimateStrength(density)) * std::pow(density, -1.0)) / 2.0;
}

auto ultimateStrength(long double density) -> long double {
  if (density < 2.5) {
    return 1.046601879 * std::pow(4.294487989, density);
  } else {
    return 13.50087381 * std::pow(1.54411359, density);
  }
}

auto calc_stellar_flux(long double a, long double b, long double c,
                              long double d, long double seff,
                              long double star_temp, long double star_lum) -> long double {
  long double t = star_temp - 5780.0;
  // std::cout << star_temp << std::endl;
  return (seff) + (a * t) + (b * std::pow(t, 2.0)) + (c * std::pow(t, 3.0)) +
         (d * std::pow(t, 4.0));
}

auto habitable_zone_distance_helper(long double effTemp,
                                           long double luminosity, int mode,
                                           long double mass) -> long double {
  long double stellar_flux = 0;
  long double a = NAN, b = NAN, c = NAN, d = NAN, seff = NAN;
  long double stellar_flux_green1 = NAN, stellar_flux_green2 = NAN, stellar_flux_moist1 = NAN,
      stellar_flux_earth1 = NAN, stellar_flux_max1 = NAN, stellar_flux_max2 = NAN, diff = NAN, percent = NAN,
      temp = NAN;

  if (mass < 0.1) {
    mass = 0.1;
  } else if (mass > 10.0) {
    mass = 10.0;
  }

  if (mode == RECENT_VENUS) {
    // stellar_flux = calc_stellar_flux(1.4316E-4, 2.9875E-9, -7.5702E-12,
    // -1.1635E-15, 1.7753, effTemp, luminosity);
    stellar_flux = calc_stellar_flux(2.136E-4, 2.533E-8, -1.332E-11, -3.097E-15,
                                     1.776, effTemp, luminosity);
  } else if (mode == RUNAWAY_GREENHOUSE) {
    // stellar_flux = calc_stellar_flux(1.3242E-4, 1.5418E-8, -7.9895E-12,
    // -1.8328E-15, 1.0512, effTemp, luminosity);
    if (mass < 1.0) {
      a = planet_radius_helper(mass, 0.1, 1.209E-4, 1.0, 1.332E-4, 5.0,
                               1.433E-4);
      b = planet_radius_helper(mass, 0.1, 1.404E-8, 1.0, 1.58E-8, 5.0,
                               1.707E-8);
      c = planet_radius_helper(mass, 0.1, -7.418E-12, 1.0, -8.308E-12, 5.0,
                               -8.968E-12);
      d = planet_radius_helper(mass, 0.1, -1.713E-15, 1.0, -1.931E-15, 5.0,
                               -2.084E-15);
      seff = planet_radius_helper(mass, 0.1, 0.99, 1.0, 1.107, 5.0, 1.188);
    } else {
      a = planet_radius_helper2(mass, 1.0, 1.332E-4, 5.0, 1.433E-4);
      b = planet_radius_helper2(mass, 1.0, 1.58E-8, 5.0, 1.707E-8);
      c = planet_radius_helper2(mass, 1.0, -8.308E-12, 5.0, -8.968E-12);
      d = planet_radius_helper2(mass, 1.0, -1.931E-15, 5.0, -2.084E-15);
      seff = planet_radius_helper2(mass, 1.0, 1.107, 5.0, 1.188);
    }
    stellar_flux = calc_stellar_flux(a, b, c, d, seff, effTemp, luminosity);
  } else if (mode == MOIST_GREENHOUSE) {
    // stellar_flux = calc_stellar_flux(8.1774E-5, 1.7063E-9, -4.3241E-12,
    // -6.6462E-16, 1.0140, effTemp, luminosity);
    stellar_flux_green1 = calc_stellar_flux(
        1.332E-4, 1.58E-8, -8.308E-12, -1.931E-15, 1.107, effTemp, luminosity);
    stellar_flux_moist1 =
        calc_stellar_flux(8.1774E-5, 1.7063E-9, -4.3241E-12, -6.6462E-16,
                          1.0140, effTemp, luminosity);
    stellar_flux_max1 =
        calc_stellar_flux(5.8942E-5, 1.6558E-9, -3.0045E-12, -5.2983E-16,
                          0.3438, effTemp, luminosity);
    diff = stellar_flux_green1 - stellar_flux_max1;
    percent = (stellar_flux_green1 - stellar_flux_moist1) / diff;
    if (mass < 1.0) {
      a = planet_radius_helper(mass, 0.1, 1.209E-4, 1.0, 1.332E-4, 5.0,
                               1.433E-4);
      b = planet_radius_helper(mass, 0.1, 1.404E-8, 1.0, 1.58E-8, 5.0,
                               1.707E-8);
      c = planet_radius_helper(mass, 0.1, -7.418E-12, 1.0, -8.308E-12, 5.0,
                               -8.968E-12);
      d = planet_radius_helper(mass, 0.1, -1.713E-15, 1.0, -1.931E-15, 5.0,
                               -2.084E-15);
      seff = planet_radius_helper(mass, 0.1, 0.99, 1.0, 1.107, 5.0, 1.188);
    } else {
      a = planet_radius_helper2(mass, 1.0, 1.332E-4, 5.0, 1.433E-4);
      b = planet_radius_helper2(mass, 1.0, 1.58E-8, 5.0, 1.707E-8);
      c = planet_radius_helper2(mass, 1.0, -8.308E-12, 5.0, -8.968E-12);
      d = planet_radius_helper2(mass, 1.0, -1.931E-15, 5.0, -2.084E-15);
      seff = planet_radius_helper2(mass, 1.0, 1.107, 5.0, 1.188);
    }
    stellar_flux_green2 =
        calc_stellar_flux(a, b, c, d, seff, effTemp, luminosity);
    stellar_flux_max2 = calc_stellar_flux(
        6.171E-5, 1.698E-9, -3.198E-12, -5.575E-16, 0.356, effTemp, luminosity);
    diff = stellar_flux_green2 - stellar_flux_max2;
    temp = diff * percent;
    stellar_flux = stellar_flux_green2 - (temp * diff);
  } else if (mode == EARTH_LIKE) {
    // stellar_flux = calc_stellar_flux(8.3104E-5, 1.7677E-9, -4.39E-12,
    // -6.79E-16, 1.0, effTemp, luminosity);
    stellar_flux_green1 = calc_stellar_flux(
        1.332E-4, 1.58E-8, -8.308E-12, -1.931E-15, 1.107, effTemp, luminosity);
    stellar_flux_earth1 = calc_stellar_flux(
        8.3104E-5, 1.7677E-9, -4.39E-12, -6.79E-16, 1.0, effTemp, luminosity);
    stellar_flux_max1 =
        calc_stellar_flux(5.8942E-5, 1.6558E-9, -3.0045E-12, -5.2983E-16,
                          0.3438, effTemp, luminosity);
    diff = stellar_flux_green1 - stellar_flux_max1;
    percent = (stellar_flux_green1 - stellar_flux_earth1) / diff;
    if (mass < 1.0) {
      a = planet_radius_helper(mass, 0.1, 1.209E-4, 1.0, 1.332E-4, 5.0,
                               1.433E-4);
      b = planet_radius_helper(mass, 0.1, 1.404E-8, 1.0, 1.58E-8, 5.0,
                               1.707E-8);
      c = planet_radius_helper(mass, 0.1, -7.418E-12, 1.0, -8.308E-12, 5.0,
                               -8.968E-12);
      d = planet_radius_helper(mass, 0.1, -1.713E-15, 1.0, -1.931E-15, 5.0,
                               -2.084E-15);
      seff = planet_radius_helper(mass, 0.1, 0.99, 1.0, 1.107, 5.0, 1.188);
    } else {
      a = planet_radius_helper2(mass, 1.0, 1.332E-4, 5.0, 1.433E-4);
      b = planet_radius_helper2(mass, 1.0, 1.58E-8, 5.0, 1.707E-8);
      c = planet_radius_helper2(mass, 1.0, -8.308E-12, 5.0, -8.968E-12);
      d = planet_radius_helper2(mass, 1.0, -1.931E-15, 5.0, -2.084E-15);
      seff = planet_radius_helper2(mass, 1.0, 1.107, 5.0, 1.188);
    }
    stellar_flux_green2 =
        calc_stellar_flux(a, b, c, d, seff, effTemp, luminosity);
    stellar_flux_max2 = calc_stellar_flux(
        6.171E-5, 1.698E-9, -3.198E-12, -5.575E-16, 0.356, effTemp, luminosity);
    diff = stellar_flux_green2 - stellar_flux_max2;
    temp = diff * percent;
    stellar_flux = stellar_flux_green2 - (temp * diff);
  } else if (mode == FIRST_CO2_CONDENSATION_LIMIT) {
    stellar_flux = calc_stellar_flux(4.4499e-5, 1.4065e-10, 2.2750e-12,
                                     -3.3509e-16, 0.5408, effTemp, luminosity);
  } else if (mode == MAXIMUM_GREENHOUSE) {
    // stellar_flux = calc_stellar_flux(5.8942E-5, 1.6558E-9, -3.0045E-12,
    // -5.2983E-16, 0.3438, effTemp, luminosity);
    stellar_flux = calc_stellar_flux(6.171E-5, 1.698E-9, -3.198E-12, -5.575E-16,
                                     0.356, effTemp, luminosity);
  } else if (mode == EARLY_MARS) {
    // stellar_flux = calc_stellar_flux(5.4513E-5, 1.5313E-9, -2.7786E-12,
    // -4.8997E-16, 0.3179, effTemp, luminosity);
    stellar_flux = calc_stellar_flux(5.547E-5, 1.526E-9, -2.874E-12, -5.011E-16,
                                     0.32, effTemp, luminosity);
  } else if (mode == TWO_AU_CLOUD_LIMIT) {
    stellar_flux = calc_stellar_flux(4.2588e-5, 1.1963e-9, -2.1709e-12,
                                     -3.8282e-16, 0.2484, effTemp, luminosity);
  }
  return stellar_flux;
}

auto habitable_zone_distance(sun &the_sun, int mode, long double mass) -> long double {
  long double stellar_flux = 0;
  // long double stellar_flux2 = 0;
  // long double combined_stellar_flux = 0;
  if (the_sun.getEffTemp() >= 2600 && the_sun.getEffTemp() <= 7200) {
    if (!the_sun.getIsCircumbinary()) {
      stellar_flux = habitable_zone_distance_helper(
          the_sun.getEffTemp(), the_sun.getLuminosity(), mode, mass);
    } else {
      stellar_flux = habitable_zone_distance_helper(
          the_sun.getCombinedEffTemp(), the_sun.getLuminosity(), mode, mass);
    }

    if (stellar_flux <= 0) {
      std::cerr << "Error! Program caluclated an invalid stellar flux!\n";
      std::cerr << "Star Name: " << the_sun.getName() << std::endl;
      std::cerr << "Star Mass: " << toString(the_sun.getMass()) << std::endl;
      std::cerr << "Star Luminosity: " << toString(the_sun.getLuminosity()) << std::endl;
      std::cerr << "Star Temperature: " << toString(the_sun.getEffTemp()) << std::endl;
      std::cerr << "Star Type: " << the_sun.getSpecType() << std::endl;
      std::cerr << "Zone: ";
      if (mode == RECENT_VENUS) {
        std::cerr << "Recent Venus";
      } else if (mode == RUNAWAY_GREENHOUSE) {
        std::cerr << "Runaway Greenhouse";
      } else if (mode == MOIST_GREENHOUSE) {
        std::cerr << "Moist Greenhouse";
      } else if (mode == EARTH_LIKE) {
        std::cerr << "Earth-like";
      } else if (mode == FIRST_CO2_CONDENSATION_LIMIT) {
        std::cerr << "First CO2 Condensation";
      } else if (mode == MAXIMUM_GREENHOUSE) {
        std::cerr << "Maximum Greenhouse";
      } else if (mode == EARLY_MARS) {
        std::cerr << "Early Mars";
      } else if (mode == TWO_AU_CLOUD_LIMIT) {
        std::cerr << "Two AU Cloud Limit\n";
      }
      std::cerr << std::endl;
      std::cerr << "Stellar Flux: " << toString(stellar_flux) << std::endl;
      exit(EXIT_FAILURE);
      return 0;
    }

    if (!the_sun.getIsCircumbinary()) {
      return sqrt(the_sun.getLuminosity() / stellar_flux);
    } else {
      // stellar_flux2 =
      // habitable_zone_distance_helper(the_sun.getSecondaryEffTemp(),
      // the_sun.getSecondaryLuminosity(), mode); combined_stellar_flux =
      // stellar_flux + stellar_flux2;
      return sqrt(the_sun.getCombinedLuminosity() / stellar_flux);
    }
  } else {
    if (!the_sun.getIsCircumbinary()) {
      if (mode == RECENT_VENUS || mode == RUNAWAY_GREENHOUSE ||
          mode == MOIST_GREENHOUSE) {
        return sqrt(the_sun.getLuminosity() / 1.51);
      } else if (mode == EARTH_LIKE) {
        return sqrt(the_sun.getLuminosity());
      } else if (mode == FIRST_CO2_CONDENSATION_LIMIT ||
                 mode == MAXIMUM_GREENHOUSE || mode == EARLY_MARS ||
                 mode == TWO_AU_CLOUD_LIMIT) {
        return sqrt(the_sun.getLuminosity() / 0.48);
      }
    } else {
      if (mode == RECENT_VENUS || mode == RUNAWAY_GREENHOUSE ||
          mode == MOIST_GREENHOUSE) {
        return sqrt(the_sun.getCombinedLuminosity() / 1.51);
      } else if (mode == EARTH_LIKE) {
        return sqrt(the_sun.getCombinedLuminosity());
      } else if (mode == FIRST_CO2_CONDENSATION_LIMIT ||
                 mode == MAXIMUM_GREENHOUSE || mode == EARLY_MARS ||
                 mode == TWO_AU_CLOUD_LIMIT) {
        return sqrt(the_sun.getCombinedLuminosity() / 0.48);
      }
    }
  }
  return 0;
}

auto calcLambda(long double distance, long double mass) -> long double {
  return (pow(mass, 2.0) / std::pow(distance, 3.0 / 2.0)) * 1.7E16;
}

void gas_giant_temperature_albedo(planet *the_planet, long double parent_mass,
                                  bool is_moon) {
  long double temp1 = NAN;
  long double temp2 = NAN;
  long double temp3 = NAN;
  long double temp4 = NAN;
  long double temp5 = NAN;
  long double temp6 = NAN;
  long double new_albedo = NAN;
  long double new_radius = NAN;
  long double new_day = NAN;
  long double new_obleteness = NAN;
  int loops = 0;
  if (the_planet->getTheSun().getAge() == 0.0) {
    the_planet->setTheSun(the_sun_clone);
  }
  temp3 = about(the_planet->getType() == tSubSubGasGiant ? SUB_NEPTUNE_ALBEDO : GAS_GIANT_ALBEDO,
                0.1);
  the_planet->setAlbedo(temp3);
  temp1 = equilibrium_temp(the_planet->getTheSun().getLuminosity(),
                           the_planet->getA(), the_planet->getAlbedo());
  the_planet->setEstimatedTemp(temp1);
  // temp4 = new_radius = gas_radius(temp1, the_planet->getDustMass(),
  // the_planet->getMass(), the_planet->getTheSun().getAge(), the_planet);
  temp4 = calcRadius(the_planet);
  the_planet->setRadius(temp4);
  temp5 = day_length(the_planet, parent_mass, is_moon);
  // the_planet->setDay(temp5);
  temp6 = calcOblateness(the_planet);
  while (true) {
    if (loops >= 1000) {
      break;
    }
    if (the_planet->getType() == tSubSubGasGiant) {
      // Gas dwarfs / sub-Neptunes are not Sudarsky giants: use a realistic
      // ice-giant-like Bond albedo (~0.3) regardless of cloud-class temperature,
      // instead of the giant water-/ammonia-cloud albedos (~0.8) that made them
      // absurdly cold. See SUB_NEPTUNE_ALBEDO in const.h for references.
      new_albedo = about(SUB_NEPTUNE_ALBEDO, 0.1);
    } else if (temp1 > TEMPERATURE_CARBON_GIANT) {
      new_albedo = about(CARBON_GIANT_ALBEDO, 0.1);
    } else if (temp1 > TEMPERATURE_CLASS_V) {
      new_albedo =
          about(getGasGiantAlbedo("V", the_planet->getTheSun().getSpecType(),
                                  the_planet->getTheSun().getLuminosity()),
                0.1);
    } else if (temp1 > TEMPERATURE_CLASS_IV) {
      new_albedo =
          about(getGasGiantAlbedo("IV", the_planet->getTheSun().getSpecType(),
                                  the_planet->getTheSun().getLuminosity()),
                0.1);
    } else if (temp1 > TEMPERATURE_CLASS_III) {
      new_albedo =
          about(getGasGiantAlbedo("III", the_planet->getTheSun().getSpecType(),
                                  the_planet->getTheSun().getLuminosity()),
                0.1);
    } else if (temp1 > TEMPERATURE_SULFUR_GIANT) {
      new_albedo = about(SULFAR_GIANT_ALBEDO, 0.1);
    } else if (temp1 > TEMPERATURE_CLASS_II) {
      new_albedo =
          about(getGasGiantAlbedo("II", the_planet->getTheSun().getSpecType(),
                                  the_planet->getTheSun().getLuminosity()),
                0.1);
    } else if (temp1 >
               TEMPERATURE_CLASS_I)  // visually, this is where it starts to
                                     // turn blue in selden's version of StarGen
    {
      new_albedo =
          about(getGasGiantAlbedo("I", the_planet->getTheSun().getSpecType(),
                                  the_planet->getTheSun().getLuminosity()),
                0.3);
    } else {
      new_albedo = about(METHANE_GIANT_ALBEDO, 0.1);
    }
    temp3 = ((new_albedo * 2.0) + temp3) / 3.0;
    temp2 = equilibrium_temp(the_planet->getTheSun().getLuminosity(),
                             the_planet->getA(), temp3);
    temp1 = (temp2 + (temp1 * 2.0)) / 3.0;
    the_planet->setEstimatedTemp(temp1);
    // new_radius = gas_radius(temp1, the_planet->getDustMass(),
    // the_planet->getMass(), the_planet->getTheSun().getAge(), the_planet);
    new_radius = calcRadius(the_planet);
    temp4 = (new_radius + (temp4 * 2.0)) / 3.0;
    the_planet->setRadius(temp4);
    new_day = day_length(the_planet, parent_mass, is_moon);
    temp5 = (temp5 + (new_day * 2.0)) / 3.0;
    // the_planet->setDay(temp5);
    new_obleteness = calcOblateness(the_planet);
    temp6 = (temp6 + (new_obleteness * 2.0)) / 3.0;
    if (temp1 > 900 && temp1 < 1400) {
      // std::cout << "blada\n";
      if (fabs(temp1 - temp2) < 0.0025 &&
          fabs(the_planet->getAlbedo() - new_albedo) < 0.001) {
        break;
      }
    } else {
      // std::cout << temp1 << " " << temp2 << " " << fabs(temp1 - temp2) << std::endl;
      if (fabs(temp1 - temp2) < 0.25) {
        break;
      }
    }
    loops++;
  }
  the_planet->setAlbedo(temp3);
  the_planet->setEstimatedTemp(temp1);
  the_planet->setRadius(temp4);
  // the_planet->setDay(temp5);
  the_planet->setOblateness(temp6);
}

auto getGasGiantAlbedo(const std::string& sudusky_class, const std::string& star_type,
                              long double luminosity) -> long double {
  int num = star_type_to_num(star_type, luminosity);

  if (sudusky_class == "I") {
    return logistal_trend(0.0081213045, -0.0696581881, 0.6651148941,
                          (long double)num);
  } else if (sudusky_class == "II") {
    return logistal_trend(0.0045058891, -0.0760456409, 0.9129152579,
                          (long double)num);
  } else if (sudusky_class == "III") {
    return logistal_trend(0.00077110769, -0.14678595, 0.1751842946,
                          (long double)num);
  } else if (sudusky_class == "IV") {
    return logistal_trend(0.00094635821, -0.1476824696, 0.0417580996,
                          (long double)num);
  } else if (sudusky_class == "V") {
    return logistal_trend(0.0205970047, -0.0332241697, 0.6005902693,
                          (long double)num);
  } else {
    std::cout << "Error!\n";
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }
}

/**
 * @brief Structure to hold gas retention calculation parameters
 */
struct GasRetentionFactors {
  long double abundance;
  long double reactivity;
  long double pressure_factor;
  long double retention_fraction;
};

/**
 * @brief Calculate gas retention factors for Argon
 *
 * Argon accumulates slowly through radioactive decay over stellar age
 */
static auto calculate_argon_retention(Chemical& the_gas, sun& the_sun,
                                     long double base_abundance) -> GasRetentionFactors {
  GasRetentionFactors factors;
  factors.abundance = base_abundance;
  factors.reactivity = 0.15 * the_sun.getAge() / 4e9;  // Accumulation factor
  factors.pressure_factor = 1.0;
  factors.retention_fraction = 1.0;
  return factors;
}

/**
 * @brief Calculate gas retention factors for Helium
 *
 * Helium abundance is affected by gas giant mass fraction
 * Helium escapes more easily from low-mass planets
 */
static auto calculate_helium_retention(Chemical& the_gas, sun& the_sun,
                                      planet* the_planet, long double base_abundance,
                                      long double pressure) -> GasRetentionFactors {
  GasRetentionFactors factors;
  // Helium enhanced in gas giants due to primordial retention
  factors.abundance = base_abundance *
                     (0.001 + (the_planet->getGasMass() / the_planet->getMass()));
  factors.pressure_factor = (0.75 + pressure);
  factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                               the_sun.getAge() / 2e9 * factors.pressure_factor);
  factors.retention_fraction = 1.0;
  return factors;
}

/**
 * @brief Calculate gas retention factors for Oxygen/O2
 *
 * Oxygen production requires stellar age > 2Gyr and specific temperature range
 * Free oxygen indicates photosynthesis or photochemical processes
 */
static auto calculate_oxygen_retention(Chemical& the_gas, sun& the_sun,
                                      planet* the_planet, long double base_abundance,
                                      long double pressure) -> GasRetentionFactors {
  GasRetentionFactors factors;
  factors.abundance = base_abundance;

  // Oxygen only accumulates on mature systems with suitable temperatures
  if (the_sun.getAge() > 2e9 &&
      (the_planet->getGasGiant() ||
       (the_planet->getSurfTemp() > 270 && the_planet->getSurfTemp() < 400))) {
    factors.pressure_factor = (0.89 + pressure / 4);
    factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                                 std::pow(the_sun.getAge() / 2e9, 0.25) * factors.pressure_factor);
  } else {
    // Default retention for young systems or unsuitable temperatures
    factors.pressure_factor = (0.75 + pressure);
    factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                                 the_sun.getAge() / 2e9 * factors.pressure_factor);
  }
  factors.retention_fraction = 1.0;
  return factors;
}

/**
 * @brief Calculate gas retention factors for CO2
 *
 * CO2 accumulates through volcanic outgassing and weathering cycles
 */
static auto calculate_co2_retention(Chemical& the_gas, sun& the_sun,
                                   planet* the_planet, long double base_abundance,
                                   long double pressure) -> GasRetentionFactors {
  GasRetentionFactors factors;
  factors.abundance = base_abundance;

  // CO2 accumulation enhanced on mature, warm planets
  if (the_sun.getAge() > 2e9 &&
      (the_planet->getGasGiant() ||
       (the_planet->getSurfTemp() > 270 && the_planet->getSurfTemp() < 400))) {
    factors.pressure_factor = (0.75 + pressure);
    factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                                 std::pow(the_sun.getAge() / 2e9, 0.5) * factors.pressure_factor);
    factors.reactivity *= 1.5;  // Enhanced CO2 retention
  } else {
    factors.pressure_factor = (0.75 + pressure);
    factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                                 the_sun.getAge() / 2e9 * factors.pressure_factor);
  }
  factors.retention_fraction = 1.0;
  return factors;
}

/**
 * @brief Calculate generic gas retention factors
 *
 * Used for gases without specific chemical behavior models
 */
static auto calculate_generic_retention(Chemical& the_gas, sun& the_sun,
                                       long double base_abundance,
                                       long double pressure) -> GasRetentionFactors {
  GasRetentionFactors factors;
  factors.abundance = base_abundance;
  factors.pressure_factor = (0.75 + pressure);
  factors.reactivity = std::pow(1.0 / (1.0 + the_gas.getReactivity()),
                               the_sun.getAge() / 2e9 * factors.pressure_factor);
  factors.retention_fraction = 1.0;
  return factors;
}

/**
 * @brief Calculate final gas amount based on retention factors
 *
 * Combines abundance, velocity retention, reactivity, and molecular weight fraction
 */
static auto calculate_gas_amount(Chemical& the_gas, planet* the_planet,
                                sun& the_sun, const GasRetentionFactors& factors,
                                long double molecular_weight_retention) -> long double {
  long double vrms = rms_vel(the_gas.getWeight(), the_planet->getExosphericTemp());
  long double pvrms = std::pow(1.0 / (1.0 + vrms / the_planet->getEscVelocity()),
                               the_sun.getAge() / 1e9);
  long double fract = 1.0 - (the_planet->getMolecWeight() / the_gas.getWeight());

  return factors.abundance * pvrms * factors.reactivity * fract;
}

/**
 * @brief Adjust gas amount to meet IPP (Inspired Partial Pressure) constraints
 *
 * Iteratively adjusts gas amount to ensure it falls within min/max IPP range.
 * - If IPP too high: reduces amount by 1% per iteration
 * - If IPP too low: increases amount by 1% per iteration
 *
 * @param the_gas The gas being adjusted
 * @param the_planet The planet whose atmosphere is being adjusted
 * @param gas_amount Reference to the gas amount (will be modified)
 * @param total_amount Reference to total gas amount (will be modified)
 */
static void adjust_gas_ipp_to_range(Chemical& the_gas, planet* the_planet,
                                    long double& gas_amount, long double& total_amount) {
  long double pressure = the_planet->getSurfPressure() * (gas_amount / total_amount);
  long double ipp = inspired_partial_pressure(the_planet->getSurfPressure(), pressure);

  int loops = 0;
  if (ipp > the_gas.getMaxIpp()) {
    // Gas level too high - reduce iteratively
    while (ipp > the_gas.getMaxIpp() && loops++ < 10000) {
      long double old_amount = gas_amount;
      gas_amount *= 0.99;  // Reduce by 1%
      total_amount -= old_amount;
      total_amount += gas_amount;
      ipp = inspired_partial_pressure(the_planet->getSurfPressure(),
                                      the_planet->getSurfPressure() * (gas_amount / total_amount));
    }
  } else if (ipp < the_gas.getMinIpp()) {
    // Gas level too low - increase iteratively
    while (ipp < the_gas.getMinIpp() && loops++ < 10000) {
      long double old_amount = gas_amount;
      if (gas_amount <= 0.0) {
        gas_amount = 1.0E-9;  // Start with tiny amount if zero
      } else {
        gas_amount *= 1.01;  // Increase by 1%
        total_amount -= old_amount;
      }
      total_amount += gas_amount;
      ipp = inspired_partial_pressure(the_planet->getSurfPressure(),
                                      the_planet->getSurfPressure() * (gas_amount / total_amount));
    }
  }
}

/**
 * @brief Adjust all gases for potentially habitable planets to meet IPP constraints
 *
 * For potentially habitable planets with suitable pressure, iteratively adjusts
 * gas amounts to ensure breathable atmosphere within safe IPP ranges.
 *
 * This implements a constraint satisfaction algorithm:
 * 1. For each gas, adjust amount to meet min/max IPP constraints
 * 2. Validate all gases meet constraints
 * 3. Repeat until all constraints satisfied or max iterations reached
 *
 * @param the_planet The planet being evaluated
 * @param amounts Array of gas amounts (will be modified)
 * @param total_amount Reference to total gas amount (will be modified)
 * @param planet_id Planet identifier for debugging
 */
static void adjust_gases_for_habitability(planet* the_planet, long double* amounts,
                                          long double& total_amount,
                                          const std::string& planet_id) {
  if (!is_potentialy_habitable(the_planet)) {
    return;  // Skip non-habitable planets
  }

  if (the_planet->getSurfPressure() < (1.2 * MIN_O2_IPP) ||
      the_planet->getSurfPressure() > MAX_HABITABLE_PRESSURE) {
    return;  // Pressure outside habitable range
  }

  std::map<int, long double> new_values;
  std::map<int, bool> do_overs_more, do_overs_less;
  bool bad_air = false;
  int counter = 0;
  const int MAX_ITERATIONS = 1000;

  do {
    // First pass: adjust each gas to meet IPP constraints
    for (int i = 0; i < gases.count(); i++) {
      new_values[i] = 0;
      adjust_gas_ipp_to_range(gases[i], the_planet, amounts[i], total_amount);
    }

    // Second pass: validate all gases meet constraints
    bad_air = false;
    for (int i = 0; i < gases.count(); i++) {
      if (new_values[i] > 0.0) {
        amounts[i] = new_values[i] * total_amount;
      }

      long double ipp = inspired_partial_pressure(
          the_planet->getSurfPressure(),
          the_planet->getSurfPressure() * amounts[i] / total_amount);

      if (ipp > gases[i].getMaxIpp()) {
        bad_air = true;
        do_overs_less[i] = true;
        do_overs_more[i] = false;
        break;
      } else if (ipp < gases[i].getMinIpp() && gases[i].getNum() == AN_O) {
        // Oxygen is critical - must meet minimum
        bad_air = true;
        do_overs_less[i] = false;
        do_overs_more[i] = true;
        break;
      } else {
        do_overs_less[i] = false;
        do_overs_more[i] = false;
      }
    }

    counter++;
    if (counter > MAX_ITERATIONS) {
      break;  // Prevent infinite loops
    }
  } while (bad_air);
}

void calculate_gases(sun &the_sun, planet *the_planet, std::string planet_id) {
  the_sun = the_planet->getTheSun();
  if ((the_planet->getSurfPressure() > 0 || the_planet->getGasGiant()) &&
      the_sun.getAge() > 0) {
    // std::cout << "test 1\n";
    // std::cout << planet_id << std::endl;
    long double *amount = nullptr;
    long double totalamount = 0;
    long double pressure = the_planet->getSurfPressure() / MILLIBARS_PER_BAR;
    amount = new long double[gases.count()];
    int n = 0;
    // std::vector<gas> temp_vector; // just incase for what ever reason, it doesn't
    // replace the gases in the atmosphere

    if (the_planet->getNumGases() > 0) {
      the_planet->clearGases();
    }

    if (is_gas_planet(the_planet)) {
      the_planet->setSurfPressure(10.0 * EARTH_SURF_PRES_IN_MILLIBARS);
      iterate_surface_temp(the_planet, true);
      pressure = the_planet->getSurfPressure() / MILLIBARS_PER_BAR;
    }

    for (int i = 0; i < gases.count(); i++) {
      long double yp =
          gases[i].getBoil() /
          (373. * ((log((pressure) + 0.001) / -5050.5) + (1.0 / 373.0)));
      // std::cout << planet_id << std::endl;
      // std::cout << gases[i].getName() << " boiling point: " << yp << std::endl;
      if (((yp >= 0 && yp < the_planet->getLowTemp()) ||
           is_gas_planet(the_planet)) &&
          (gases[i].getWeight() >= the_planet->getMolecWeight())) {

        // Calculate gas-specific retention factors
        GasRetentionFactors factors;
        long double base_abundance = gases[i].getAbunds();

        if (gases[i].getSymbol() == "Ar") {
          factors = calculate_argon_retention(gases[i], the_sun, base_abundance);
        } else if (gases[i].getSymbol() == "He") {
          factors = calculate_helium_retention(gases[i], the_sun, the_planet,
                                              base_abundance, pressure);
        } else if (gases[i].getSymbol() == "O" || gases[i].getSymbol() == "O2") {
          factors = calculate_oxygen_retention(gases[i], the_sun, the_planet,
                                              base_abundance, pressure);
        } else if (gases[i].getSymbol() == "CO2") {
          factors = calculate_co2_retention(gases[i], the_sun, the_planet,
                                           base_abundance, pressure);
        } else {
          factors = calculate_generic_retention(gases[i], the_sun,
                                               base_abundance, pressure);
        }

        // Calculate final gas amount using retention factors
        amount[i] = calculate_gas_amount(gases[i], the_planet, the_sun, factors, 1.0);

        // Debug output for verbose mode
        if (flag_verbose & 0x4000 &&
            (gases[i].getSymbol() == "O" || gases[i].getSymbol() == "N" ||
             gases[i].getSymbol() == "Ar" || gases[i].getSymbol() == "He" ||
             gases[i].getSymbol() == "CO2")) {
          long double vrms = rms_vel(gases[i].getWeight(), the_planet->getExosphericTemp());
          long double pvrms = std::pow(1.0 / (1.0 + vrms / the_planet->getEscVelocity()),
                                       the_sun.getAge() / 1e9);
          long double fract = 1.0 - (the_planet->getMolecWeight() / gases[i].getWeight());

          std::cerr << toString(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)
               << " " << gases[i].getSymbol() << ", " << toString(amount[i])
               << " = a " << toString(factors.abundance) << " * p " << toString(pvrms)
               << " * r " << toString(factors.reactivity) << " * p2 "
               << toString(factors.pressure_factor)
               << " * f " << toString(fract) << "\t("
               << toString(100.0 *
                           (the_planet->getGasMass() / the_planet->getMass()))
               << "%)\n";
        }

        totalamount += amount[i];
        if (amount[i] > 0.0) {
          n++;
        } else {
          /*std::cout << planet_id << std::endl;
          std::cout << "No " << gases[i].getName() << std::endl;
          std::cout << "abund: " << abund << std::endl;
          std::cout << "pvrms: " << pvrms << std::endl;
          std::cout << "react: " << react << std::endl;
          std::cout << "fract: " << fract << std::endl;*/
        }
      } else {
        amount[i] = 0.0;
      }
    }

    // Adjust gas amounts for potentially habitable planets to meet IPP constraints
    if (n > 0) {
      // Generate planet ID for debugging if not provided
      if (planet_id == "" && is_potentialy_habitable(the_planet)) {
        std::stringstream ss;
        ss << toString(the_planet->getA()) << " " << toString(the_planet->getMoonA());
        planet_id = ss.str();
      }

      // Adjust gases to meet habitability constraints
      adjust_gases_for_habitability(the_planet, amount, totalamount, planet_id);

      // Assign final gas amounts to planet atmosphere
      for (int i = 0, n = 0; i < gases.count(); i++) {
        if (amount[i] > 0.0) {
          gas substance;
          substance.setNum(gases[i].getNum());
          substance.setSurfPressure(the_planet->getSurfPressure() *
                                    amount[i] / totalamount);
          the_planet->addGas(substance);

          // Check for oxygen poisoning in verbose mode
          if (flag_verbose & 0x2000) {
            if ((the_planet->getGas(n).getNum() == AN_O) &&
                inspired_partial_pressure(
                    the_planet->getSurfPressure(),
                    the_planet->getGas(n).getSurfPressure()) >
                    gases[i].getMaxIpp()) {
              std::cerr << planet_id << "\t Poisoned by O2\n";
            }
          }
          n++;
        }
      }
    }
    delete[] amount;

    // Validate atmosphere
    if (the_planet->getNumGases() && (the_planet->getSurfPressure() /
                                      EARTH_SURF_PRES_IN_MILLIBARS) > 0.001) {
      // Atmosphere successfully generated
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Snow line (water-ice condensation distance), luminosity-scaled. The 2.7 AU */
/* Solar-System value scales as sqrt(L) so the grain temperature at the snow  */
/* line is ~170 K independent of L. (Hayashi 1981; canonical 2.7 AU @ 1 Lsun.)*/
/*--------------------------------------------------------------------------*/
auto snow_line_au(long double luminosity) -> long double {
  long double lum = luminosity > 0.0 ? luminosity : 1.0;
  return SNOW_LINE_AU_AT_1LSUN * std::sqrt(lum);
}

/*--------------------------------------------------------------------------*/
/* Disk midplane grain (condensation) temperature proxy at distance a: the    */
/* zero-albedo radiative-equilibrium temperature. NOT the planet's later      */
/* surface/equilibrium temperature.                                          */
/*--------------------------------------------------------------------------*/
auto disk_condensation_temp(long double luminosity, long double a) -> long double {
  long double lum = luminosity > 0.0 ? luminosity : 1.0;
  return equilibrium_temp(lum, a, 0.0);
}

/*--------------------------------------------------------------------------*/
/* Bulk composition from the condensation sequence (replaces the old random  */
/* zone-binned draw). Inside the snow line no water ice condenses; beyond it  */
/* the ice mass fraction rises with distance (colder), capped at             */
/* ICE_MAX_FRACTION. The refractory budget is split into iron and rock by the */
/* solar Fe ratio (~0.33; Lodders 2003), with the hot inner disk iron-        */
/* enriched as silicates fail to fully condense (Wang 2019 devolatilization). */
/* Solar relative abundances are assumed (the star carries no Fe/Mg/Si).      */
/* Modest per-body scatter keeps diversity. Exactly the (imf, rmf, cmf)       */
/* triplet the radius code consumes (iron implicit = 1 - imf - rmf). RNG is   */
/* drawn only through the existing per-body guards, in fixed order ->         */
/* determinism-safe; the old data-dependent redraw loop is gone.             */
/*--------------------------------------------------------------------------*/
void assign_composition(planet *the_planet, sun &the_sun, bool is_moon) {
  (void)is_moon;  // composition follows the body's heliocentric distance
  long double lum = the_sun.getLuminosity();
  long double a = the_planet->getA();
  long double a_snow = snow_line_au(lum);
  long double t_cond = disk_condensation_temp(lum, a);

  if (the_planet->getImf() == 0 && the_planet->getRmf() == 0) {
    long double ice_scatter = random_number(0.0, 1.0);
    long double iron_scatter = about(1.0, COMPOSITION_SCATTER);

    // Ice only beyond the snow line; fraction grows with how far beyond (colder).
    long double f_ice = 0.0;
    if (a > a_snow) {
      long double cold = 1.0 - (a_snow / a);  // 0 at the snow line -> 1 far out
      f_ice = ICE_MAX_FRACTION * cold * (0.7 + 0.6 * ice_scatter);
      if (f_ice < 0.0) {
        f_ice = 0.0;
      }
      if (f_ice > ICE_MAX_FRACTION) {
        f_ice = ICE_MAX_FRACTION;
      }
    }

    long double iron_frac = IRON_REFRACTORY_FRACTION * iron_scatter;
    if (t_cond > T_SILICATE_CONDENSE) {  // hot inner disk: iron-enriched residue
      long double devol = (t_cond - T_SILICATE_CONDENSE) / 500.0;
      if (devol > 1.0) {
        devol = 1.0;
      }
      iron_frac = iron_frac + (0.70 - iron_frac) * devol;
    }
    if (iron_frac < 0.05) {
      iron_frac = 0.05;
    }
    if (iron_frac > 0.95) {
      iron_frac = 0.95;
    }

    long double refractory = 1.0 - f_ice;
    the_planet->setImf(f_ice);
    the_planet->setRmf(refractory * (1.0 - iron_frac));  // iron implicit = refractory*iron_frac
  }

  if (the_planet->getCmf() == 0) {
    long double cmf = about(SOLAR_CMF, COMPOSITION_SCATTER);  // carbon-of-rock, small/solar
    if (cmf < 0.0) {
      cmf = 0.0;
    }
    if (cmf > 0.2) {
      cmf = 0.2;
    }
    the_planet->setCmf(cmf);
  }

  // Deterministic clamp (replaces the old data-dependent RNG redraw loop).
  if ((the_planet->getImf() + the_planet->getRmf()) > 1.0) {
    the_planet->setRmf(1.0 - the_planet->getImf());
  }
}

auto is_gas_planet(planet *the_planet) -> bool {
  if (the_planet->getGasMass() == 0) {
    return false;
  }
  if (the_planet->getType() == tSubSubGasGiant ||
      the_planet->getType() == tSubGasGiant ||
      the_planet->getType() == tGasGiant ||
      the_planet->getType() == tBrownDwarf) {
    return true;
  }
  return false;
}

auto is_earth_like_size(planet *the_planet) -> bool {
  if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.5 &&
       (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 2.0) ||
      ((convert_km_to_eu(the_planet->getRadius())) >= 0.8 &&
       (convert_km_to_eu(the_planet->getRadius())) <= 1.25)) {
    return true;
  }
  return false;
}

bool is_earth_like(planet* the_planet) {
    // Early checks for basic habitability and size
    if (!is_habitable(the_planet) || !is_earth_like_size(the_planet) || the_planet->getEsi() < 0.8) {
        return false;
    }

    // Calculate derived values
    long double rel_temp = (the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER) - EARTH_AVERAGE_CELSIUS;
    long double seas = the_planet->getHydrosphere() * 100.0;
    long double clouds = the_planet->getCloudCover() * 100.0;
    long double pressure = the_planet->getSurfPressure() / EARTH_SURF_PRES_IN_MILLIBARS;
    long double ice = the_planet->getIceCover() * 100.0;
    long double gravity = the_planet->getSurfGrav();
    long double iron = (1.0 - (the_planet->getRmf() + the_planet->getImf())) * 100.0;

    // Check various conditions
    if (the_planet->getImf() > 0.0 ||
        iron < 20 || iron > 60 ||
        gravity < 0.8 || gravity > 1.2 ||
        rel_temp < -2.0 || rel_temp > 3.0 ||
        seas < 50.0 || seas > 80.0 ||
        ice > 10 ||
        clouds < 40.0 || clouds > 80.0 ||
        pressure < 0.5 || pressure > 2.0 ||
        the_planet->getType() == tWater ||
        the_planet->getType() == tOil ||
        the_planet->getMaxTemp() >= the_planet->getBoilPoint()) {
        return false;
    }

    return true;
}

auto is_habitable_jovian_conservative(planet *the_planet) -> bool {
  if (!is_habitable_jovian(the_planet)) {
    return false;
  } else if (the_planet->getA() <
                 habitable_zone_distance(
                     the_sun_clone, MOIST_GREENHOUSE,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) ||
             the_planet->getA() >
                 habitable_zone_distance(
                     the_sun_clone, MAXIMUM_GREENHOUSE,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) {
    return false;
  }
  return true;
}

auto is_habitable_jovian(planet *the_planet) -> bool {
  sun the_sun = the_planet->getTheSun();

  // if (is_gas_planet(the_planet) && the_planet->getEstimatedTerrTemp() >=
  // FREEZING_POINT_OF_WATER && the_planet->getEstimatedTerrTemp() <=
  // (EARTH_AVERAGE_KELVIN + 10.0)/* && the_sun.getAge() > 2.0E9*/)
  if (!is_gas_planet(the_planet)) {
    return false;
  } else if (fabs(the_planet->getHzd()) > 1.0) {
    return false;
  }
  return true;
}

auto is_terrestrial(planet *the_planet) -> bool {
  if (the_planet->getType() == tTerrestrial ||
      the_planet->getType() == tWater ||
      ((the_planet->getType() == t1Face || the_planet->getType() == tIce ||
        the_planet->getType() == tMartian ||
        the_planet->getType() == tVenusian) &&
       fabs(the_planet->getHzd()) < 1.0) ||
      breathability(the_planet) == BREATHABLE ||
      (!is_gas_planet(the_planet) && the_planet->getSurfPressure() > 0.001 &&
       the_planet->getSurfTemp() > (FREEZING_POINT_OF_WATER - 15.0) &&
       the_planet->getSurfTemp() < (EARTH_AVERAGE_KELVIN + 15.0))) {
    return true;
  }
  return false;
}

auto calcOblateness(planet *the_planet) -> long double {
  long double multiplier = 0;
  long double oblateness = 0;
  long double mass_in_eu = 0;
  long double planetary_mass_in_grams = 0;
  long double equatorial_radius_in_cm = 0;
  long double k2 = 0;
  long double ang_velocity = 0;
  if (is_gas_planet(the_planet)) {
    if (the_planet->getDay() == 0) {
      // Day length not yet calculated - return 0 oblateness as placeholder
      return 0.0;
    }
    mass_in_eu = the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES;
    if (the_planet->getType() == tSubGasGiant ||
        the_planet->getType() == tSubSubGasGiant) {
      multiplier = 4.94E-12;
    } else if (the_planet->getType() == tGasGiant) {
      multiplier = 5.56E-12;
    }
    oblateness = multiplier * (pow(the_planet->getRadius(), 3.0) /
                               (mass_in_eu * std::pow(the_planet->getDay(), 2.0)));
  } else {
    planetary_mass_in_grams = the_planet->getMass() * SOLAR_MASS_IN_GRAMS;
    equatorial_radius_in_cm = the_planet->getRadius() * CM_PER_KM;
    k2 = calculate_moment_of_inertia_coeffient(the_planet);
    if (the_planet->getDay() == 0) {
      // Day length not yet calculated - return 0 oblateness as placeholder
      // This can happen if calcOblateness() is called before day length is set
      return 0.0;
    }
    ang_velocity =
        RADIANS_PER_ROTATION / (SECONDS_PER_HOUR * the_planet->getDay());
    oblateness = calcOblateness_improved(ang_velocity, planetary_mass_in_grams,
                                         equatorial_radius_in_cm, k2);
  }
  if (oblateness > 0.5) {
    oblateness = 0.5;
  }
  return oblateness;
}

auto calcPhlPressure(planet *the_planet) -> long double {
  return (100 * 0.87 * pow2(the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) /
         (87.0 * pow3(convert_km_to_eu(the_planet->getRadius())));
}

auto is_habitable_optimistic(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_optimistic(the_planet)) {
    return false;
  } else if (breathability(the_planet) != BREATHABLE) {
    return false;
  }
  return true;
}

auto calcHzd(planet *the_planet) -> long double {
  sun the_sun = the_planet->getTheSun();
  long double inner_edge = habitable_zone_distance(
      the_sun, RECENT_VENUS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
  long double outer_edge = habitable_zone_distance(
      the_sun, EARLY_MARS, the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES);
  return ((2.0 * the_planet->getA()) - outer_edge - inner_edge) /
         (outer_edge - inner_edge);
}

auto calcHzcHelper(long double m1, long double r1, long double k1,
                          long double k2, long double k3, long double m) -> long double {
  return r1 * std::pow(10.0, k1 + (log10(m / m1) / 3.0) - (k2 * std::pow(m / m1, k3)));
}

auto calcHzc(planet *the_planet) -> long double {
  // long double ro = calcHzcHelper(5.52, 4.43, -0.209396, 0.0807, 0.375,
  // the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES); long double ri =
  // calcHzcHelper(5.80, 2.52, -0.209490, 0.0804, 0.394, the_planet->getMass() *
  // SUN_MASS_IN_EARTH_MASSES);
  long double r = convert_km_to_eu(the_planet->getRadius());
  long double ro =
      convert_km_to_eu(radius_improved(the_planet->getMass(), 1.0, 0, 0, false,
                                       the_planet->getOrbitZone(), the_planet));
  long double ri =
      convert_km_to_eu(radius_improved(the_planet->getMass(), 0.0, 0, 0, false,
                                       the_planet->getOrbitZone(), the_planet));
  return ((2.0 * r) - ro - ri) / (ro - ri);
}

auto calcV(long double t, long double m) -> long double { return sqrt((2E-2 * t) / m); }

auto calcHza(planet *the_planet) -> long double {
  long double veh = calcV(the_planet->getEstimatedTerrTemp(), 1.0);
  long double ven = calcV(the_planet->getEstimatedTerrTemp(), 14.0);
  return ((2.0 * sqrt((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) /
                      (the_planet->getRadius() / KM_EARTH_RADIUS))) -
          veh - ven) /
         (veh - ven);
}

auto calcEsiHelper(long double value, long double ref_value,
                          long double weight, long double n) -> long double {
  return std::pow(1.0 - fabs((value - ref_value) / (value + ref_value)), weight / n);
}

auto calcEsiComponents(planet *the_planet) -> EsiComponents {
  return {
      calcEsiHelper(convert_km_to_eu(the_planet->getRadius()), 1.0, 0.57),
      calcEsiHelper(the_planet->getDensity() / EARTH_DENSITY, 1.0, 1.07),
      calcEsiHelper((the_planet->getEscVelocity() / CM_PER_KM) / 11.186, 1.0, 0.70),
      calcEsiHelper(the_planet->getSurfTemp(), EARTH_AVERAGE_KELVIN, 5.58)};
}

auto calcEsi(planet *the_planet) -> long double {
  EsiComponents c = calcEsiComponents(the_planet);
  return c.r * c.d * c.v * c.t;
}

auto calcSphHelper(long double min, long double max, long double opt,
                          long double x, long double w) -> long double {
  if (x > min && x < max) {
    return std::pow(fabs(((x - min) * (x - max)) /
                    (((x - min) * (x - max)) - std::pow(x - opt, 2.0))),
               w);
  } else {
    return 0;
  }
}

auto calcSph(planet *the_planet) -> long double {
  long double ht =
      calcSphHelper(-8.3, 39.0, 24.1,
                    the_planet->getSurfTemp() - FREEZING_POINT_OF_WATER, 3.0);
  // long double hrh = 1.0;
  long double hrh =
      calcSphHelper(9.2, 100.0, 77.5, calcRelHumidity(the_planet), 2.0);
  return ht * hrh;
}

/**
 * @brief Set derived habitability caveat flags on the planet.
 *
 * "In the habitable zone" is necessary but not sufficient, especially around M
 * dwarfs. These flags surface well-known caveats computed from already-derived
 * quantities (host effective temperature/mass, orbital distance, the HZ distance
 * metric, surface gravity, rotation state):
 *  - tidally_locked: synchronous or spin-orbit-resonant rotation.
 *  - pms_desiccation_risk: an M-dwarf HZ terrestrial likely spent the host's
 *    super-luminous pre-main-sequence phase in a runaway greenhouse, losing its
 *    water and possibly building abiotic O2 (a "mirage Earth"); Luger & Barnes
 *    2015, Astrobiology 15, 119.
 *  - high_xuv_escape_risk: a close-in M-dwarf terrestrial at elevated atmospheric
 *    escape risk from quiescent XUV and flares.
 * See research/modern/06-habitable-zone-climate.md.
 */
void set_habitability_flags(planet *the_planet) {
  sun host = the_planet->getTheSun();
  long double host_mass = host.getMass();
  // M dwarf: M_star < ~0.6 Msun. Keyed on mass, which is reliably populated; the
  // host effective temperature is not always derived on the -m generation path
  // (it can default to a placeholder), so it is not used here.
  bool m_dwarf = host_mass > 0.0 && host_mass < 0.6;

  long double year_hours = the_planet->getOrbPeriod() * 24.0;
  bool locked = the_planet->getResonantPeriod() ||
                (year_hours > 0.0 && the_planet->getDay() >= year_hours * 0.9999);
  the_planet->setTidallyLocked(locked);

  bool rocky = !is_gas_planet(the_planet);
  bool in_hz = std::fabs((double)the_planet->getHzd()) <= 1.0;

  the_planet->setPmsDesiccationRisk(m_dwarf && rocky && in_hz);
  the_planet->setHighXuvEscapeRisk(m_dwarf && rocky && the_planet->getA() < 0.4 &&
                                   the_planet->getSurfGrav() < 1.0);

  // On a cold HZ world, outgassed CO2 can condense out before the carbonate-
  // silicate thermostat warms the planet, risking irreversible glaciation
  // (Turbet et al. 2017, EPSL 476, 11). Flag rocky HZ planets currently below
  // the water freezing point.
  the_planet->setCo2CollapseRisk(
      rocky && in_hz && the_planet->getSurfTemp() < FREEZING_POINT_OF_WATER);
}

/**
 * @brief Closed-form 1-D-climate surface temperature for an Earth-like CO2-H2O
 * atmosphere, from Lehmer, Catling & Krissansen-Totton 2020 (Nature Comms 11,
 * 6153; arXiv:2012.00819, eq. 8). A 4th-order bivariate polynomial in
 * X = ln(pCO2[bar]) and Y = S/S_earth, fit to the VPL 1-D radiative-convective
 * model (max error ~3%). Returns the full surface temperature in K (greenhouse
 * and Bond albedo are implicit). Caller must ensure the validity domain:
 * S in [0.35, 1.05] S_earth, pCO2 in [1e-6, 10] bar.
 * See research/modern/10-greenhouse-Ts-polynomial.md.
 *
 * @param instellation incident flux S normalized to the solar constant (S/S_earth)
 * @param pco2_bar CO2 partial pressure in bar
 * @return surface temperature in Kelvin
 */
auto lehmer_surface_temp(long double instellation, long double pco2_bar) -> long double {
  // c[i][j] multiplies X^i * Y^j (i = power of ln(pCO2), j = power of S/S_earth).
  static const long double c[5][5] = {
      {  4.809L, 1045.0L, -1496.0L, 1064.0L, -281.1L},
      {-222.0L,  1414.0L, -2964.0L, 2655.0L, -868.4L},
      {-68.44L,   446.4L,  -978.4L,  907.5L, -304.6L},
      { -6.737L,   44.41L,  -98.86L,   92.87L, -31.48L},
      { -0.206L,    1.364L,   -3.059L,   2.892L,  -0.985L},
  };
  const long double X = std::log(pco2_bar);  // natural log; pCO2 in bar
  const long double Y = instellation;
  long double t = 0.0L;
  long double xi = 1.0L;  // X^i
  for (const auto& row : c) {
    long double yj = 1.0L;  // Y^j
    for (long double cij : row) {
      t += cij * xi * yj;
      yj *= Y;
    }
    xi *= X;
  }
  return t;
}

/**
 * @brief Is the Lehmer 2020 1-D-climate polynomial applicable to this planet,
 * and if so return its inputs? Rocky planets only, with instellation
 * S = L/a^2 in [0.35, 1.05] S_earth and CO2 partial pressure in [1e-6, 10] bar.
 *
 * Single source of truth for the validity domain, shared by the diagnostic
 * setter (set_climate_model_temp) and the driver (apply_lehmer_surface_temp).
 */
static auto lehmer_applicable(planet *the_planet, long double &s_out,
                              long double &pco2_bar_out) -> bool {
  if (is_gas_planet(the_planet)) {
    return false;
  }
  long double a = the_planet->getA();
  if (a <= 0.0) {
    return false;
  }
  long double instellation = the_planet->getTheSun().getLuminosity() / (a * a);
  if (instellation < 0.35 || instellation > 1.05) {
    return false;
  }
  long double pco2_mb = 0.0;  // CO2 partial pressure, millibars
  for (int i = 0; i < the_planet->getNumGases(); i++) {
    if (the_planet->getGas(i).getNum() == AN_CO2) {
      pco2_mb = the_planet->getGas(i).getSurfPressure();
      break;
    }
  }
  long double pco2_bar = pco2_mb / MILLIBARS_PER_BAR;
  if (pco2_bar < 1.0e-6 || pco2_bar > 10.0) {
    return false;
  }
  s_out = instellation;
  pco2_bar_out = pco2_bar;
  return true;
}

/**
 * @brief Set the diagnostic Lehmer 2020 1-D-climate surface temperature on the
 * planet, or 0 if it is outside the model's validity domain. (Diagnostic field
 * read by the display; the *actual* surf_temp is driven by
 * apply_lehmer_surface_temp.)
 */
void set_climate_model_temp(planet *the_planet) {
  long double s = NAN;
  long double pco2_bar = NAN;
  the_planet->setClimateModelTemp(
      lehmer_applicable(the_planet, s, pco2_bar) ? lehmer_surface_temp(s, pco2_bar) : 0.0);
}

/**
 * @brief Promote the Lehmer 2020 1-D radiative-convective climate polynomial to
 * DRIVE the surface temperature of rocky planets inside its validity domain,
 * replacing the hand-tuned Fogg 1985 grey-opacity result with a GCM-anchored,
 * CO2-aware temperature. Outside the domain (and for gas planets) the Fogg
 * result computed by iterate_surface_temp is left untouched (the fallback).
 *
 * Minimal and safe: it overrides only surf_temp (and the reported greenhouse
 * rise + min/max temp range), then lets the caller re-run the habitability
 * classifiers on the new temperature. It deliberately does NOT re-derive the
 * hydrosphere/cloud/ice fractions: those were already iterated to convergence by
 * the Fogg energy balance, and re-deriving them here risks re-establishing an
 * ocean on a planet that boiled off. The fractions therefore remain those of the
 * Fogg solution while the reported temperature is the (more accurate) Lehmer one.
 */
void apply_lehmer_surface_temp(planet *the_planet) {
  long double s = NAN;
  long double pco2_bar = NAN;
  if (!lehmer_applicable(the_planet, s, pco2_bar)) {
    return;  // out of domain: keep the Fogg surface temperature
  }
  long double ts = lehmer_surface_temp(s, pco2_bar);
  // Greenhouse rise reported relative to the no-greenhouse equilibrium baseline,
  // matching how iterate_surface_temp defines greenhsRise.
  long double baseline = est_temp(the_planet->getTheSun().getREcosphere(
                                      the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES),
                                  the_planet->getA(), the_planet->getAlbedo());
  the_planet->setSurfTemp(ts);
  the_planet->setGreenhsRise(ts - baseline);
  set_temp_range(the_planet);
}

auto is_potentialy_habitable_optimistic_size(planet *the_planet) -> bool {
  /*if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 10.0 ||
   (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) < 0.1) // The Plantary
   Habitablity Laboratory believes habitable planets have to be in the range of
   0.1 to 10 earth masses in size.
   *  {
   *    return false;
   }
   else if ((the_planet->radius / KM_EARTH_RADIUS) > 2.5 || (the_planet->radius
   / KM_EARTH_RADIUS) < 0.5)  // The Plantary Habitablity Laboratory believes
   habitable planets have to be in the range of 0.5 to 2.5 earth radii in size.
   {
     return false;
   }*/
  if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.1 &&
       (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 10.0) ||
      ((convert_km_to_eu(the_planet->getRadius())) >= 0.5 &&
       (convert_km_to_eu(the_planet->getRadius())) <= 2.5)) {
    return true;
  }
  return false;
}

auto is_potentialy_habitable_optimistic(planet *the_planet) -> bool {
  sun the_sun = the_planet->getTheSun();
  std::string spec_type = the_sun.getSpecType();
  if (compare_string_char(spec_type, 1, "?")) {
    the_planet->setTheSun(the_sun_clone);
    the_sun = the_planet->getTheSun();
    spec_type = the_sun.getSpecType();
  }
  std::string star_type = getStarType(spec_type);
  if (the_sun.getMass() <
      0.3)  // the Plantary Habitablity Laboratory feels that stars less that it
            // is very unlikey 0.3 solar masses due to the tidal heating planets
            // would expirence before their orbits stablize.
  {
    return false;
  } else if (star_type == "O")  // Types O, B, and A don't live long enough to
                                // produce habitable worlds and are two bright
                                // for photosysisis to occure. (Deleted B & A
								// to encourage science fantasy planets. ;) )
								// ~ PlutonianEmpire
  {
    return false;
  }
  // else if ((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) > 10.0 ||
  // (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) < 0.1) // The Plantary
  // Habitablity Laboratory believes habitable planets have to be in the range
  // of 0.1 to 10 earth masses in size.
  else if (!is_potentialy_habitable_optimistic_size(the_planet)) {
    return false;
  } else if (fabs(the_planet->getHzd()) >
             1.0)  // The planet has to be in the zone where water would in
                   // theory be a liquid.
  {
    return false;
  } else if (fabs(the_planet->getHzc()) >
             1.0)  // The planet can't have too much iron or too much water or
                   // gas making up its mass.
  {
    return false;
  } else if (fabs(the_planet->getHza()) >
             1.0)  // The planet can't have the potential of having a too thick
                   // or too thin atmosphere.
  {
    return false;
  } else if (the_planet->getEsi() <
             0.5)  // The planet must be similar enough to Earth to at least
                   // support extremeophiles.
  {
    return false;
  }
  return true;
}

auto calcRelHumidity(planet *the_planet) -> long double {
  long double surf_area = NAN, hydro_mass = NAN, staturated_mass = NAN, water_vapor_in_kg = NAN,
      max_possible_water_vapor_in_kg = NAN;
  if (the_planet->getMolecWeight() > WATER_VAPOR) {
    return 0.0;
  } else {
    surf_area = 4.0 * PI * pow2(the_planet->getRadius());
    hydro_mass =
        the_planet->getHydrosphere() * surf_area * EARTH_WATER_MASS_PER_AREA;
    staturated_mass = 1.0 * surf_area * EARTH_WATER_MASS_PER_AREA;
    water_vapor_in_kg =
        (0.00000001 * hydro_mass) *
        exp(Q2_36 * (the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN));
    max_possible_water_vapor_in_kg =
        (0.00000001 * staturated_mass) *
        exp(Q2_36 * (the_planet->getSurfTemp() - EARTH_AVERAGE_KELVIN));
    return (water_vapor_in_kg / max_possible_water_vapor_in_kg) * 100.0;
  }
}

auto getPlantLifeAlbedo(const std::string& star_type, long double luminosity) -> long double {
  int num = star_type_to_num(star_type, luminosity);

  if (num >= 31 && num <= 33) {
    return linear_trend(-0.05, 2.45, num);
  } else if (num >= 34 && num <= 37) {
    return linear_trend(-1.0 / 30.0, 29.0 / 15.0, num);
  } else if (num >= 38 && num <= 40) {
    return linear_trend(-0.1, 4.5, num);
  } else if (num >= 41 && num <= 53) {
    return linear_trend(-0.025, 1.525, num);
  } else if (num >= 54 && num <= 57) {
    return linear_trend(-1.0 / 30.0, 2.0, num);
  } else if (num >= 58 && num <= 64) {
    return linear_trend(-5.0 / 600.0, 7.0 / 12.0, num);
  } else if (num >= 65 && num <= 68) {
    return linear_trend(-1.0 / 300.0, 29.0 / 60.0, num);
  }
  return ROCKY_ALBEDO;
}

auto calcFlux(long double temperature, long double wavelength) -> long double {
  wavelength /= 1000.0;
  wavelength /= 1000000.0;
  long double first = 2.0 * (long double)H * std::pow((long double)C, 2.0);
  long double second = std::pow(wavelength, 5);
  long double third = (long double)H * (long double)C;
  long double fourth = wavelength * (long double)KB * temperature;
  long double fifth = third / fourth;
  long double sixth = exp(fifth);
  long double seventh = sixth - 1.0;
  // std::cout << toString(wavelength) << ": " << toString(first) << " " <<
  // toString(second) << " " << toString(third) << " " << toString(fourth) << "
  // " << toString(fifth) << " " << toString(sixth) << " " << toString(seventh)
  // << std::endl;
  return (first / second) * (1.0 / seventh);
}

auto calculate_moment_of_inertia_coeffient(planet *the_planet) -> long double {
  long double result = NAN;
  result = calculate_moment_of_inertia(the_planet) /
           (the_planet->getMass() * pow2(the_planet->getRadius()));
  return result;
}

auto calculate_moment_of_inertia(planet *the_planet) -> long double {
  long double result = NAN;
  result = (2.0 / 5.0) * the_planet->getMass() * pow2(the_planet->getRadius());
  return result;
}

auto calcOblateness_improved(long double angular_velocity,
                                    long double planetary_mass_in_grams,
                                    long double equatorial_radius_in_cm,
                                    long double k2) -> long double {
  long double result = NAN;
  result =
      ((pow2(angular_velocity) * pow3(equatorial_radius_in_cm)) /
       (GRAV_CONSTANT * planetary_mass_in_grams)) *
      std::pow((5.0 / 2.0) * pow2(1.0 - ((3.0 / 2.0) * k2)) + (2.0 / 5.0), -1.0);
  return result;
}

auto calcLuminosity(planet *the_planet) -> long double {
  long double star_luminosity = the_planet->getTheSun().getLuminosity();
  if (star_luminosity == 0.0) {
    the_planet->setTheSun(the_sun_clone);
    star_luminosity = the_planet->getTheSun().getLuminosity();
  }
  return pow2(1.0 / the_planet->getA()) * star_luminosity;
}

auto calcGasRadius(planet *the_planet) -> long double {
  long double lower = NAN, upper = NAN, result = NAN, density = NAN;
  if (the_planet->getCoreRadius() <= 0.0) {
    the_planet->setCoreRadius(radius_improved(
        the_planet->getDustMass(), the_planet->getImf(), the_planet->getRmf(),
        the_planet->getCmf(), the_planet->getGasGiant(),
        the_planet->getOrbitZone(), the_planet));
    ;
  }
  if (the_planet->getTheSun().getAge() == 0) {
    the_planet->setTheSun(the_sun_clone);
  }
  if (convert_su_to_eu(the_planet->getMass()) < 17.0) {
    result = mini_neptune_radius(the_planet);
  } else if (convert_su_to_eu(the_planet->getMass()) < 20.0) {
    lower = mini_neptune_radius(the_planet);
    upper = gas_radius(the_planet->getEstimatedTemp(),
                       the_planet->getDustMass(), the_planet->getMass(),
                       the_planet->getTheSun().getAge(), the_planet);
    result = rangeAdjust(convert_su_to_eu(the_planet->getMass()), lower, upper,
                         17.0, 20.0);
  } else {
    result = gas_radius(the_planet->getEstimatedTemp(),
                        the_planet->getDustMass(), the_planet->getMass(),
                        the_planet->getTheSun().getAge(), the_planet);
  }
  if (the_planet->getCoreRadius() <= 0.0) {
    the_planet->setCoreRadius(radius_improved(
        the_planet->getDustMass(), the_planet->getImf(), the_planet->getRmf(),
        the_planet->getCmf(), the_planet->getGasGiant(),
        the_planet->getOrbitZone(), the_planet));
    ;
  }
  if (result < the_planet->getCoreRadius()) {
    result = mini_neptune_radius(the_planet);
    // result = gas_dwarf_radius(the_planet);
  }
  density = volume_density(the_planet->getMass(), result);
  while ((density / EARTH_DENSITY) < 0.01) {
    result *= 0.99;
    density = volume_density(the_planet->getMass(), result);
  }
  return result;
}

auto calcSolidRadius(planet *the_planet) -> long double {
  return radius_improved(the_planet->getMass(), the_planet->getImf(),
                         the_planet->getRmf(), the_planet->getCmf(),
                         the_planet->getGasGiant(), the_planet->getOrbitZone(),
                         the_planet);
}

auto calcRadius(planet *the_planet) -> long double {
  long double result = NAN;
  if (the_planet->getCoreRadius() <= 0.0) {
    the_planet->setCoreRadius(radius_improved(
        the_planet->getDustMass(), the_planet->getImf(), the_planet->getRmf(),
        the_planet->getCmf(), the_planet->getGasGiant(),
        the_planet->getOrbitZone(), the_planet));
    ;
  }
  if (the_planet->getGasGiant() || the_planet->getGasMass() > 0.0) {
    result = calcGasRadius(the_planet);
  } else {
    result = calcSolidRadius(the_planet);
  }
  return result;
}

auto planet_radius_helper(long double planet_mass, long double mass1,
                                 long double radius1, long double mass2,
                                 long double radius2, long double mass3,
                                 long double radius3, bool use_cache) -> long double {
  // some imput validation for debuging purposes
  bool show_debug = false;

  if (mass1 != 0.0 && radius1 == 0.0) {
    std::cout << "invalid radius for " << mass1 << std::endl;
    show_debug = true;
  } else if (mass2 != 0.0 && radius2 == 0.0) {
    std::cout << "invalid radius for " << mass2 << std::endl;
    show_debug = true;
  } else if (mass3 != 0.0 && radius3 == 0.0) {
    std::cout << "invalid radius for " << mass3 << std::endl;
    show_debug = true;
  }
  if (show_debug) {
    std::cout << std::endl;
    std::cout << "Please send an email to omega13a@yahoo.com of this problem with "
            "the following debug info: "
         << std::endl;
    std::cout << "Input was:\n";
    std::cout << "planet_mass = " << planet_mass << std::endl;
    std::cout << "mass1 = " << std::to_string((long double)mass1) << std::endl;
    std::cout << "radius1 = " << std::to_string((long double)radius1) << std::endl;
    std::cout << "mass2 = " << std::to_string((long double)mass2) << std::endl;
    std::cout << "radius2 = " << std::to_string((long double)radius2) << std::endl;
    std::cout << "mass3 = " << std::to_string((long double)mass3) << std::endl;
    std::cout << "radius3 = " << std::to_string((long double)radius3) << std::endl;
    exit(EXIT_FAILURE);
  }
  long double radius = 0.0;
  long double a = 0.0;
  long double b = 0.0;
  long double c = 0.0;
  std::map<long double, long double> cache_name;
  long double coeff[3];
  std::vector<long double> coeff_cache;
  if (use_cache) {
    cache_name[mass1] = radius1;
    cache_name[mass2] = radius2;
    cache_name[mass3] = radius3;
    coeff_cache = polynomial_cache[cache_name];
    if (!coeff_cache.empty()) {
      for (int i = 0; i < 3; i++) {
        coeff[i] = coeff_cache[i];
      }
      a = coeff[0];
      b = coeff[1];
      c = coeff[2];
    } else {
      quadfix(mass1, radius1, mass2, radius2, mass3, radius3, a, b, c);
      coeff[0] = a;
      coeff[1] = b;
      coeff[2] = c;
      while (!coeff_cache.empty()) {
        coeff_cache.pop_back();
      }
      for (long double & i : coeff) {
        coeff_cache.push_back(i);
      }
      polynomial_cache[cache_name] = coeff_cache;
    }
  } else {
    quadfix(mass1, radius1, mass2, radius2, mass3, radius3, a, b, c);
  }
  radius = quad_trend(a, b, c, planet_mass);
  return radius;
}

auto planet_radius_helper2(long double planet_mass, long double mass1,
                                  long double radius1, long double mass2,
                                  long double radius2) -> long double {
  long double radius = 0.0;
  long double a = 0.0;
  long double b = 0.0;
  logfix(mass1, radius1, mass2, radius2, a, b);
  radius = ln_trend(a, b, planet_mass);
  return radius;
}

auto planet_radius_helper3(long double temperature,
                                  long double temperature1, long double radius1,
                                  long double temperature2,
                                  long double radius2) -> long double {
  long double a = 0.0;
  long double b = 0.0;
  long double radius = 0.0;
  long double adjusted_temperature = NAN;
  long double adjusted_temperature1 = temperature1 / 1000.0;
  long double adjusted_temperature2 = temperature2 / 1000.0;
  e_fix(adjusted_temperature1, radius1, adjusted_temperature2, radius2, a, b);
  radius = e_trend(a, b, temperature);
  return radius;
}

// these should be in utils.cpp but because of a some sort of class between
// const.h and the boost libary, it is here instead.
auto convert_su_to_eu(long double mass) -> long double {
  return mass * SUN_MASS_IN_EARTH_MASSES;
}

auto convert_au_to_km(long double au) -> long double { return au * KM_PER_AU; }

auto is_potentialy_habitable_conservative_size(planet *the_planet) -> bool {
  if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.5 &&
       (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 5.0) ||
      ((convert_km_to_eu(the_planet->getRadius())) >= 0.8 &&
       (convert_km_to_eu(the_planet->getRadius())) <= 1.5)) {
    return true;
  }
  return false;
}

auto is_potentialy_habitable_conservative(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_optimistic(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // potentionally habitable by optimistic\n";
    return false;
  } else if (!is_potentialy_habitable_conservative_size(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // conservative size\n";
    return false;
  } else if (the_planet->getA() <
                 habitable_zone_distance(
                     the_sun_clone, RUNAWAY_GREENHOUSE,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) ||
             the_planet->getA() >
                 habitable_zone_distance(
                     the_sun_clone, MAXIMUM_GREENHOUSE,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": wrong
    // distance for extended\n";
    return false;
  }
  return true;
}

auto is_habitable_conservative(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_conservative(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // potentionally habitable by conservative\n";
    return false;
  } else if (breathability(the_planet) != BREATHABLE) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": Not
    // breathable for conservative\n";
    return false;
  }
  return true;
}

auto is_potentialy_habitable_extended_size(planet *the_planet) -> bool {
  if (((the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) >= 0.1 &&
       (the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) <= 10.0) ||
      ((convert_km_to_eu(the_planet->getRadius())) >= 0.5 &&
       (convert_km_to_eu(the_planet->getRadius())) <= 2.5)) {
    return true;
  }
  return false;
}

auto is_potentialy_habitable_extended(planet *the_planet) -> bool {
  sun the_sun = the_planet->getTheSun();
  std::string star_type = the_sun.getSpecType();
  if (!is_potentialy_habitable_extended_size(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not extended
    // size\n";
    return false;
  } else if (the_sun.getMass() <
             0.3)  // the Plantary Habitablity Laboratory feels that stars less
                   // that it is very unlikey 0.3 solar masses due to the tidal
                   // heating planets would expirence before their orbits
                   // stablize.
  {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": too small a
    // star for extended\n";
    return false;
  } else if (star_type == "O")  // Types O, B, and A don't live long enough to
                                // produce habitable worlds and are two bright
                                // for photosysisis to occure. (Deleted B & A
								// to encourage science fantasy planets. ;) )
								// ~ PlutonianEmpire
  {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": wrong star
    // for extended\n";
    return false;
  } else if (fabs(the_planet->getHzc()) >
             1.0)  // The planet can't have too much iron or too much water or
                   // gas making up its mass.
  {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": wrong
    // composition for extended\n";
    return false;
  } else if (fabs(the_planet->getHza()) >
             1.0)  // The planet can't have the potential of having a too thick
                   // or too thin atmosphere.
  {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": potential for
    // too thick or thin an atmosphere for extended\n";
    return false;
  } else if (the_planet->getA() <
                 habitable_zone_distance(
                     the_sun_clone, RECENT_VENUS,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES) ||
             the_planet->getA() >
                 habitable_zone_distance(
                     the_sun_clone, TWO_AU_CLOUD_LIMIT,
                     the_planet->getMass() * SUN_MASS_IN_EARTH_MASSES)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": wrong
    // distance for extended\n";
    return false;
  }
  return true;
}

auto is_habitable_extended(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_extended(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // potentionally extended\n";
    return false;
  } else if (breathability(the_planet) != BREATHABLE) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": Not
    // breathable for extended\n";
    return false;
  }
  return true;
}

auto is_potentialy_habitable_earth_like(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_conservative(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // potentionally habitable by conservative\n";
    return false;
  } else if (!is_earth_like_size(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": not
    // earth-like size\n";
    return false;
  } else if (the_planet->getEsi() < 0.8) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": too low esi
    // for earth-like\n";
    return false;
  }
  return true;
}

auto is_habitable_earth_like(planet *the_planet) -> bool {
  if (!is_potentialy_habitable_earth_like(the_planet)) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": Not
    // potentialy earth-like\n";
    return false;
  } else if (breathability(the_planet) != BREATHABLE) {
    // std::cout << flag_seed << "-" << the_planet->getPlanetNo() << ": Not
    // breathable for earth-like\n";
    return false;
  }
  return true;
}

auto is_potentialy_habitable(planet *the_planet) -> bool {
  if (is_potentialy_habitable_earth_like(the_planet)) {
    return true;
  } else if (is_potentialy_habitable_conservative(the_planet)) {
    return true;
  } else if (is_potentialy_habitable_optimistic(the_planet)) {
    return true;
  } else if (is_potentialy_habitable_extended(the_planet)) {
    return true;
  }
  return false;
}

auto is_habitable(planet *the_planet) -> bool {
  if (is_habitable_earth_like(the_planet)) {
    return true;
  } else if (is_habitable_conservative(the_planet)) {
    return true;
  } else if (is_habitable_optimistic(the_planet)) {
    return true;
  } else if (is_habitable_extended(the_planet)) {
    return true;
  }
  return false;
}

auto convert_km_to_eu(long double km) -> long double { return km / KM_EARTH_RADIUS; }

void makeHabitable(StarGenerator* gen, sun &the_sun, planet *the_planet, const std::string& planet_id,
                   bool is_moon, bool do_gases) {
  // std::cout << planet_id << " " << the_planet->getA() << std::endl;
  if (!is_gas_planet(the_planet) && is_potentialy_habitable(the_planet) &&
      the_planet->getMinTemp() < the_planet->getBoilPoint() &&
      (the_planet->getSurfPressure() < (1.2 * MIN_O2_IPP) ||
       the_planet->getSurfPressure() > MAX_HABITABLE_PRESSURE)) {
    // std::cout << "test3 " << planet_id << std::endl;
    the_planet->setSurfPressure(calcPhlPressure(the_planet) *
                                EARTH_SURF_PRES_IN_MILLIBARS);
    while (the_planet->getSurfPressure() > 6000) {
      the_planet->setSurfPressure(the_planet->getSurfPressure() - 1.0);
    }
    iterate_surface_temp(the_planet, do_gases);
    if (do_gases) {
      the_planet->clearGases();
      calculate_gases(the_sun, the_planet, planet_id);
    }
    // std::cout << "test4 " << planet_id << std::endl;
    assign_type(gen, the_sun, the_planet, planet_id, is_moon, do_gases, true);
    // std::cout << "test5 " << planet_id << std::endl;
  }
}