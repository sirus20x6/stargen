#include "structs.h"

#include <math.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "const.h"
#include "enviro.h"
#include "star_temps.h"
#include "utils.h"

using namespace std;

/**
 * @brief Construct a new star::star object
 * 
 * @param l 
 * @param m 
 * @param et 
 * @param t 
 * @param m2 
 * @param e 
 * @param d 
 * @param i 
 * @param a 
 * @param kp 
 * @param des 
 * @param cel 
 * @param n 
 */
star::star(long double l, long double m, long double et, string t,
           long double m2, long double e, long double d, long double i,
           long double a, planet* kp, string des, bool cel, string n) {
  luminosity = l;
  mass = m;
  eff_temp = et;
  spec_type = t;
  mass2 = m2;
  luminosity2 = 0;
  eff_temp2 = 0;
  spec_type2 = "";
  eccentricity = e;
  distance = d;
  inc = i;
  an = a;
  known_planets = kp;
  desig = des;
  in_celestia = cel;
  name = n;
  if (luminosity == 0) {
    calcLuminosity();
  }
  if (mass == 0) {
    calcMass();
  }
  if (eff_temp == 0) {
    calcEffTemp();
  }
  if (spec_type.empty()) {
    spec_type = eff_temp_to_spec_type(eff_temp, luminosity);
  }
  isCircumbinary = false;
}

star::star(star2& data) {
  luminosity = data.luminosity;
  mass = data.mass;
  eff_temp = data.eff_temp;
  spec_type = data.spec_type;
  mass2 = data.m2;
  eccentricity = data.e;
  distance = data.a;
  inc = data.inc;
  an = data.an;
  known_planets = NULL;
  desig = data.desig;
  if (data.in_celestia == 0) {
    in_celestia = false;
  } else {
    in_celestia = true;
  }
  name = data.name;
  if (luminosity == 0) {
    calcLuminosity();
  }
  if (mass == 0) {
    calcMass();
  }
  if (eff_temp == 0) {
    calcEffTemp();
  }
  if (spec_type.empty()) {
    spec_type = eff_temp_to_spec_type(eff_temp, luminosity);
  }
  isCircumbinary = false;
}

/**
 * @brief Destroy the star::star object
 * 
 */
star::~star() {
  luminosity = 0;
  mass = 0;
  eff_temp = 0;
  spec_type = "";
  mass2 = 0;
  eccentricity = 0;
  distance = 0;
  inc = 0;
  an = 0;
  desig = "";
  in_celestia = false;
  name = "";
  isCircumbinary = false;
}

/**
 * @brief Calculate Luminosity
 * 
 */
void star::calcLuminosity() { luminosity = mass_to_luminosity(mass); }

/**
 * @brief Calculate Mass
 * 
 */
void star::calcMass() { mass = luminosity_to_mass(luminosity); }

/**
 * @brief Calculate Eff Temp
 * 
 */
void star::calcEffTemp() { eff_temp = spec_type_to_eff_temp(spec_type); }

star::star(star& right) {
  luminosity = right.luminosity;
  mass = right.mass;
  eff_temp = right.eff_temp;
  spec_type = right.spec_type;
  mass2 = right.mass2;
  eccentricity = right.eccentricity;
  distance = right.distance;
  inc = right.inc;
  an = right.an;
  known_planets = right.known_planets;
  desig = right.desig;
  in_celestia = right.in_celestia;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  luminosity2 = right.luminosity2;
  eff_temp2 = right.eff_temp2;
  spec_type2 = right.spec_type2;
  if (luminosity == 0) {
    calcLuminosity();
  }
  if (mass == 0) {
    calcMass();
  }
  if (eff_temp == 0) {
    calcEffTemp();
  }
  if (spec_type.empty()) {
    spec_type = eff_temp_to_spec_type(eff_temp, luminosity);
  }
}

star::star(const star& right) {
  luminosity = right.luminosity;
  mass = right.mass;
  eff_temp = right.eff_temp;
  spec_type = right.spec_type;
  mass2 = right.mass2;
  eccentricity = right.eccentricity;
  distance = right.distance;
  inc = right.inc;
  an = right.an;
  known_planets = right.known_planets;
  desig = right.desig;
  in_celestia = right.in_celestia;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  luminosity2 = right.luminosity2;
  eff_temp2 = right.eff_temp2;
  spec_type2 = right.spec_type2;
  if (luminosity == 0) {
    calcLuminosity();
  }
  if (mass == 0) {
    calcMass();
  }
  if (eff_temp == 0) {
    calcEffTemp();
  }
  if (spec_type.empty()) {
    spec_type = eff_temp_to_spec_type(eff_temp, luminosity);
  }
}

/// @brief 
/// @param right 
/// @return 
auto star::operator=(star& right) -> star {
  luminosity = right.luminosity;
  mass = right.mass;
  eff_temp = right.eff_temp;
  spec_type = right.spec_type;
  mass2 = right.mass2;
  eccentricity = right.eccentricity;
  distance = right.distance;
  inc = right.inc;
  an = right.an;
  known_planets = right.known_planets;
  desig = right.desig;
  in_celestia = right.in_celestia;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  luminosity2 = right.luminosity2;
  eff_temp2 = right.eff_temp2;
  spec_type2 = right.spec_type2;
  if (luminosity == 0) {
    calcLuminosity();
  }
  if (mass == 0) {
    calcMass();
  }
  if (eff_temp == 0) {
    calcEffTemp();
  }
  if (spec_type.empty()) {
    spec_type = eff_temp_to_spec_type(eff_temp, luminosity);
  }
  return *this;
}

/// @brief 
/// @param strm 
/// @param obj 
/// @return 
auto operator<<(ostream& strm, star& obj) -> ostream& {
  string is_in_celestia = "";

  if (obj.in_celestia) {
    is_in_celestia = "+P";
  }

  strm << setprecision(4);
  strm << left << setw(30 + obj.extra_spaces) << obj.name << fixed << right
       << setw(3) << is_in_celestia << setw(8) << obj.mass << setw(12)
       << obj.luminosity << setw(13) << obj.eff_temp << setw(11)
       << obj.spec_type;
  return strm;
}

