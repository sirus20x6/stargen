#include "utils.h"

#include <gsl/gsl_multifit.h>

#include <boost/algorithm/string/erase.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <cstdlib>
#include <string>

#include "const.h"
#include "enviro.h"

using namespace boost;
using namespace std;

long seed = 0;
long jseed = 0;
long ifrst = 0;
long nextn = 0;

auto compare_string_char(string &a_string, int place, const char *a_character,
                         int length) -> bool {
  if (a_string.compare(place, length, a_character) == 0) {
    return true;
  }
  return false;
}

/*string replaceStrChar(string str, const char *old, const char *the_new)
{
  bool found;
  // set our locator equal to the first appearance of any character in replace
  size_t i = str.find_first_of(replace);

  while (found != string::npos)
  { // While our position in the sting is in range.
    str[found] = ch; // Change the character at position.
    found = str.find_first_of(replace, found+1); // Relocate again.
  }

  return str; // return our new string.
}*/

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
auto my_itoa(int value, char *result, int base) -> char * {
  // check that the base if valid
  if (base < 2 || base > 36) {
    *result = '\0';
    return result;
  }

  char *ptr = result, *ptr1 = result, tmp_char = 0;
  int tmp_value = 0;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ =
        "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxy"
        "z"[35 + (tmp_value - value * base)];
  } while (value);

  // Apply negative sign
  if (tmp_value < 0) {
    *ptr++ = '-';
  }
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}

auto float_to_string(long double number) -> string {
  return lexical_cast<string>(number);
}

auto random_number(long double inner, long double outer) -> long double {
  long double range = NAN;

  range = outer - inner;
  return (((long double)rand()) / (long double)(RAND_MAX)) * range + inner;
}

/*-----------------------------------------------------------------*/
/* This function returns a value within a certain variation of the */
/* exact value given it in 'value'.                                */
/*-----------------------------------------------------------------*/

auto about(long double value, long double variation) -> long double {
  return (value + (value * random_number(-variation, variation)));
}

auto random_eccentricity(long double ecc_coef) -> long double {
  long double e = NAN;
  e = 1.0 - pow(random_number(0.0, 1.0), ecc_coef);

  if (e > .99)  // Note that this coresponds to a random number less than
                // 10E-26. It happens with GNU C for -S254 -W27
  {
    e = 0.99;
  }

  return e;
}

// seb (for polar axis inclination, aka obliquity)
// gaussian (normal) random number function
// using Box-Muller transform
// ref: http://en.literateprograms.org/Box-Muller_transform_%28C%29

auto gaussian(long double sigma) -> long double {
  long double x = NAN, y = NAN, r = NAN;
  do {
    x = random_number(-1.0, 1.0);
    y = random_number(-1.0, 1.0);
    r = x * x + y * y;
  } while (r == 0 || r > 1.0);
  r = sigma * sqrt(-2.0 * log(r) / r);
  return x * r;
}

/* ==================================================
 *      Generator         Range (x)     Mean         Variance
 *      Poisson(m)        x = 0,...     m            m
 * Returns a Poisson distributed non-negative integer.
 * NOTE: use m > 0
 * ==================================================
 */
auto poisson(long double m) -> long {
  long double t = 0.0;
  long x = 0;

  while (t < m) {
    t += exponential(1.0);
    x++;
  }

  return x - 1;
}

/* =========================================================
 *      Generator         Range (x)     Mean         Variance
 *      Exponential(m)    x > 0         m            m*m
 * Returns an exponentially distributed positive real number.
 * NOTE: use m > 0.0
 * =========================================================
 */
auto exponential(long double m) -> long double {
  return (-1.0 * m) * log(1.0 - random_number(0.0, 1.0));
}

auto quad_trend(long double a, long double b, long double c, long double x)
    -> long double {
  return (a * pow2(x)) + (b * x) + c;
}

auto ln_trend(long double a, long double b, long double x) -> long double {
  return a + (b * log(x));
}

auto logistal_trend(long double a, long double b, long double c, long double x)
    -> long double {
  return c / (1 + (a * exp(-1.0 * b * x)));
}

// code from http://rosettacode.org/wiki/Polynomial_regression#C
auto polynomialfit(int obs, int degree, double *dx, double *dy, double *store)
    -> bool /* n, p */
{
  gsl_multifit_linear_workspace *ws = nullptr;
  gsl_matrix *cov = nullptr, *X = nullptr;
  gsl_vector *y = nullptr, *c = nullptr;
  double chisq = NAN;

  int i = 0, j = 0;

  X = gsl_matrix_alloc(obs, degree);
  y = gsl_vector_alloc(obs);
  c = gsl_vector_alloc(degree);
  cov = gsl_matrix_alloc(degree, degree);

  for (i = 0; i < obs; i++) {
    gsl_matrix_set(X, i, 0, 1.0);
    for (j = 0; j < degree; j++) {
      gsl_matrix_set(X, i, j, pow(dx[i], j));
    }
    gsl_vector_set(y, i, dy[i]);
  }

  ws = gsl_multifit_linear_alloc(obs, degree);
  gsl_multifit_linear(X, y, c, cov, &chisq, ws);

  /* store result ... */
  for (i = 0; i < degree; i++) {
    store[i] = gsl_vector_get(c, i);
  }

  gsl_multifit_linear_free(ws);
  gsl_matrix_free(X);
  gsl_matrix_free(cov);
  gsl_vector_free(y);
  gsl_vector_free(c);
  return true; /* we do not "analyse" the result (cov matrix mainly)
  to know if the fit is "good" */
}

auto soft(long double v, long double max, long double min) -> long double {
  long double dv = v - min;
  long double dm = max - min;
  return (lim(2.0 * dv / dm - 1.0) + 1.0) / 2.0 * dm + min;
}

