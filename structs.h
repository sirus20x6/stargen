#ifndef STRUCTS_H
#define STRUCTS_H
#pragma once

#include <string>
#include <vector>
//#include <boost/concept_check.hpp>
#include "c_structs.h"

using namespace std;

using planet_type = enum planet_type
{
  tUnknown,
  tRock,
  tVenusian,
  tTerrestrial,
  tGasGiant,
  tMartian,
  tWater,
  tIce,
  tSubGasGiant,
  tSubSubGasGiant,
  tAsteroids,
  t1Face,
  tBrownDwarf,  //seb
  tIron,
  tCarbon,
  tOil
};

class gas;
auto operator<<(ostream &, gas &) -> ostream &;

class gas
{
private:
  int num;
  long double surfPressure;
public:
  gas();
  void setNum(int);
  auto getNum() -> int;
  void setSurfPressure(long double);
  auto getSurfPressure() -> long double;
  auto operator<(gas &) -> bool;
  auto operator==(gas &) -> bool;
  friend auto operator<<(ostream &, gas &) -> ostream &;
};

class sun
{
private:
  long double luminosity;
  long double mass;
  long double effTemp;
  string specType;
  long double age;
  string name;
  bool isCircumbinary;
  long double secondaryMass;
  long double secondaryLuminosity;
  long double secondaryEffTemp;
  long double combinedEffTemp;
  string secondarySpecType;
  long double seperation;
  long double eccentricity;
public:
  sun();
  sun(long double, long double, long double, string, long double, string);
  sun(sun &);
  sun(const sun&);
  void setLuminosity(long double);
  auto getLuminosity() -> long double;
  void setMass(long double);
  auto getMass() -> long double;
  void setEffTemp(long double);
  auto getEffTemp() -> long double;
  void setSpecType(string);
  auto getSpecType() -> string;
  auto getLife() -> long double;
  void setAge(long double);
  auto getAge() -> long double;
  void setName(string);
  auto getName() -> string;
  auto getREcosphere(long double) -> long double;
  void setIsCircumbinary(bool);
  auto getIsCircumbinary() -> bool;
  void setSecondaryMass(long double);
  auto getSecondaryMass() -> long double;
  void setSecondaryLuminosity(long double);
  auto getSecondaryLuminosity() -> long double;
  void setSecondaryEffTemp(long double);
  auto getSecondaryEffTemp() -> long double;
  void setSecondarySpecType(string);
  auto getSecondarySpecType() -> string;
  auto getCombinedLuminosity() -> long double;
  auto getCombinedMass() -> long double;
  void setSeperation(long double);
  auto getSeperation() -> long double;
  void setEccentricity(long double);
  auto getEccentricity() -> long double;
  auto getSecondaryLife() -> long double;
  auto getMinStableDistance() -> long double;
  auto getCombinedEffTemp() -> long double;
  //sun operator=(sun &);
};

