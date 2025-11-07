# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/user/stargen/_deps/json-src"
  "/home/user/stargen/_deps/json-build"
  "/home/user/stargen/_deps/json-subbuild/json-populate-prefix"
  "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/tmp"
  "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/src"
  "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/user/stargen/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
