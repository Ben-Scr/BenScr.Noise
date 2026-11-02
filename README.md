# BenScr.Noise

Seeded, coherent 2D and 3D value noise for procedural terrain, textures, and simulation fields.
The C++20 library provides quintic interpolation, normalized fractal composition, and bulk
filling into caller-owned memory. It has no runtime dependencies beyond the standard library.

Created by Ben Schneider — https://github.com/Ben-Scr

## Quick start

```cpp
#include <benscr/noise/generator.hpp>
#include <array>

namespace Noise = BenScr::Noise;

int main()
{
    Noise::Generator generator{42};
    const double height = generator.SampleFractal2D(0.25, -0.75);
    std::array<double, 64> field{};
    generator.FillFractal2D(field, {8, 8, -0.5, -0.5, 0.1, 0.1});
    return height >= -1.0 && height <= 1.0 ? 0 : 1;
}
```

See the [complete executable example](https://github.com/Ben-Scr/BenScr.Noise/blob/master/examples/noise.cpp).

## Install with native NuGet

In a Visual Studio native C++ project, install **BenScr.Noise 1.0.0** through NuGet Package
Manager, or its Package Manager Console:

```powershell
Install-Package BenScr.Noise -Version 1.0.0
```

Set the project's language standard to C++20 or newer. The native `packages.config`
integration imports `build/native/BenScr.Noise.props` and `.targets`: these add the include
directory and compile `src/generator.cpp` once with your project. Do not add that source a
second time. The package ships source, not a prebuilt ABI. Architecture, CRT selection,
optimization, Debug/Release settings, and debug symbols follow the consuming project.
The verified native integration is Windows x64, MSVC v143, Debug and Release. Managed
projects and `PackageReference` native integration are outside this package's tested surface.

## Install with CMake

Build and install the repository with CMake 3.28 or newer and a C++20 compiler:

```sh
cmake -S . -B out/build -DCMAKE_BUILD_TYPE=Release
cmake --build out/build --config Release
cmake --install out/build --config Release --prefix /your/prefix
```

In the consuming CMake project:

```cmake
find_package(BenScrNoise 1 CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE BenScr::Noise)
```

Configure it with `-DCMAKE_PREFIX_PATH=/your/prefix`. The installed static library is specific
to the compiler, architecture, and build configuration used to create it. Rebuild for other
toolchains or CRT settings. For `add_subdirectory`, link the same `BenScr::Noise` target;
tests and examples default to off when this library is not the top-level project.

## Contracts

| Operation | Behavior |
| --- | --- |
| `Sample2D`, `Sample3D` | Quintic-interpolated lattice value noise in `[-1,1]`; 2D equals the 3D slice at z=0. |
| `SampleFractal2D`, `SampleFractal3D` | Amplitude-weighted octave sum divided by total amplitude, bounded to `[-1,1]`. |
| `Fill2D`, `Fill3D` | Fill an exact-sized `std::span<double>` in row-major order, x fastest. |
| `FillFractal2D`, `FillFractal3D` | The same layout and sampling formula as individual fractal samples. |

All coordinates must be finite and within `[-1e9,1e9]`, including each scaled fractal
coordinate. `Octaves` is 1–32, `Frequency` is finite and positive, `Lacunarity` is finite
and at least 1, and `Gain` is finite in `[0,1]`. Defaults are `{5,1.0,2.0,0.5}`. Octave
frequencies and coordinates are validated even when an amplitude is zero. Octaves reuse
the same seed, multiplying coordinates by `Frequency * Lacunarity^octave` through successive
multiplication and amplitude by successive powers of `Gain`.

Grid steps may be negative or zero but must be finite. Coordinates are `Origin + index*Step`;
3D index is `(z*Height+y)*Width+x`. Each dimension is at most 1,048,576 and the total at most
16,777,216 doubles (128 MiB). Products are checked before multiplication. The span length
must match exactly. Zero dimensions require an empty span; origins, steps, settings, and
endpoints of nonzero axes are still validated. All validation completes before any writes.
Invalid coordinates, settings, or span lengths throw `std::invalid_argument`; size limits
throw `std::length_error`. Invalid calls leave the destination unchanged.

The generator owns only its 64-bit seed. It keeps no views, global state, or hidden caches.
Valid sampling and filling allocate no dynamic memory. Callers own output buffers and
must keep them valid and exclusively writable during a fill. Concurrent const calls are
safe when no thread assigns the same generator and output buffers do not overlap.

## Determinism and precision

Algorithm version 1 fixes the seed and integer hash contract. Negative lattice coordinates
convert to unsigned 64-bit values modulo 2^64; every arithmetic operation below wraps
modulo 2^64. Right shifts are logical. With lattice coordinates `(x,y,z)`:

```text
G  = 0x9E3779B97F4A7C15
P1 = 0xBF58476D1CE4E5B9
P2 = 0x94D049BB133111EB
h  = (seed + G) XOR (uint64(x)*G) XOR (uint64(y)*P1) XOR (uint64(z)*P2)
h  = (h XOR (h >> 30)) * P1
h  = (h XOR (h >> 27)) * P2
h  = h XOR (h >> 31)
lattice = double(h >> 11) * 2^-52 - 1
```

The containing cell uses `floor`, including negative coordinates. Each fractional axis
uses the clamped quintic weight `t^3*(t*(6*t-15)+10)`. Corners are interpolated along x,
then y, then z with `std::lerp`. The ideal function has continuous first and second
derivatives across cell boundaries; finite precision introduces rounding. This is value
noise, not a gradient/Perlin or Simplex implementation, and is not a cryptographic source.

IEEE 754 binary64 `double` is required at compile time. At the coordinate limit, representable
spacing is approximately 1.19e-7: smaller changes may collapse to the same input. Using
local coordinates preserves more fractional precision. The integer hash and lattice
conversion are exact under the default floating-point environment, while interpolation,
fractal accumulation, compiler contraction, and standard-library implementations can cause
small rounding differences. There is no cross-toolchain bit-identical floating-point promise.
Bulk and scalar evaluations may also differ by rounding when coordinate expressions or
arithmetic are compiled differently; use a tolerance for floating-point comparisons.
Do not enable fast-math or alter the default floating-point environment when relying on
the finite-input checks and documented bounds.

Frozen independent lattice vectors include seed 42 at `(0,0,0)` = `0.4831297575436466`
and `(-1,2,-3)` = `0.5360976881195421`. Independent half-cell references and the full
hash values are in [algorithm notes](https://github.com/Ben-Scr/BenScr.Noise/blob/master/docs/ALGORITHM.md).

## Verification and license

The [verification workflow](https://github.com/Ben-Scr/BenScr.Noise/actions/workflows/ci.yml)
builds Windows/MSVC and Linux/GCC 14 and Clang 18, including Linux ASan/UBSan. It runs
contract tests, a standalone-header check, an executable example, a relocated CMake
consumer, and package-content checks. Windows additionally restores the actual native
NuGet archive into fresh `packages.config` consumers in Debug and Release. Exact executed
results belong to each run's artifact manifest; CI configuration alone is not a passed run.
macOS, ARM, 32-bit builds, MSVC sanitizers, and alternate floating-point modes are unverified.

[Release verification](https://github.com/Ben-Scr/BenScr.Noise/blob/master/docs/RELEASE.md)
describes pinned tools and public-feed verification. See
[migration notes](https://github.com/Ben-Scr/BenScr.Noise/blob/master/MIGRATION.md) and
[provenance](https://github.com/Ben-Scr/BenScr.Noise/blob/master/docs/PROVENANCE.md).
The custom [license](https://github.com/Ben-Scr/BenScr.Noise/blob/master/LICENSE.txt) requires
attribution, including **Created by Ben Schneider — https://github.com/Ben-Scr** in user-facing
software as specified by the license.
