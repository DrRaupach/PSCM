# Planck Star Color Mapping — PixInsight Process

This directory contains a native C++ PixInsight process port of
[`PlanckStarColorMapping.js`](../../scripts/DrRaupach/PSCM/PlanckStarColorMapping.js).
The module changes the active image in place and provides the same processing
choices and user-facing numeric controls.

## Project layout

```text
PlanckStarColorMapping/
  docs/                 Process specification, build notes, transfer guide
  ../sdk/PCL/<PI-version>/ Shared PixInsight PCL source snapshots
  skills/               Place for project-specific agent/developer skills
  PSCMProcess.*         Process and parameter metadata
  PSCMInstance.*        Per-instance parameters and pixel algorithm
  PSCMInterface.*       PixInsight user interface
  PSCMModule.cpp        Native module entry point and module metadata
```

PCL snapshots are kept locally by PixInsight version and ignored by version
control to avoid bundling host/toolchain-specific resources. They are shared
under `../sdk/PCL/`, one level above the process project. Snapshots for
PixInsight 1.8.9-3, 1.9.3, and 1.9.6 are available there; their PCL licenses
and provenance are documented beside each snapshot. The process currently
specifies PixInsight 1.9.3 as its minimum. A local Windows x64 module build for
PixInsight 1.8.9-3 is available, but it has not been installed or tested in
that host and does not change the declared minimum. See
[SDK setup](docs/BUILDING.md) and [the shared SDK index](../sdk/PCL/README.md).

## Controls

| UI parameter | Process parameter | Range | Default |
| --- | --- | ---: | ---: |
| Image Input Type | `imageType` | HOO, HSO, SHO, RGB, RGB (ignore G) | HOO |
| Color Saturation Factor | `colorSaturationFactor` | 0.00–2.00 | 1.00 |
| Protect Background (X*MAD) | `protectBackground` | 0.5–12.0 | 1.5 |
| Unphysical | `spreadSpectralClasses` | enabled/disabled | disabled |
| Spread Spectral Classes by | `spectralSpread` | 1.00–2.00 | 1.00 |

Each numeric control has a PixInsight-style clear/reset icon that restores its
factory default from the process parameter metadata. The standard process
toolbar also provides real-time preview; parameter changes refresh the preview
using the active view's median and MAD statistics.

The process instance stores all five values, so process icons and saved process
instances remain reproducible. Unlike the script, the native process does not
read/write the script's `Settings` keys.

See the [UI mockup](docs/ui-mockup.html) for a visual preview. It is
illustrative; exact styling depends on PixInsight's native interface theme.

## Image requirements

The active target must be a non-complex image with at least three channels.
Processing is intended for linear, color-calibrated data. The first three
channels are mapped as RGB; any additional channels are left unchanged.
Statistics are computed from the target view. The process updates pixel data
in place and supports PixInsight's normal undo behavior.

## Algorithm notes

The port preserves the source's wavelength assignments, MAD-based channel
ordering, background weighting, ratio regularization, variance-weighted
two-pair estimate, linear RGB/Lab/LCh conversions, saturation control, and
spectral-spread adjustment. Exponential blackbody ratios are evaluated after
subtracting the largest exponent, which is algebraically equivalent after
normalization and avoids overflow. Zero-signal and zero-MAD cases are guarded
to avoid propagating NaNs.

The mapping is not a substitute for color calibration and is not recommended
for stretched images.

## User interface

The interface shows the complete workflow help text from the JavaScript
script, including its explicit line breaks. Its real-time preview uses the same
mapping implementation and full-view median/MAD statistics as process
execution. See the [UI mockup](docs/ui-mockup.html) for a visual preview; the
native interface still requires an in-host smoke test.

## Status

Source and process metadata are prepared. Windows x64 modules compile and link
against PCL 2.7.0 for PixInsight 1.8.9-3 and PCL 2.15.0 for PixInsight 1.9.6.
The 1.8.9-3 Windows build embeds the icon as XPM, and the 1.9.6 Windows build
embeds its SVG. Neither build has been loaded or smoke-tested in its
corresponding host. A separate build and smoke test against the declared 1.9.3
minimum is still required.

Project-owned macOS x86_64 build files are prepared for PCL 2.7.0, but a
`.dylib` requires compilation on macOS and has not been produced in this
Windows environment.

Project-owned Apple Silicon arm64 build files are also prepared. They derive
arm64 makefiles from the PCL 2.7.0 macOS x64 makefiles and target
`lib/macos/arm64`; host compilation and verification are still required.
