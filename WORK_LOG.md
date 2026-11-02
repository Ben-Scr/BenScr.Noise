# Verification work log

- Established the version 1 seed hash, coordinate limits, grid memory contract, and normalized
  octave semantics before implementation.
- Added independently derived lattice and half-cell references, negative-coordinate and
  continuity checks, finite-range checks, overflow rejection, bulk equivalence, and concurrent
  const sampling tests. Release builds retain all contract checks.
- Added standalone-header compilation, executable examples, relocatable CMake exports, and
  native `packages.config` source-consumer verification in Debug and Release.
- Release evidence is generated per clean source commit in `release-manifest.json` and the
  CI artifacts. Public-feed verification has its own report and is not assumed by local tests.
