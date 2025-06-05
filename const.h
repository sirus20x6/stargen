#ifndef CONST_H
#define CONST_H

#include <cmath>

constexpr double PI = 3.1415926536;
constexpr double RADIANS_PER_ROTATION = 2.0 * PI;

constexpr double ECCENTRICITY_COEFF = 0.077;         /* Dole's was 0.077			*/
constexpr double PROTOPLANET_MASS = 1.0E-15;         /* Units of solar masses	*/
constexpr double CHANGE_IN_EARTH_ANG_VEL = -1.3E-15; /* Units of radians/sec/year*/
//#define CHANGE_IN_EARTH_ANG_VEL (-9.38547486033519553073e-15)		/* Units
//of radians/sec/year. From http://www.ridgenet.net/~do_while/sage/v5i5d.htm*/
constexpr double SOLAR_MASS_IN_GRAMS = 1.98892E33;
constexpr double SOLAR_MASS_IN_KILOGRAMS = 1.98892E30;
constexpr double EARTH_MASS_IN_GRAMS = 5.9742E27;
constexpr double EARTH_RADIUS = 6.378137E8; /* Units of cm */
constexpr double EARTH_DENSITY = 5.515; /* Units of g/cc			*/
constexpr double KM_EARTH_RADIUS = 6378.137;
constexpr double EARTH_ACCELERATION = 980.665; /* Units of cm/sec2 */
constexpr double EARTH_AXIAL_TILT = 23.44; /* Units of degrees			*/
constexpr double EARTH_EXOSPHERE_TEMP = 1273.0; /* Units of degrees Kelvin	*/
constexpr double SUN_MASS_IN_EARTH_MASSES = 332775.64;
constexpr double ASTEROID_MASS_LIMIT = 0.001; /* Units of Earth Masses	*/
constexpr double EARTH_EFFECTIVE_TEMP = 250.0; /* Units of degrees Kelvin (was 255) */
constexpr double CLOUD_COVERAGE_FACTOR = 1.839E-8; /* Km2/kg					*/
constexpr double EARTH_WATER_MASS_PER_AREA = 3.83E15; /* grams per square km*/
constexpr double EARTH_SURF_PRES_IN_MILLIBARS = 1013.25;
constexpr double EARTH_SURF_PRES_IN_MMHG = 760.; /* Dole p. 15				*/
constexpr double EARTH_SURF_PRES_IN_PSI = 14.696;
constexpr double MMHG_TO_MILLIBARS = EARTH_SURF_PRES_IN_MILLIBARS / EARTH_SURF_PRES_IN_MMHG;
constexpr double PSI_TO_MILLIBARS = EARTH_SURF_PRES_IN_MILLIBARS / EARTH_SURF_PRES_IN_PSI;
//Dole p. 15
constexpr double H20_ASSUMED_PRESSURE = 47. * MMHG_TO_MILLIBARS;
constexpr double MIN_O2_IPP = 53. * MMHG_TO_MILLIBARS;
constexpr double MAX_O2_IPP = 400. * MMHG_TO_MILLIBARS;
//Dole p. 16
constexpr double MAX_HE_IPP = 61000. * MMHG_TO_MILLIBARS;
constexpr double MAX_NE_IPP = 3900. * MMHG_TO_MILLIBARS;
constexpr double MIN_N2_IPP = 10. * MMHG_TO_MILLIBARS;
constexpr double MAX_N2_IPP = 2330. * MMHG_TO_MILLIBARS;
constexpr double MAX_AR_IPP = 1220. * MMHG_TO_MILLIBARS;
constexpr double MAX_KR_IPP = 350. * MMHG_TO_MILLIBARS;
constexpr double MAX_XE_IPP = 160. * MMHG_TO_MILLIBARS;
constexpr double MIN_CO2_IPP = 0.05 * MMHG_TO_MILLIBARS;
constexpr double MAX_CO2_IPP = 14. * MMHG_TO_MILLIBARS;
constexpr double MAX_HABITABLE_PRESSURE = 118 * PSI_TO_MILLIBARS;
// The next gases are listed as poisonous in parts per million by volume at 1
// atm:
constexpr double PPM_PRSSURE = EARTH_SURF_PRES_IN_MILLIBARS / 1000000.;
// Dole p. 18
constexpr double MAX_F_IPP = 0.1 * PPM_PRSSURE;
constexpr double MAX_CL_IPP = 1.0 * PPM_PRSSURE;
constexpr double MAX_NH3_IPP = 100. * PPM_PRSSURE;
constexpr double MAX_O3_IPP = 0.1 * PPM_PRSSURE;
constexpr double MAX_CH4_IPP = 50000. * PPM_PRSSURE;
constexpr double EARTH_CONVECTION_FACTOR = 0.43;

