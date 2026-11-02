# Migration

Version 1.0.0 introduces a new C++ API and package, `BenScr.Noise`. There is no earlier
public Noise package or source API being renamed or removed in this repository. Existing
BenScr packages remain independent and are not required by this library.

All public types and methods are new: `Generator`, `FractalSettings`, `Grid2D`, `Grid3D`,
`GetSeed`, `Sample2D`, `Sample3D`, both fractal sampling variants, and all four filling
variants. The CMake package is `BenScrNoise`, its target `BenScr::Noise`, and the installed
header is `<benscr/noise/generator.hpp>`.

The first public-header baseline and algorithm version 1 establish compatibility for later
releases. Source compilation is the native NuGet delivery contract; no prebuilt ABI
compatibility or managed .NET binding is promised.
