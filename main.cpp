#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "Planetary_Habitability_Laboratory.h"
#include "andromeda.h"
#include "dole.h"
#include "elements.h"
#include "ic3094.h"
#include "jimb.h"
#include "omega_galaxy.h"
#include "PerformanceMonitor.h"
#include "planets.h"
#include "radius_tables.h"
#include "ring_universe.h"
#include "solstation.h"
#include "star_trek.h"
#include "stargen.h"
#include "structs.h"
#include "utils.h"

/**
 * @brief Struct to hold all command line arguments
 */
struct CommandLineArgs {
  actions action = aGenerate;
  std::string flag_char = "?";
  std::string path = SUBDIR;
  std::string url_path_arg = "";
  std::string filename_arg = "";
  std::string arg_name = "";

  bool use_stdout = false;
  long double mass_arg = 0.0;
  long double luminosity_arg = 0.0;
  long int seed_arg = 0;
  int count_arg = 1;
  int increment_arg = 1;
  int num_threads = 1;  // Number of threads for parallel generation
  catalog star_catalog{};
  int sys_no_arg = 0;

  long double ratio_arg = 0.0;
  long double ecc_coef_arg = 0.077;  // seb: dole value
  long double inner_planet_factor_arg = 0.3;  // seb: dole value

  int flags_arg = 0;
  int out_format = ffHTML;
  int graphic_format = gfGIF;
};

void initData();
void initConfig();  // Initialize configuration defaults
void usage(std::string);
bool parseCommandLine(int argc, char** argv, const std::string& prognam, CommandLineArgs& args);
bool validateArguments(const CommandLineArgs& args);

void printAknowledgement() {
  std::cout << "Web systems (-W) taken from\n"
       << "\thttp://www.solstation.com/stars.htm, Wikipedia, and various research papers\n"
       << "AU systems (-F) taken from stories by C.J. Cherry\n"
       << "Manticore systems (-B) taken from stories by David Weber\n"
       << "StarGen: " << stargen_revision << std::endl;
}

void printExamples() {
  std::cout << "Examples:\n"
          "10000 systems with 1 as the seed for the first system around a\n"
          "custom star with moons and migrated planets and only save ones with "
          "an earthlike planet:\n\n"
          "stargen -m1.09 -y1.12609 -BG0V -b6215 -M -r -s1 -n10000 -E\n\n"
          "10000 systems with 1 as the seed for the first system around a\n"
          "custom star in a circumbinary system with moons and migrated "
          "planets and only save ones with an earthlike planet:\n\n"
          "stargen -m1.09 -y1.12609 -BG0V -b6215 -CB -w0.75 -j0.178473 -X4493 -NK3V "
          "-d0.11146 -f0.011 -M -r -s1 -n10000 -E\n\n"
          "10000 systems with 1 as the seed for the first system around a predefined star:\n\n"
          "stargen -W73 -M -r -s1 -n10000 -E\n\n"
          "10000 systems with 1 as the seed for the first system around a\n"
          "custom star with a distant companion star with moons and migrated "
          "planets and only save ones with an earthlike planet:\n\n"
          "stargen -m1.09 -y1.12609 -BG0V -b6215 -w0.75 -d1114.6 -f0.011 -M -r -s1 -n10000 -E\n\n";
}

/**
 * @brief Parse command line arguments
 * @return false if parsing failed or help was requested
 */