class planet
{
private:
  int planetNo;
  long double a;					/* semi-major axis of solar orbit (in AU)*/
  long double e;					/* eccentricity of solar orbit		 */
  long double axialTilt;			/* units of degrees					 */
  bool gasGiant;			/* TRUE if the planet is a gas giant */
  long double dustMass;			/* mass, ignoring gas				 */
  long double gasMass;			/* mass, ignoring dust				 */
  long double imf;				/* ice mass fraction */
  long double rmf;				/* rock mass fraction */
  long double cmf;				/* fraction of rock that's carbon instead of silicate*/
  long double moonA;				/* semi-major axis of lunar orbit (in AU)*/
  long double moonE;				/* eccentricity of lunar orbit		 */
  long double coreRadius;		/* radius of the rocky core (in km)	 */
  long double radius;				/* equatorial radius (in km)		 */
  int orbitZone;			/* the 'zone' of the planet			 */
  long double density;			/* density (in g/cc)				 */
  long double orbPeriod;			/* length of the local year (days)	 */
  long double day;				/* length of the local day (hours)	 */
  bool resonantPeriod;	/* TRUE if in resonant rotation		 */
  long double escVelocity;		/* units of cm/sec					 */
  long double surfAccel;			/* units of cm/sec2					 */
  long double surfGrav;			/* units of Earth gravities			 */
  long double rmsVelocity;		/* units of cm/sec					 */
  long double molecWeight;		/* smallest molecular weight retained*/
  long double volatileGasInventory;
  long double surfPressure;		/* units of millibars (mb)			 */
  bool greenhouseEffect;	/* runaway greenhouse effect?		 */
  long double boilPoint;			/* the boiling point of water (Kelvin)*/
  long double albedo;				/* albedo of the planet				 */
  long double exosphericTemp;	/* units of degrees Kelvin			 */
  long double estimatedTemp;     /* quick non-iterative estimate (K)  */
  long double estimatedTerrTemp;/* for terrestrial moons and the like*/
  long double surfTemp;			/* surface temperature in Kelvin	 */
  long double greenhsRise;		/* Temperature rise due to greenhouse */
  long double highTemp;			/* Day-time temperature              */
  long double lowTemp;			/* Night-time temperature			 */
  long double maxTemp;			/* Summer/Day						 */
  long double minTemp;			/* Winter/Night						 */
  long double hydrosphere;		/* fraction of surface covered		 */
  long double cloudCover;		/* fraction of surface covered		 */
  long double iceCover;			/* fraction of surface covered		 */
  sun theSun;
  vector<gas> atmosphere;
  planet_type type;				/* Type code						 */
  int minorMoons;
  long double inclination;
  long double ascendingNode;
  long double longitudeOfPericenter;
  long double meanLongitude;
  long double hzd;
  long double hzc;
  long double hza;
  long double esi;
  long double sph;
  long double oblateness;
  bool deleteable;
  long double knownRadius;
  void estimateMass();
  //vector<planet *> temp_moon;
public:
  planet();
  planet(int, long double, long double, long double, bool, long double, long double, planet*, planet*);
  ~planet();
  void setPlanetNo(int);
  auto getPlanetNo() -> int;
  void setA(long double);
  auto getA() -> long double;
  void setE(long double);
  auto getE() -> long double;
  void setAxialTilt(long double);
  auto getAxialTilt() -> long double;
  auto getMass() -> long double;
  void setGasGiant(bool);
  auto getGasGiant() -> bool;
  void setDustMass(long double);
  auto getDustMass() -> long double;
  void setGasMass(long double);
  auto getGasMass() -> long double;
  void setImf(long double);
  auto getImf() -> long double;
  void setRmf(long double);
  auto getRmf() -> long double;
  void setCmf(long double);
  auto getCmf() -> long double;
  void setMoonA(long double);
  auto getMoonA() -> long double;
  void setMoonE(long double);
  auto getMoonE() -> long double;
  void setCoreRadius(long double);
  auto getCoreRadius() -> long double;
  void setRadius(long double);
  auto getRadius() -> long double;
  void setOrbitZone(int);
  auto getOrbitZone() -> int;
  void setDensity(long double);
  auto getDensity() -> long double;
  void setOrbPeriod(long double);
  auto getOrbPeriod() -> long double;
  void setDay(long double);
  auto getDay() -> long double;
  void setResonantPeriod(bool);
  auto getResonantPeriod() -> bool;
  void setEscVelocity(long double);
  auto getEscVelocity() -> long double;
  void setSurfAccel(long double);
  auto getSurfAccel() -> long double;
  void setSurfGrav(long double);
  auto getSurfGrav() -> long double;
  void setRmsVelocity(long double);
  auto getRmsVelocity() -> long double;
  void setMolecWeight(long double);
  auto getMolecWeight() -> long double;
  void setVolatileGasInventory(long double);
  auto getVolatileGasInventory() -> long double;
  void setSurfPressure(long double);
  auto getSurfPressure() -> long double;
  void setGreenhouseEffect(bool);
  auto getGreenhouseEffect() -> bool;
  void setBoilPoint(long double);
  auto getBoilPoint() -> long double;
  void setAlbedo(long double);
  auto getAlbedo() -> long double;
  void setExosphericTemp(long double);
  auto getExosphericTemp() -> long double;
  void setEstimatedTemp(long double);
  auto getEstimatedTemp() -> long double;
  void setEstimatedTerrTemp(long double);
  auto getEstimatedTerrTemp() -> long double;
  void setSurfTemp(long double);
  auto getSurfTemp() -> long double;
  void setGreenhsRise(long double);
  auto getGreenhsRise() -> long double;
  void setHighTemp(long double);
  auto getHighTemp() -> long double;
  void setLowTemp(long double);
  auto getLowTemp() -> long double;
  void setMaxTemp(long double);
  auto getMaxTemp() -> long double;
  void setMinTemp(long double);
  auto getMinTemp() -> long double;
  void setHydrosphere(long double);
  auto getHydrosphere() -> long double;
  void setCloudCover(long double);
  auto getCloudCover() -> long double;
  void setIceCover(long double);
  auto getIceCover() -> long double;
  void setType(planet_type);
  auto getType() -> planet_type;
  void setTheSun(sun &);
  auto getTheSun() -> sun &;
  auto getNumGases() -> int;
  void addGas(gas);
  auto getGas(int) -> gas;
  void setMinorMoons(int);
  auto getMinorMoons() -> int;
  void setInclination(long double);
  auto getInclination() -> long double;
  void setAscendingNode(long double);
  auto getAscendingNode() -> long double;
  void setLongitudeOfPericenter(long double);
  auto getLongitudeOfPericenter() -> long double;
  void setMeanLongitude(long double);
  auto getMeanLongitude() -> long double;
  void clearGases();
  void setHzd(long double);
  auto getHzd() -> long double;
  void setHzc(long double);
  auto getHzc() -> long double;
  void setHza(long double);
  auto getHza() -> long double;
  void setEsi(long double);
  auto getEsi() -> long double;
  void setSph(long double);
  auto getSph() -> long double;
  void setOblateness(long double);
  auto getOblateness() -> long double;
  auto getEquatrorialRadius() -> long double;
  auto getPolarRadius() -> long double;
  //void addMoon(planet *);
  //planet *getMoon(int);
  //int getMoonCount();
  //void deleteMoon(int);
  void setDeletable(bool);
  auto getDeletable() -> bool;
  void setKnownRadius(long double);
  auto getKnownRadius() -> long double;
  void sortMoons();
  auto operator<(planet &) -> bool;
  auto operator==(planet &) -> bool;
  planet *first_moon;
  planet *next_planet;
  planet *reconnect_to;
  planet *first_moon_backup;
};

