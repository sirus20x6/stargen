#include "jimb.h"
#include "const.h"
#include "planets.h"

catalog jimb;

void initJimb()
{
  jimb.setArg("F");
  // 			L				Mass			Eff_Temp	Spec Type	Mass2			Eccen.	SMAxis	 	inclination	ascending node	Planets		Designation		InCel	Name
  star AlphaSS1(	0.000427,			0.17, 			3134,		(char*)"M4V",		0,			0,	0,		0,		0,		nullptr,		(char*)"HIP 87937", 		false,		(char*)"Alpha SS1:Barnard's Star:BY Dra:BY Draconis");
  star BetaSS2(		1.52,				1.105,			5790,		(char*)"G2V",		0.934,			0.519,	23.7,		82.22,		67.30,		nullptr,		(char*)"ALF Cen A",		false,		(char*)"Beta SS2:Rigel Kentaurus A:Alpha Centauri A");
  star BetaSS2B(	0.446,				0.934,			5260,		(char*)"K1V",		1.105,			0.519,	23.7,		82.22,		67.30,		alfcentbb,		(char*)"ALF Cen B", 		true,		(char*)"Beta SS2:Rigel Kentaurus B:Alpha Centauri B");
  star HIP104214(	0.0859,				0.70,			4536,		(char*)"K5V",		0.63,			0.40,	86.4,		132,		165,		nullptr,		(char*)"HIP 104214",		false,		(char*)"Bryant's Star SS3:61 Cyg A:61 Cygni A:Gl 820 A");
  star HIP104217(	0.0405,				0.63,			4077,		(char*)"K7V",		0.70,			0.40,	86.4,		132,		165,		nullptr,		(char*)"HIP 104217",		false,		(char*)"Bryant's Star SS3:61 Cyg B:61 Cygni B:Gl 820 B");
  star Ross154(		0.000501,			0.2,			3340,		(char*)"M3V",		0,			0,	0,		0,		0,		nullptr,		(char*)"Ross 154",		false,		(char*)"Glory SS4:Ross 154:Gl 729:V1216 Sgr");
  star Kruger60A(	0.00159,			0.271,			3180,		(char*)"M3V",		0.176,			0.410,	9.76,		34.04,		120.09,		nullptr,		(char*)"Kruger 60 A", 		false,		(char*)"Venture SS5:Gl 860 A:Kruger 60 A:HD 239960 A:HIP 110893 A");
  star Kruger60B(	0.000379,			0.176,			2890,		(char*)"M4V",		0.271,			0.410,	9.76,		34.04,		120.09,		nullptr,		(char*)"Kruger 60 B",		false,		(char*)"Venture SS5:Gl 860 A:D0 Cephei:Kruger 60 B:HD 239960 B:HIP 110893 B");
  star EVLac(		0.00178,			0.29,			3400,		(char*)"M3V",		0,			0,	0,		0,		0,		nullptr,		(char*)"EV Lac",		false,		(char*)"Galileo SS6:EV Lacertae");
  star Ross248(		0.000104,			0.12,			2799,		(char*)"M6V",		0,			0,	0,		0,		0,		nullptr,		(char*)"Ross 248",		false,		(char*)"Thule SS7:Ross 248:HH Andromedae");
  star Groombridge34A(	0.0064,				0.40,			3730,		(char*)"M1V",		0.08,			0,	147.8,		67.9,		308.4,		nullptr,		(char*)"Groombridge 34 A",	false,		(char*)"Olympus SS8:GX And:GX Andromedae:ADS 246 A");
  star Groombridge34B(	0.000411,			0.08,			3000,		(char*)"M3V",		0.40,			0,	147.8,		67.9,		308.4,		nullptr,		(char*)"Groombridge 34 B",	false,		(char*)"Olympus SS8:GQ And:GQ Andromedae:ADS 246 B");
  star Luyten7268A(	0.0000553,			0.11,			2670,		(char*)"M5V",		0.10,			0.62,	5.5,		101.3,		159.5,		nullptr,		(char*)"Luyten 726-8 A",	false,		(char*)"Eldorado SS9:BL Ceti:Gliese 65 A");
  star Luyten7268B(	0.0000355,			0.10,			0,		(char*)"M6V",		0.11,			0.62,	5.5,		101.3,		159.5,		nullptr,		(char*)"Luyten 726-8 B",	false,		(char*)"Eldorado SS9:UV Ceti:Gliese 65 B");
  star HIP8102(		0.458,				0.783,			5344,		(char*)"G8V",		0,			0,	0,		0,		0,		taucetb,		(char*)"HIP 8102",		true,		(char*)"Pell SS10:TAU Cet:Tau Ceti:52 Cet:Gliese 71:HD 10700");
  star HIP16537(	0.284,				0.83,			5084,		(char*)"K2V",		0,			0,	0,		0,		0,		EPSEric,		(char*)"HIP 16537",		true,		(char*)"Viking SS11:EPS Eri:Epsilon Eridani");
  star HIP3829(		0.000173,			0.63,			6220,		(char*)"DZ8",		0,			0,	4.0,		0,		0,		nullptr,		(char*)"HIP 3829",		false,		(char*)"Mariner SS12:Van Maanen's Star:Gliese 35:Wolf 28");
  star HIP117473(	0.00775,			0.51,			0,		(char*)"M2V",		0,			0,	0,		0,		0,		cyteen,		(char*)"HIP 117473",		true,		(char*)"Cyteen SS13:Lalande 46650:BR Psc");
  star HIP108296(	abs2luminosity((-1.24)),	1.6,			4035,		(char*)"M3III",	0,			0,	0,		0,		0,		hd208527b,		(char*)"HIP 108296",		true,		(char*)"Esperance:HR 8372:BD+20 5046:HD 209527");
  star HIP3765(		0.241,				0.83,			5829,		(char*)"K2V",		0,			0,	0,		0,		0,		nullptr,		(char*)"HIP 3765",		false,		(char*)"Fargone:Gliese 33:Wolf 25:BD+04 123");
  star GJ1243(		0.000915,			0.3,			0,		(char*)"M4V",		0,			0,	0,		0,		0,		nullptr,		(char*)"GJ 1243",		false,		(char*)"Paradise:GJ 1243:G 208-42");
  star HIP109388(	0.00486,			0.36,			3601,		(char*)"M3V",		0,			0,	0,		0,		0,		GL849b,		(char*)"HIP 109388",		true,		(char*)"Wyatts:Gl 849");
  star ILAqr(		0.00157,			0.32,			3473,		(char*)"M4V",		0,			0,	0,		0,		0,		ILAqrd,		(char*)"IL Aqr",		true,		(char*)"Russell's:Ross 780:HIP 113020:IL Aquarii");
  star GJ2005A(		0.000017,			0.115,			0,		(char*)"M6V",		0.157,			0.05,	25.48,		43.34,		341.84,		nullptr,		(char*)"GJ 2005 A",		false,		(char*)"Voyager:GJ 2005 A:LHS 1070 A");
  star GJ2005B(		0.0000165,			0.0785,			0,		(char*)"M6V",		0.0785,			0.024,	7.14,		43.34,		341.84,		nullptr,		(char*)"GJ 2005 B",		false,		(char*)"Voyager:GJ 2005 B:LHS 1070 B");
  star GJ2005C(		0.000015,			0.0785,			0,		(char*)"M6V",		0.0785,			0.024,	7.14,		43.34,		341.84,		nullptr,		(char*)"GJ 2005 C",		false,		(char*)"Voyager:GJ 2005 C:LHS 1070 C");
  star Ross775(		0.00292,			0.43,			0,		(char*)"M3V",		0,			0,	0.6,		0,		0,		nullptr,		(char*)"Ross 775",		false,		(char*)"Pan-Paris:GL 829:HIP 106106");
  star ZETRet(		0.991,				0.998,			5859,		(char*)"G2V",		0,			0,	0,		0,		0,		nullptr,		(char*)"ZET Ret",		false,		(char*)"Gehenna:Zeta2 Reticuli");
  star BETCas(		29.6,				2.0,			7079,		(char*)"F2III",	0,			0,	0,		0,		0,		nullptr,		(char*)"BET Cas",		false,		(char*)"[Union st]:Beta Cassopeiae");
  star Eri82(		0.617,				0.97,			5401,		(char*)"G8V",		0,			0,	0.667,		0,		0,		HD20794b,		(char*)"82 Eri",		true,		(char*)"[Union st]:82 Eridani:HIP 15510:HD 20794");
  star KapteynsStar(	0.00377,			0.34,			3570,		(char*)"M1V",		0,			0,	0,		0,		0,		nullptr,		(char*)"Kapteyn's Star",	false,		(char*)"[Union st]:VZ Pictoris:HIP 24186");
  star BETHyi(		3.55,				1.1,			5872,		(char*)"G2IV",		0,			0,	0,		0,		0,		BETHyib,		(char*)"HIP 2021",		true,		(char*)"Istra:BET Hyi:Beta Hydri II");
  star Eri40A(		0.359,				0.89,			5300,		(char*)"K1V",		0,			0,	0.667,		0,		0,		nullptr,		(char*)"40 Eri A",		false,		(char*)"Endeavor:40 Eridani A:Keid A:HIP 19849 A:HD 26965 A");
  star ALFCar(		14800.,				8,			7350,		(char*)"F0Ib",		0,			0,	0,	 	0,		0,		nullptr,		(char*)"ALF Car",		false,		(char*)"Canopus");
  jimb.addStar(AlphaSS1);
  jimb.addStar(BetaSS2);
  jimb.addStar(BetaSS2B);
  jimb.addStar(HIP104214);
  jimb.addStar(HIP104217);
  jimb.addStar(Ross154);
  jimb.addStar(Kruger60A);
  jimb.addStar(Kruger60B);
  jimb.addStar(EVLac);
  jimb.addStar(Ross248);
  jimb.addStar(Groombridge34A);
  jimb.addStar(Groombridge34B);
  jimb.addStar(Luyten7268A);
  jimb.addStar(Luyten7268B);
  jimb.addStar(HIP8102);
  jimb.addStar(HIP16537);
  jimb.addStar(HIP3829);
  jimb.addStar(HIP117473);
  jimb.addStar(HIP108296);
  jimb.addStar(HIP3765);
  jimb.addStar(GJ1243);
  jimb.addStar(HIP109388);
  jimb.addStar(ILAqr);
  jimb.addStar(GJ2005A);
  jimb.addStar(GJ2005B);
  jimb.addStar(GJ2005C);
  jimb.addStar(Ross775);
  jimb.addStar(ZETRet);
  jimb.addStar(BETCas);
  jimb.addStar(Eri82);
  jimb.addStar(KapteynsStar);
  jimb.addStar(BETHyi);
  jimb.addStar(Eri40A);
  jimb.addStar(ALFCar);
  /*{},
{}
*/
}