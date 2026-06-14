#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>   // for size_t
#include <algorithm>  // for replace
#include <cmath>      // for isnan
#include <cstdio>     // for std::snprintf (standard-stable float formatting)
#include <iomanip>    // for operator<<, setprecision
#include <iostream>   // for cout, basic_ostream<>::__ostream_...
#include <sstream>    // for std::stringstream (not guaranteed via <iostream>)
#include <map>        // for std::map
#include <string>     // for std::string, to_string
#include <type_traits>  // for std::is_floating_point_v, std::is_same_v
#include <vector>     // for std::vector
#include "stargen.h"  // for decimals_arg
#include "tracy/Tracy.hpp"

// Per-thread active RNG context. When non-null, the free RNG functions
// (random_number/about/random_eccentricity/gaussian/exponential/randf/srandf)
// draw from it instead of the global g_random_context, so each parallel worker
// uses its own deterministic per-system stream. nullptr -> fall back to global.
class RandomContext;
extern thread_local RandomContext* t_active_random_context;
void set_active_random_context(RandomContext* ctx);

auto compare_string_char(std::string& a_string, int place, const char* a_character,
                         int length = 1) -> bool;
// std::string replaceStrChar(std::string, const char *, const char *);
auto float_to_string(long double) -> std::string;

// Output-escaping helpers. Each is a no-op on strings containing none of the
// relevant special characters, so default/clean output stays byte-identical.
//   escapeXmlText  - HTML element text and SVG/XML text: & < >
//   escapeXmlAttr  - HTML/XML attribute values (any quote style): & < > " '
//   escapeCsvField - inside the single-quoted CSV fields this code emits:
//                    ' -> '' (doubling), CR/LF -> space
//   escapeCelestiaId - inside Celestia's double-quoted identifiers (the parser
//                    has no backslash escape): " -> ', CR/LF -> space
auto escapeXmlText(const std::string&) -> std::string;
auto escapeXmlAttr(const std::string&) -> std::string;
auto escapeCsvField(const std::string&) -> std::string;
auto escapeCelestiaId(const std::string&) -> std::string;
auto random_number(long double, long double) -> long double;
auto random_number_int(int min, int max) -> int;
auto about(long double, long double) -> long double;
auto random_eccentricity(long double) -> long double;
auto gaussian(long double) -> long double;
auto poisson(long double) -> long;
auto exponential(long double) -> long double;
auto quad_trend(long double, long double, long double, long double)
    -> long double;
auto ln_trend(long double, long double, long double) -> long double;
auto logistal_trend(long double, long double, long double, long double)
    -> long double;
//auto polynomialfit(int, int, double*, double*, double*) -> bool;
void polyRegression(const std::vector<double>& x, const std::vector<double>& y, double *store);
auto soft(long double, long double, long double) -> long double;
auto lim(long double) -> long double;
auto remove_spaces(std::string) -> std::string;
auto randf() -> double;
void srandf(long);
auto fix_inclination(long double) -> long double;
auto linear_trend(long double, long double, long double) -> long double;
auto my_strtoupper(const std::string&) -> std::string;
auto star_type_to_num(const std::string& spec_type, long double luminosity,
                      int run = 1) -> int;
void logfix(long double, long double, long double, long double, long double&,
            long double&);
auto rangeAdjust(long double, long double, long double, long double,
                 long double) -> long double;
void e_fix(long double, long double, long double, long double, long double&,
           long double&);
auto e_trend(long double, long double, long double) -> long double;
void quadfix(long double, long double, long double, long double, long double,
             long double, long double&, long double&, long double&);
auto quintic_trend(long double, long double, long double, long double,
                   long double, long double, long double) -> long double;

template <typename T>
auto toString(T val, int decimals = 0) -> std::string;

template <typename T>
auto getNumDecimals(T /*val*/) -> size_t;

template <typename T>
auto replaceStrChar(std::string /*str*/, T /*old*/, T /*the_new*/) -> std::string;

template <typename T>
void quicksort(std::vector<T>& /*v*/, int /*first*/, int /*last*/);

template <typename T>
auto is_close(T a, T b, long double percent = 1) -> bool;

template <typename T>
void writeVector(std::vector<T>& v, const std::string& separator = "  ");

// display the key-value pairs in the std::map. follow the output of
// each pair by separator. default value of separator = "  "
template <typename Key, typename T>
void writeMap(const std::map<Key, T>& m, const std::string& separator = "\n");

template <typename T>
auto toString(T val, int decimals) -> std::string {
  std::stringstream ss;
  std::string output;

  if (decimals_arg != 0) {
    decimals = decimals_arg;
  }

  ss.str("");

  if (std::isnan(val)) {
    ss << val;
  } else {
    if (decimals == 0) {
      // ss << std::showpoint << std::fixed << std::setprecision(getNumDecimals(val));
      if constexpr (std::is_floating_point_v<T>) {
        // C++26 (libstdc++ P2587) changed std::to_string for floating-point
        // types from printf "%f"/"%Lf" (6 fractional digits) to a shortest
        // round-trip representation. That silently altered every toString()
        // value in CSV/HTML/JSON output. Replicate the historical "%f"/"%Lf"
        // behavior directly so output stays byte-identical and standard-stable.
        if constexpr (std::is_same_v<T, long double>) {
          int n = std::snprintf(nullptr, 0, "%Lf", val);
          output.resize(n);
          std::snprintf(output.data(), n + 1, "%Lf", val);
        } else {
          double d = static_cast<double>(val);
          int n = std::snprintf(nullptr, 0, "%f", d);
          output.resize(n);
          std::snprintf(output.data(), n + 1, "%f", d);
        }
      } else {
        output = std::to_string(val);
      }
    } else {
      ss << std::showpoint << std::fixed << std::setprecision(decimals);
      ss << val;
      output = ss.str();
    }
  }

  return output;
}

