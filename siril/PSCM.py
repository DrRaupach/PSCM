# =========================================================================================================
# Planck Star Color Mapping (PSCM)
# Maps HOO/HSO/SHO/... stars to naturally colored stars using Planck's law of black body radiation.
# Copyright (C) 2026 Dr. Rainer Raupach
# =========================================================================================================

import sys
import os
import sirilpy as s
import numpy as np
import math

from collections import namedtuple

s.ensure_installed("PyQt6")
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider, QPushButton, QComboBox, QCheckBox, QFrame, QTextBrowser
from PyQt6.QtCore import Qt

# --- Version information ---
TITLE = 'Planck Star Color Mapping (PSCM)'
VERSION = 'V1.0beta'
DEVELOPER = 'Dr. Rainer Raupach'

# --- Default values ---
DEFAULT_COLOR_SATURATION = 1.0
DENOMINATOR_COLOR_SATURATION = 100
DEFAULT_PROTECT_BACKGROUND = 1.5
DENOMINATOR_PROTECT_BACKGROUND = 10
DEFAULT_SPECTRAL_SPREAD = 1.0
DENOMINATOR_SPECTRAL_SPREAD = 100

# --- Wavelengths ---
LAMBDA_SII = 672.4
LAMBDA_HA = 656.3
LAMBDA_OIII = 500.7
LAMBDA_R = 622.0
LAMBDA_G = 530.0
LAMBDA_B = 476.0

# --- UI element styles ---
SLIDER_STYLE = """
            QSlider::groove:horizontal {
                border: 1px solid #999999;
                height: 4px;
                background: #cccccc;
                margin: 0px 0;
                border-radius: 2px;
            }
            QSlider::handle:horizontal {
                background: #0078d7;
                border: 1px solid #005a9e;
                width: 15px;
                height: 15px;
                margin: -7px 0;
                border-radius: 7.5px;
            }
            QSlider::handle:horizontal:hover {
                background: #005a9e;
            }
            QSlider::handle:horizontal:disabled {
                background: #a0a0a0;
                border: 1px solid #808080;
            }
        """

# Image type parameters (combobox name, number of pairs to be used, index matrix for pairs)
ImageTypePars = namedtuple("ImageTypePars", ["Name", "Wavelength", "NoOfPairs", "Pairs"])
ImageTypePresets = [
    ImageTypePars("HOO", (LAMBDA_HA, LAMBDA_OIII, LAMBDA_OIII), 1, np.array([0,2]).reshape(-1, 2)),
    ImageTypePars("HSO", (LAMBDA_HA, LAMBDA_SII, LAMBDA_OIII), 2,  np.array([[0,2],[1,2]]).reshape(-1, 2)),
    ImageTypePars("SHO", (LAMBDA_SII, LAMBDA_HA, LAMBDA_OIII), 2,  np.array([[0,2],[1,2]]).reshape(-1, 2)),
    ImageTypePars("RGB", (LAMBDA_R, LAMBDA_G, LAMBDA_B), 2,  np.array([[0,1],[0,2]]).reshape(-1, 2)),
    ImageTypePars("RGB (ignore G)", (LAMBDA_R, LAMBDA_G, LAMBDA_B), 1,  np.array([0,2]).reshape(-1, 2)),
]

def rgb2lch(r, g, b):
    # --- linear RGB → XYZ (D65) ---
    X = 0.4124564*r + 0.3575761*g + 0.1804375*b
    Y = 0.2126729*r + 0.7151522*g + 0.0721750*b
    Z = 0.0193339*r + 0.1191920*g + 0.9503041*b

    # --- normalize by D65 white point ---
    X /= 0.95047
    Y /= 1.00000
    Z /= 1.08883

    # --- XYZ → Lab ---
    def f(t):
        tp = np.maximum(t, 0)
        tn = np.maximum(-t, 0)
        return np.where(t > 0.008856, np.where(t < 0, -np.power(tn, 1.0/3.0), np.power(tp, 1.0/3.0)), 7.787*t + 16.0/116.0)

    fx = f(X)
    fy = f(Y)
    fz = f(Z)

    L = 116.0*fy - 16.0
    a = 500.0*(fx - fy)
    b2 = 200.0*(fy - fz)

    # --- Lab → LCH ---
    C = np.sqrt(a*a + b2*b2)
    h = np.atan2(b2, a) * 180.0 / math.pi
    h = np.where(h < 0, h + 360.0, h)

    return L, C, h