class dust
{
private:
  long double innerEdge;
  long double outerEdge;
  bool dustPresent;
  bool gasPresent;
public:
  dust();
  ~dust();
  void setInnerEdge(long double);
  auto getInnerEdge() -> long double;
  void setOuterEdge(long double);
  auto getOuterEdge() -> long double;
  void setDustPresent(bool);
  auto getDustPresent() -> bool;
  void setGasPresent(bool);
  auto getGasPresent() -> bool;
  dust *next_band;
};

class star;

auto operator<<(ostream &, star &) -> ostream &;

class star
{
private:
  long double luminosity{};
  long double mass{};
  long double eff_temp{};
  string spec_type;
  long double mass2{};
  long double eccentricity{};
  long double distance{};
  long double inc{};
  long double an{};
  planet *known_planets{};
  string desig;
  bool in_celestia{};
  string name;
  int extra_spaces{};
  void calcLuminosity();
  void calcMass();
  void calcEffTemp();
  bool isCircumbinary{};
  long double eff_temp2{};
  string spec_type2;
  long double luminosity2{};
public:
  star()
  = default;
  star(long double, long double, long double, string, long double, long double, long double, long double, long double, planet *, string, bool, string);
  star(star &);
  star(const star &);
  star(star2 &);
  ~star();
  void setExtraSpaces(int);
  auto getName() -> string;
  auto getExtraSpaces() -> int;
  auto getInc() -> long double;
  auto getAn() -> long double;
  auto getKnownPlanets() -> planet *;
  auto getInCelestia() -> bool;
  auto getMass() -> long double;
  auto getLuminosity() -> long double;
  auto getEffTemp() -> long double;
  auto getSpecType() -> string;
  auto getDesig() -> string;
  void setDistance(long double);
  auto getDistance() -> long double;
  void setEccentricity(long double);
  auto getEccentricity() -> long double;
  void setMass2(long double);
  auto getMass2() -> long double;
  void setLuminosity2(long double);
  auto getLuminosity2() -> long double;
  void setEffTemp2(long double);
  auto getEffTemp2() -> long double;
  void setSpecType2(string);
  auto getSpecType2() -> string;
  void setIsCircumbinary(bool);
  auto getIsCircumbinary() -> bool;
  auto operator=(star &) -> star;
  //star operator=(const star);
  auto operator<(star &) -> bool;
  auto operator==(star &) -> bool;
  friend auto operator<<(ostream &, star &) -> ostream &;
};