template <typename T>
auto getNumDecimals(T val) -> size_t {
  if (std::isnan(val)) {
    return 0;
  }

  /*size_t decimals = 0;
  for (int i = 1; val != floor(val); i++)
  {
    decimals = i;
    val *= 10;
  }

  if (decimals == 0)
  {
    decimals = 1;
  }

  return decimals;*/
  int count = 0;
  val = fabs(val);
  val = val - int(val);
  while (fabs(val) >= 0.0000001) {
    val = val * 10;
    count++;
    val = val - int(val);
  }
  return count;
}

// stuff below this point are algoritms taken from "Data Structors With C++
// Using STL (second edition)" by William Ford and William Topp I had to put
// these here because due to a bug in gcc/g++, they can't be in a .c or .cpp
// file... *grumble*
template <typename T>
auto pivotIndex(std::vector<T>& v, int first, int last) -> int {
  // index for the midpoint of [first,last) and the
  // indices that scan the index range in tandem
  int mid = 0;
  int scanUp = 0;
  int scanDown = 0;
  // pivot value and object used for exchanges
  T pivot;
  T temp;

  if (first == last) {
    // cout << "returing last\n";
    return last;
  }
  if (first == (last - 1)) {
    // cout << "returing first\n";
    return first;
  } else {
    mid = (last + first) / 2;
    pivot = v[mid];

    // exchange the pivot and the low end of the range
    // and initialize the indices scanUp and scanDown.
    v[mid] = v[first];
    v[first] = pivot;

    scanUp = first + 1;
    scanDown = last - 1;

    // manage the indices to locate elements that are in
    // the wrong sublist; stop when scanDown <= scanUp
    for (;;) {
      // move up lower sublist; stop when scanUp enters
      // upper sublist or identifies an element >= pivot
      while (scanUp <= scanDown && v[scanUp] < pivot) {
        // cout << "scanning up " << pivot << " as pivot point.\n";
        scanUp++;
      }

      // scan down upper sublist; stop when scanDown locates
      // an element <= pivot; we guarantee we stop at arr[first]
      while (pivot < v[scanDown]) {
        // cout << "scanning down using " << pivot << " as pivot point." <<
        // std::endl;
        scanDown--;
      }

      // if indices are not in their sublists, partition complete
      if (scanUp >= scanDown) {
        // cout << "stoped scanning\n";
        break;
      }

      // indices are still in their sublists and identify
      // two elements in wrong sublists. exchange
      temp = v[scanUp];
      v[scanUp] = v[scanDown];
      v[scanDown] = temp;

      scanUp++;
      scanDown--;
    }

    // copy pivot to index (scanDown) that partitions sublists
    // and return scanDown
    v[first] = v[scanDown];
    v[scanDown] = pivot;
    return scanDown;
  }
}

template <typename T>
void quicksort(std::vector<T>& v, int first, int last) {
  // index of the pivot
  int pivotLoc = 0;
  // temp used for an exchange when [first,last) has
  // two elements
  T temp;

  // if the range is not at least two elements, return
  if ((last - first) <= 1) {
    // cout << "test1\n";
    // return;
  }
  // if sublist has two elements, compare v[first] and
  // v[last-1] and exchange if necessary
  else if ((last - first) == 2) {
    // cout << "test2\n";
    if (v[last - 1] < v[first]) {
      // cout << "test3\n";
      temp = v[last - 1];
      v[last - 1] = v[first];
      v[first] = temp;
    }
    // cout << "test4\n";
    // return;
  } else {
    // cout << "before pivot index call.\n";

    pivotLoc = pivotIndex(v, first, last);

    // cout << "before first recursive call.\n";
    // writeVector(v, "\n");

    // make the recursive call
    quicksort(v, first, pivotLoc);

    // cout << "before second recursive call.\n";
    // writeVector(v, "\n");

    // make the recursive call
    quicksort(v, pivotLoc + 1, last);

    // cout << "after second recursive call.\n";
    // writeVector(v, "\n");
  }
}

template <typename T>
auto replaceStrChar(std::string str, T old, T the_new) -> std::string {
  std::replace(str.begin(), str.end(), old, the_new);
  return str;
}

template <typename T>
auto is_close(T a, T b, long double percent) -> bool {
  long double range = std::fabs((long double)b) * (percent / 100.0L);

  return static_cast<bool>(a > (b - range) && a < (b + range));
}

template <typename T>
void writeVector(std::vector<T>& v, const std::string& separator) {
  // capture the size of the std::vector in n
  int i = 0;
  int n = v.size();

  for (i = 0; i < n; i++) {
    std::cout << v[i] << separator;
  }
  std::cout << std::endl;
}

template <typename Key, typename T>
void writeMap(const std::map<Key, T>& m, const std::string& separator) {
  typename std::map<Key, T>::const_iterator iter = m.begin();

  while (iter != m.end()) {
    std::cout << (*iter).first << ": " << (*iter).second << separator;
    iter++;
  }
}

#endif