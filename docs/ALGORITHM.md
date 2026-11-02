# Algorithm version 1

The [README](https://github.com/Ben-Scr/BenScr.Noise/blob/master/README.md#determinism-and-precision)
defines the integer mapping and interpolation order. The following reference values were
derived independently using arbitrary-precision integer operations explicitly reduced modulo
2^64; they were not obtained by recording this library's output.

| Seed | Lattice x,y,z | Final hash | Value |
| --- | --- | --- | --- |
| 0 | 0,0,0 | `e220a8397b1dcdaf` | 0.7666216164272852 |
| 42 | 0,0,0 | `bdd732262feb6e95` | 0.4831297575436466 |
| 42 | 1,0,0 | `a759ea27d4727622` | 0.30743147797410897 |
| 42 | -1,2,-3 | `c49ed95af7a25033` | 0.5360976881195421 |
| 18446744073709551615 | 1000000000,-1000000000,0 | `74829d5e6a803823` | -0.0897639550194056 |

At t=0.5 the quintic weight is exactly 0.5, so summing independently hashed corners gives
seed 42's 2D `(0.5,0.5)` value `0.2849549233317711` and 3D `(0.5,0.5,0.5)` value
`-0.10635038160140473`. At t=0.25 the exact weight is `0.103515625`. Tests use these
independent expectations rather than a second call to the implementation under test.
Lattice tests require exact results; interpolation tests allow 1e-14 absolute difference.
The boundary-continuity check uses 1e-9 for points 1e-5 either side of a lattice plane.
Those are test tolerances, not universal mathematical error bounds.

The unsigned mixer follows the well-known SplitMix64 finalization constants. This library
defines a separate coordinate hash, and has no SplitMix64 stream-state API. No external
implementation source or third-party runtime is incorporated. All interpolation uses the
C++ standard library and the stated elementary quintic polynomial.

Changing seed mapping, interpolation order, normalization, or coordinate limits requires
an explicit compatibility decision and updated reference vectors. Public declarations are
tracked by the normalized-LF SHA-256 in `docs/public-header-baseline.json`.
