import numpy as np
from pysiril.siril import Siril
from pysiril.wrapper import Wrapper
import os

# --- Siril Setup (This assumes Siril is running or can be started) ---
app = Siril()
cmd = Wrapper(app)

# --- Wavelengths ---
LAMBDA_SII = 672.4
LAMBDA_HA = 656.3
LAMBDA_OIII = 500.7
LAMBDA_R = 622.0
LAMBDA_G = 530.0
LAMBDA_B = 476.0


# ---------------------------------------------------------------------
# Conversion of LCh -> RGB
# ---------------------------------------------------------------------
def lch_to_rgb(L, C, h):
    # LCh -> Lab
    hr = np.radians(h)
    a = C * np.cos(hr)
    b = C * np.sin(hr)

    # Lab -> XYZ
    fy = (L + 16.0) / 116.0
    fx = fy + (a / 500.0)
    fz = fy - (b / 200.0)

    def finv(t):
        limit = 0.008856        
        return np.where(t**3 > limit, 
                        t**3, 
                        (t - 16.0/116.0) / 7.787)

    X = finv(fx)
    Y = finv(fy)
    Z = finv(fz)

    # denormalization
    X *= 0.95047
    Y *= 1.00000
    Z *= 1.08883

    # XYZ -> linear RGB
    r =  3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z
    g = -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z
    b2 = 0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z

    # clamping to [0, 1]
    r = np.clip(r, 0, 1)
    g = np.clip(g, 0, 1)
    b2 = np.clip(b2, 0, 1)

    return r, g, b2
    
    
# ---------------------------------------------------------------------
# Conversion of RGB -> LCh
# ---------------------------------------------------------------------
def rgb_to_lch(r, g, b):
    # linear RGB -> XYZ
    X = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b
    Y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b
    Z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b

    # normalization
    X /= 0.95047
    Y /= 1.00000
    Z /= 1.08883

    # XYZ -> Lab
    def f(t):
        limit = 0.008856
        return np.where(t > limit, 
                        np.power(np.maximum(t, 0), 1/3), 
                        7.787 * t + 16/116)

    fx = f(X)
    fy = f(Y)
    fz = f(Z)

    L = 116.0 * fy - 16.0
    a = 500.0 * (fx - fy)
    b2 = 200.0 * (fy - fz)

    # Lab -> LCH
    C = np.sqrt(a**2 + b2**2)
    h = np.degrees(np.arctan2(b2, a))
    
    # restrict to [0, 360]
    h = np.where(h < 0, h + 360, h)

    return L, C, h
    
    
def get_lch_for_ratio(dbeta):
    # tbd
    pass

def saturation_corr_factor(dbeta, C):
    # tbd
    pass

def process_image(image_path, image_type, saturation_factor, protect_background_g, spectral_spread):

    # cmd.load(os.path.splitext(image_path)[0]) 
    # img_np = cmd.get_numpy_image()

    # median = ...
    # mad = ...

    # Init depending on settings tbd ...
    lambda0 = LAMBDA_OIII
    lambda1 = LAMBDA_HA
    
    indexCh0 = 2 
    indexCh1 = 0 
    
    ch0Bgr = median[indexCh0]
    ch1Bgr = median[indexCh1]
    ch0MAD = mad[indexCh0]
    lambda_factor = lambda0 * lambda1 / (lambda0 - lambda1)

    ch0 = img_np[:, :, indexCh0] - ch0Bgr
    ch1 = img_np[:, :, indexCh1] - ch1Bgr
    
    # Weighting function
    w = 1.0 - np.exp(-ch0*ch0/ch0MAD/ch0MAD/protect_background_g/protect_background_g)
    
    # ch1/ch0 ratio (regularized)
    R = w * np.minimum(np.abs(ch1/ch0), 1.0e3) + (1.0 - w)
    
    # inverse temperature difference
    dbeta = np.log(R) * lambda_factor
    
    # ...

    # view.endProcess(); # equivalentin Siril: cmd.save()
    pass

# Example call:
# process_image("image.fit", "HOO", 1.0, 6.0, 1.0)