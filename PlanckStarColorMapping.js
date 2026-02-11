#feature-id    PlanckStarColorMapping : Utilities > PlanckStarColorMapping

#feature-info \
Maps HOO/HSO/SHO/... stars to naturally colored stars using Planck's law of black body radiation.<br/>\
Copyright (C) 2026 Dr. Rainer Raupach<br/>

#define TITLE "Planck Star Color Mapping (PSCM)"
#define VERSION "V1.1.5"
#define DEVELOPER "Dr. Rainer Raupach"

#define DEFAULT_COLOR_SATURATION (1.0)
#define DEFAULT_PROTECT_BACKGROUND (6)
#define DEFAULT_SPECTRAL_SPREAD (1.0)

#define LAMBDA_SII (672.4)
#define LAMBDA_HA (656.3)
#define LAMBDA_OIII (500.7)
#define LAMBDA_R (622.0)
#define LAMBDA_G (530.0)
#define LAMBDA_B (476.0)

#include <pjsr/UndoFlag.jsh>
#include <pjsr/SectionBar.jsh>
#include <pjsr/NumericControl.jsh>
#include <pjsr/DataType.jsh>

var myConfig = {};

Object.defineProperty(myConfig, "imageTypeOptions", {
   value: ["HOO", "HSO", "SHO", "RGB", "RGB (ignore G)"],
   writable: false,
   enumerable: true,
   configurable: false
});

// Handling of UI parameter restoring
var SETTINGS_KEY_BASE = "PSCM/";
var KEY_IMAGETYPE = SETTINGS_KEY_BASE + "imageType";
var KEY_SATURATION_VALUE = SETTINGS_KEY_BASE + "saturationValue";
var KEY_PROTECTBGR_VALUE = SETTINGS_KEY_BASE + "protectBgrValue";
var KEY_UNPHYSICAL_VALUE = SETTINGS_KEY_BASE + "unphysicalState";
var KEY_SPREADING_VALUE = SETTINGS_KEY_BASE + "spreadingValue";


