# SPDX-License-Identifier: MIT
# DominionInterior - layout of the shared 2K atlas (pixels, origin top-left, y down). 2.56 px per cm: a 1024 px wide
# trim is exactly one 400 cm grid cell, and every trim's pattern is periodic over that width, so faces of
# neighbouring kit pieces line up with no seam.
ATLAS = 2048
PX_PER_CM = 2.56

# name: (x, y, w, h)
REGIONS = {
    "armourA":  (0,    0,    1024, 384),   # gloss black armour, two 200 cm panels
    "armourB":  (1024, 0,    1024, 384),   # the same with stencilled bay marks
    "gunmetal": (0,    384,  1024, 256),   # ribbed structure, rivet rows
    "ceiling":  (1024, 384,  1024, 384),   # dark ceiling panels with vent slots
    "cable":    (0,    640,  1024, 128),   # cable tray
    "hazard":   (0,    768,  1024, 128),   # hazard chevrons
    "kick":     (0,    896,  1024, 128),   # floor edge / tread trim
    "triad":    (1024, 768,  256,  256),   # three-chevron emblem, embossed, scuffed, half covered
    "vent":     (1280, 768,  256,  256),   # vent grille
    "hatch":    (1536, 768,  256,  256),   # access hatch
    "sw_black": (1792, 768,  128,  128),   # plain swatches
    "sw_gun":   (1920, 768,  128,  128),
    "sw_crim":  (1792, 896,  128,  128),
    "sw_light": (1920, 896,  128,  128),
    "grating":  (0,    1024, 1024, 512),   # floor grating lane, 200 cm wide
    "screen":   (1024, 1024, 512,  320),   # dark console screen
    "keys":     (1024, 1344, 512,  192),   # console key panel
    "petal":    (1536, 1024, 512,  512),   # reactor petal armour
    "plateA":   (0,    1536, 512,  512),   # floor plate 200 x 200 cm
    "plateB":   (512,  1536, 512,  512),   # floor plate with a bay mark
    "pad":      (1024, 1536, 512,  512),   # landing pad quarter (corner = pad centre)
    "torn":     (1536, 1536, 512,  512),   # scorched, torn metal
}
