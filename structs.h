#ifndef STRUCTS_H
#define STRUCTS_H
#pragma once

#include <iosfwd>  // for ostream, std
#include <string>  // for string, basic_string
#include <vector>  // for vector
#include <iostream>
#include <memory>
class ChemTable;  // lines 496-496
class Chemical;  // lines 427-427
class catalog;  // lines 395-395
class gas;  // lines 30-30
class star;  // lines 326-326
struct star2;

using namespace std;

using planet_type = enum planet_type {
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
  tBrownDwarf,  // seb
  tIron,
  tCarbon,
  tOil
};

class gas;
auto operator<<(ostream &, gas &) -> ostream &;

class gas {
 private:
  int num{0};
  long double surfPressure{0};

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

class sun {
 private:
  long double luminosity{0};
  long double mass{0};
  long double effTemp{0};
  string specType{""};
  long double age{0};
  string name{""};
  bool isCircumbinary{false};
  long double secondaryMass{0};
  long double secondaryLuminosity{0};
  long double secondaryEffTemp{0};
  long double combinedEffTemp{0};
  string secondarySpecType{""};
  long double seperation{};
  long double eccentricity{};

 public:
  sun();
  sun(long double, long double, long double, string, long double, string);
  sun(sun &);
  sun(const sun &);
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
  // sun operator=(sun &);
};

class planet {
 private:
  int planetNo{0};
  long double a{0};         /* semi-major axis of solar orbit (in AU)*/
  long double e{0};         /* eccentricity of solar orbit		 */
  long double axialTilt{0}; /* units of degrees */
  bool gasGiant{false};         /* TRUE if the planet is a gas giant */
  long double dustMass{0};  /* mass, ignoring gas				 */
  long double gasMass{0};   /* mass, ignoring dust				 */
  long double imf{0};       /* ice mass fraction */
  long double rmf{0};       /* rock mass fraction */
  long double cmf{0};       /* fraction of rock that's carbon instead of silicate*/
  long double moonA{0};     /* semi-major axis of lunar orbit (in AU)*/
  long double moonE{0};     /* eccentricity of lunar orbit		 */
  long double coreRadius{0};  /* radius of the rocky core (in km)	 */
  long double radius{0};      /* equatorial radius (in km)		 */
  int orbitZone{0};           /* the 'zone' of the planet			 */
  long double density{0};     /* density (in g/cc)				 */
  long double orbPeriod{0};   /* length of the local year (days)	 */
  long double day{0};         /* length of the local day (hours)	 */
  bool resonantPeriod{false};     /* TRUE if in resonant rotation		 */
  long double escVelocity{0}; /* units of cm/sec */
  long double surfAccel{0};   /* units of cm/sec2   */
  long double surfGrav{0};    /* units of Earth gravities			 */
  long double rmsVelocity{0}; /* units of cm/sec */
  long double molecWeight{0}; /* smallest molecular weight retained*/
  long double volatileGasInventory{0};
  long double surfPressure{0};      /* units of millibars (mb)			 */
  bool greenhouseEffect{false};         /* runaway greenhouse effect?		 */
  long double boilPoint{0};         /* the boiling point of water (Kelvin)*/
  long double albedo{0};            /* albedo of the planet				 */
  long double exosphericTemp{0};    /* units of degrees Kelvin    */
  long double estimatedTemp{0};     /* quick non-iterative estimate (K)  */
  long double estimatedTerrTemp{0}; /* for terrestrial moons and the like*/
  long double surfTemp{0};          /* surface temperature in Kelvin	 */
  long double greenhsRise{0};       /* Temperature rise due to greenhouse */
  long double highTemp{0};          /* Day-time temperature              */
  long double lowTemp{0};           /* Night-time temperature			 */
  long double maxTemp{0};           /* Summer/Day           */
  long double minTemp{0};           /* Winter/Night           */
  long double hydrosphere{0};       /* fraction of surface covered		 */
  long double cloudCover{0};        /* fraction of surface covered		 */
  long double iceCover{0};          /* fraction of surface covered		 */
  sun theSun;
  vector<gas> atmosphere;
  planet_type type{tUnknown}; /* Type code */
  int minorMoons{};
  long double inclination{0};
  long double ascendingNode{0};
  long double longitudeOfPericenter{0};
  long double meanLongitude{0};
  long double hzd{};
  long double hzc{};
  long double hza{};
  long double esi{};
  long double sph{};
  long double oblateness{};
  bool deleteable{true};
  long double knownRadius{};
  void estimateMass();
  // vector<planet *> temp_moon;
 public:
  planet();
  planet(int, long double, long double, long double, bool, long double,
         long double, planet *, planet *);
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
  // void addMoon(planet *);
  // planet *getMoon(int);
  // int getMoonCount();
  // void deleteMoon(int);
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

class dust {
private:
    long double innerEdge{0};
    long double outerEdge{0};
    bool dustPresent{true};
    bool gasPresent{true};
    bool inUse{false};

    static int activeCount;
    static int peakCount;

public:
    dust() = default;

    void reset() {
        innerEdge = 0;
        outerEdge = 0;
        dustPresent = true;
        gasPresent = true;
        inUse = false;
    }

    static void resetCounts() {
        activeCount = 0;
        peakCount = 0;
        std::cout << "Dust counts reset." << std::endl;
    }