// -----------------------------------------------------------------------------
function ScaleImageDialog() {

    this.__base__ = Dialog;
    this.__base__();

    // Title
    this.windowTitle = "Planck Star Color Mapping (PSCM)";

    this.helpLabel = new Label(this);
    this.helpLabel.styleSheet = this.scaledStyleSheet(
                                   "QWidget#" + this.helpLabel.uniqueId + " {"
                                   + "border: 1px solid gray;"
                                   + "padding: 0.25em;"
                                   + "}");
    this.helpLabel.wordWrapping = true;
    this.helpLabel.useRichText = true;
    this.helpLabel.text = "<p><strong>" + TITLE + " version " + VERSION + "</strong><br/>"
                          + "Copyright &copy; 2026 " + DEVELOPER + "</p>";

    this.info = new Label(this);
    this.info.margin = 0;
    this.info.text = "How to use PSCM?\n" + "(example/typical workflow for HOO)\n\n"
                        + "1.  Load matching LINEAR Ha and OIII images after DBE/Graxpert.\n\n"
                        + "2.  Combine to HOO color image using ChannelCombination.\n\n"
                        + "3.  Apply ImageSolver to find astrometric solution on HOO image.\n\n"
                        + "4.  Apply SPCC with 'Red filter' at 656.3, 'Green/Blue filter' at 500.7 in \n"
                        + "     'Narrowband mode' and 'Optimize for Stars' checked. The white reference \n"
                        + "     should not be ''too hot''. 'Average Galaxy' (~4500K) is a good choice.\n\n"
                        + "5.  Derive the Starless image (e.g. by SXT), also the Stars in unscreen mode.\n"
                        + "     [Stars can also be calculated manually using PixelMath by ~(~HOO / ~Starless)]\n\n"
                        + "6.  Apply PSCM to the Star image which transforms the HOO colors\n"
                        + "     to black body colors according to the stars' temperatures.\n\n"
                        + "7.  Combine the PSCM mapped Stars with the Starless by screening,\n"
                        + "     e.g. using PixelMath by ~(~Starless * ~Stars)\n\n"
                        + "Voilà! You now have a bi-color HOO image with (almost) naturally colored stars\n"
                        + "without the need of an additional RGB image, still in lineal domain.\n"
                        + "Continue with stretching and further post-processing as usual.\n\n"
                        + "In general: the wavelengths in SPCC for S|H|O should be 672.4|656.3|500.7\n"
                        + "assigned to the respective filter slot. For RGB, the standard filters can be used.\n";

    // Input image type
    this.imageTypeLabel = new Label(this);
    this.imageTypeLabel.text = "Input Image Type:";
    this.imageTypeLabel.textAlignment = TextAlign_Right | TextAlign_VertCenter;
    this.imageTypeLabel.minWidth = 200;

    this.imageType = new ComboBox(this);
    for (var n = 0; n < myConfig.imageTypeOptions.length; n++) {
      this.imageType.addItem(myConfig.imageTypeOptions[n]);
    }
    this.imageType.currentItem = Settings.read(KEY_IMAGETYPE, DataType_Int32);


    this.imageTypeSizer = new HorizontalSizer;
    this.imageTypeSizer.spacing = 4;
    this.imageTypeSizer.add(this.imageTypeLabel);
    this.imageTypeSizer.add(this.imageType, 100);

    // Saturation factor
    this.colorSaturationEdit = new NumericControl(this);
    this.colorSaturationEdit.label.text = "Color Saturation Factor:";
    this.colorSaturationEdit.toolTip = "Color Saturation Factor. Increased saturation may increase color errors of the input image.";
    this.colorSaturationEdit.label.minWidth = 200;
    this.colorSaturationEdit.setRange(0.00, 2.00);
    this.colorSaturationEdit.setPrecision(2);
    this.colorSaturationEdit.slider.setRange(0, 200);
    this.colorSaturationEdit.slider.stepSize = 1;
    this.colorSaturationEdit.slider.pageSize = 10;
    let saturationValue = Settings.read(KEY_SATURATION_VALUE, DataType_Double);
    if (null == saturationValue) saturationValue = DEFAULT_COLOR_SATURATION;
    this.colorSaturationEdit.setValue(saturationValue); // must be set at the end!

    // Protect background
    this.protectBackgroundEdit = new NumericControl(this);
    this.protectBackgroundEdit.label.text = "Protect Background (X*MAD):";
    this.protectBackgroundEdit.toolTip = "Multiple of background noise (MAD).";
    this.protectBackgroundEdit.label.minWidth = 200;
    this.protectBackgroundEdit.setRange(0.5, 12.0);
    this.protectBackgroundEdit.setPrecision(1);
    this.protectBackgroundEdit.slider.setRange(0, 100);
    this.protectBackgroundEdit.slider.stepSize = 1;
    this.protectBackgroundEdit.slider.pageSize = 10;
    let protectBgrValue = Settings.read(KEY_PROTECTBGR_VALUE, DataType_Double);
    if (null == protectBgrValue) protectBgrValue = DEFAULT_PROTECT_BACKGROUND;
    this.protectBackgroundEdit.setValue(protectBgrValue); // must be set at the end!

    // Spectral spreading
    this.spreadSpectralClass = new NumericControl(this);
    this.spreadSpectralClass.label.text = "Spread Spectral Classes by:";
    this.spreadSpectralClass.toolTip = "Artifically spreads the color temperature with respect to the white reference, i.e. makes stars hotter than the white reference even hotter and vice versa.";
    this.spreadSpectralClass.label.minWidth = 200;
    this.spreadSpectralClass.setRange(1.00, 2.00);
    this.spreadSpectralClass.setPrecision(2);
    this.spreadSpectralClass.slider.setRange(0, 100);
    this.spreadSpectralClass.slider.stepSize = 1;
    this.spreadSpectralClass.slider.pageSize = 10;
    let spreadingValue = Settings.read(KEY_SPREADING_VALUE, DataType_Double);
    if (null == spreadingValue) spreadingValue = DEFAULT_SPECTRAL_SPREAD;
    this.spreadSpectralClass.setValue(spreadingValue); // must be set at the end!

    this.unphysicalContent = new Control(this);
    //this.unphysicalContent.hide();

    this.unphysicalContent.sizer = new VerticalSizer;
    this.unphysicalContent.sizer.add(this.spreadSpectralClass);

    this.unphysicalBar = new SectionBar(this, "Unphysical");
    this.unphysicalBar.enableCheckBox();
    let unphysicalState = Settings.read(KEY_UNPHYSICAL_VALUE, DataType_Boolean);
    if (null == unphysicalState) unphysicalState = false;
    this.unphysicalBar.checkBox.checked = unphysicalState;
    this.unphysicalBar.setSection(this.unphysicalContent);

    // OK / Cancel Buttons
    this.ok_Button = new PushButton(this);
    this.ok_Button.text = "OK";
    this.ok_Button.onClick = () => {
        this.ok();
    };

    this.cancel_Button = new PushButton(this);
    this.cancel_Button.text = "Cancel";
    this.cancel_Button.onClick = () => {
        this.cancel();
    };

    // Layout
    this.buttonSizer = new HorizontalSizer;
    this.buttonSizer.spacing = 10;
    this.buttonSizer.addStretch();
    this.buttonSizer.add(this.ok_Button);
    this.buttonSizer.add(this.cancel_Button);

    this.sizer = new VerticalSizer;
    this.sizer.margin = 10;
    this.sizer.spacing = 10;
    this.sizer.add(this.helpLabel);
    this.sizer.add(this.info);
    this.sizer.add(this.imageTypeSizer);
    this.sizer.add(this.colorSaturationEdit);
    this.sizer.add(this.protectBackgroundEdit);
    this.sizer.add(this.unphysicalBar);
    this.sizer.add(this.unphysicalContent);
    this.sizer.add(this.buttonSizer);

    this.adjustToContents();
}