/// @brief 
/// @return 
auto star::getName() -> string { return name; }

/// @brief 
/// @param e 
void star::setExtraSpaces(int e) { extra_spaces = e; }

/// @brief 
/// @return 
auto star::getExtraSpaces() -> int { return extra_spaces; }

/// @brief 
/// @return 
auto star::getAn() -> long double { return an; }

/// @brief 
/// @return 
auto star::getInc() -> long double { return inc; }

/// @brief 
/// @return 
auto star::getKnownPlanets() -> planet* { return known_planets; }

/// @brief 
/// @return 
auto star::getInCelestia() -> bool { return in_celestia; }

/// @brief 
/// @return 
auto star::getEffTemp() -> long double { return eff_temp; }

/// @brief 
/// @return 
auto star::getLuminosity() -> long double { return luminosity; }

/// @brief 
/// @return 
auto star::getMass() -> long double { return mass; }

/// @brief 
/// @return 
auto star::getSpecType() -> string { return spec_type; }

/// @brief 
/// @return 
auto star::getDesig() -> string { return desig; }

/// @brief 
/// @return 
auto star::getDistance() -> long double { return distance; }

/// @brief 
/// @return 
auto star::getEccentricity() -> long double { return eccentricity; }

/// @brief 
/// @return 
auto star::getMass2() -> long double { return mass2; }

/// @brief 
/// @return 
auto star::getEffTemp2() -> long double { return eff_temp2; }

/// @brief 
/// @return 
auto star::getIsCircumbinary() -> bool { return isCircumbinary; }

/// @brief 
/// @return 
auto star::getLuminosity2() -> long double { return luminosity2; }

/// @brief 
/// @return 
auto star::getSpecType2() -> string { return spec_type2; }

/// @brief 
/// @param d 
void star::setDistance(long double d) { distance = d; }

/// @brief 
/// @param e 
void star::setEccentricity(long double e) { eccentricity = e; }

/// @brief 
/// @param e 
void star::setEffTemp2(long double e) { eff_temp2 = e; }

/// @brief 
/// @param b 
void star::setIsCircumbinary(bool b) { isCircumbinary = b; }

/// @brief 
/// @param l 
void star::setLuminosity2(long double l) { luminosity2 = l; }

/// @brief 
/// @param m 
void star::setMass2(long double m) { mass2 = m; }

/// @brief 
/// @param s 
void star::setSpecType2(string s) { spec_type2 = std::move(s); }

/// @brief 
/// @param right 
/// @return 
auto star::operator<(star& right) -> bool {
  if (desig == "Sol" && right.desig == "Sol") {
    return false;
  } else if (desig == "Sol") {
    return true;
  } else if (right.desig == "Sol") {
    return false;
  } else if (name < right.name) {
    return true;
  }
  return false;
}

/// @brief 
/// @param right 
/// @return 
auto star::operator==(star& right) -> bool {
  if (name == right.name) {
    return true;
  }
  return false;
}

/// @brief 
catalog::catalog() : extra_spaces(0) { }

/// @brief 
/// @param a 
catalog::catalog(string a) : arg(a), extra_spaces(0) {
  
  
}

/// @brief 
/// @return 
auto catalog::count() -> int { return stars.size(); }

/// @brief 
/// @param the_star 
void catalog::addStar(star& the_star) {
  string star_name;
  int name_length = 0;
  int diff = 0;

  stars.push_back(the_star);

  star_name = the_star.getName();
  name_length = star_name.length();
  if (name_length > 30) {
    diff = name_length - 30;
    if (diff > extra_spaces) {
      extra_spaces = diff;
    }
  }
}

/// @brief 
/// @param sub 
/// @return 
auto catalog::operator[](const int& sub) -> star& {
  if (sub < 0 || sub > (stars.size() - 1)) {
    cerr << "Non existant star!\n";
    exit(EXIT_FAILURE);
  }
  return stars[sub];
}

/// @brief 
catalog::~catalog() {
  /*planet *planets = NULL;
  planet *node = NULL;
  planet *next = NULL;
  for (int i = 0; i < stars.size(); i++)
  {
    planets = stars[i].getKnownPlanets();
    if (planets != NULL)
    {
      for (node = planets; node != NULL; node = next)
      {
        next = node->next_planet;
        delete node;
      }
    }
  }*/
  stars.clear();
}

/// @brief 
/// @param strm 
/// @param obj 
/// @return 
auto operator<<(ostream& strm, catalog& obj) -> ostream& {
  int total_stars = obj.count();
  strm << setw(14) << "Name" << setw(37 + obj.extra_spaces) << "Mass"
       << setw(12) << "Luminosity" << setw(13) << "Temperature" << setw(11)
       << "Star Type\n";
  strm << fixed;

  for (int i = 0; i < total_stars; i++) {
    obj[i].setExtraSpaces(obj.extra_spaces);
    strm << setw(8) << right << i << ". " << obj[i] << endl;
  }

  return strm;
}

/// @brief 
/// @param right 
/// @return 
auto catalog::operator=(catalog& right) -> catalog {
  arg = right.arg;
  stars.clear();
  int new_stars = right.count();
  for (int i = 0; i < new_stars; i++) {
    stars.push_back(right[i]);
  }
  return *this;
}

/// @brief 
/// @param a 
void catalog::setArg(string a) { arg = std::move(a); }

/// @brief 
/// @return 
auto catalog::getArg() -> string { return arg; }

/// @brief 
void catalog::sort() {
  // writeVector(stars, "\n");
  quicksort(stars, 0, stars.size());
}

/// @brief 
Chemicle::Chemicle() : num(0), symbol(""), htmlSymbol(""), name(""), weight(0), melt(0), boil(0), density(0), pzero(0), c(0), n(0), abunde(0), abunds(0), reactivity(0), maxIpp(0), minIpp(0) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  fixBoil();
  fixMelt();
  fixAbunde();
  fixAbunds();
  fixMaxIpp();
  fixMaxIpp();
  setSpaces();
}