    static int getActiveCount() { return activeCount; }
    static int getPeakCount() { return peakCount; }

    friend class DustPool;

// Initialize static members

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

class DustPool {
private:
    static const int POOL_SIZE = 20;  // Adjust based on your maximum observed usage
    std::vector<std::unique_ptr<dust>> pool;

public:
    DustPool() {
        for (int i = 0; i < POOL_SIZE; ++i) {
            pool.push_back(std::make_unique<dust>());
        }
        std::cout << "Dust pool initialized with " << POOL_SIZE << " objects." << std::endl;
    }

    dust* acquireDust() {
        for (auto& dustPtr : pool) {
            if (!dustPtr->inUse) {
                dustPtr->inUse = true;
                dust::activeCount++;
                if (dust::activeCount > dust::peakCount) {
                    dust::peakCount = dust::activeCount;
                }
                std::cout << "Dust object acquired. Active count: " << dust::activeCount 
                          << ", Peak count: " << dust::peakCount << std::endl;
                return dustPtr.get();
            }
        }
        std::cerr << "Warning: Dust pool exhausted!" << std::endl;
        return nullptr;
    }

    void releaseDust(dust* d) {
        if (d && d->inUse) {
            d->reset();
            d->inUse = false;
            dust::activeCount--;
            std::cout << "Dust object released. Active count: " << dust::activeCount << std::endl;
        }
    }

    ~DustPool() {
        std::cout << "Dust pool destroyed. Final active count: " << dust::activeCount 
                  << ", Peak count: " << dust::peakCount << std::endl;
    }
};



class star;

auto operator<<(ostream &, star &) -> ostream &;

class star {
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
  star() = default;
  star(long double, long double, long double, string, long double, long double,
       long double, long double, long double, planet *, string, bool, string);
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
  auto operator=(const star &) -> star;
  // star operator=(const star);
  auto operator<(star &) -> bool;
  auto operator==(const star &) -> bool;
  friend auto operator<<(ostream &, star &) -> ostream &;
};

class catalog;
auto operator<<(ostream &, catalog &) -> ostream &;

class catalog {
 private:
  int extra_spaces{0};
  string arg;
  vector<star> stars;

 public:
  catalog();
  catalog(string);
  ~catalog();
  void setArg(string);
  void addStar(star &);
  auto getArg() -> string;
  auto count() -> unsigned long;
  auto operator[](const int &) -> star &;
  auto operator=(catalog &) -> catalog&;
  friend auto operator<<(ostream &, catalog &) -> ostream &;
  void sort();
};

class gen {
 public:
  gen();
  ~gen();
  dust *dusts;
  planet *planets;
  gen *next;
};

class Chemical;
auto operator<<(ostream &, Chemical &) -> ostream &;

class Chemical {
 private:
  int num{0};
  string symbol{""};
  string htmlSymbol{""};
  string name{""};
  long double weight{0};
  long double melt{0};
  long double boil{0};
  long double density{0};
  long double pzero{0};
  long double c{0};
  long double n{0};
  long double abunde{0};
  long double abunds{0};
  long double reactivity{0};
  long double maxIpp{0};
  long double minIpp{0};
  int nameSpaces{};
  int symbolSpaces{};
  void fixMelt();
  void fixBoil();
  void fixMaxIpp();
  void fixMinIpp();
  void fixAbunde();
  void fixAbunds();
  void setSpaces();

 public:
  Chemical();
  Chemical(int, string, string, string, long double, long double, long double,
           long double, long double, long double, long double, long double,
           long double, long double, long double, long double);
  Chemical(Chemical &);
  Chemical(const Chemical &);
  ~Chemical();
  auto getNum() -> int { return num; }
  void setMelt(long double);
  void setBoil(long double);
  void setAbunde(long double);
  void setAbunds(long double);
  void setMaxIpp(long double);
  void setMinIpp(long double);
  auto getNameSpaces() -> int;
  auto getSymbolSpaces() -> int;
  auto operator=(Chemical const&) -> Chemical&;
  auto operator<(Chemical &) -> bool;
  auto operator==(Chemical &) -> bool;
  auto getAbunde() -> long double { return abunde; }
  auto getAbunds() -> long double { return abunds; }
  auto getBoil() -> long double { return boil; }
  auto getC() -> long double { return c; }
  auto getDensity() -> long double { return density; }
  auto getHtmlSymbol() -> string { return htmlSymbol; }
  auto getMaxIpp() -> long double { return maxIpp; }
  auto getMelt() -> long double { return melt; }
  auto getMinIpp() -> long double { return minIpp; }
  auto getN() -> long double { return n; }
  auto getName() -> string { return name; }
  auto getPzero() -> long double { return pzero; }
  auto getReactivity() -> long double { return reactivity; }
  auto getSymbol() -> string { return symbol; }
  auto getWeight() -> long double { return weight; }
  friend auto operator<<(ostream &, Chemical &) -> ostream &;
};

class ChemTable;
auto operator<<(ostream &, ChemTable &) -> ostream &;

class ChemTable {
 private:
  vector<Chemical> chemicles;

 public:
  ChemTable() = default;
  void addChemicle(const Chemical&);
  auto count() -> const unsigned long;
  auto operator[](const int &) -> Chemical &;
  friend auto operator<<(ostream &, ChemTable &) -> ostream &;
};

#endif