// -----------------------------------------------------------------------------
// Convert LCH -> RGB
function lchToRgb(L, C, h)
{
    // --- LCH → Lab ---
    var hr = h * Math.PI / 180;
    var a = C * Math.cos(hr);
    var b = C * Math.sin(hr);

    // --- Lab → XYZ ---
    var fy = (L + 16) / 116;
    var fx = fy + a / 500;
    var fz = fy - b / 200;

    function finv(t)
    {
        var t3 = t*t*t;
        return (t3 > 0.008856) ? t3
                               : (t - 16/116) / 7.787;
    }

    var X = finv(fx);
    var Y = finv(fy);
    var Z = finv(fz);

    // --- Denormalize D65 ---
    X *= 0.95047;
    Y *= 1.00000;
    Z *= 1.08883;

    // --- XYZ → linear RGB ---
    var r =  3.2404542*X - 1.5371385*Y - 0.4985314*Z;
    var g = -0.9692660*X + 1.8760108*Y + 0.0415560*Z;
    var b2 = 0.0556434*X - 0.2040259*Y + 1.0572252*Z;

    // --- clamp ---
    function clamp(x)
    {
        return x < 0 ? 0 : (x > 1 ? 1 : x);
    }

    return {
        r: clamp(r),
        g: clamp(g),
        b: clamp(b2)
    };
}

// -----------------------------------------------------------------------------
// Convert RGB -> LCH
function rgbToLch(r, g, b)
{
    // --- linear RGB → XYZ (D65) ---
    var X = 0.4124564*r + 0.3575761*g + 0.1804375*b;
    var Y = 0.2126729*r + 0.7151522*g + 0.0721750*b;
    var Z = 0.0193339*r + 0.1191920*g + 0.9503041*b;

    // --- Normalize by D65 white point ---
    X /= 0.95047;
    Y /= 1.00000;
    Z /= 1.08883;

    // --- XYZ → Lab ---
    function f(t)
    {
        return (t > 0.008856) ? (t < 0 ? -Math.pow(-t, 1/3) : Math.pow(t, 1/3))
                              : (7.787*t + 16/116);
    }

    var fx = f(X);
    var fy = f(Y);
    var fz = f(Z);

    var L = 116*fy - 16;
    var a = 500*(fx - fy);
    var b2 = 200*(fy - fz);

    // --- Lab → LCH ---
    var C = Math.sqrt(a*a + b2*b2);
    var h = Math.atan2(b2, a) * 180 / Math.PI;
    if (h < 0) h += 360;

    return {
        L: L,
        C: C,
        h: h
    };
}