class catalog;
auto operator<<(ostream &, catalog &) -> ostream &;

class catalog
{
private:
  int extra_spaces;
  string arg;
  vector<star> stars;
public:
  catalog();
  catalog(string);
  ~catalog();
  void setArg(string);
  void addStar(star &);
  auto getArg() -> string;
  auto count() -> int;
  auto operator[](const int &) -> star &;
  auto operator=(catalog &) -> catalog;
  friend auto operator<<(ostream &, catalog &) -> ostream &;
  void sort();
};

class gen
{
public:
  gen();
  ~gen();
  dust *dusts;
  planet *planets;
  gen *next;
};

class Chemicle;
auto operator<<(ostream &, Chemicle &) -> ostream &;

class Chemicle
{
private:
  int num;
  string symbol;
  string htmlSymbol;
  string name;
  long double weight;
  long double melt;
  long double boil;
  long double density;
  long double pzero;
  long double c;
  long double n;
  long double abunde;
  long double abunds;
  long double reactivity;
  long double maxIpp;
  long double minIpp;
  int nameSpaces;
  int symbolSpaces;
  void fixMelt();
  void fixBoil();
  void fixMaxIpp();
  void fixMinIpp();
  void fixAbunde();
  void fixAbunds();
  void setSpaces();
public:
  Chemicle();
  Chemicle(int, string, string, string, long double, long double, long double, long double, long double, long double, long double, long double, long double, long double, long double, long double);
  Chemicle(Chemicle &);
  Chemicle(const Chemicle &);
  ~Chemicle();
  auto getNum() -> int;
  auto getSymbol() -> string;
  auto getHtmlSymbol() -> string;
  auto getName() -> string;
  auto getWeight() -> long double;
  void setMelt(long double);
  auto getMelt() -> long double;
  void setBoil(long double);
  auto getBoil() -> long double;
  auto getDensity() -> long double;
  auto getPzero() -> long double;
  auto getC() -> long double;
  auto getN() -> long double;
  void setAbunde(long double);
  auto getAbunde() -> long double;
  void setAbunds(long double);
  auto getAbunds() -> long double;
  auto getReactivity() -> long double;
  void setMaxIpp(long double);
  auto getMaxIpp() -> long double;
  void setMinIpp(long double);
  auto getMinIpp() -> long double;
  auto getNameSpaces() -> int;
  auto getSymbolSpaces() -> int;
  auto operator=(Chemicle &) -> Chemicle;
  auto operator<(Chemicle &) -> bool;
  auto operator==(Chemicle &) -> bool;
  friend auto operator<<(ostream &, Chemicle &) -> ostream &;
};

class ChemTable;
auto operator<<(ostream &, ChemTable &) -> ostream &;

class ChemTable
{
private:
  vector<Chemicle> chemicles;
public:
  ChemTable()
  = default;
  void addChemicle(Chemicle);
  auto count() -> int;
  auto operator[](const int &) -> Chemicle &;
  friend auto operator<<(ostream &, ChemTable &) -> ostream &;
};

#endif
