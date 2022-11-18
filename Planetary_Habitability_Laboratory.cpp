#include "Planetary_Habitability_Laboratory.h"
#include "const.h"
#include "planets.h"

using namespace std;

catalog phl;

void initPlanetaryHabitabilityLaboratory()
{
  phl.setArg("PHL");
  // 			L			Mass			Eff_Temp	Spec Type	Mass2			Eccen.	SMAxis	 	inclination	ascending node	Planets			Designation		InCel	Name
  star sol(		1.0,			1.00,			5780,			(char*)"G2V",		0,			0,	0,		0,		0,		mercury,			(char*)"Sol",			true,		(char*)"The Solar System");
  phl.addStar(sol);
  star gliese667C(	0.0137,			0.33,			3350,			(char*)"M2V",		(0.73 + 0.69),		0,	230,		0,		0,		gliese667Cb,			(char*)"GJ 667C",		true,		(char*)"Gliese 667C");
  phl.addStar(gliese667C);
  star kepler62(	0.200138,		0.69,			4869,			(char*)"K2V",		0,			0,	0,		0,		0,		kepler62b,			(char*)"Kepler-62",		true,		(char*)"Kepler-62");
  phl.addStar(kepler62);
  star kepler283(	0.1045,			0.596,			4351,			(char*)"",		0,			0,	0,		0,		0,		kepler283b,			(char*)"Kepler-283",		true,		(char*)"Kepler-283");
  phl.addStar(kepler283);
  star kepler296(	0.091709,		0.61,			4292,			(char*)"",		0,			0,	0,		0,		0,		kepler296b,			(char*)"Kepler-296",		true,		(char*)"Kepler-296");
  phl.addStar(kepler296);
  star TAUCet(		0.52,			0.783,			5344,			(char*)"G8V",		0,			0,	0,		0,		0,		taucetb,			(char*)"TAU Cet",		true,		(char*)"Tau Ceti");
  phl.addStar(TAUCet);
  star gliese180(	0.013018,		0.43,			3371,			(char*)"M2V",		0,			0,	0,		0,		0,		gliese180b,			(char*)"GJ 180",		true,		(char*)"Gliese 180");
  phl.addStar(gliese180);
  star gliese581(	0.01209,		0.31,			3498,			(char*)"M3V",		0,			0,	0,	 	0,		0,		gliese581e,			(char*)"Gl 581",		true,		(char*)"Gliese 581");
  phl.addStar(gliese581);
  star gliese163(	0.021975,		0.4,			3500,			(char*)"M4V",		0,			0,	0,		0,		0,		gliese163b,			(char*)"GJ 163",		true,		(char*)"Gliese 163");
  phl.addStar(gliese163);
  star hd40307(		0.233304,		0.77,			4977,			(char*)"K2V",		0,			0,	0,		0,		0,		hd40307b,			(char*)"HD 40307",		true,		(char*)"HIP 27887");
  phl.addStar(hd40307);
  star kepler61(	0.089801,		0.64,			4017,			(char*)"K7V",		0,			0,	0,		0,		0,		kepler61b,			(char*)"Kepler-61",		true,		(char*)"Kepler-61");
  phl.addStar(kepler61);
  star gliese422(	0.011015,		0.32,			3323,			(char*)"M4V",		0,			0,	0,		0,		0,		gliese422b,			(char*)"GJ 422",		true,		(char*)"Gliese 422");
  phl.addStar(gliese422);
  star kepler22(	0.797227,		0.97,			5518,			(char*)"G5V",		0,			0,	0,		0,		0,		kepler22b,			(char*)"Kepler-22",		true,		(char*)"Kepler-22");
  phl.addStar(kepler22);
  star kepler298(	0.119959,		0.65,			4465,			(char*)"",		0,			0,	0,		0,		0,		kepler298b,			(char*)"Kepler-298",		true,		(char*)"Kepler-298");
  phl.addStar(kepler298);
  star kapteyn(		0.011984,		0.28,			3550,			(char*)"M1V",		0,			0,	0,		0,		0,		kapteynb,			(char*)"Kapteyn",		true,		(char*)"Kapteyn's Star");
  phl.addStar(kapteyn);
  star kepler186(	0.0412,			0.48,			3788,			(char*)"M0V",		0,			0,	0,		0,		0,		kepler186b,			(char*)"Kepler-186",		true,		(char*)"Kepler-186");
  phl.addStar(kepler186);
  star kepler174(	0.195593,		0.68,			4880,			(char*)"",		0,			0,	0,		0,		0,		kepler174b,			(char*)"Kepler-174",		true,		(char*)"Kepler-174");
  phl.addStar(kepler174);
  star gliese682(	0.002003,		0.27,			3028,			(char*)"M4V",		0,			0,	0,		0,		0,		gliese682b,			(char*)"GJ 682",		true,		(char*)"Gliese 682");
  phl.addStar(gliese682);
  // These are stars that have gas giants that could have habitable moons
  star hd38529(		5.626708,		1.48,			5697,			(char*)"G4IV",		0,			0,	0,		0,		0,		hd38529b,			(char*)"HD 38529",		true,		(char*)"HIP 27253");
  phl.addStar(hd38529);
  star hd202206(	1.020379,		1.13,			5750,			(char*)"G6V",		0,			0,	0,		0,		0,		hd202206b,			(char*)"HD 202206",		true,		(char*)"HIP 104903");
  phl.addStar(hd202206);
  star hd8673(		3.598954,		1.31,			6413,			(char*)"F7V",		0,			0,	0,		0,		0,		hd8673b,			(char*)"HD 8673",		true,		(char*)"HIP 6702");
  phl.addStar(hd8673);
  star hd22781(		0.310872,		0.74,			5072,			(char*)"K0V",		0,			0,	0,		0,		0,		hd22781b,			(char*)"HD 22781",		true,		(char*)"HIP 17187");
  phl.addStar(hd22781);
  star hd217786(	1.833286,		1.02,			5966,			(char*)"F8V",		0,			0,	0,		0,		0,		hd217786b,			(char*)"HD 217786",		true,		(char*)"HIP 113834");
  phl.addStar(hd217786);
  star hd106270(	5.665917,		1.32,			5638,			(char*)"G5IV",		0,			0,	0,		0,		0,		hd106270b,			(char*)"HD 106270",		true,		(char*)"HIP 59625");
  phl.addStar(hd106270);
  star hd38801(		4.270497,		1.36,			5222,			(char*)"K0IV",		0,			0,	0,		0,		0,		hd38801b,			(char*)"HD 38801",		true,		(char*)"HIP 27384");
  phl.addStar(hd38801);
  star hd39091(		4.755538,		1.1,			5888,			(char*)"G1IV",		0,			0,	0,		0,		0,		hd39091b,			(char*)"HD 39091",		true,		(char*)"HIP 26394");
  phl.addStar(hd39091);
  star upsand(		3.55405,		1.27,			6212,			(char*)"F8V",		mass(0.00163),		0,	750,	 	0,		0,		UPSAndAb,			(char*)"UPS And A",		true,		(char*)"Upsilon Andromedae A");
  phl.addStar(upsand);
  star hd141937(	1.242381,		1.1,			5925,			(char*)"G2V",		0,			0,	0,		0,		0,		hd141937b,			(char*)"HD 141937",		true,		(char*)"HIP 77740");
  phl.addStar(hd141937);
  star hd33564(		1.656514,		1.25,			6250,			(char*)"F6V",		0,			0,	0,		0,		0,		hd33564b,			(char*)"HD 33564",		true,		(char*)"HIP 25110");
  phl.addStar(hd33564);
  star hd23596(		4.710355,		1.27,			5888,			(char*)"F8V",		0,			0,	0,		0,		0,		hd23596b,			(char*)"HD 23596",		true,		(char*)"HIP 17747");
  phl.addStar(hd23596);
  star hd222582(	1.219453,		0.99,			5662,			(char*)"G5V",		0,			0,	0,		0,		0,		hd222582b,			(char*)"HD 222582",		true,		(char*)"HIP 116906");
  phl.addStar(hd222582);
  star hd86264(		4.715981,		1.42,			6210,			(char*)"F7V",		0,			0,	0,		0,		0,		hd86264b,			(char*)"HD 86264",		true,		(char*)"HIP 48780");
  phl.addStar(hd86264);
  star hd196067(	6.680477,		1.28,			5940,			(char*)"G0V",		0,			0,	0,		0,		0,		hd196067b,			(char*)"HD 196067",		true,		(char*)"HIP 102125");
  phl.addStar(hd196067);
  star hd10697(		2.68764,		1.15,			5641,			(char*)"G5IV",		0,			0,	0,		0,		0,		hd10697b,			(char*)"HD 10697",		true,		(char*)"HIP 8159");
  phl.addStar(hd10697);
  star hd28185(		0.859647,		1.24,			5482,			(char*)"G5V",		0,			0,	0,		0,		0,		hd28185b,			(char*)"HD 28185",		true,		(char*)"HIP 20723");
  phl.addStar(hd28185);
  star hd132406(	1.830276,		1.09,			5885,			(char*)"G0V",		0,			0,	0,		0,		0,		hd132406b,			(char*)"HD 132406",		true,		(char*)"HIP 73146");
  phl.addStar(hd132406);
  star hd13908(		4.026799,		1.29,			6255,			(char*)"F8V",		0,			0,	0,		0,		0,		hd13908b,			(char*)"HD 13908",		true,		(char*)"HIP 10743");
  phl.addStar(hd13908);
  star hd2039(		1.643056,		0.98,			5947,			(char*)"G2V",		0,			0,	0,		0,		0,		hd2039b,			(char*)"HD 2039",		true,		(char*)"HIP 1931");
  phl.addStar(hd2039);
  star hd82943(		1.339867,		1.18,			5874,			(char*)"G0V",		0,			0,	0,		0,		0,		hd82943c,			(char*)"HD 82943",		true,		(char*)"HIP 47007");
  phl.addStar(hd82943);
  star moa2011blg293l(	0.547008,		0.86,			5180,			(char*)"",		0,			0,	0,		0,		0,		moa2011blg293lb,		(char*)"MOA-2011-BLG-293L",	true,		(char*)"MOA-2011-BLG-293L");
  phl.addStar(moa2011blg293l);
  star hd213240(	2.572906,		1.22,			5975,			(char*)"G4IV",		0,			0,	0,		0,		0,		hd213240b,			(char*)"HD 213240",		true,		(char*)"HIP 111143");
  phl.addStar(hd213240);
}