bool parseCommandLine(int argc, char** argv, const std::string& prognam, CommandLineArgs& args) {
  bool first_part_of_name = true;

  for (int i = 0; i < argc; i++) {
    bool skip = false;
    std::string temp_string = argv[i];

    if (!compare_string_char(temp_string, 0, "-")) {
      // Not a flag - it's part of the system name
      if (temp_string != prognam) {
        if (first_part_of_name) {
          first_part_of_name = false;
          args.arg_name = temp_string;
        } else {
          args.arg_name.append(" ");
          args.arg_name.append(temp_string);
        }
      }
      continue;
    }

    // Handle stdout flag
    if (compare_string_char(temp_string, 1, "-")) {
      args.use_stdout = true;
      continue;
    }

    // Handle PHL catalog
    if (compare_string_char(temp_string, 1, "PHL", 3)) {
      args.star_catalog = phl;
      args.sys_no_arg = temp_string.length() > 2 ?
        atoi(temp_string.substr(4, temp_string.length() - 4).c_str()) : 0;
      args.flag_char = args.star_catalog.getArg();
      continue;
    }

    // Handle significant numbers
    if (compare_string_char(temp_string, 1, "sn", 2)) {
      decimals_arg = atoi(temp_string.substr(3, temp_string.length() - 3).c_str());
      continue;
    }

    // Handle circumbinary flag
    if (compare_string_char(temp_string, 1, "CB", 2)) {
      args.flags_arg |= fIsCircubinaryStar;
      continue;
    }

    // Handle max age
    if (compare_string_char(temp_string, 1, "MY", 2)) {
      max_age_backup = max_age = atof(temp_string.substr(3, temp_string.length() - 3).c_str());
      continue;
    }

    // Handle max distance
    if (compare_string_char(temp_string, 1, "md", 2)) {
      max_distance_arg = atof(temp_string.substr(3, temp_string.length() - 3).c_str());
      continue;
    }

    // Handle single character flags
    const char flag = temp_string.length() > 1 ? temp_string[1] : '\0';

    switch (flag) {
      case 's':  // Seed
        args.seed_arg = atol(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'm':  // Mass
        args.mass_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'y':  // Luminosity
        args.luminosity_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'Y':  // Min age
        min_age = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'n':  // Count
        args.count_arg = atoi(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'i':  // Increment
        args.increment_arg = atoi(temp_string.substr(2, temp_string.length() - 2).c_str());
        break;

      case 'T':  // Number of threads for parallel generation
        args.num_threads = atoi(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (args.num_threads < 1) args.num_threads = 1;  // Minimum 1 thread
        break;

      case 'x':  // Use solar system
        args.flag_char = temp_string.substr(1, 1).c_str();
        args.flags_arg |= fUseSolarsystem;
        if (args.mass_arg == 0.0) args.mass_arg = 1.0;
        break;

      case 'a':  // Reuse solar system
        if (!compare_string_char(temp_string, 2, "k")) {
          args.flag_char = temp_string.substr(1, 1).c_str();
          args.flags_arg |= fReuseSolarsystem;
          return true;  // Break out of argument parsing
        }
        break;

      case 'D':  // Dole catalog
      case 'W':  // Sol station catalog
      case 'F':  // Jimb catalog
      case 'O':  // Omega galaxy catalog
      case 'R':  // Ring universe catalog
      case 'I':  // IC3094 catalog
      case 'U':  // Andromeda catalog
      case 'G':  // Star Trek catalog
        if (flag == 'D') args.star_catalog = dole;
        else if (flag == 'W') args.star_catalog = solstation;
        else if (flag == 'F') args.star_catalog = jimb;
        else if (flag == 'O') args.star_catalog = omega_galaxy;
        else if (flag == 'R') args.star_catalog = ring_universe;
        else if (flag == 'I') args.star_catalog = ic3094;
        else if (flag == 'U') args.star_catalog = andromeda;
        else if (flag == 'G') args.star_catalog = star_trek;

        args.sys_no_arg = temp_string.length() > 2 ?
          atoi(temp_string.substr(2, temp_string.length() - 2).c_str()) : 0;
        args.flag_char = args.star_catalog.getArg();
        break;

      case 'o':  // Output filename
        if (i + 1 < argc) {
          args.filename_arg = argv[i + 1];
          skip = true;
        }
        break;

      case 't':  // Text output
        args.out_format = ffTEXT;
        break;

      case 'e':  // CSV output
        if (!compare_string_char(temp_string, 2, "x")) {
          args.out_format = ffCSV;
        } else if (compare_string_char(temp_string, 1, "ex", 2)) {
          printExamples();
          return false;
        }
        break;

      case 'C':  // CSV download
        args.out_format = ffCSVdl;
        break;

      case 'c':  // Celestia
        args.out_format = ffCELESTIA;
        break;

      case 'J':  // JSON or Jovian
        if (compare_string_char(temp_string, 2, "S")) {
          args.out_format = ffJSON;
        } else {
          args.flags_arg |= fDoGases | fOnlyJovianHabitable;
        }
        break;

      case 'V':  // SVG graphics
        args.graphic_format = gfSVG;
        break;

      case 'S':  // SVG output
        args.graphic_format = gfSVG;
        args.out_format = ffSVG;
        break;

      case 'k':  // Use known planets
        args.flags_arg |= fUseKnownPlanets;
        break;

      case 'K':  // Use known planets only
        args.flags_arg |= fUseKnownPlanets | fNoGenerate;
        break;

      case 'p':  // Path or performance
        if (compare_string_char(temp_string, 1, "path")) {
          if (i + 1 < argc) {
            args.path = argv[i + 1];
            skip = true;
          }
        } else if (!compare_string_char(temp_string, 2, "ath")) {
          performanceMonitor.setEnabled(true);
        }
        break;

      case 'u':  // URL path
        if (i + 1 < argc) {
          args.url_path_arg = argv[i + 1];
          skip = true;
        }
        break;

      case 'g':  // Do gases
        args.flags_arg |= fDoGases;
        break;

      case 'v':  // Verbose
        if (temp_string.length() > 2) {
          sscanf(temp_string.substr(2, temp_string.length() - 2).c_str(), "%x", &flag_verbose);
          if (flag_verbose & 0x0001) {
            args.flags_arg |= fDoGases;
          }
        } else {
          args.action = aListVerbosity;
        }
        break;

      case 'l':  // List catalog
        args.action = aListCatalog;
        break;

      case 'L':  // List catalog as HTML
        args.action = aListCatalogAsHTML;
        break;

      case 'z':  // Size check
        args.action = aSizeCheck;
        break;

      case 'Z':  // List gases
        args.action = aListGases;
        break;

      case 'M':  // Do moons
        args.flags_arg |= fDoMoons;
        break;

      case 'r':  // Do migration
        args.flags_arg |= fDoMigration;
        break;

      case 'H':  // Only habitable
        args.flags_arg |= fDoGases | fOnlyHabitable;
        break;

      case '2':  // Only multi-habitable
        args.flags_arg |= fDoGases | fOnlyMultiHabitable;
        break;

      case '3':  // Only three habitable
        args.flags_arg |= fDoGases | fOnlyThreeHabitable;
        break;

      case 'E':  // Only earthlike
        args.flags_arg |= fDoGases | fOnlyEarthlike;
        break;

      case 'P':  // Only potentially habitable
        args.flags_arg |= fDoGases | fOnlyPotentialHabitable;
        break;

      case 'A':  // Accrete dust density ratio
        args.ratio_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (args.ratio_arg <= 0.0) {
          std::cout << "Accrete dust density coefficient -A (" << args.ratio_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'Q':  // Eccentricity coefficient
        args.ecc_coef_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (args.ecc_coef_arg <= 0.0) {
          std::cout << "Accrete eccentricity coeffecient -Q (" << args.ecc_coef_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'q':  // Inner planet factor
        args.inner_planet_factor_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (args.inner_planet_factor_arg <= 0.0) {
          std::cout << "Accrete inner dust boundary -q (" << args.inner_planet_factor_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'w':  // Companion mass
        compainion_mass_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (compainion_mass_arg <= 0.0) {
          std::cout << "Mass of compainion object -w (" << compainion_mass_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'f':  // Companion eccentricity
        compainion_eccentricity_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (compainion_eccentricity_arg <= 0.0) {
          std::cout << "Eccentritiy of compainion object's orbit (" << compainion_eccentricity_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'd':  // Companion distance
        compainion_distant_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (compainion_distant_arg <= 0.0) {
          std::cout << "Distance of compainion object -d (" << compainion_distant_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'b':  // Star temperature
        temp_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (temp_arg <= 0.0) {
          std::cout << "Temperature of star -b (" << temp_arg << ") must be > 0.0" << std::endl;
          return false;
        }
        break;

      case 'B':  // Spectral type
        type_arg = temp_string.substr(2, temp_string.length() - 2).c_str();
        break;

      case 'j':  // Companion luminosity
        compainion_lum_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (compainion_lum_arg <= 0.0) {
          std::cout << "Luminosity of companion star j (" << compainion_lum_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'X':  // Companion temperature
        compainion_eff_arg = atof(temp_string.substr(2, temp_string.length() - 2).c_str());
        if (compainion_eff_arg <= 0.0) {
          std::cout << "Temperature of companion star X (" << compainion_eff_arg << ") must be > 0.0\n";
          return false;
        }
        break;

      case 'N':  // Companion spectral type
        companion_spec_arg = temp_string.substr(2, temp_string.length() - 2).c_str();
        break;

      case 'h':  // Help
        usage(prognam);
        return false;

      default:
        // Check for special cases handled earlier
        if (compare_string_char(temp_string, 1, "ak", 2)) {
          printAknowledgement();
          return false;
        }
        usage(prognam);
        return false;
    }

    if (skip) {
      i++;
    }
  }

  return true;
}

/**
 * @brief Validate command line arguments
 */
bool validateArguments(const CommandLineArgs& args) {
  if (args.use_stdout) {
    if (args.flags_arg & (fOnlyHabitable | fOnlyMultiHabitable | fOnlyJovianHabitable | fOnlyEarthlike)) {
      if (args.count_arg > 50000) {
        std::cout << "Sorry, you cannot set the Repeat count > 50,000 even if you "
                "use a filter, due to system resource issues." << std::endl;
        return false;
      }
    } else {
      if (args.count_arg > 1000) {
        std::cout << "Sorry, you cannot set the Repeat count > 1,000 unless you use "
                "a filter, due to system resource issues." << std::endl;
        return false;
      }
    }
  }
  return true;
}

int main(int argc, char **argv) {
#ifdef macintosh
  _ftype    = 'TEXT';
  _fcreator = 'R*ch';
  argc      = ccommand(&argv);
#endif

  initData();

  // Extract program name
  std::string prognam = argv[0];
  if (const char* c = strrchr(prognam.c_str(), DIRSEP[0]); c != nullptr) {
    prognam = c + 1;
  }

  // Check for help
  if (argc <= 1) {
    usage(prognam);
    return EXIT_FAILURE;
  }

  // Parse command line arguments
  CommandLineArgs args;
  if (!parseCommandLine(argc, argv, prognam, args)) {
    return EXIT_FAILURE;
  }

  // Validate arguments
  if (!validateArguments(args)) {
    return EXIT_FAILURE;
  }

  // Set global flag for backward compatibility
  flags_arg_clone = args.flags_arg;

  // Run the star generation
  ZoneScoped;
  return stargen(
    args.action,
    args.flag_char,
    args.path,
    args.url_path_arg,
    args.filename_arg,
    args.arg_name,
    prognam,
    args.mass_arg,
    args.luminosity_arg,
    args.seed_arg,
    args.count_arg,
    args.increment_arg,
    args.star_catalog,
    args.sys_no_arg,
    args.ratio_arg,
    args.ecc_coef_arg,
    args.inner_planet_factor_arg,
    args.flags_arg,
    args.out_format,
    args.graphic_format,
    args.num_threads
  );
}

/**
 * @brief Initialize Data
 *
 */
void initData() {
  initConfig();  // Initialize configuration defaults
  initRadii();
  initGases();
  initPlanets();
  initDole();
  initSolStation();
  initJimb();
  initOmegaGalaxy();
  initRingUniverse();
  initIC3094();
  initAndromeda();
  initStarTrek();
  initPlanetaryHabitabilityLaboratory();
}

void usage(std::string program) {
  std::cout << "Usage: " << program
       << " [options] [system name]\n"
          "  Options:\n"
          "Seed values:\n"
          "    -s#  Set random number seed [default: from time]\n"
          "    -i#  Number to increment each random seed [default: 1]\n"
          "    -n#  Specify number of systems [default: 1]\n"
          "    -A#  set accretion dust density ratio_arg to # [default: 0.0]\n"
          "    -q#  set accretion inner dust border to # [default: 0.3]\n"
          "    -Q#  set accretion planetesimal seed eccentricity coefficient "
          "to # [default: 0.077]\n"
          "Preset seeds:\n"
          "    -k   Use known planets as planitesimal seeds [from internal "
          "tables]\n"
          "    -K   Generate only known planets [from internal tables]\n"
          "    -x   Use the Solar System's masses/orbits\n"
          "    -a   Use the Solar System's masses/orbits varying Earth\n"
          "Stars:\n"
          "\tPlease note that for a custom star, you need to specify a mass "
          "and/or a luminosity\n\tas well as a spectral type and/or a temperature. Other wise the "
          "program will not work.\n"
          "    -m#  Specify stellar mass # [fraction of Sun's mass] (optional "
          "if -y is used)\n"
          "    -y#  Specify stellar luminosity # [fraction of Sun's "
          "luminosity] (optional if -m is used)\n"
          "    -Y#  Specify minimum age for star (years) (optional)\n"
          "    -MY# Specify maximum age for star (years) (optional)\n"
          "    -b#  The temperature of the star (optional if -B is used)\n"
          "    -B   Spectral type of the star (optional if -b is used)\n"
          "    -CB  Make this a circumbinary system like Tatoonine in Star "
          "Wars (optional)\n"
          "    -w#  The mass of a companion star (optional and required if the "
          "-CB option is used)\n"
          "    -j#  The luminosity of a companion star (optional and required "
          "if the -CB option is used)\n"
          "    -X#  The temperature of a companion star (optional and required "
          "if the -CB option is used)\n"
          "    -N   Spectral type of the companion star (optional and required "
          "if the -CB option is used)\n"
          "    -d#  The distance to a companion star (optional and required if "
          "the -CB option is used)\n"
          "    -f#  The eccentricity of the orbit of the companion star "
          "(optional and required if the -CB option is used)\n"
          "  For a predefined star:\n"
          "    -D   Use all of Dole's "
       << dole.count()
       << " nearby stars\n"
          "    -D#  Use Dole's system #\n"
          "    -F   Use all "
       << jimb.count()
       << " AU systems\n"
          "    -F#  Use AU system #\n"
          "    -W   Use all "
       << solstation.count()
       << " nearby stars taken from the Web\n"
          "    -W#  Use Web system #\n"
          "    -O   Use all "
       << omega_galaxy.count()
       << " fictious stars in the fictious Omega Galaxy\n"
          "    -O#  Use Omega Galaxy system #\n"
          "    -R   Use all "
       << ring_universe.count()
       << " fictious stars in the fictious Ring Universe galaxy\n"
          "    -R#  Use Ring Universe system #\n"
          "    -I   Use all "
       << ic3094.count()
       << " fictious stars in IC 3094 that cham generated\n"
          "    -I#  Use IC 3094 system #\n"
          "    -U   Use all "
       << andromeda.count()
       << " fictious stars in the Andromeda Galaxy that cham generated\n"
          "    -U#  Use Andromeda Galaxy system #\n"
          "    -G   Use the "
       << star_trek.count()
       << " predefined stars from Star Trek\n"
          "    -G#  Use Star Trek system #\n"
          "    -PHL Use the "
       << phl.count()
       << " predefined stars listed at the Planetary Habitability Library\n"
          "    -PHL#Use potentially habitable system #\n"
          "    -l   List stars of selected table and exit\n"
          "    -L   List stars of selected table as HTML and exit\n"
          "Filters:\n"
          "    Please note that these options are only usefull if you are making\n"
          "\ta large batch of systems and only want to save certain ones.\n"
          "    -E   Only systems with earthlike planets\n"
          "    -H   Only systems with habitable planets\n"
          "    -2   Only systems with 2 or more habitable planets\n"
          "    -3   Only systems with 3 or more habitable planets\n"
          "    -T   Only systems with habitable planets more than 2 Earth "
          "Masses in size\n"
          "    -P   Only systems with planets habitable by the Planetary "
          "Habitability Laboratory's criteria\n"
          "    -J   Only systems with Jovian planets in habitable region\n"
          "    -g   Include atmospheric gases\n"
          "    -v   List verbosities [hex values] and exit\n"
          "    -v#  Set output verbosity [hex value]\n"
          "    -V   Use std::vector graphics [SVG] images [default: GIF]\n"
          "    -z   Do numeric size check and exit\n"
          "    -Z   Dump tables used for gases and exit\n"
          "File specs:\n"
          "    --   use stdout\n"
          "    -o   Name for the output file(s) [default: taken from star name]\n"
          "    -p   Path for where the output file(s) are saved [default: ./html]\n"
          "    -u   Internet URL path for/in the output file(s) [default: none]\n"
          "Output formats: (only one is generated) default HTML to file\n"
          "    -c   Celestia .ssc to stdout\n"
          "    -C   Excel .csv [dl: no thumbnail html] to file\n"
          "    -JS  JSON .json [dl: no thumbnail html] to file\n"
          "    -e   Excel .csv to file\n"
          "    -S   Vector graphics (SVG) to file\n"
          "    -t   Text to stdout\n"
          "    -sn# Number of decimal places for numbers\n"
          "Other:\n"
          "    -M   Generate moons (highly experimental and incomplete)\n"
          "    -r   Allow planet migration after forming. (highly experimental)\n"
          "    -p   Enable performance monitoring\n"
          "    -ak  Acknowledgement\n"
          "    -ex  Examples\n";
}
