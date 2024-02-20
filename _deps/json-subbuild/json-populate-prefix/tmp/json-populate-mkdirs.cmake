# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/run/media/sirus/aux/code/git/stargen/_deps/json-src"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-build"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/tmp"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/src"
  "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/run/media/sirus/aux/code/git/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