auto lim(long double x) -> long double { return x / pow1_4(1 + x * x * x * x); }

auto remove_spaces(string str) -> string {
  erase_all(str, " ");
  return str;
}

auto randf() -> double {
  long mplier = 16807;
  long modlus = 2147483647;
  long mobymp = 127773;
  long momdmp = 2836;
  long hvlue = 0, lvlue = 0, testv = 0;

  if (ifrst == 0) {
    nextn = jseed;
    ifrst = 1;
  }

  hvlue = nextn / mobymp;
  lvlue = nextn % mobymp;
  testv = (mplier * lvlue) - (momdmp * hvlue);

  if (testv > 0) {
    nextn = testv;
  } else {
    nextn = testv + modlus;
  }

  return (double)nextn / (double)modlus;
}

void srandf(long a_seed) {
  jseed = a_seed;
  ifrst = 0;
}

auto fix_inclination(long double inclination) -> long double {
  // we can't have a negative number
  inclination = abs(inclination);
  // nor we can't an inclination greater than 180 degrees.
  inclination = fmod(inclination, 180.0);
  return inclination;
}

auto linear_trend(long double m, long double b, long double x) -> long double {
  return (m * x) + b;
}

auto my_strtoupper(const string &the_string) -> string {
  const char *temp = the_string.c_str();
  char temp2 = 0;
  stringstream ss;
  string output;

  ss.str("");
  for (int i = 0; i < the_string.length(); i++) {
    temp2 = toupper(temp[i]);
    ss << temp2;
  }
  output = ss.str();
  ss.str("");
  return output;
}

auto star_type_to_num(const string &spec_type, long double luminosity, int run)
    -> int {
  stringstream ss;
  string arr[] = {
      "blada", "O0", "O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9", "B0",
      "B1",    "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "A0", "A1", "A2",
      "A3",    "A4", "A5", "A6", "A7", "A8", "A9", "F0", "F1", "F2", "F3", "F4",
      "F5",    "F6", "F7", "F8", "F9", "G0", "G1", "G2", "G3", "G4", "G5", "G6",
      "G7",    "G8", "G9", "K0", "K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8",
      "K9",    "M0", "M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8", "M9", "L0",
      "L1",    "L2", "L3", "L4", "L5", "L6", "L7", "L8", "L9", "T0", "T1", "T2",
      "T3",    "T4", "T5", "T6", "T7", "T8", "T9", "Y0", "Y1", "Y2", "Y3", "Y4",
      "Y5",    "Y6", "Y7", "Y8", "Y9"};
  int arrSize = sizeof(arr) / sizeof(string);
  vector<string> star_types(arr, arr + arrSize);
  int num = 0;
  bool found = false;
  string new_spec_type;
  long double temperature = NAN;

  for (int i = 0; i < star_types.size(); i++) {
    if (spec_type.find(star_types[i]) != string::npos) {
      found = true;
      num = i;
      break;
    } else {
      found = false;
    }
  }

  if (!found &&
      run == 1)  // if it can't find it, make a spectral type it can try.
  {
    ss.str("");
    ss << getStarType(spec_type) << getSubType(spec_type) << "V";
    return star_type_to_num(ss.str(), luminosity, run + 1.0);
  } else if (!found &&
             run == 2)  // If it still can't find it, make a spectral type based
                        // on the temperature and try again.
  {
    temperature = spec_type_to_eff_temp(spec_type);
    new_spec_type = eff_temp_to_spec_type(temperature, luminosity);
    return star_type_to_num(new_spec_type, luminosity, run + 1.0);
  } else if (!found &&
             run >=
                 3)  // if it still can't find it, use the Sun's spectral type.
  {
    return star_type_to_num("G2V", luminosity, run + 1.0);
  } else {
    return num;
  }
}

void logfix(long double x, long double y, long double w, long double z,
            long double &a, long double &b) {
  a = ((y * log(w)) - (z * log(x))) / (log(w) - log(x));
  b = (z - y) / (log(w) - log(x));
}

auto rangeAdjust(long double x, long double y1, long double y2,
                 long double lower, long double upper) -> long double {
  long double range = upper - lower;
  long double upper_fraction = (x - lower) / range;
  long double lower_fraction = 1.0 - upper_fraction;
  long double result = (lower_fraction * y1) + (upper_fraction * y2);
  return result;
}

void e_fix(long double x, long double y, long double w, long double z,
           long double &a, long double &b) {
  a = ((exp(x) * z) - (exp(w) * y)) / (exp(x) - exp(w));
  b = (y - z) / (exp(x) - exp(w));
}

auto e_trend(long double a, long double b, long double x) -> long double {
  return a + (b * exp(x));
}

void quadfix(long double x, long double y, long double w, long double z,
             long double p, long double q, long double &a, long double &b,
             long double &c) {
  a = ((q * (w - x)) - (w * y) + (p * (y - z)) + (x * z)) /
      ((p - w) * (p - x) * (w - x));
  b = ((q * (pow2(x) - pow2(w))) + (pow2(w) * y) - (pow2(x) * z) +
       (pow2(p) * (z - y))) /
      ((p - w) * (p - x) * (w - x));
  c = ((q * w * x * (w - x)) +
       (p * ((p * w * y) - (pow2(w) * y) - (p * x * z) + (pow2(x) * z)))) /
      ((p - w) * (p - x) * (w - x));
}

auto quintic_trend(long double a, long double b, long double c, long double d,
                   long double e, long double f, long double x) -> long double {
  return (a * pow(x, 5.0)) + (b * pow4(x)) + (c * pow3(x)) + (d * pow2(x)) +
         (e * x) + f;
}
