# Closed-form surface-temperature polynomial T_s(S, pCO2)

Source: **Lehmer, Catling & Krissansen-Totton (2020)**, "Carbonate-Silicate Cycle
Predictions of Earth-like Planetary Climates and Testing the Habitable Zone Concept,"
Nature Communications 11, 6153 (arXiv:2012.00819), **equation 8** (Methods §4.1).

Fit to the Virtual Planetary Laboratory 1-D radiative-convective climate model for an
Earth-like CO2-H2O atmosphere (Manabe-Wetherald humidity; if pCO2 < 1 bar the total
pressure is topped up to 1 bar with N2). Bond albedo is computed dynamically by the 1-D
model and is therefore *implicit* in the fit. Maximum error vs the 1-D model ~3%.

**Variables:** `X = ln(pCO2)` with pCO2 in **bar**; `Y = S / S_earth` (instellation
normalized to the solar constant).

**Validity domain:** S in [0.35, 1.05] S_earth; pCO2 in [1e-6, 10] bar.

```
Ts(S, pCO2) =  4.809
             − 222.0  X      − 68.44  X^2   − 6.737  X^3   − 0.206  X^4
             + 1414   X Y    + 446.4  X^2 Y + 44.41  X^3 Y + 1.364  X^4 Y
             − 2964   X Y^2  − 978.4  X^2 Y^2− 98.86  X^3 Y^2− 3.059  X^4 Y^2
             + 2655   X Y^3  + 907.5  X^2 Y^3+ 92.87  X^3 Y^3+ 2.892  X^4 Y^3
             − 868.4  X Y^4  − 304.6  X^2 Y^4− 31.48  X^3 Y^4− 0.985  X^4 Y^4
             + 1045   Y      − 1496   Y^2   + 1064   Y^3   − 281.1  Y^4
```

As a 5x5 coefficient matrix `c[i][j]` (term = c[i][j] · X^i · Y^j, i=power of X, j=power of Y):

| i\j | j=0 | j=1 | j=2 | j=3 | j=4 |
|---|---|---|---|---|---|
| i=0 | 4.809 | 1045 | −1496 | 1064 | −281.1 |
| i=1 | −222.0 | 1414 | −2964 | 2655 | −868.4 |
| i=2 | −68.44 | 446.4 | −978.4 | 907.5 | −304.6 |
| i=3 | −6.737 | 44.41 | −98.86 | 92.87 | −31.48 |
| i=4 | −0.206 | 1.364 | −3.059 | 2.892 | −0.985 |

T_s is returned in Kelvin.

## Verification
Earth check: pCO2 ≈ 280 ppm = 2.8e-4 bar → X = −8.18; Y = 1.0 → T_s ≈ 298 K (Earth's
mean ~288 K; the idealized 1-bar CO2-H2O model runs a few K warm). Coefficients
transcribed from the arXiv PDF (pdftotext), not from MathML.

## Integration notes for StarGen
This polynomial returns the FULL surface temperature (greenhouse included; albedo
implicit), for a narrow Earth-like regime — it is NOT a drop-in for `green_rise`
(which returns only a greenhouse increment on top of an effective temperature).
A safe integration overrides the final surface temperature with this polynomial ONLY
for rocky planets inside the validity domain (S in [0.35,1.05], pCO2 in [1e-6,10] bar,
~1 bar Earth-like CO2-H2O atmosphere), falling back to StarGen's existing iterative
solver everywhere else (Venus-like, thin/thick atmospheres, gas giants, hot/cold
planets). Requires the CO2 partial pressure and instellation S = (L/Lsun)/(a/AU)^2.
Keep `grnhouse`/`green_rise` (fallback + unit tests #6/#7).