def lch2rgb(L, C, h):
    # --- LCH → Lab ---
    hr = h * math.pi / 180.0
    a = C * np.cos(hr)
    b = C * np.sin(hr)

    # --- Lab → XYZ ---
    fy = (L + 16.0) / 116.0
    fx = fy + a / 500.0
    fz = fy - b / 200.0

    def finv(t):
        t3 = t*t*t
        return np.where(t3 > 0.008856, t3, (t - 16.0/116.0) / 7.787)

    X = finv(fx)
    Y = finv(fy)
    Z = finv(fz)

    # --- denormalize D65 ---
    X *= 0.95047
    Y *= 1.00000
    Z *= 1.08883

    # --- XYZ → linear RGB ---
    r =  3.2404542*X - 1.5371385*Y - 0.4985314*Z
    g = -0.9692660*X + 1.8760108*Y + 0.0415560*Z
    b2 = 0.0556434*X - 0.2040259*Y + 1.0572252*Z

    rgb = np.zeros((L.shape[0],L.shape[1],3), dtype=np.float64)
    rgb[:,:,0] = np.clip(r, 0, 1)
    rgb[:,:,1] = np.clip(g, 0, 1)
    rgb[:,:,2] = np.clip(b2, 0, 1)
    
    return rgb
        
def getChForRatio(dbeta):
    # returns the color for a given temperature difference in LCh
    r = np.exp(dbeta/LAMBDA_R)
    g = np.exp(dbeta/LAMBDA_G)
    b = np.exp(dbeta/LAMBDA_B)
    rgbNorm = np.maximum(r, np.maximum(g, b))
    r /= rgbNorm
    g /= rgbNorm
    b /= rgbNorm

    _, C, h = rgb2lch(r, g, b)
    
    drgb = 1.0 - r*g*b
    drgb *= drgb
    Cf = 1.0 - np.exp(-drgb/1e-12)

    return C, h, Cf

def saturationCorrFactor(dbeta, C):
    CNeutral, hNeutral, _ = getChForRatio(dbeta)
    cCorrFactor = C / np.maximum(CNeutral, 1e-3)
    cCorrFactor = np.where(dbeta < 0, np.power(cCorrFactor, 0.5), np.power(cCorrFactor, 2)) # dbeta < 0 means hotter stars

    return cCorrFactor
        
def InitForImageType(imageTypeEntry, mad):
    pars = ImageTypePresets[imageTypeEntry]    
    
    # sort pairs by mad
    for n in range(pars.NoOfPairs):
        if mad[pars.Pairs[n][1]] < mad[pars.Pairs[n][0]]:
            pars.Pairs[n][0], pars.Pairs[n][1] = pars.Pairs[n][1], pars.Pairs[n][0]

    return pars
        