/// @brief 
/// @param nu 
/// @param s 
/// @param h 
/// @param na 
/// @param w 
/// @param m 
/// @param b 
/// @param d 
/// @param p 
/// @param c2 
/// @param n2 
/// @param ae 
/// @param as 
/// @param r 
/// @param ma 
/// @param mi 
Chemicle::Chemicle(int nu, string s, string h, string na, long double w,
                   long double m, long double b, long double d, long double p,
                   long double c2, long double n2, long double ae,
                   long double as, long double r, long double ma,
                   long double mi) {
  num = nu;
  symbol = s;
  htmlSymbol = h;
  name = na;
  weight = w;
  melt = m;
  boil = b;
  density = d;
  pzero = p;
  c = c2;
  n = n2;
  abunde = ae;
  abunds = as;
  reactivity = r;
  maxIpp = ma;
  minIpp = mi;
  fixBoil();
  fixMelt();
  fixAbunde();
  fixAbunds();
  fixMaxIpp();
  fixMaxIpp();
  setSpaces();
}

/// @brief 
/// @param right 
Chemicle::Chemicle(Chemicle& right) : num(right.num), symbol(right.symbol), htmlSymbol(right.htmlSymbol), name(right.name), weight(right.weight), melt(right.melt), boil(right.boil), density(right.density), pzero(right.pzero), c(right.c), n(right.n), abunde(right.abunde), abunds(right.abunds), reactivity(right.reactivity), maxIpp(right.maxIpp), minIpp(right.minIpp) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  fixBoil();
  fixMelt();
  fixAbunde();
  fixAbunds();
  fixMaxIpp();
  fixMaxIpp();
  setSpaces();
}

/// @brief 
/// @param right 
Chemicle::Chemicle(const Chemicle& right) : num(right.num), symbol(right.symbol), htmlSymbol(right.htmlSymbol), name(right.name), weight(right.weight), melt(right.melt), boil(right.boil), density(right.density), pzero(right.pzero), c(right.c), n(right.n), abunde(right.abunde), abunds(right.abunds), reactivity(right.reactivity), maxIpp(right.maxIpp), minIpp(right.minIpp) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  fixBoil();
  fixMelt();
  fixAbunde();
  fixAbunds();
  fixMaxIpp();
  fixMaxIpp();
  setSpaces();
}

/// @brief 
Chemicle::~Chemicle() {
  num = 0;
  symbol = "";
  htmlSymbol = "";
  name = "";
  weight = 0;
  melt = 0;
  boil = 0;
  density = 0;
  pzero = 0;
  c = 0;
  n = 0;
  abunde = 0;
  abunds = 0;
  reactivity = 0;
  maxIpp = 0;
  minIpp = 0;
  nameSpaces = 0;
  symbolSpaces = 0;
}



/// @brief 
/// @param right 
/// @return 
auto Chemicle::operator=(const Chemicle& right) -> Chemicle& {
  num = right.num;
  symbol = right.symbol;
  htmlSymbol = right.htmlSymbol;
  name = right.name;
  weight = right.weight;
  melt = right.melt;
  boil = right.boil;
  density = right.density;
  pzero = right.pzero;
  c = right.c;
  n = right.n;
  abunde = right.abunde;
  abunds = right.abunds;
  reactivity = right.reactivity;
  maxIpp = right.maxIpp;
  minIpp = right.minIpp;
  fixBoil();
  fixMelt();
  fixAbunde();
  fixAbunds();
  fixMaxIpp();
  fixMaxIpp();
  setSpaces();
  return *this;
}

void Chemicle::setBoil(long double b) {
  boil = b;
  fixBoil();
}

void Chemicle::setMelt(long double m) {
  melt = m;
  fixMelt();
}

void Chemicle::setMaxIpp(long double ma) {
  maxIpp = ma;
  fixMaxIpp();
}

void Chemicle::setMinIpp(long double mi) {
  minIpp = mi;
  fixMinIpp();
}

void Chemicle::setAbunde(long double a) {
  abunde = a;
  fixAbunde();
}

void Chemicle::setAbunds(long double a) {
  abunds = a;
  fixAbunds();
}

void Chemicle::fixAbunde() {
  if (abunde <= 0) {
    if (num > 92) {
      abunde = 1.0E-41;
    } else {
      abunde = 1.0E-21;
    }
  }
}

void Chemicle::fixAbunds() {
  if (abunds <= 0) {
    if (num > 92) {
      abunds = 1.0E-41;
    } else {
      abunds = 1.0E-21;
    }
  }
}

void Chemicle::fixBoil() {
  if (boil <= 0) {
    boil = melt + 100.0;
  }
}

void Chemicle::fixMaxIpp() {
  if (maxIpp <= 0) {
    maxIpp = INCREDIBLY_LARGE_NUMBER;
  }
}

void Chemicle::fixMelt() {
  if (boil > 0 && melt <= 0) {
    melt = boil * (2.0 / 3.0);
  } else if (melt <= 0) {
    melt = 1100.0;
  }
}

void Chemicle::fixMinIpp() {
  // do nothing
}

void Chemicle::setSpaces() {
  nameSpaces = 18 - name.length();
  symbolSpaces = 15 - symbol.length();
}

auto Chemicle::getNameSpaces() -> int { return nameSpaces; }

auto Chemicle::getSymbolSpaces() -> int { return symbolSpaces; }

auto Chemicle::operator<(Chemicle& right) -> bool {
  long double xx = abunds * abunde;
  long double yy = right.abunds * right.abunde;

  // cout << xx << " < " << yy << endl;

  if (xx > yy) {
    return true;
  } else if (xx == yy) {
    if (weight < right.weight) {
      return true;
    } else if (weight == right.weight) {
      if (boil < right.boil) {
        return true;
      } else if (boil == right.boil) {
        if (num < right.num) {
          return true;
        }
      }
    }
  }
  return false;
}

auto Chemicle::operator==(Chemicle& right) -> bool {
  long double xx = abunds * abunde;
  long double yy = right.abunds * right.abunde;

  if (xx == yy && weight == right.weight && boil == right.boil &&
      num == right.num) {
    return true;
  }
  return false;
}

