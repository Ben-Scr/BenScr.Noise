# Provenance and attribution

Created by Ben Schneider — https://github.com/Ben-Scr

The noise API and implementation are new for this repository; no earlier application or
private source was migrated. The original public repository is
https://github.com/Ben-Scr/BenScr.Noise. Each package's repository metadata records its exact
source commit. The [algorithm notes](https://github.com/Ben-Scr/BenScr.Noise/blob/master/docs/ALGORITHM.md)
identify the public numerical constants and independent reference derivation.

`LICENSE.txt` is the unchanged Ben Schneider license used by BenScr.MathParser. The original
copyright year and attribution requirements are preserved. Package verification compares
the archive's license against this source file byte for byte.

`branding/BenScr_Icon_Master.png` is the supplied, unchanged BenScr master logo: 1254×1254
RGBA PNG, 795819 bytes, SHA-256
`a5dcabceb766e13c1840a95130ee98a458e12e3713fd9b07f3ceae75a1f5b993`.
It is embedded without recoloring, resizing, or replacing transparency as `icon.png`.
The `.nuspec` points directly to that embedded file, and the archive audit verifies its bytes.

Build and native-source package layout follow the established BenScr.Console conventions.
No console domain code or repository history is included. The runtime library has no external
dependencies. NuGet.CommandLine is a pinned verification tool downloaded from its official
NuGet package; it is not shipped in the library archive.