class PSCM(QWidget):
    
    def __init__(self, siril_instance):
        super().__init__()
        self.siril = siril_instance
        
        # get the Siril image object
        with self.siril.image_lock():
            self.img_obj = self.siril.get_image()
            
        # generate copy for undo
        self.original_data = self.img_obj.data.copy()
        
        self.initUI()
        
    def initUI(self):        
        self.setWindowTitle(f'{TITLE}')
        
        #self.setStyleSheet("""
        #    QWidget {
        #        background-color: #2d2d2d;
        #        color: #efefef;
        #        border: none;
        #    }
        #""")
        
        layout = QVBoxLayout()
        
        # UI elements
        # Script information
        scriptInfo = QTextBrowser()
        scriptInfo.setStyleSheet("""
            QTextBrowser {
                background-color: #f0f0f0;
                border: 1px solid #000000;
            }
        """)
        scriptInfo.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        scriptInfo.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        scriptInfo.setFixedHeight(42)
        scriptInfo.setHtml('<p><strong>' + TITLE + ' version ' + VERSION + '</strong><br/>' + 'Copyright &copy; 2026 ' + DEVELOPER + '</p>')
        layout.addWidget(scriptInfo)        
        
        # how to ...
        howtoInfo = QTextBrowser()
        howtoInfo.setStyleSheet("""
            QTextBrowser {
                background-color: transparent;
                border: 0px solid #000000;
            }
        """)
        howtoInfo.setHtml("""
            How to use PSCM?<br/>
            (example/typical workflow for HOO)
            <table style="margin-top: 4px; margin-bottom: 4px">
                <tr>
                    <td>1.<td>Load matching LINEAR Ha and OIII images after Background Extraction, e.g. Graxpert.
                </tr>
                <tr>
                    <td>2.<td>Combine to HOO color image by assigning Ha to R and OIII to G and B.
                </tr>
                <tr>
                    <td>3.<td>Apply ImageSolver to find astrometric solution on HOO image.
                </tr>
                <tr>
                    <td>4.<td>Apply SPCC with 'Red filter' at 656.3, 'Green/Blue filter' at 500.7 in 'Narrowband mode' and 'Optimize for Stars' checked. The white reference should not be ''too hot''. 'Average Galaxy' (~4500K) is a good choice.
                </tr>
                <tr>
                    <td>5.<td>Derive the Starless image, also the Stars in unscreen mode, i.e. negative division.
                </tr>
                <tr>
                    <td>6.<td>Apply PSCM to the Star image which transforms the HOO colors to black body colors according to the stars' temperatures.
                </tr>
                <tr>
                    <td>7.<td>Combine the PSCM mapped Stars with the Starless by screening, i.e. negative multiplication.
                </tr>
            </table>
            Voilà! You now have a bi-color HOO image with (almost) naturally colored stars without the need of an additional RGB image, still in linear domain. Continue with stretching and further post-processing as usual.<br/><br/>
            In general: the wavelengths in SPCC for S|H|O should be 672.4|656.3|500.7 assigned to the respective filter slot. For RGB, the standard filters can be used.
        """)
        howtoInfo.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        howtoInfo.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        howtoInfo.setFixedHeight(360)
        
        layout.addWidget(howtoInfo)        
        
        # Input image type
        parameter_ImageType = QHBoxLayout()
        
        self.labelImageType = QLabel('Input Image Type')
        #self.labelImageType.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.labelImageType.setFixedWidth(200) # fixed width, avoids junping in UI
        parameter_ImageType.addWidget(self.labelImageType)
        
        self.comboboxImageType = QComboBox()
        self.comboboxImageType.setFixedWidth(250)
        for i in range(len(ImageTypePresets)):
            self.comboboxImageType.addItem(ImageTypePresets[i].Name)
        #self.comboboxImageType.currentIndexChanged.connect(self.on_item_clicked) # needed if reaction to item change required
        parameter_ImageType.addWidget(self.comboboxImageType)    
        
        layout.addLayout(parameter_ImageType)        
        
        # Saturation
        parameter_Saturation = QHBoxLayout()
        
        self.labelSaturation = QLabel(f'Color Saturation Factor: {DEFAULT_COLOR_SATURATION: .2f}')
        #self.labelSaturation.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.labelSaturation.setFixedWidth(200) # fixed width, avoids junping in UI
        parameter_Saturation.addWidget(self.labelSaturation)
        
        self.sliderSaturation = QSlider(Qt.Orientation.Horizontal)
        self.sliderSaturation.setRange(0, 200) # 0.0 to 2.0
        self.sliderSaturation.setValue(int(DEFAULT_COLOR_SATURATION * DENOMINATOR_COLOR_SATURATION))
        self.sliderSaturation.valueChanged.connect(self.update_saturation)
        self.sliderSaturation.setStyleSheet(SLIDER_STYLE)        
        parameter_Saturation.addWidget(self.sliderSaturation)
        
        layout.addLayout(parameter_Saturation)        
        
        # Background protection
        parameter_Protect = QHBoxLayout()
        
        self.labelProtect = QLabel(f'Protect Background (X*MAD): {DEFAULT_PROTECT_BACKGROUND: .2f}')
        #self.labelProtect.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.labelProtect.setFixedWidth(200) # fixed width, avoids junping in UI
        parameter_Protect.addWidget(self.labelProtect)
        
        self.sliderProtect = QSlider(Qt.Orientation.Horizontal)
        self.sliderProtect.setRange(5, 120) # 0.5 to 12.0
        self.sliderProtect.setValue(int(DEFAULT_PROTECT_BACKGROUND * DENOMINATOR_PROTECT_BACKGROUND))
        self.sliderProtect.valueChanged.connect(self.update_protect)
        self.sliderProtect.setStyleSheet(SLIDER_STYLE)
        parameter_Protect.addWidget(self.sliderProtect)                
        layout.addLayout(parameter_Protect)        
        
        # Unphysical checkbox
        parameter_Unphysical = QHBoxLayout()        
        self.checkboxUnphysical = QCheckBox()
        self.checkboxUnphysical.setFixedWidth(16)
        self.checkboxUnphysical.stateChanged.connect(self.update_unphysical)
        parameter_Unphysical.addWidget(self.checkboxUnphysical)
        self.labelUnphysical = QLabel('Unphysical')
        parameter_Unphysical.addWidget(self.labelUnphysical)
        layout.addLayout(parameter_Unphysical)
        
        # Unphysical section
        self.sectionUnphysical = QFrame()
        self.sectionUnphysical.setFrameShape(QFrame.Shape.StyledPanel)
        
        layout_Unphysical = QVBoxLayout()  
        
        # Spread spectral classes
        parameter_Spreading = QHBoxLayout()
        
        self.labelSpreading = QLabel(f'Spread Spectral Classes by: {DEFAULT_SPECTRAL_SPREAD: .2f}')
        #self.labelSpreading.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.labelSpreading.setFixedWidth(200) # fixed width, avoids junping in UI
        parameter_Spreading.addWidget(self.labelSpreading)
        
        self.sliderSpreading = QSlider(Qt.Orientation.Horizontal)
        self.sliderSpreading.setRange(100, 200) # 1.0 to 2.0
        self.sliderSpreading.setValue(int(DEFAULT_SPECTRAL_SPREAD * DENOMINATOR_SPECTRAL_SPREAD))
        self.sliderSpreading.valueChanged.connect(self.update_spreading)
        self.sliderSpreading.setStyleSheet(SLIDER_STYLE)
        parameter_Spreading.addWidget(self.sliderSpreading)
        
        layout_Unphysical.addLayout(parameter_Spreading)
        
        self.sectionUnphysical.setLayout(layout_Unphysical)
        self.sectionUnphysical.setEnabled(0)
        
        layout.addWidget(self.sectionUnphysical)
        
        # buttons in one row
        btn_layout = QHBoxLayout()
        
        btn_undo = QPushButton('↺ Undo')
        btn_undo.clicked.connect(self.undo_changes)
        btn_layout.addWidget(btn_undo)
        
        btn_apply = QPushButton('Apply PSCM')
        btn_apply.clicked.connect(self.applyPSCM)
        btn_layout.addWidget(btn_apply)
        
        layout.addLayout(btn_layout)
        
        self.setLayout(layout)
        self.resize(400, 140)
        
    def update_saturation(self, value):
        c_factor = value / DENOMINATOR_COLOR_SATURATION
        self.labelSaturation.setText(f'Color Saturation Factor: {c_factor:.2f}')

    def update_protect(self, value):
        pbgr_factor = value / DENOMINATOR_PROTECT_BACKGROUND
        self.labelProtect.setText(f'Protect Background (X*MAD): {pbgr_factor:.2f}')
        
    def update_spreading(self, value):
        spread_factor = value / DENOMINATOR_SPECTRAL_SPREAD
        self.labelSpreading.setText(f'Spread Spectral Classes by: {spread_factor:.2f}')
        
    def update_unphysical(self, state):
        is_active = (state == 2)
        self.sectionUnphysical.setEnabled(is_active)

    def analyzeImage(self, imaRGB):    
        nChannels = imaRGB.shape[2]
        median = np.zeros(nChannels, dtype=np.float64)
        mad = np.zeros(nChannels, dtype=np.float64)
        
        for n in range(nChannels):
            median[n] = np.median(imaRGB[:,:,n])
            mad[n] = np.median(np.abs(imaRGB[:,:,n] - median[n]))
            
        self.siril.log(f' > Median = {median}')
        self.siril.log(f' > MAD    = {mad}')
            
        return median, mad

    def applyPSCM(self):
        try:
            self.siril.log("Applying PSCM...", s.LogColor.GREEN)
            
            # clone image data and clamp to 0..1
            img_normalized = np.clip(self.original_data, 0, 1).astype(np.float64)
            
            # check if this is an image with three channels
            if len(img_normalized.shape) < 3 or (img_normalized.shape[0] != 3 and img_normalized.shape[2] != 3):
                self.siril.log("Input image must be a color image (image with three channels)!", s.LogColor.RED)
                return
            
            # dynamic axis correction: check if the three channels are swapped and adapt for scikit-image
            swapped = False
            if img_normalized.shape[0] == 3 and len(img_normalized.shape) == 3:
                img_normalized = np.moveaxis(img_normalized, 0, -1)
                swapped = True
            self.siril.log(f' > Image size: {img_normalized.shape[1]} x {img_normalized.shape[0]} x {img_normalized.shape[2]}')

            # analyze input image
            median, mad = self.analyzeImage(img_normalized)
            
            # get parameters for current image type
            pars = InitForImageType(self.comboboxImageType.currentIndex(), mad)
            
            g = self.sliderProtect.value() / DENOMINATOR_PROTECT_BACKGROUND
            
            colorSaturationFactor = self.sliderSaturation.value() / DENOMINATOR_COLOR_SATURATION
            
            fSpectralClass = 1.0
            if self.checkboxUnphysical.isChecked():
                fSpectralClass = self.sliderSpreading.value() / DENOMINATOR_SPECTRAL_SPREAD
            
            # RGB -> Lch
            self.siril.log(' > Converting image to LCh...')
            L, C, h = rgb2lch(img_normalized[:,:,0],img_normalized[:,:,1],img_normalized[:,:,2])

            # Planck mapping
            self.siril.log(' > Planck mapping...')
            self.siril.log(f' > Preset {pars.Name}: {pars.NoOfPairs} pair(s) of channels to be processed.')
            match pars.NoOfPairs:
                # one pair seperated for speed and memory
                case 1: 
                    indexCh0 = pars.Pairs[0][0]
                    indexCh1 = pars.Pairs[0][1]
                    
                    ch0Bgr = median[indexCh0]
                    ch1Bgr = median[indexCh1]

                    ch0MAD = mad[indexCh0]

                    lambda0 = pars.Wavelength[indexCh0]
                    lambda1 = pars.Wavelength[indexCh1]
                    self.siril.log(f' > Pair 0: ({indexCh0},{indexCh1}) -> ({lambda0},{lambda1}) nm')
                    lambdaFactor = lambda0*lambda1 / (lambda0 - lambda1)

                    ch0 = np.maximum(img_normalized[:,:,indexCh0] - ch0Bgr, 1.0e-12)
                    ch1 = img_normalized[:,:,indexCh1] - ch1Bgr

                    # weighting function (to protect background)
                    w = 1.0 - np.exp(-ch0*ch0/ch0MAD/ch0MAD/g/g)
                    
                    # ch1/ch0 ratio (regularized to 1.0 at low signal)
                    R = w * np.minimum(np.abs(ch1/ch0), 1.0e3) + (1 - w)

                    # inverse temperature difference
                    dbeta = np.log(R) * lambdaFactor
                    
                # any case with number of pairs larger than one
                case _:
                    w = np.ones((img_normalized.shape[0], img_normalized.shape[1]), dtype=np.float64) # must be initialized with 1
                    vinv_total = np.zeros((img_normalized.shape[0], img_normalized.shape[1]), dtype=np.float64)
                    dbeta = np.zeros((img_normalized.shape[0], img_normalized.shape[1]), dtype=np.float64)
                    
                    for n in range(pars.NoOfPairs):
                        indexCh0 = pars.Pairs[n][0]
                        indexCh1 = pars.Pairs[n][1]
                    
                        ch0Bgr = median[indexCh0]
                        ch1Bgr = median[indexCh1]

                        ch0MAD = mad[indexCh0]
                        ch1MAD = mad[indexCh1]
                    
                        lambda0 = pars.Wavelength[indexCh0]
                        lambda1 = pars.Wavelength[indexCh1]
                        self.siril.log(f' > Pair {n}: ({indexCh0},{indexCh1}) -> ({lambda0},{lambda1}) nm')
                        lambdaFactor = lambda0*lambda1 / (lambda0 - lambda1)
                    
                        ch0 = np.maximum(img_normalized[:,:,indexCh0] - ch0Bgr, 1.0e-12)
                        ch1 = img_normalized[:,:,indexCh1] - ch1Bgr
                        
                        # weighting function (to protect background)
                        wn = 1.0 - np.exp(-ch0*ch0/ch0MAD/ch0MAD/g/g)
                        w = np.minimum(w, wn)
                        
                        # ch1/ch0 ratio (regularized to 1.0 at low signal)
                        R = wn * np.minimum(np.abs(ch1/ch0), 1.0e3) + (1 - wn)
                        
                        # prepare variance weighted mixing
                        vinv = np.maximum(lambdaFactor*lambdaFactor/R/R*(ch0MAD*ch0MAD/ch0/ch0 + ch1MAD*ch1MAD/ch1/ch1), 1e-12)
                        vinv_total += vinv
                        
                        # inverse temperature difference
                        dbeta += np.log(R) * lambdaFactor * vinv
                        
                    # normalize temperature difference
                    dbeta /= vinv_total
                                            
            # calculate Planck color
            CPlanck, hPlanck, Cf = getChForRatio(fSpectralClass * dbeta)
                
            # adjust color saturation
            C_new = C
            C_new *= Cf
            C_new *= colorSaturationFactor
            C_new *= w
            if fSpectralClass > 1.0:
                C_new *= saturationCorrFactor(dbeta, CPlanck)

            # replace color and convert to RGB
            self.siril.log(' > Converting image to RGB...')
            rgb_new = lch2rgb(L, C_new, hPlanck).astype(np.float32)
            
            # revert axis correction if applicable
            if swapped:
                rgb_new = np.moveaxis(rgb_new, -1, 0)
            
            # update data and relaod image
            self._push_to_siril(rgb_new)
            self.siril.log("PSCM done.", s.LogColor.GREEN)
            
        except Exception as e:
            self.siril.log(f"Error in PSCM: {e}", s.LogColor.RED)

    def undo_changes(self):
        try:
            self.siril.log("Undoing PSCM...")
            # restore original image
            self._push_to_siril(self.original_data)
            self.siril.log("Undo done.")
        except Exception as e:
            self.siril.log(f"Error during undo: {e}", s.LogColor.RED)

    def _push_to_siril(self, data):
        # aux function for displaying result
        with self.siril.image_lock():
            self.siril.set_image_pixeldata(data)            
            
        #temp_filename = "pscm_temp_output.fit"
        #self.siril.cmd(f"save {temp_filename}")
        #self.siril.cmd(f"load {temp_filename}")
        
        #if os.path.exists(temp_filename):
        #    os.remove(temp_filename)

def main():
    siril = s.SirilInterface()
    try:
        siril.connect()
    except Exception as e:
        siril.log(f"Connection to Siril failed: {e}", s.LogColor.RED)
        return
    
    app = QApplication(sys.argv)
    ex = PSCM(siril)
    ex.show()
    app.exec()
    
    siril.disconnect()

if __name__ == '__main__':
    main()