auto operator<<(ostream& strm, Chemicle& obj) -> ostream& {
  strm << left << obj.getNum() << ".\t" << right << obj.getSymbol();
  for (int i = 0; i < obj.getSymbolSpaces(); i++) {
    strm << " ";
  }
  strm << obj.getName();
  for (int i = 0; i < obj.getNameSpaces(); i++) {
    strm << " ";
  }
  strm << fixed << right << setprecision(4) << setw(10) << obj.getWeight()
       << setw(22) << obj.getMinIpp() << setw(20) << obj.getMaxIpp() << setw(15)
       << scientific << (obj.getAbunde() * obj.getAbunds());
  return strm;
}

void ChemTable::addChemicle(const Chemicle &chem) {
  chemicles.push_back(chem);
  quicksort(chemicles, 0, chemicles.size());
}

auto ChemTable::count() -> int { return chemicles.size(); }

auto ChemTable::operator[](const int& index) -> Chemicle& {
  if (index < 0 || index >= chemicles.size()) {
    cerr << "ERROR: Subscript out of range.\n";
    exit(EXIT_FAILURE);
  }
  return chemicles[index];
}

auto operator<<(ostream& strm, ChemTable& obj) -> ostream& {
  cout << "Num\t" << setw(6) << "Symbol" << setw(13) << "Name" << setw(24)
       << "Weight" << setw(22) << "Min breathable IPP" << setw(20)
       << "Max Breathable IPP" << setw(15) << "Abund\n";
  strm << fixed;
  for (int i = 0; i < obj.count(); i++) {
    strm << setprecision(0) << obj[i] << endl;
  }
  return strm;
}

sun::sun() {
  luminosity = 0;
  mass = 0;
  effTemp = 0;
  specType = "";
  age = 0;
  name = "";
  isCircumbinary = false;
  secondaryMass = 0;
  secondaryLuminosity = 0;
  secondaryEffTemp = 0;
  secondarySpecType = "";
  combinedEffTemp = 0;
}

sun::sun(long double l, long double m, long double t, string s, long double a,
         string n) {
  luminosity = l;
  mass = m;
  effTemp = t;
  specType = s;
  age = a;
  name = n;
  isCircumbinary = false;
  secondaryMass = 0;
  secondaryLuminosity = 0;
  secondaryEffTemp = 0;
  secondarySpecType = "";
  combinedEffTemp = 0;
  if (luminosity == 0) {
    luminosity = mass_to_luminosity(mass);
  }
  if (mass == 0) {
    mass = luminosity_to_mass(luminosity);
  }
  // if (effTemp == 0)
  // {
  //   effTemp == spec_type_to_eff_temp(specType);
  // }
  if (specType.empty()) {
    specType = eff_temp_to_spec_type(effTemp, luminosity);
  }
}

sun::sun(sun& right) {
  luminosity = right.luminosity;
  mass = right.mass;
  effTemp = right.effTemp;
  specType = right.specType;
  age = right.age;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  secondaryMass = right.secondaryMass;
  secondaryLuminosity = right.secondaryLuminosity;
  secondaryEffTemp = right.secondaryEffTemp;
  secondarySpecType = right.secondarySpecType;
  seperation = right.seperation;
  eccentricity = right.eccentricity;
  combinedEffTemp = right.combinedEffTemp;
  if (luminosity == 0) {
    luminosity = mass_to_luminosity(mass);
  }
  if (mass == 0) {
    mass = luminosity_to_mass(luminosity);
  }
  // if (effTemp == 0)
  // {
  //   effTemp == spec_type_to_eff_temp(specType);
  // }
  if (specType.empty()) {
    specType = eff_temp_to_spec_type(effTemp, luminosity);
  }
}

sun::sun(const sun& right) {
  luminosity = right.luminosity;
  mass = right.mass;
  effTemp = right.effTemp;
  specType = right.specType;
  age = right.age;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  secondaryMass = right.secondaryMass;
  secondaryLuminosity = right.secondaryLuminosity;
  secondaryEffTemp = right.secondaryEffTemp;
  secondarySpecType = right.secondarySpecType;
  seperation = right.seperation;
  eccentricity = right.eccentricity;
  combinedEffTemp = right.combinedEffTemp;
  if (luminosity == 0) {
    luminosity = mass_to_luminosity(mass);
  }
  if (mass == 0) {
    mass = luminosity_to_mass(luminosity);
  }
  // if (effTemp == 0)
  // {
  //   effTemp == spec_type_to_eff_temp(specType);
  // }
  if (specType.empty()) {
    specType = eff_temp_to_spec_type(effTemp, luminosity);
  }
}

/*sun sun::operator=(sun& right)
{
  luminosity = right.luminosity;
  mass = right.mass;
  effTemp = right.effTemp;
  specType = right.specType;
  age = right.age;
  name = right.name;
  isCircumbinary = right.isCircumbinary;
  secondaryMass = right.secondaryMass;
  secondaryLuminosity = right.secondaryLuminosity;
  secondaryEffTemp = right.secondaryEffTemp;
  secondarySpecType = right.secondarySpecType;
  seperation = right.seperation;
  eccentricity = right.eccentricity;
}*/

auto sun::getAge() -> long double { return age; }

auto sun::getEffTemp() -> long double {
  if (effTemp == 0) {
    effTemp = spec_type_to_eff_temp(specType);
  }
  return effTemp;
}

auto sun::getLife() -> long double { return 1.0E10 * (mass / luminosity); }

auto sun::getSecondaryLife() -> long double {
  return 1.0E10 * (secondaryMass / secondaryLuminosity);
}

auto sun::getLuminosity() -> long double { return luminosity; }

auto sun::getMass() -> long double { return mass; }

auto sun::getName() -> string { return name; }

auto sun::getREcosphere(long double mass) -> long double {
  return habitable_zone_distance(*this, EARTH_LIKE, mass);
}

