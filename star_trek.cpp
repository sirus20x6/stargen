#include "star_trek.h"
#include "const.h"
#include "planets.h"

catalog star_trek;

void initStarTrek()
{
  star_trek.setArg("G");
  // 			L					Mass						Eff_Temp	Spec Type	Mass2			Eccen.	SMAxis	 	inclination	ascending node	Planets		Designation	InCel	Name
  star sol(		1.0,					1.00,						5780,			(char*)"G2V",		0,			0,	0,		0,		0,		mercury,		(char*)"Sol",		true,		(char*)"The Solar System");
  star_trek.addStar(sol);
  star bajor(		abs2luminosity(4.7),			mass(abs2luminosity(4.7)),			5780,			(char*)"G2V",		0,			0,	0,		0,		0,		bajorI,			(char*)"Bajor",	true,		(char*)"B'hava'el");
  star_trek.addStar(bajor);
  star cardassia(	abs2luminosity((-1.1)),			mass(abs2luminosity((-1.1))),			0,			(char*)"K0V",		0,			0,	0,		0,		0,		nullptr,			(char*)"Cardassia",	true,		(char*)"Cardassia");
  star_trek.addStar(cardassia);
  star talos(		abs2luminosity(0.1),			mass(abs2luminosity(0.1)),			0,			(char*)"M0IV",		0,			0,	0,		0,		0,		nullptr,			(char*)"Talos",	true,		(char*)"Talos Prime");
  star_trek.addStar(talos);
  star deneb_kaitos(	139.1,					2.8,						4797,			(char*)"K0III",	0,			0,	0,		0,		0,		nullptr,			(char*)"Deneb Kaitos",	true,		(char*)"Deneb");
  star_trek.addStar(deneb_kaitos);
  star Eri40(		0.46,					0.84,						5300,			(char*)"K1V",		(0.2+0.5),    		0,	418.,		0,		0,		nullptr,			(char*)"40 Eri",	true,		(char*)"Vulcan");
  star_trek.addStar(Eri40);
  star beta_rigel(	abs2luminosity(6.2),			mass(abs2luminosity(6.2)),			0,			(char*)"A5V",		0,			0,	0,		0,		0,		nullptr,			(char*)"BET Rig",	true,		(char*)"Beta Rigel");
  star_trek.addStar(beta_rigel);
  star aldebaran(	518,					1.7,						3910,			(char*)"K5III",	0,			0,	0,		0,		0,		nullptr,			(char*)"ALF Tau",	true,		(char*)"Aldebran");
  star_trek.addStar(aldebaran);
  star altair(		10.6,					1.79,						6900,			(char*)"A7V",		0,			0,	0,		0,		0,		nullptr,			(char*)"ALF Aql",	true,		(char*)"Altair");
  star_trek.addStar(altair);
  star antaresa(	57500,					12.4,						3400,			(char*)"M1I-b",	10,			0,	529,		0,		0,		nullptr,			(char*)"ALF Sco A",	true,		(char*)"Antares A");
  star_trek.addStar(antaresa);
  star antaresb(	0,					10,						18500,			(char*)"B2V",		12.4,			0,	529,		0,		0,		nullptr,			(char*)"ALF Sco B",	true,		(char*)"Antares B");
  star_trek.addStar(antaresb);
  star arcturus(	170,					1.1,						4290,			(char*)"K1III",	0,			0,	0,		0,		0,		nullptr,			(char*)"ALF Boo",	true,		(char*)"Arcturus");
  star_trek.addStar(arcturus);
  star aaamazzara(	abs2luminosity(2.04),			1.85,						8400,			(char*)"A2V",		0,			0,	0,		0,		0,		nullptr,			(char*)"EPS Sep",	true,		(char*)"Aaamazzara");
  star_trek.addStar(aaamazzara);
  star sirius(		25.4,					2.02,						9940,			(char*)"A1V",		0.978,			0.5923,	19.8,		0,		0,		nullptr,			(char*)"ALF CMa",	true,		(char*)"Sirius");
  star_trek.addStar(sirius);
  star ALFCenA(		1.519,					1.10,						5790,			(char*)"G2V",		0.907,			0.52,	23.7,		0,		0,		nullptr,			(char*)"ALF Cen A",	true,		(char*)"Alpha Centauri A");
  star_trek.addStar(ALFCenA);
  star ALFCenB(		0.5,					0.907,						5260,			(char*)"K1V",		1.10,			0.52,	23.7,		0,		0,		alfcentbb,		(char*)"ALF Cen B",	true,		(char*)"Alpha Centauri B");
  star_trek.addStar(ALFCenB);
  star procyon(		6.93,					1.42,						6530,			(char*)"F5V",		0.602,			0.407,	15,		0,		0,		nullptr,			(char*)"ALF CMi",	true,		(char*)"Andoria");
  star_trek.addStar(procyon);
  star antos(		abs2luminosity(vis2abs(5.2, 98)),	mass(abs2luminosity(vis2abs(5.2, 98))),		0,			(char*)"G2IV",		0,			0,	0,		0,		0,		nullptr,			(char*)"KAP Del",	true,		(char*)"Antos");
  star_trek.addStar(antos);
  star ardana(		abs2luminosity(vis2abs(3.88, 124)),	mass(abs2luminosity(vis2abs(3.88, 124))),	0,			(char*)"K2III",	0,			0,	0,		0,		0,		nullptr,			(char*)"MU Leo",	true,		(char*)"Ardana");
  star_trek.addStar(ardana);
  star argelius(	11.5,					AVE(1.62, 1.7),					6739,			(char*)"F3V",		0,			0,	0,		0,		0,		nullptr,			(char*)"IOT Leo",	true,		(char*)"Argelius");
  star_trek.addStar(argelius);
  star aurelia(		62,					2.26,						4963,			(char*)"G8III",	0,			0,	0,		0,		0,		nullptr,			(char*)"XI Her",	true,		(char*)"Aurelia");
  star_trek.addStar(aurelia);
  star axanar(		0.34,					0.82,						5084,			(char*)"K2V",		0,			0,	0,		0,		0,		nullptr,			(char*)"EPS Eri",	true,		(char*)"Axanar");
  star_trek.addStar(axanar);
  
  star_trek.sort();
}