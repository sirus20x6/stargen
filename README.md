![CI](https://github.com/sirus20x6/stargen/actions/workflows/main.yml/badge.svg)

This is a fork of a version of stargen from https://github.com/omega13a/stargen

My goal is to update and modernize the code, and then add features in that order. I had found an older version of accrete/accrete2 written in K&R C that had been partially updated to ANSI C. I added JSON exporting to that before stumbling on this newer fork. I have been going back and forth between modernizing by hand and using static analyzers to improve the code, but some of it is so old the analyzers have trouble.


Then I found comments from this old sister fork https://github.com/fusiongyro/starform I'm trying to bring over the appropriate comments one by one in a painstakingly manual way, while updating them to doxygen style. the code diverges from whats in this branch so it's not always easy to find where to shoe horn it in.

Strangly some of the code in that fork is more modular / object oriented / better despite having fewer features. I may steal some of it for refactoring

## TODO
- JSON support for exporting solar data
- Convert existing data in the code over to JSON
- Remove all global variables
- Make the code more modular and compliant with modern C++ code
- Add multithreading probably through OpenMP

## Build instructions
```bash
git clone https://github.com/sirus20x6/stargen.git
cd stargen
cmake ./CMakeLists.txt
cmake --build .
```

## Tip use 
```bash
make -jx
```
where x is number of threads +1 to compile faster

## Command Line Usage
```
Usage: stargen [options] [system name]
  Options:
Seed values:
    -s#  Set random number seed [default: from time]
    -i#  Number to increment each random seed [default: 1]
    -n#  Specify number of systems [default: 1]
    -A#  set accretion dust density ratio_arg to # [default: 0.0]
    -q#  set accretion inner dust border to # [default: 0.3]
    -Q#  set accretion planetesimal seed eccentricity coefficient to # [default: 0.077]
Preset seeds:
    -k   Use known planets as planitesimal seeds [from internal tables]
    -K   Generate only known planets [from internal tables]
    -x   Use the Solar System's masses/orbits
    -a   Use the Solar System's masses/orbits varying Earth
Stars:
	Please note that for a custom star, you need to specify a mass and/or a luminosity
	as well as a spectral type and/or a temperature. Other wise the program will not work.
    -m#  Specify stellar mass # [fraction of Sun's mass] (optional if -y is used)
    -y#  Specify stellar luminosity # [fraction of Sun's luminosity] (optional if -m is used)
    -Y#  Specify minimum age for star (years) (optional)
    -MY# Specify maximum age for star (years) (optional)
    -b#  The temperature of the star (optional if -B is used)
    -B   Spectral type of the star (optional if -b is used)
    -CB  Make this a circumbinary system like Tatoonine in Star Wars (optional)
    -w#  The mass of a companion star (optional and required if the -CB option is used)
    -j#  The luminosity of a companion star (optional and required if the -CB option is used)
    -X#  The temperature of a companion star (optional and required if the -CB option is used)
    -N   Spectral type of the companion star (optional and required if the -CB option is used)
    -d#  The distance to a companion star (optional and required if the -CB option is used)
    -f#  The eccentricity of the orbit of the companion star (optional and required if the -CB option is used)
  For a predefined star:
    -D   Use all of Dole's 16 nearby stars
    -D#  Use Dole's system #
    -F   Use all 34 AU systems
    -F#  Use AU system #
    -W   Use all 78 nearby stars taken from the Web
    -W#  Use Web system #
    -O   Use all 307283 fictious stars in the fictious Omega Galaxy
    -O#  Use Omega Galaxy system #
    -R   Use all 60001 fictious stars in the fictious Ring Universe galaxy
    -R#  Use Ring Universe system #
    -I   Use all 5001 fictious stars in IC 3094 that cham generated
    -I#  Use IC 3094 system #
    -U   Use all 1001 fictious stars in the Andromeda Galaxy that cham generated
    -U#  Use Andromeda Galaxy system #
    -G   Use the 22 predefined stars from Star Trek
    -G#  Use Star Trek system #
    -PHL Use the 41 predefined stars listed at the Planetary Habitability Library
    -PHL#Use potentially habitable system #
    -l   List stars of selected table and exit
    -L   List stars of selected table as HTML and exit
Filters:
    Please note that these options are only usefull if you are making
	a large batch of systems and only want to save certain ones.
    -E   Only systems with earthlike planets
    -H   Only systems with habitable planets
    -2   Only systems with 2 or more habitable planets
    -3   Only systems with 3 or more habitable planets
    -T   Only systems with habitable planets more than 2 Earth Masses in size
    -P   Only systems with planets habitable by the Planetary Habitability Laboratory's criteria
    -J   Only systems with Jovian planets in habitable region
    -g   Include atmospheric gases
    -v   List verbosities [hex values] and exit
    -v#  Set output verbosity [hex value]
    -V   Use vector graphics [SVG] images [default: GIF]
    -z   Do numeric size check and exit
    -Z   Dump tables used for gases and exit
File specs:
    --   use stdout
    -o   Name for the output file(s) [default: taken from star name]
    -p   Path for where the output file(s) are saved [default: ./html]
    -u   Internet URL path for/in the output file(s) [default: none]
Output formats: (only one is generated) default HTML to file
    -c   Celestia .ssc to stdout
    -C   Excel .csv [dl: no thumbnail html] to file
    -JS  JSON .json [dl: no thumbnail html] to file
    -e   Excel .csv to file
    -S   Vector graphics (SVG) to file
    -t   Text to stdout
    -sn# Number of decimal places for numbers
Other:
    -M   Generate moons (highly experimental and incomplete)
    -r   Allow planet migration after forming. (highly experimental)
    -ak  Acknowledgement
    -ex  Examples
```

## HTML

![image](https://user-images.githubusercontent.com/5103327/202611592-becc5423-bad3-40ae-a075-0a1667ec0e87.png)

![sol](https://user-images.githubusercontent.com/5103327/202611824-00d230a4-3ace-4111-98a6-07b54285c309.png)