auto sun::getSpecType() -> string {
  if (specType.empty()) {
    specType = eff_temp_to_spec_type(effTemp, luminosity);
  }
  return specType;
}

void sun::setAge(long double a) { age = a; }

void sun::setEffTemp(long double e) {
  effTemp = e;
  if (specType.empty()) {
    specType = eff_temp_to_spec_type(effTemp, luminosity);
  }
}

void sun::setLuminosity(long double l) { luminosity = l; }

void sun::setMass(long double m) { mass = m; }

void sun::setName(string n) { name = std::move(n); }

void sun::setSpecType(string s) {
  specType = std::move(s);
  specType = my_strtoupper(specType);
  if (effTemp == 0) {
    effTemp = spec_type_to_eff_temp(specType);
  }
}

auto sun::getIsCircumbinary() -> bool { return isCircumbinary; }

auto sun::getSecondaryEffTemp() -> long double { return secondaryEffTemp; }

auto sun::getSecondaryLuminosity() -> long double { return secondaryLuminosity; }

auto sun::getSecondaryMass() -> long double { return secondaryMass; }

auto sun::getSecondarySpecType() -> string { return secondarySpecType; }

void sun::setIsCircumbinary(bool cb) {
  isCircumbinary = cb;
  if (isCircumbinary == false) {
    combinedEffTemp = 0;
  }
}

void sun::setSecondaryEffTemp(long double et) {
  secondaryEffTemp = et;
  if (secondarySpecType.empty()) {
    secondarySpecType =
        eff_temp_to_spec_type(secondaryEffTemp, secondaryLuminosity);
  }
}

void sun::setSecondaryLuminosity(long double l) {
  secondaryLuminosity = l;
  if (secondaryMass == 0) {
    secondaryMass = luminosity_to_mass(secondaryLuminosity);
  }
}

void sun::setSecondaryMass(long double m) {
  secondaryMass = m;
  if (secondaryLuminosity == 0) {
    secondaryLuminosity = mass_to_luminosity(secondaryMass);
  }
}

void sun::setSecondarySpecType(string t) {
  secondarySpecType = std::move(t);
  if (secondaryEffTemp == 0) {
    secondaryEffTemp = spec_type_to_eff_temp(secondarySpecType);
  }
}

auto sun::getCombinedLuminosity() -> long double {
  return luminosity + secondaryLuminosity;
}

auto sun::getCombinedMass() -> long double { return mass + secondaryMass; }

auto sun::getEccentricity() -> long double { return eccentricity; }

auto sun::getSeperation() -> long double { return seperation; }

void sun::setEccentricity(long double e) { eccentricity = e; }

void sun::setSeperation(long double s) { seperation = s; }

auto sun::getMinStableDistance() -> long double {
  long double u = NAN;
  if (!isCircumbinary) {
    return 0;
  } else {
    u = mass / (mass + secondaryMass);
    return seperation *
           (1.60 + (5.10 * eccentricity) - (2.22 * pow2(eccentricity)) +
            (4.12 * u) - (4.27 * eccentricity * u) - (5.09 * pow2(u)) +
            (4.61 * pow2(eccentricity) * pow2(u)));
  }
}

auto sun::getCombinedEffTemp() -> long double {
  long double f1 = NAN, f2 = NAN, prev = NAN, curr = NAN, peak = NAN, temperature = NAN, a = NAN, m = NAN, b = NAN;
  if (combinedEffTemp != 0) {
    return combinedEffTemp;
  }
  if (!isCircumbinary)  // This shouldn't happen but if it does, just return the
                        // temp of the star
  {
    combinedEffTemp = effTemp;
    return effTemp;
  } else {
    // effTemp = 5800;
    // secondaryEffTemp = 4400;
    curr = peak = 0;
    // todo this is bad. figure out what accuracy for peak is
    // then change the loop counter to not be a long double.
    for (long double i = 0; i < 1500.0; i += ACCURACY_FOR_PEAK) {
      prev = curr;
      f1 = calcFlux(effTemp, i);
      f2 = calcFlux(secondaryEffTemp, i);
      curr = f1 + f2;
      // cout << i << ": " << f1 << " " << f2 << " " << curr << endl;
      if (prev > curr) {
        peak = i - ACCURACY_FOR_PEAK;
        break;
      }
    }
    // cout << "Primary Temp: " << toString(effTemp) << endl;
    // cout << "Secondary Temp: " << toString(secondaryEffTemp) << endl;
    // cout << "Peak: " << toString(peak) << endl;;
    a = 0.0;
    m = 1000000000.0;
    b = 1.0;
    peak = std::pow((peak - a) / m, 1 / b);
    temperature = (0.0028977685 / peak) - 273.15;
    a = 273.15;
    m = 1.0;
    b = 1.0;
    temperature = (pow(temperature, b) * m) + a;
    // cout << "Combined temp: " << toString(temperature) << endl;
    // exit(EXIT_FAILURE);
    combinedEffTemp = temperature;
    return temperature;
  }
}

planet::planet() : planetNo(0), a(0), e(0), axialTilt(0), gasGiant(false), dustMass(0), gasMass(0), imf(0), rmf(0), cmf(0), moonA(0), moonE(0), coreRadius(0), radius(0), orbitZone(0), density(0), orbPeriod(0), day(0), resonantPeriod(false), escVelocity(0), surfAccel(0), surfGrav(0), rmsVelocity(0), molecWeight(0), volatileGasInventory(0), surfPressure(0), greenhouseEffect(false), boilPoint(0), albedo(0), exosphericTemp(0), estimatedTemp(0), estimatedTerrTemp(0), surfTemp(0), greenhsRise(0), highTemp(0), lowTemp(0), maxTemp(0), minTemp(0), hydrosphere(0), cloudCover(0), iceCover(0), inclination(0), ascendingNode(0), longitudeOfPericenter(0), meanLongitude(0), first_moon(NULL), next_planet(NULL), reconnect_to(NULL), first_moon_backup(NULL), type(tUnknown), deleteable(true) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}