constexpr double FREEZING_POINT_OF_WATER = 273.15;

constexpr double EARTH_AVERAGE_CELSIUS = 14.0;
constexpr double EARTH_AVERAGE_KELVIN = EARTH_AVERAGE_CELSIUS + FREEZING_POINT_OF_WATER;
constexpr double DAYS_IN_A_YEAR = 365.256;
//		gas_retention_threshold = 5.0;  		/* ratio of esc vel to
//RMS vel */
constexpr double GAS_RETENTION_THRESHOLD = 6.0; /* ratio of esc vel to RMS vel */

constexpr double ICE_ALBEDO = 0.7;
constexpr double CLOUD_ALBEDO = 0.52;
constexpr double GAS_GIANT_ALBEDO = 0.5; /* albedo of a gas giant	*/

// Albedo values for various celestial body classifications
constexpr double CLASS_I_ALBEDO = 0.57;
constexpr double CLASS_II_ALBEDO = 0.81;
constexpr double CLASS_III_ALBEDO = 0.12;
constexpr double CLASS_IV_ALBEDO = 0.03;
constexpr double CLASS_V_ALBEDO = 0.55;
constexpr double CARBON_GIANT_ALBEDO = 0.01;
constexpr double SULFAR_GIANT_ALBEDO = 0.63;
constexpr double METHANE_GIANT_ALBEDO = 0.3;
constexpr double AIRLESS_ICE_ALBEDO = 0.5;
constexpr double EARTH_ALBEDO = 0.3;
constexpr double GREENHOUSE_TRIGGER_ALBEDO = 0.20;
constexpr double ROCKY_ALBEDO = 0.15;
constexpr double ROCKY_AIRLESS_ALBEDO = 0.07;
constexpr double WATER_ALBEDO = 0.04;

// Temperature constants for various celestial bodies
constexpr double TEMPERATURE_NEPTUNE = 48.1;
constexpr double TEMPERATURE_URANUS = 60.3;
constexpr double TEMPERATURE_CLASS_I = 81.0;
constexpr double TEMPERATURE_SATURN = 85.1;
constexpr double TEMPERATURE_CLASS_II = 150.0;
constexpr double TEMPERATURE_SULFUR_GIANT = 320.0;
constexpr double TEMPERATURE_CLASS_III = 360.0;
constexpr double TEMPERATURE_CLASS_IV = 900.0;
constexpr double TEMPERATURE_CLASS_V = 1400.0;
constexpr double TEMPERATURE_CARBON_GIANT = 2240.0;

constexpr double SECONDS_PER_HOUR = 3600.0;
constexpr double CM_PER_AU = 1.495978707E13;
constexpr double CM_PER_KM = 1.0E5;
constexpr double KM_PER_AU = CM_PER_AU / CM_PER_KM;
constexpr double CM_PER_METER = 100.0;
constexpr double MILLIBARS_PER_BAR = 1000.00;

constexpr double GRAV_CONSTANT = 6.672E-8;
constexpr double MOLAR_GAS_CONST = 8314.41;
constexpr double K = 50.0;
constexpr double B = 1.2E-5;
constexpr double DUST_DENSITY_COEFF = 2.0E-3;
constexpr double ALPHA = 5.0;
constexpr double NDENSITY = 3.0;
constexpr double J = 1.46E-19;
// Define INCREDIBLY_LARGE_NUMBER based on whether HUGE_VAL is available or not
#ifdef HUGE_VAL
constexpr double INCREDIBLY_LARGE_NUMBER = HUGE_VAL;
#else
constexpr double INCREDIBLY_LARGE_NUMBER = 9.9999E37;
#endif

/*	Now for a few molecular weights (used for RMS velocity calcs):	   */
/*	This table is from Dole's book "Habitable Planets for Man", p. 38  */

