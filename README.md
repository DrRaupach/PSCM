# PSCM
Planck Star Color Mapping - SCRIPT for Pleiades Astrophoto PixInsight

<p><b>Motivation:</b><br>
Stars in color images composed from narrowband data do not exhibit their true colors as would be expected from visual observation or from an (L)RGB acquisition. Even spectrophotometric calibration procedures such as SPCC do not resolve this issue, since they apply only linear corrections. While the resulting intensities at the respective emission-line wavelengths are physically correct, the stellar colors are not realistic.
Unless RGB data have already been acquired — for example, to represent reflection nebulae — they are often added specifically to obtain visually consistent stellar colors. However, unfavorable weather conditions frequently prevent the acquisition of supplementary RGB data. The concept presented here offers a method to derive realistic stellar colors directly from narrowband data alone.
</p>
<p><b>The Idea of PSCM:</b><br>
Stars are, to first approximation, ideal blackbody radiators. According to Planck’s radiation law (PRL), they emit a characteristic spectrum determined by their surface temperature. Under this assumption, the color of a star is uniquely defined by its temperature. If the temperature can be estimated from arbitrary spectral measurements, the corresponding color can be computed from Planck’s law.
Due to the mathematical properties of the Planck distribution, it is sufficient to know the (calibrated) intensity ratio at two different wavelengths. The approach therefore consists of first determining the stellar temperature from intensities measured in narrowband data and then, in reverse, synthesizing the color that the corresponding blackbody spectrum would exhibit in an RGB image.
</p>
<p><b>PixInsight Integration:</b><br>
To use PSCM in PixInsight add the following repository link in RESOURCES -> Updates -> Manage Repositories: https://drraupach.github.io/PSCM/<br>
Requires minimum PixInsight version 1.9.3 with update-rsc.auth equal to or newer than Feb 13th 2026.<br>
</p><br>

<p><b>Typical workflow, exemplarily for HOO:</b>
<ol>
<li>Load matching LINEAR Ha and OIII images after DBE/Graxpert.</li>
<li>Combine to HOO color image using ChannelCombination.</li>
<li>Apply ImageSolver to find astrometric solution on HOO image.</li>
<li>Apply SPCC with 'Red filter' at 656.3, 'Green/Blue filter' at 500.7 in 'Narrowband mode' and 'Optimize for Stars' checked. The white reference should not be ''too hot''. 'Average Galaxy' (~4500K) is a good choice.</li>
<li>Derive the Starless image (e.g. by SXT), also the Stars in unscreen mode.<br/>
[Stars can also be calculated manually using PixelMath by ~(~HOO / ~Starless).]</li>
<li>Apply PSCM to the Star image which transforms the HOO colors to black body colors according to the stars' temperatures.</li>
<li>Combine the PSCM mapped Stars with the Starless by screening — e.g. using PixelMath by ~(~Starless * ~Stars).</li>
</ol>
Voilà! You now have a bi-color HOO image with (almost) naturally colored stars without the need of an additional RGB image, still in lineal domain.<br>
</p><br>

<p>
<b>Notes</b><br>
<ul>
<li>It turns out that in the approximated formulation, the white reference temperature is irrelevant and therefore does not need to be specified in SPCM. A more detailed analysis shows that the approximation holds well when the white reference point lies in the range of 4000–6000 K. Accordingly, color calibration using an “average galaxy” (~4500 K) is appropriate, but a G2V reference star (~5780 K), such as the Sun, also works well.</li>
<li>For color calibration with SPCC prior to applying SPCM, wavelengths of 672.4 nm (SII), 656.3 nm (Hα), and 500.7 nm (OIII) are appropriate, with bandwidths corresponding to the respective narrowband filters and with the options “Narrowband mode” and “Optimize for stars” enabled. In the case of RGB data, the standard filters for the given imaging setup may be used.</li>
<li>The input image must be a color image with three channels. However, in certain cases two channels may be identical, as in an “HOO” composition.</li>
<li>SPCM requires linear images, since only under linear conditions are intensity ratios preserved correctly. Application to stretched (nonlinear) images is not recommended.</li>
<li>All input data — whether a combination of narrowband or RGB data — must have identical resolution (i.e., point spread function). Otherwise, chromatic halos present in the input will propagate into corresponding blackbody-color artifacts. Application of BXT may improve convergence of the color channels where necessary.</li>
<li>Background gradients should be removed (e.g., using GraXpert), since they may otherwise translate into color errors in the transformed image.</li>
<li>SPCM can be applied not only to combinations of narrowband data but also to RGB images. In this case, colors are constrained to physically plausible blackbody colors, even if the RGB intensity ratios after SPCC are not ideal. For example, a green cast can be removed. If the green channel in particular is affected by errors, the input image type “RGB (ignore G)” is recommended.</li>
</ul>
</p>