planet::planet(int n, long double a2, long double e2, long double t, bool gg,
               long double d, long double g, planet* moon, planet* next) : planetNo(n), a(a2), e(e2), axialTilt(t), gasGiant(gg), dustMass(d), gasMass(g), imf(0), rmf(0), cmf(0), moonA(0), moonE(0), coreRadius(0), radius(0), orbitZone(0), density(0), orbPeriod(0), day(0), resonantPeriod(false), escVelocity(0), surfAccel(0), surfGrav(0), rmsVelocity(0), molecWeight(0), volatileGasInventory(0), surfPressure(0), greenhouseEffect(false), boilPoint(0), albedo(0), exosphericTemp(0), estimatedTemp(0), estimatedTerrTemp(0), surfTemp(0), greenhsRise(0), highTemp(0), lowTemp(0), maxTemp(0), minTemp(0), hydrosphere(0), cloudCover(0), iceCover(0), inclination(0), ascendingNode(0), longitudeOfPericenter(0), meanLongitude(0), first_moon(moon), next_planet(next), reconnect_to(next), first_moon_backup(moon), type(tUnknown), deleteable(true) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}

planet::~planet() {
  planet* node = nullptr;
  planet* next = nullptr;

  a = 0;
  e = 0;
  axialTilt = 0;
  gasGiant = false;
  dustMass = 0;
  gasMass = 0;
  imf = 0;
  rmf = 0;
  cmf = 0;
  moonA = 0;
  moonE = 0;
  coreRadius = 0;
  radius = 0;
  orbitZone = 0;
  density = 0;
  orbPeriod = 0;
  day = 0;
  resonantPeriod = false;
  escVelocity = 0;
  surfAccel = 0;
  surfGrav = 0;
  rmsVelocity = 0;
  molecWeight = 0;
  volatileGasInventory = 0;
  surfPressure = 0;
  greenhouseEffect = false;
  boilPoint = 0;
  albedo = 0;
  exosphericTemp = 0;
  estimatedTemp = 0;
  estimatedTerrTemp = 0;
  surfTemp = 0;
  greenhsRise = 0;
  highTemp = 0;
  lowTemp = 0;
  maxTemp = 0;
  minTemp = 0;
  hydrosphere = 0;
  cloudCover = 0;
  iceCover = 0;
  inclination = 0;
  ascendingNode = 0;
  longitudeOfPericenter = 0;
  meanLongitude = 0;
  while (!atmosphere.empty()) {
    atmosphere.pop_back();
  }
  if (first_moon != nullptr) {
    for (node = first_moon; node != nullptr; node = next) {
      next = node->next_planet;
      if (node->getDeletable()) {
        delete node;
      } else {
        node->next_planet = node->reconnect_to;
      }
    }
  }
  if (first_moon_backup != nullptr) {
    first_moon = first_moon_backup;
  }
}

void planet::addGas(gas g) {
  bool unique = true;
  // this shouldn't be necessary but for some weird reason, it is.
  for (auto & i : atmosphere) {
    if (g.getNum() == i.getNum()) {
      unique = false;
      break;
    }
  }
  if (unique) {
    atmosphere.push_back(g);
    quicksort(atmosphere, 0, atmosphere.size());
  }
}

/*void planet::addMoon(planet* moon)
{
  temp_moon.push_back(moon);
}*/

void planet::clearGases() {
  while (!atmosphere.empty()) {
    atmosphere.pop_back();
  }
}

/*void planet::deleteMoon(int i)
{
  planet *the_moon = temp_moon[i];
  vector<planet *>::iterator it;
  if (temp_moon[i] != NULL)
  {
    if (temp_moon[i]->next_planet == NULL)
    {
      delete temp_moon[i];
    }
    else
    {
      for (it = temp_moon.begin(); it != temp_moon.end(); it++)
      {
        if (*it == the_moon)
        {
          temp_moon.erase(it);
          break;
        }
      }
    }
  }
}*/

void planet::estimateMass() {
  dustMass = quintic_trend(0.126438418015041, -0.971586985798294,
                           2.30299559510187, 0.0114340742264797,
                           -0.747842501143578, 0.27996106732024, knownRadius);
}

auto planet::getA() -> long double { return a; }

auto planet::getAlbedo() -> long double { return albedo; }

auto planet::getAscendingNode() -> long double { return ascendingNode; }

auto planet::getAxialTilt() -> long double { return axialTilt; }

auto planet::getBoilPoint() -> long double { return boilPoint; }

auto planet::getCloudCover() -> long double { return cloudCover; }

auto planet::getCmf() -> long double { return cmf; }

auto planet::getCoreRadius() -> long double { return coreRadius; }

auto planet::getDay() -> long double { return day; }

auto planet::getDeletable() -> bool { return deleteable; }

auto planet::getDensity() -> long double { return density; }

auto planet::getDustMass() -> long double { return dustMass; }

auto planet::getE() -> long double { return e; }

auto planet::getEquatrorialRadius() -> long double {
  return radius * (1.0 + getOblateness());
}

auto planet::getEscVelocity() -> long double { return escVelocity; }

auto planet::getEsi() -> long double { return esi; }

auto planet::getEstimatedTemp() -> long double { return estimatedTemp; }

auto planet::getEstimatedTerrTemp() -> long double { return estimatedTerrTemp; }

auto planet::getExosphericTemp() -> long double { return exosphericTemp; }

auto planet::getGas(int i) -> gas { return atmosphere[i]; }

auto planet::getGasGiant() -> bool { return gasGiant; }

auto planet::getGasMass() -> long double { return gasMass; }

auto planet::getGreenhouseEffect() -> bool { return greenhouseEffect; }

auto planet::getGreenhsRise() -> long double { return greenhsRise; }

auto planet::getHighTemp() -> long double { return highTemp; }

auto planet::getHydrosphere() -> long double { return hydrosphere; }

auto planet::getHza() -> long double { return hza; }

auto planet::getHzc() -> long double { return hzc; }

auto planet::getHzd() -> long double { return hzd; }

auto planet::getIceCover() -> long double { return iceCover; }

auto planet::getImf() -> long double { return imf; }

auto planet::getInclination() -> long double { return inclination; }