// Atomic and molecular weights
constexpr double ATOMIC_HYDROGEN = 1.0;
constexpr double MOL_HYDROGEN = 2.0;
constexpr double HELIUM = 4.0;
constexpr double ATOMIC_NITROGEN = 14.0;
constexpr double ATOMIC_OXYGEN = 16.0;
constexpr double METHANE = 16.0;
constexpr double AMMONIA = 17.0;
constexpr double WATER_VAPOR = 18.0;
constexpr double NEON = 20.2;
constexpr double MOL_NITROGEN = 28.0;
constexpr double CARBON_MONOXIDE = 28.0;
constexpr double NITRIC_OXIDE = 30.0;
constexpr double MOL_OXYGEN = 32.0;
constexpr double HYDROGEN_SULPHIDE = 34.1;
constexpr double ARGON = 39.9;
constexpr double CARBON_DIOXIDE = 44.0;
constexpr double NITROUS_OXIDE = 44.0;
constexpr double NITROGEN_DIOXIDE = 46.0;
constexpr double OZONE = 48.0;
constexpr double SULPH_DIOXIDE = 64.1;
constexpr double SULPH_TRIOXIDE = 80.1;
constexpr double KRYPTON = 83.8;
constexpr double XENON = 131.3;

//	And atomic numbers, for use in ChemTable indexes
constexpr int AN_H = 1;
constexpr int AN_HE = 2;
constexpr int AN_N = 7;
constexpr int AN_O = 8;
constexpr int AN_F = 9;
constexpr int AN_NE = 10;
constexpr int AN_P = 15;
constexpr int AN_CL = 17;
constexpr int AN_AR = 18;
constexpr int AN_BR = 35;
constexpr int AN_KR = 36;
constexpr int AN_I = 53;
constexpr int AN_XE = 54;
constexpr int AN_HG = 80;
constexpr int AN_AT = 85;
constexpr int AN_RN = 86;
constexpr int AN_FR = 87;

constexpr int AN_NH3 = 900;
constexpr int AN_H2O = 901;
constexpr int AN_CO2 = 902;
constexpr int AN_O3 = 903;
constexpr int AN_CH4 = 904;
constexpr int AN_CH3CH2OH = 905;

/*	The following defines are used in the kothari_radius function in
 */
/*	file enviro.c.
 */
 // Additional constants for calculations
constexpr double A1_20 = 6.485E12;  /* All units are in cgs system.	 */
constexpr double A2_20 = 4.0032E-8; /*	 ie: cm, g, dynes, etc.		 */
constexpr double BETA_20 = 5.71E12;

constexpr double JIMS_FUDGE = 1.004;

constexpr double SUNMAG = 4.83;         /* absolute magnitude of the sun */
constexpr double N2 = 2.51188643150958; /* 5th root of 100 */

/*	 The following defines are used in determining the fraction of a planet
 */
/*	covered with clouds in function cloud_fraction in file enviro.c.
 */
constexpr double Q1_36 = 1.258E19; /* grams	*/
constexpr double Q2_36 = 0.0698;   /* 1/Kelvin */

constexpr double PARSEC = 3.2615638; /*light years*/

constexpr double AVOGADRO = 6.02214179E23;
constexpr double H = 6.62606E-34;    /*Planck constant*/
constexpr double C = 299792458;     /*Speed of light*/
constexpr double KB = (MOLAR_GAS_CONST / 1000.0) / AVOGADRO; /*Boltzmann constant*/
constexpr double ACCURACY_FOR_PEAK = 0.01;


constexpr double pow2(double a) {
    return a * a;
}

constexpr double pow3(double a) {
    return a * a * a;
}

constexpr double pow4(double a) {
    return a * a * a * a;
}

// Note: std::sqrt and std::pow are not constexpr until C++20, so these functions
// cannot be used in a constexpr context in earlier standards.
constexpr double pow1_4(double a) {
    return std::sqrt(std::sqrt(a));
}

constexpr double pow1_3(double a) {
    return std::pow(a, 1.0 / 3.0);
}

