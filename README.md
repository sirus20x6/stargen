This is a fork of a version of stargen from https://github.com/omega13a/stargen

My goal is to update and modernize the code, and then add features in that order. I had found an older version of accrete/accrete2 written in K&R C that had been partially updated to ANSI C. I added JSON exporting to that before stumbling on this newer fork. I have been going back and forth between modernizing by hand and using static analyzers to improve the code, but some of it is so old the analyzers have trouble.


Then I found comments from this old sister fork https://github.com/fusiongyro/starform I'm trying to bring over the appropriate comments one by one in a painstakingly manual way, while updating them to doxygen style. the code diverges from whats in this branch so it's not always easy to find where to shoe horn it in.

Strangly some of the code in that fork is more modular / object oriented / better despite having fewer features. I may steal some of it for refactoring

Build instructions
```bash
git clone https://github.com/sirus20x6/stargen.git
cd stargen
cmake ./CMakeLists.txt
make
```

tip use 
```
make -jx
```
where x is number of threads +1 to compile faster


I believe it would also be good to get rid of all the hard coded start catalogs and just add support for importing stars through json