// -----------------------------------------------------------------------------
// Gets the color for a given temperature in LCh
function getLChForRatio(dbeta)
{
   let r = Math.exp(dbeta/LAMBDA_R);
   let g = Math.exp(dbeta/LAMBDA_G);
   let b = Math.exp(dbeta/LAMBDA_B);
   let rgbNorm = Math.max(r, g, b);
   r /= rgbNorm;
   g /= rgbNorm;
   b /= rgbNorm;

   var lch = rgbToLch(r, g, b);
   var drgb = 1.0 - r*g*b;
   drgb *= drgb;
   var Cf = 1.0 - Math.exp(-drgb/1e-12);

   return {
      L: lch.L,
      C: lch.C,
      h: lch.h,
      Cf: Cf
   };
}

// -----------------------------------------------------------------------------
// Build settings for selected image type
function InitForImageType(imageType, mad) {

   function findIndex(array, predicate) {
      for (var i = 0; i < array.length; ++i) {
         if (predicate(array[i])) {
            return i;
         }
      }
      return -1;
   }

   // find matching index in image type presets
   let match = findIndex(myConfig.imageTypeOptions, function(v) {return v == imageType} );

   if (match == -1) {
      throw new Error("No configuration found for " + imageType + ".");
   }

   function WaveLengthPair(index0, lambda0, index1, lambda1) {
      this.index0 = index0;
      this.lambda0 = lambda0;
      this.index1 = index1;
      this.lambda1 = lambda1;
   }

   var wavelengthPairs = [];

   switch (imageType) {
      case "HOO":
         if (mad.at(2) < mad.at(0)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_OIII, 0, LAMBDA_HA));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_HA, 2, LAMBDA_OIII));
         }
         break;

      case "HSO":
         if (mad.at(2) < mad.at(0)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_OIII, 0, LAMBDA_HA));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_HA, 2, LAMBDA_OIII));
         }
         if (mad.at(2) < mad.at(1)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_OIII, 1, LAMBDA_SII));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(1, LAMBDA_SII, 2, LAMBDA_OIII));
         }
         break;

      case "SHO":
         if (mad.at(2) < mad.at(0)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_OIII, 0, LAMBDA_SII));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_SII, 2, LAMBDA_OIII));
         }
         if (mad.at(2) < mad.at(1)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_OIII, 1, LAMBDA_HA));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(1, LAMBDA_HA, 2, LAMBDA_OIII));
         }
         break;

      case "RGB":
         if (mad.at(0) < mad.at(1)) {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_R, 1, LAMBDA_G));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(1, LAMBDA_G, 0, LAMBDA_R));
         }
         if (mad.at(0) < mad.at(2)) {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_R, 2, LAMBDA_B));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_B, 0, LAMBDA_R));
         }
         break;

      case "RGB (ignore G)":
         if (mad.at(2) < mad.at(0)) {
            wavelengthPairs.push(new WaveLengthPair(2, LAMBDA_B, 0, LAMBDA_R));
         }
         else {
            wavelengthPairs.push(new WaveLengthPair(0, LAMBDA_R, 2, LAMBDA_B));
         }
         break;

      default:
         throw new Error("Init not defined for " + imageType + ".");
   }

   return wavelengthPairs;
}