auto planet::getKnownRadius() -> long double { return knownRadius; }

auto planet::getLongitudeOfPericenter() -> long double { return longitudeOfPericenter; }

auto planet::getLowTemp() -> long double { return lowTemp; }

auto planet::getMass() -> long double { return dustMass + gasMass; }

auto planet::getMaxTemp() -> long double { return maxTemp; }

auto planet::getMeanLongitude() -> long double { return meanLongitude; }

auto planet::getMinorMoons() -> int { return minorMoons; }

auto planet::getMinTemp() -> long double { return minTemp; }

auto planet::getMolecWeight() -> long double { return molecWeight; }

/*planet* planet::getMoon(int i)
{
  return temp_moon[i];
}*/

auto planet::getMoonA() -> long double { return moonA; }

/*int planet::getMoonCount()
{
  return temp_moon.size();
}*/

auto planet::getMoonE() -> long double { return moonE; }

auto planet::getOblateness() -> long double {
  // return oblateness;
  long double multiplier = 0;
  long double result = 0;
  long double mass_in_eu = 0;
  long double planetary_mass_in_grams = 0;
  long double equatorial_radius_in_cm = 0;
  long double k2 = 0;
  long double ang_velocity = 0;
  planet* the_planet(this);

  if (type == tBrownDwarf || type == tGasGiant || type == tSubGasGiant ||
      type == tSubSubGasGiant) {
    mass_in_eu = (dustMass + gasMass) * SUN_MASS_IN_EARTH_MASSES;
    if (type == tSubGasGiant || type == tSubSubGasGiant) {
      multiplier = 4.94E-12;
    } else if (type == tGasGiant) {
      multiplier = 5.56E-12;
    }
    result = multiplier * (pow(radius, 3.0) / (mass_in_eu * std::pow(day, 2.0)));
  } else {
    planetary_mass_in_grams = getMass() * SOLAR_MASS_IN_GRAMS;
    equatorial_radius_in_cm = radius * CM_PER_KM;
    k2 = calculate_moment_of_inertia_coeffient(the_planet);
    while (day == 0) {
      cerr << "Error! The day is 0 hours long!\n";
      exit(EXIT_FAILURE);
    }
    ang_velocity = RADIANS_PER_ROTATION / (SECONDS_PER_HOUR * day);
    result = calcOblateness_improved(ang_velocity, planetary_mass_in_grams,
                                     equatorial_radius_in_cm, k2);
  }
  if (result > 0.5) {
    result = 0.5;
  }
  return result;
}

auto planet::getOrbitZone() -> int { return orbitZone; }

auto planet::getOrbPeriod() -> long double { return orbPeriod; }

auto planet::getPlanetNo() -> int { return planetNo; }

auto planet::getPolarRadius() -> long double {
  return radius * (1.0 - getOblateness());
}

auto planet::getRadius() -> long double { return radius; }

auto planet::getResonantPeriod() -> bool { return resonantPeriod; }

auto planet::getRmf() -> long double { return rmf; }

auto planet::getRmsVelocity() -> long double { return rmsVelocity; }

auto planet::getSph() -> long double { return sph; }

auto planet::getSurfAccel() -> long double { return surfAccel; }

auto planet::getSurfGrav() -> long double { return surfGrav; }

auto planet::getSurfPressure() -> long double { return surfPressure; }

auto planet::getSurfTemp() -> long double { return surfTemp; }

auto planet::getTheSun() -> sun& { return theSun; }

auto planet::getType() -> planet_type { return type; }

auto planet::getVolatileGasInventory() -> long double { return volatileGasInventory; }

void planet::setA(long double a2) { a = a2; }

void planet::setAlbedo(long double al) { albedo = al; }

void planet::setAscendingNode(long double as) { ascendingNode = as; }

void planet::setAxialTilt(long double ax) { axialTilt = fix_inclination(ax); }

void planet::setBoilPoint(long double b) { boilPoint = b; }

void planet::setCloudCover(long double c) { cloudCover = c; }

void planet::setCmf(long double c) { cmf = c; }

void planet::setCoreRadius(long double c) { coreRadius = c; }

void planet::setDay(long double d) { day = d; }

void planet::setDeletable(bool d) { deleteable = d; }

void planet::setDensity(long double d) {
  if (d < 0) {
    d *= 1;
  }
  density = d;
}

void planet::setDustMass(long double d) { dustMass = d; }

void planet::setE(long double e2) { e = e2; }

void planet::setEscVelocity(long double es) { escVelocity = es; }

void planet::setEsi(long double es) { esi = es; }

void planet::setEstimatedTemp(long double es) { estimatedTemp = es; }

void planet::setEstimatedTerrTemp(long double es) { estimatedTerrTemp = es; }

void planet::setExosphericTemp(long double ex) { exosphericTemp = ex; }

void planet::setGasGiant(bool g) { gasGiant = g; }

void planet::setGasMass(long double g) {
  if (g > 0.0)  // make sure the planet is set as a gas giant if gas makes up
                // part of its mass
  {
    gasGiant = true;
  } else {
    gasGiant = false;
  }
  gasMass = g;
}

void planet::setGreenhouseEffect(bool g) { greenhouseEffect = g; }

void planet::setGreenhsRise(long double g) { greenhsRise = g; }

void planet::setHighTemp(long double h) { highTemp = h; }

void planet::setHydrosphere(long double h) { hydrosphere = h; }

void planet::setHza(long double h) { hza = h; }

void planet::setHzc(long double h) { hzc = h; }

void planet::setHzd(long double h) { hzd = h; }

void planet::setIceCover(long double i) {
  if (i > 1.0) {
    i = 0.0;
  }
  iceCover = i;
}

void planet::setImf(long double i) {
  if (i > 1.0) {
    i = 0.0;
  }
  imf = i;
}

void planet::setInclination(long double in) {
  inclination = fix_inclination(in);
}

void planet::setKnownRadius(long double k) {
  long double water_min = NAN, water_max = NAN, rock_min = NAN, rock_max = NAN;
  knownRadius = k;
  estimateMass();
  if (dustMass > EM(2.0)) {
  } else {
  }
}

