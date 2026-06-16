#ifndef STARGEN_UNITS_H
#define STARGEN_UNITS_H

// StarGen unit vocabulary built on Aurora's "au" units library (vendored at
// third_party/au/au.hh). au gives compile-time dimensional analysis with zero
// runtime overhead and a `long double` representation that matches StarGen's
// pervasive long-double arithmetic.
//
// DESIGN
//  * EXACT units (length, grams-mass, pressure, time, force) have exact rational
//    relationships, so au conversions among them are deterministic and -- where
//    the factor is an integer or power of ten -- bit-identical to StarGen's
//    hand-rolled literal multiplies (verified: AU->cm). au already ships
//    meters/grams/seconds/pascals/bars/newtons/astronomical_units/...; below we
//    add the few StarGen needs that au does not (mmHg, atmospheres, dynes) plus
//    maker aliases (centimeters, kilometers, kilograms, millibars).
//  * MEASURED constants (solar mass, Earth mass, Newtonian G, Stefan-Boltzmann
//    sigma) are NOT exact unit relationships -- they are physical measurements.
//    They are therefore au *quantities* carrying StarGen's historical numeric
//    values, NOT au Units, so they round-trip bit-identically.
//
// Adoption is incremental: at any legacy boundary, extract a raw long double
// with `q.in(unit)` or wrap one with `maker(x)`.

#include "au/au.hh"

namespace sgu {  // "StarGen units"

// ---- representation alias --------------------------------------------------
template <typename U>
using QtyLD = au::Quantity<U, long double>;

// ---- maker aliases for au-shipped units ------------------------------------
constexpr auto centimeters = au::centi(au::meters);
constexpr auto kilometers = au::kilo(au::meters);
constexpr auto kilograms = au::kilo(au::grams);
constexpr auto millibars = au::milli(au::bars);

// ---- custom EXACT units au does not ship -----------------------------------
// 1 mmHg = 101325/760 Pa exactly (since 1 atm = 760 mmHg = 101325 Pa).
struct MillimetersOfMercury
    : decltype(au::Pascals{} * au::mag<101325>() / au::mag<760>()) {};
constexpr auto mm_hg = au::QuantityMaker<MillimetersOfMercury>{};

// 1 standard atmosphere = 101325 Pa exactly.
struct Atmospheres : decltype(au::Pascals{} * au::mag<101325>()) {};
constexpr auto atmospheres = au::QuantityMaker<Atmospheres>{};

// 1 dyne = 1e-5 N exactly (CGS unit of force).
struct Dynes : decltype(au::Newtons{} / au::mag<100000>()) {};
constexpr auto dynes = au::QuantityMaker<Dynes>{};

}  // namespace sgu

#endif  // STARGEN_UNITS_H
