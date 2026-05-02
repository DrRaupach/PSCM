import sys
import os
import sirilpy as s
import numpy as np

from collections import namedtuple

s.ensure_installed("PyQt6")
s.ensure_installed("scikit-image")

from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider, QPushButton, QComboBox, QCheckBox, QFrame, QTextBrowser
from PyQt6.QtCore import Qt

# algorithms for color conversion
from skimage.color import rgb2lab, lab2rgb

# Version information
TITLE = 'Planck Star Color Mapping (PSCM)'
VERSION = 'V1.2.0'
DEVELOPER = 'Dr. Rainer Raupach'

# --- Wavelengths ---
LAMBDA_SII = 672.4
LAMBDA_HA = 656.3
LAMBDA_OIII = 500.7
LAMBDA_R = 622.0
LAMBDA_G = 530.0
LAMBDA_B = 476.0

# Default values
DEFAULT_COLOR_SATURATION = 1.0
DENOMINATOR_COLOR_SATURATION = 100
DEFAULT_PROTECT_BACKGROUND = 1.5
DENOMINATOR_PROTECT_BACKGROUND = 10
DEFAULT_SPECTRAL_SPREAD = 1.0
DENOMINATOR_SPECTRAL_SPREAD = 100

# Image type parameters (combobox name, number of pairs to be used, index matrix for pairs)
ImageTypePars = namedtuple("ImageTypePars", ["Name", "NoOfPairs", "Pairs"])
ImageTypePresets = [
    ImageTypePars("HOO", 1, ((0,2))),
    ImageTypePars("HSO", 2, ((0,2),(1,2))),
    ImageTypePars("SHO", 2, ((0,2),(1,2))),
    ImageTypePars("RGB", 2, ((0,1),(0,2))),
    ImageTypePars("RGB (ignore G)", 1, ((0,2)))
]

def rgb2lch(imaRGB):
    # RGB -> CIE Lab
    lab = rgb2lab(imaRGB)
            
    # Lab -> Lch
    L = lab[:, :, 0]
    a = lab[:, :, 1]
    _b = lab[:, :, 2]
            
    C = np.sqrt(a**2 + _b**2)
    h = np.arctan2(_b, a)
        
    return L,C,h        

def lch2rgb(L, C, h):
    lab = np.zeros((L.shape[0],L.shape[1],3), dtype=np.float64)
    lab[:, :, 0] = L
    lab[:, :, 1] = C * np.cos(h)
    lab[:, :, 2] = C * np.sin(h)
            
    # CIE Lab -> RGB
    rgb = lab2rgb(lab)
    
    return rgb
        
def InitForImageType(imageTypeEntry, mad):
    a = 1
        

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
        btn_apply.clicked.connect(self.applySPCM)
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
            
        self.siril.log(f'    Median = {median}')
        self.siril.log(f'    MAD    = {mad}')
            
        return median, mad

    def applySPCM(self):
        try:
            self.siril.log("Applying PSCM...", s.LogColor.GREEN)
            c_factor = self.sliderSaturation.value() / 100.0
            
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
            self.siril.log(f'    Image size: {img_normalized.shape[1]} x {img_normalized.shape[0]} x {img_normalized.shape[2]}');

            # analyze input image
            median, mad = self.analyzeImage(img_normalized)
                        
            # RGB -> Lch
            L,C,h = rgb2lch(img_normalized)
            
            # manipulate saturation
            C_new = C * c_factor
            
            # Protect luminance
            protection_mask = np.clip((100.0 - L) / 30.0, 0, 1) 
            C_new = C + (C_new - C) * protection_mask
            
            # Lch -> RGB
            rgb_new = lch2rgb(L, C_new, h)
            
            # clamp RGB to 0..1
            new_data = np.clip(rgb_new, 0, 1).astype(np.float32)
            
            # revert axis correction if applicable
            if swapped:
                new_data = np.moveaxis(new_data, -1, 0)
            
            # update data and relaod image
            self._push_to_siril(new_data)
            self.siril.log("PSCM done.", s.LogColor.GREEN)
            
        except Exception as e:
            self.siril.log(f"Error in PSCM: {e}", s.LogColor.RED)

    def undo_changes(self):
        try:
            print("Undoing PSCM...")
            # restore original image
            self._push_to_siril(self.original_data)
            print("Undo done.")
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