// -----------------------------------------------------------------------------
function process(dialog) {

    var window = ImageWindow.activeWindow;
    if (window.isNull) {
        throw new Error("No active image window found.");
    }

    var view = window.currentView;
    view.beginProcess(UndoFlag_PixelData);

    // get image data
    var img = view.image;

    // check image properties
    if (img.numberOfChannels < 3) {
       throw new Error("Image must be a color image. Monochrome image are not allowed.");
    }

    // analyze image with respect to background and noise
    var median = view.computeOrFetchProperty("Median");
    console.writeln("Median of image: ("
       + median.at(0).toExponential(4) + ", "
       + median.at(1).toExponential(4) + ", "
       + median.at(2).toExponential(4) + ")");

    var mad = view.computeOrFetchProperty("MAD");
    console.writeln("MAD of image: ("
       + mad.at(0).toExponential(4) + ", "
       + mad.at(1).toExponential(4) + ", "
       + mad.at(2).toExponential(4) + ")");

    // Initialize for selected image type
    var pars = InitForImageType(dialog.imageType.itemText(dialog.imageType.currentItem), mad);

    function saturationCorrFactor(dbeta, C) {

       let lchPixelNeutral = getLChForRatio(dbeta);
       let cCorrFactor = C / Math.max(lchPixelNeutral.C, 1e-3);
       if (dbeta < 0) { // hotter stars
          cCorrFactor = Math.pow(cCorrFactor, 0.5);
       }
       else { // cooler stars
          cCorrFactor = Math.pow(cCorrFactor, 2);
       }

       return cCorrFactor;
    }

    // get parameters from UI
    var colorSaturationFactor = dialog.colorSaturationEdit.value;
    var fSpectralClass = dialog.unphysicalBar.checkBox.checked ? dialog.spreadSpectralClass.value : 1.0;
    var g = dialog.protectBackgroundEdit.value;

    switch (pars.length) {

       // process data with one pair to evaluate
       case 1:
          let indexCh0 = pars[0].index0;
          let indexCh1 = pars[0].index1;

          let ch0Bgr = median.at(indexCh0);
          let ch1Bgr = median.at(indexCh1);

          let ch0MAD = mad.at(indexCh0);

          let lambda0 = pars[0].lambda0;
          let lambda1 = pars[0].lambda1;

          let lambdaFactor = lambda0*lambda1 / (lambda0 - lambda1);

          for (var y = 0; y < img.height; y++) {
             for (var x = 0; x < img.width; x++) {

                let ch0 = img.sample(x, y, indexCh0) - ch0Bgr;
                let ch1 = img.sample(x, y, indexCh1) - ch1Bgr;

                // weighting function (to protect background)
                let w = 1.0 - Math.exp(-ch0*ch0/ch0MAD/ch0MAD/g/g);

                // ch1/ch0 ratio (regularized to 1.0 at low signal)
                let R =  w * Math.min(Math.abs(ch1/ch0), 1.0e3) + (1 - w);

                // inverse temperature difference
                let dbeta = Math.log(R) * lambdaFactor;

                // convert ratio to color in LCh
                let lchPixel = getLChForRatio(fSpectralClass * dbeta);

                let cFactor = colorSaturationFactor;
                cFactor *= lchPixel.Cf;
                cFactor *= fSpectralClass > 1.0 ? saturationCorrFactor(dbeta, lchPixel.C) : 1.0;

                // convert original pixel to LCh
                let LCH = rgbToLch(img.sample(x, y, 0), img.sample(x, y, 1), img.sample(x, y, 2));

                // replace color and convert to RGB
                let RGB = lchToRgb(LCH.L, cFactor * LCH.C, lchPixel.h);

                img.setSample(RGB.r, x, y, 0);
                img.setSample(RGB.g, x, y, 1);
                img.setSample(RGB.b, x, y, 2);
             }
          }
          break;

       // process data with two pairs to evaluate
       case 2:
          let indexCh00 = pars[0].index0;
          let indexCh01 = pars[0].index1;
          let indexCh10 = pars[1].index0;
          let indexCh11 = pars[1].index1;

          let ch00Bgr = median.at(indexCh00);
          let ch01Bgr = median.at(indexCh01);
          let ch10Bgr = median.at(indexCh10);
          let ch11Bgr = median.at(indexCh11);

          let ch00MAD = mad.at(indexCh00);
          let ch01MAD = mad.at(indexCh01);
          let ch10MAD = mad.at(indexCh10);
          let ch11MAD = mad.at(indexCh11);

          let lambda00 = pars[0].lambda0;
          let lambda01 = pars[0].lambda1;
          let lambda10 = pars[1].lambda0;
          let lambda11 = pars[1].lambda1;

          let lambdaFactor0 = lambda00*lambda01 / (lambda00 - lambda01);
          let lambdaFactor1 = lambda10*lambda11 / (lambda10 - lambda11);

          for (var y = 0; y < img.height; y++) {
             for (var x = 0; x < img.width; x++) {

                let ch00 = img.sample(x, y, indexCh00) - ch00Bgr;
                let ch01 = img.sample(x, y, indexCh01) - ch01Bgr;
                let ch10 = img.sample(x, y, indexCh10) - ch10Bgr;
                let ch11 = img.sample(x, y, indexCh11) - ch11Bgr;

                // weighting function (to protect background)
                let w0 = 1.0 - Math.exp(-ch00*ch00/ch00MAD/ch00MAD/g/g);
                let w1 = 1.0 - Math.exp(-ch10*ch10/ch10MAD/ch10MAD/g/g);

                // chx1/chx0 ratio (regularized to 1.0 at low signal)
                let R0 =  w0 * Math.min(Math.abs(ch01/ch00), 1.0e3) + (1.0 - w0);
                let R1 =  w1 * Math.min(Math.abs(ch11/ch10), 1.0e3) + (1.0 - w1);

                // inverse temperature difference
                let dbeta0 = Math.log(R0) * lambdaFactor0;
                let dbeta1 = Math.log(R1) * lambdaFactor1;

                // variance weighted mixing to average dbeta
                let var0 =  Math.max(lambdaFactor0*lambdaFactor0/R0/R0*(ch00MAD*ch00MAD/ch00/ch00 + ch01MAD*ch01MAD/ch01/ch01), 1e-12);
                let var1 =  Math.max(lambdaFactor1*lambdaFactor1/R1/R1*(ch10MAD*ch10MAD/ch10/ch10 + ch11MAD*ch11MAD/ch11/ch11), 1e-12);
                let mix0 = var1 / (var0 + var1);

                let dbeta = mix0 * dbeta0 + (1.0 - mix0) * dbeta1;

                // convert ratio to color in LCh
                let lchPixel = getLChForRatio(fSpectralClass * dbeta);

                let cFactor = colorSaturationFactor;
                cFactor *= lchPixel.Cf;
                cFactor *= fSpectralClass > 1.0 ? saturationCorrFactor(dbeta, lchPixel.C) : 1.0;

                // convert original pixel to LCh
                let LCH = rgbToLch(img.sample(x, y, 0), img.sample(x, y, 1), img.sample(x, y, 2));

                // replace color and convert to RGB
                let RGB = lchToRgb(LCH.L, cFactor * LCH.C, lchPixel.h);

                img.setSample(RGB.r, x, y, 0);
                img.setSample(RGB.g, x, y, 1);
                img.setSample(RGB.b, x, y, 2);
             }
          }
          break;

       default:
          throw new Error("No algorithm for " + pars.length + " pairs implemented.");
    }

    view.endProcess();
}

// -----------------------------------------------------------------------------
// Store UI parameters
function storeUI(dialog) {
    Settings.write(KEY_IMAGETYPE, DataType_Int32, dialog.imageType.currentItem);
    Settings.write(KEY_SATURATION_VALUE, DataType_Double, dialog.colorSaturationEdit.value);
    Settings.write(KEY_PROTECTBGR_VALUE, DataType_Double, dialog.protectBackgroundEdit.value);
    Settings.write(KEY_UNPHYSICAL_VALUE, DataType_Boolean, dialog.unphysicalBar.checkBox.checked);
    Settings.write(KEY_SPREADING_VALUE, DataType_Double, dialog.spreadSpectralClass.value);
};

// =============================================================================
// inherit from Dialog
ScaleImageDialog.prototype = new Dialog;


// main function
function main() {

    var dialog = new ScaleImageDialog();

    if (!dialog.execute()) {
        return;
    }

    process(dialog);

    storeUI(dialog);
}

// run Script
main();