void planet::setLongitudeOfPericenter(long double lon) {
  longitudeOfPericenter = lon;
}

void planet::setLowTemp(long double l) { lowTemp = l; }

void planet::setMaxTemp(long double m) { maxTemp = m; }

void planet::setMeanLongitude(long double me) { meanLongitude = me; }

void planet::setMinorMoons(int m) { minorMoons = m; }

void planet::setMinTemp(long double m) { minTemp = m; }

void planet::setMolecWeight(long double m) { molecWeight = m; }

void planet::setMoonA(long double m) { moonA = m; }

void planet::setMoonE(long double m) { moonE = m; }

auto planet::getNumGases() -> int { return atmosphere.size(); }

void planet::setOblateness(long double o) { oblateness = o; }

void planet::setOrbitZone(int o) { orbitZone = o; }

void planet::setOrbPeriod(long double o) { orbPeriod = o; }

void planet::setPlanetNo(int n) { planetNo = n; }

void planet::setRadius(long double r) {
  if (r < 0) {
    r *= -1;
  }
  radius = r;
}

void planet::setResonantPeriod(bool r) { resonantPeriod = r; }

void planet::setRmf(long double r) { rmf = r; }

void planet::setRmsVelocity(long double r) { rmsVelocity = r; }

void planet::setSph(long double s) { sph = s; }

void planet::setSurfAccel(long double s) { surfAccel = s; }

void planet::setSurfGrav(long double s) { surfGrav = s; }

void planet::setSurfPressure(long double s) { surfPressure = s; }

void planet::setSurfTemp(long double s) { surfTemp = s; }

void planet::setTheSun(sun& s) { theSun = s; }

void planet::setType(planet_type t) { type = t; }

void planet::setVolatileGasInventory(long double v) {
  volatileGasInventory = v;
}

void planet::sortMoons() {
  planet* prev = nullptr;
  planet* next = nullptr;
  planet* node = nullptr;
  // planet *temp = NULL;
  vector<planet*> temp_vector;
  long double roche_limit = 0;
  long double hill_sphere = 0;
  for (node = first_moon; node != nullptr; node = next) {
    next = node->next_planet;
    node->next_planet = nullptr;
    temp_vector.push_back(node);
  }
  first_moon = nullptr;
  for (auto & i : temp_vector) {
    // we shouldn't have to do this here but some moons that shouldn't
    // be possible somehow slip throw the cracks...
    hill_sphere =
        a * KM_PER_AU * std::pow(getMass() / (3.0 * theSun.getMass()), 1.0 / 3.0);
    roche_limit =
        2.44 * radius * std::pow(density / i->getDensity(), 1.0 / 3.0);
    if ((roche_limit * 1.5) >= (hill_sphere / 3.0)) {
      delete i;
    } else if ((i->getMoonA() * KM_PER_AU) < (roche_limit * 1.5)) {
      delete i;
    } else if ((i->getMoonA() * KM_PER_AU) > (hill_sphere / 3.0)) {
      delete i;
    }
    // if it passed all those tests, we can place the moon.
    else {
      if (first_moon == nullptr) {
        first_moon = i;
      } else if (i->getMoonA() < first_moon->getMoonA()) {
        i->next_planet = first_moon;
        first_moon = i;
      } else if (first_moon->next_planet == nullptr) {
        first_moon->next_planet = i;
        i->next_planet = nullptr;
      } else {
        next = first_moon;
        while (next != nullptr && next->getMoonA() < i->getMoonA()) {
          prev = next;
          next = next->next_planet;
        }
        i->next_planet = next;
        if (prev != nullptr) {
          prev->next_planet = i;
        }
      }
    }
  }
}

auto planet::operator<(planet& right) -> bool {
  if (a < right.a) {
    return true;
  } else if (moonA < right.moonA) {
    return true;
  } else if (e < right.e) {
    return true;
  } else if (moonE < right.moonE) {
    return true;
  }
  return false;
}

auto planet::operator==(planet& right) -> bool {
  if (a == right.a && moonA == right.moonA && e == right.e &&
      moonE == right.moonE) {
    return true;
  }
  return false;
}

dust::dust() : innerEdge(0), outerEdge(0), dustPresent(true), gasPresent(true), next_band(NULL) {
  
  
  
  
  
}

dust::~dust() {
  innerEdge = 0;
  outerEdge = 0;
  dustPresent = false;
  gasPresent = false;
  // next_band = NULL;
}

auto dust::getDustPresent() -> bool { return dustPresent; }

auto dust::getGasPresent() -> bool { return gasPresent; }

auto dust::getInnerEdge() -> long double {
  if (innerEdge < outerEdge) {
    return innerEdge;
  } else {
    innerEdge = 0;
    return innerEdge;
  }
}

auto dust::getOuterEdge() -> long double { return outerEdge; }

void dust::setDustPresent(bool d) { dustPresent = d; }

void dust::setGasPresent(bool g) { gasPresent = g; }

void dust::setInnerEdge(long double i) { innerEdge = i; }

void dust::setOuterEdge(long double o) { outerEdge = o; }

gen::gen() : dusts(NULL), planets(NULL), next(NULL) {
  
  
  
}

gen::~gen() {
  // dusts = NULL;
  // planets = NULL;
  // next = NULL;
}

gas::gas() : num(0), surfPressure(0) {
  
  
}

auto gas::getNum() -> int { return num; }

auto gas::getSurfPressure() -> long double { return surfPressure; }

void gas::setNum(int n) { num = n; }

void gas::setSurfPressure(long double s) { surfPressure = s; }

auto gas::operator<(gas& right) -> bool {
  if (surfPressure > right.surfPressure) {
    return true;
  }
  return false;
}

auto gas::operator==(gas& right) -> bool {
  if (surfPressure == right.surfPressure) {
    return true;
  }
  return false;
}

auto operator<<(ostream& strm, gas& obj) -> ostream& {
  strm << obj.getNum() << " - " << obj.getSurfPressure() << endl;
  return strm;
}