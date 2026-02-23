# PSCM
Planck Star Color Mapping - SCRIPT for Pleiades Astrophoto PixInsight


Motivation:<br/>
Stars in color images composed from narrowband data do not exhibit their true colors as would be expected from visual observation or from an (L)RGB acquisition. Even spectrophotometric calibration procedures such as SPCC do not resolve this issue, since they apply only linear corrections. While the resulting intensities at the respective emission-line wavelengths are physically correct, the stellar colors are not realistic.
Unless RGB data have already been acquired — for example, to represent reflection nebulae — they are often added specifically to obtain visually consistent stellar colors. However, unfavorable weather conditions frequently prevent the acquisition of supplementary RGB data. The concept presented here offers a method to derive realistic stellar colors directly from narrowband data alone.

The Idea of PSCM:<br/>
Stars are, to first approximation, ideal blackbody radiators. According to Planck’s radiation law (PRL), they emit a characteristic spectrum determined by their surface temperature. Under this assumption, the color of a star is uniquely defined by its temperature. If the temperature can be estimated from arbitrary spectral measurements, the corresponding color can be computed from Planck’s law.
Due to the mathematical properties of the Planck distribution, it is sufficient to know the (calibrated) intensity ratio at two different wavelengths. The approach therefore consists of first determining the stellar temperature from intensities measured in narrowband data and then, in reverse, synthesizing the color that the corresponding blackbody spectrum would exhibit in an RGB image.

To use PSCM in PixInsight add the following repository link in RESOURCES -> Updates -> Manage Repositories: https://drraupach.github.io/PSCM/

Requires minimum PixInsight version 1.8.9 with update-rsc.auth equal to or newer than Feb 13th 2026.
