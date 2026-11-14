# SPDX-License-Identifier: MIT
# Kit (planet outposts) - layout of the one shared 2K trim sheet (pixels, origin top-left, y down).
# 2.56 px per cm: 1024 px is one 400 cm grid cell, so faces of neighbouring pieces line up with no seam.
ATLAS = 2048
PX_PER_CM = 2.56

# name: (x, y, w, h)
REGIONS = {
    "wall":     (0,    0,    1024, 768),   # 400 x 300 cm wall face: kick band, two 200 cm light panels, seam, light slot, top frame
    "wallB":    (1024, 0,    1024, 768),   # the same with a vent, a stencil and a hazard corner
    "floor":    (0,    768,  1024, 1024),  # 400 x 400 cm floor: four 200 cm plates, tread dots, worn walk lane
    "grate":    (1024, 768,  512,  512),   # 200 x 200 cm catwalk grating
    "screen":   (1536, 768,  512,  256),   # console screen (dark glass, grid)
    "vent":     (1536, 1024, 256,  256),   # vent grille
    "hatch":    (1792, 1024, 256,  256),   # access hatch
    "roof":     (1024, 1280, 512,  512),   # 200 x 200 cm underside panel
    "frame":    (1536, 1280, 512,  128),   # dark gunmetal frame strip, 200 cm long, rivets
    "hazard":   (1536, 1408, 512,  128),   # hazard chevrons, 200 cm
    "kick":     (1536, 1536, 512,  128),   # tread / kick plate, 200 cm
    "rail":     (1536, 1664, 512,  128),   # painted rail metal, 200 cm
    "tread":    (0,    1792, 512,  256),   # stair tread 200 x 100 cm (nosing strip on one long edge)
    "pillar":   (1024, 1792, 512,  256),   # column panel 200 x 100 cm
    "console":  (1536, 1792, 512,  256),   # console side panel 200 x 100 cm
    "sw_dark":  (512,  1792, 128,  128),   # plain swatches
    "sw_gun":   (640,  1792, 128,  128),
    "sw_white": (768,  1792, 128,  128),
    "sw_yel":   (896,  1792, 128,  128),
    "sw_glass": (512,  1920, 128,  128),
    "sw_black": (640,  1920, 128,  128),
    "sw_floor": (768,  1920, 128,  128),
    "sw_rub":   (896,  1920, 128,  128),
}