constexpr double pow1_2(double a) {
    return std::sqrt(a);
}
// mass based on luminosity (a) */
constexpr double mass(double a) {
    if (a <= (0.3815 * std::pow(0.6224, 2.5185))) {
        return 1.46613 * std::pow(a, 0.3970617431010522);
    } else if (a <= 1) {
        return std::pow(a, 0.2197319270490002);
    } else if (a <= std::pow(3.1623, 4.351)) {
        return std::pow(a, 0.2298322224775914);
    } else if (a <= (2.7563 * std::pow(16, 3.4704))) {
        return 0.746654 * std::pow(a, 0.2881512217611803);
    } else {
        return 0.221579 * std::pow(a, 0.4023659115599726);
    }
}

/* calculates luminosity based on absolute magnitude (a) */
constexpr double abs2luminosity(double a) { return std::pow(N2, (SUNMAG - a)); }
constexpr double vis2abs(double v, double d) { return v - 5 * (std::log10(d / PARSEC) - 1); }

constexpr double JUPITER_MASS = 317.8; /* mass of Jupiter in Earth Masses */
constexpr double EM(double x) {
    return x / SUN_MASS_IN_EARTH_MASSES;
}

constexpr double AVE(double x, double y) {
    return (x + y) / 2.0;
}

constexpr double ADD(double x, double y) {
    return x + y;
}

constexpr double DIVIDE(double x, double y) {
    return x / y;
}

constexpr double JUPITER_CORE_RATIO(double x) {
    return EM((x / JUPITER_MASS) * 10.0);
}

constexpr double JUPITER_GAS_RATIO(double x) {
    return EM(x) - JUPITER_CORE_RATIO(x);
}

constexpr double KM_JUPITER_RADIUS = 69911.0; /* average radius of Jupiter in km */
// Units of g/cc (derived from ((EARTH_DENSITY - (IRON_DENSITY / 3))/(2/3)))
constexpr double ROCK_DENSITY = 4.7825;
constexpr double SiC_DENSITY = 3.22;
constexpr double CARBON_DENSITY = SiC_DENSITY;
constexpr double GRAPHITE_DENSITY = 2.25;
constexpr double IRON_DENSITY = 7.874;
constexpr double FeS_DENSITY = 4.77;
// Units of g/cc (this is actually the density of ice VII)
constexpr double ICE_DENSITY = 1.46;
constexpr double MgO_DENSITY = 3.56;
constexpr double MgSiO3_DENSITY = 4.10;


/* Years. Value from "Regarding the Criteria for Planethood And
   Proposed Planetary Classification Schemes by S. Allan Stern and
   Harold F. Levison. Used to determine what is a planet and what is
   a dwarf planet */
constexpr double K2 = 1.7E+16;


// Climate classification constants
constexpr int RECENT_VENUS = 1;
constexpr int RUNAWAY_GREENHOUSE = 2;
constexpr int MOIST_GREENHOUSE = 3;
constexpr int MAXIMUM_GREENHOUSE = 4;
constexpr int EARLY_MARS = 5;
constexpr int TWO_AU_CLOUD_LIMIT = 6;
constexpr int FIRST_CO2_CONDENSATION_LIMIT = 7;
constexpr int EARTH_LIKE = 8;

// Atmosphere composition constants
constexpr int NONE = 0;
constexpr int BREATHABLE = 1;
constexpr int UNBREATHABLE = 2;
constexpr int POISONOUS = 3;


// Define the color scheme. Black, Brown and Beige (with a nod to the Duke)

// Main page colors: Beige BG, Dark brown text, Red links
constexpr const char* BGCOLOR = "#FFCC99";
constexpr const char* TXCOLOR = "#330000";
constexpr const char* LINKCOLOR = "#990000";
constexpr const char* ALINKCOLOR = "#FF0000";

// Contrasting headers: Light brown with black text
constexpr const char* BGHEADER = "#CC9966";
constexpr const char* TXHEADER = "#000000";

// Space, background for planets, black with sand colored letters
constexpr const char* BGSPACE = "#000000";
constexpr const char* TXSPACE = "#FFE6CC";

// Main table color scheme: Sand with black (space reversed)
constexpr const char* BGTABLE = "#FFE6CC";
constexpr const char* TXTABLE = "#000000";

// Notices: Post-It yellow with normal text
constexpr const char* BGNOTE = "#FFFF66";
constexpr const char* TXNOTE = TXCOLOR; // Reuses TXCOLOR for text color in notes

// URL for StarGen's main page
constexpr const char* STARGEN_URL = "http://www.eldacur.com/~brons/NerdCorner/StarGen/StarGen.html";

#endif
