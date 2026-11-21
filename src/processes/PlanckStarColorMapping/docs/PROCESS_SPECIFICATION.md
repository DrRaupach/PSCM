# Process specification

## Host behavior

- Process ID: `PlanckStarColorMapping`
- Category: `ColorSpaces`
- Target: active view, modified in place
- Undo: pixel data
- Accepted images: non-complex, three or more channels
- Channels: RGB triplet at indices 0–2 is transformed; subsequent channels are
  untouched

## Image-type wavelength pairs

Wavelengths are in nanometers. For each pair, the channel with lower MAD is
placed first, as in the JavaScript implementation.

| Type | Candidate pairs |
| --- | --- |
| HOO | Ha 656.3 / OIII 500.7 |
| HSO | Ha 656.3 / OIII 500.7; SII 672.4 / OIII 500.7 |
| SHO | SII 672.4 / OIII 500.7; Ha 656.3 / OIII 500.7 |
| RGB | R 622.0 / G 530.0; R 622.0 / B 476.0 |
| RGB (ignore G) | R 622.0 / B 476.0 |

Each numeric slider has an adjacent PixInsight-style reset icon that restores
its factory default from the corresponding process parameter metadata:
saturation 1.00, background protection 1.5, or spectral spread 1.00.

The spread checkbox is labeled "Unphysical".

The interface includes the standard real-time preview control. Preview
generation applies the same mapping and active-view median/MAD statistics as
normal process execution, and updates when process parameters change. Complex
or fewer-than-three-channel images are not previewed.

The interface displays the complete workflow help text from the JavaScript
script, preserving its explicit line breaks.

For each pair the process estimates the inverse-temperature difference from
the background-subtracted intensity ratio and the pair wavelengths. In two-pair
modes, the two estimates are mixed using their estimated variances. It then
replaces the input hue in LCh with the blackbody hue, retaining the original
lightness and scaling chroma using the selected saturation and background
protection controls.

## Parameter persistence

PixInsight process instances serialize the process parameters as
`imageType`, `colorSaturationFactor`, `protectBackground`,
`spreadSpectralClasses`, and `spectralSpread`. The native process does not
automatically migrate old JavaScript `Settings` entries or old process icons.
The source script remains the reference for UI defaults and behavior.

## Known compatibility differences

- The source script writes selected controls to global PixInsight `Settings`;
  this process instead persists values in process instances.
- Zero-MAD, zero-denominator and extreme exponential cases are explicitly
  stabilized. Their normal-signal output follows the source algorithm.
- The source script reports median/MAD in its console; this initial native port
  does not print those diagnostics.

These are intentional host- and numerical-safety differences, not changes to
the image-type mappings or the normal-input color transform.
