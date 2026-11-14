# SPDX-License-Identifier: MIT
# Kit, story props: the two ground decals (procedural, PIL): T_Kit_DragMark_* (two scuffed furrows, alpha-masked) and
# T_Kit_ScorchLines_* (blast streaks fanning from a point). BC carries the mask in alpha (M_Kit_Decal: opacity = BC alpha);
# N flat, ORM rough non-metal, E black.   python ArtSource/EngProps/Kit/scripts/kit_story_decals.py
import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures")


def flat(name, w, h):
    Image.new("RGB", (w, h), (128, 128, 255)).save(os.path.join(OUT, name + "_N.png"))
    Image.new("RGB", (w, h), (255, 235, 0)).save(os.path.join(OUT, name + "_ORM.png"))
    Image.new("RGB", (w, h), (0, 0, 0)).save(os.path.join(OUT, name + "_E.png"))


def drag_mark():
    w, h = 1024, 256
    rnd = random.Random(4)
    a = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(a)
    for yc in (h * 0.32, h * 0.68):
        for k in range(40):
            x0 = rnd.uniform(0, w)
            ln = rnd.uniform(60, 260)
            yy = yc + rnd.gauss(0, 10)
            d.line([(x0, yy), (x0 + ln, yy + rnd.gauss(0, 3))], fill=int(rnd.uniform(120, 230)), width=int(rnd.uniform(4, 14)))
    a = a.filter(ImageFilter.GaussianBlur(3))
    edge = Image.new("L", (w, h), 0)
    ImageDraw.Draw(edge).rectangle((40, 0, w - 40, h), fill=255)
    a = Image.composite(a, Image.new("L", (w, h), 0), edge.filter(ImageFilter.GaussianBlur(30)))
    bc = Image.new("RGB", (w, h), (46, 36, 26))
    bc.putalpha(a)
    bc.save(os.path.join(OUT, "T_Kit_DragMark_BC.png"))
    flat("T_Kit_DragMark", w, h)


def scorch_lines():
    w, h = 1024, 512
    rnd = random.Random(9)
    a = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(a)
    ox, oy = 90, h // 2
    for k in range(34):
        ang = math.radians(rnd.uniform(-28, 28))
        ln = rnd.uniform(300, 900)
        wd = rnd.uniform(6, 26)
        d.line([(ox, oy), (ox + math.cos(ang) * ln, oy + math.sin(ang) * ln * 1.6)], fill=int(rnd.uniform(140, 255)), width=int(wd))
    d.ellipse((ox - 70, oy - 70, ox + 70, oy + 70), fill=255)
    a = a.filter(ImageFilter.GaussianBlur(5))
    bc = Image.new("RGB", (w, h), (14, 12, 11))
    bc.putalpha(a)
    bc.save(os.path.join(OUT, "T_Kit_ScorchLines_BC.png"))
    flat("T_Kit_ScorchLines", w, h)


def light_pool():
    """A warm radial pool: alpha falls off from the centre; the emissive carries the warm light."""
    w = 512
    a = Image.new("L", (w, w), 0)
    d = ImageDraw.Draw(a)
    for r in range(w // 2, 0, -2):
        v = int(255 * (1.0 - r / (w / 2)) ** 1.6)
        d.ellipse((w / 2 - r, w / 2 - r, w / 2 + r, w / 2 + r), fill=v)
    bc = Image.new("RGB", (w, w), (120, 90, 55))
    bc.putalpha(a)
    bc.save(os.path.join(OUT, "T_Kit_LightPool_BC.png"))
    Image.new("RGB", (w, w), (128, 128, 255)).save(os.path.join(OUT, "T_Kit_LightPool_N.png"))
    Image.new("RGB", (w, w), (255, 200, 0)).save(os.path.join(OUT, "T_Kit_LightPool_ORM.png"))
    Image.merge("RGB", [a.point(lambda v: v), a.point(lambda v: int(v * 0.7)), a.point(lambda v: int(v * 0.35))]).save(
        os.path.join(OUT, "T_Kit_LightPool_E.png"))


def insignia_scorched():
    """ART-18 clue: Triad's three-chevron unit mark as it shows under damaged Null armour: the chevrons only, no bar or dot
    marks, no text; burnt, eroded, two thirds hidden by black plating that has not flaked away. Faded squad light grey,
    scorched at the edges. Reads as "that shape is familiar", never as a label."""
    from PIL import ImageChops
    w = 1024
    rnd = random.Random(12)
    mark = Image.new("L", (w, w), 0)
    d = ImageDraw.Draw(mark)
    for k in range(3):                                       # the three chevrons (same proportions as T_DI_Decals' TRIAD_MARK)
        y = 230 + k * 145
        d.polygon([(232, y + 140), (512, y), (792, y + 140), (792, y + 215), (512, y + 75), (232, y + 215)], fill=255)
    reveal = Image.new("L", (w, w), 0)                       # flaked plating: only patches of the mark show through
    rd = ImageDraw.Draw(reveal)
    for i in range(9):
        cx, cy = rnd.gauss(520, 190), rnd.gauss(450, 140)
        r = rnd.uniform(45, 115)
        rd.ellipse((cx - r, cy - r * rnd.uniform(0.5, 1.0), cx + r, cy + r * rnd.uniform(0.5, 1.0)), fill=255)
    reveal = reveal.filter(ImageFilter.GaussianBlur(14)).point(lambda v: 255 if v > 110 else int(v * 2.0))
    noise = Image.effect_noise((w, w), 60).filter(ImageFilter.GaussianBlur(2)).point(lambda v: max(0, min(255, int(140 + (v - 128) * 1.6))))
    A = ImageChops.multiply(ImageChops.darker(mark, reveal), noise).filter(ImageFilter.GaussianBlur(1.5))
    # colour: faded light grey in the open, scorched brown-black toward the plating edges
    edge = reveal.filter(ImageFilter.GaussianBlur(20)).point(lambda v: max(0, min(255, int(v * 1.6 - 76))))
    bc = Image.composite(Image.new("RGB", (w, w), (150, 152, 150)), Image.new("RGB", (w, w), (45, 32, 24)), edge)
    bc.putalpha(A)
    bc.save(os.path.join(OUT, "T_Kit_InsigniaScorched_BC.png"))
    Image.new("RGB", (w, w), (128, 128, 255)).save(os.path.join(OUT, "T_Kit_InsigniaScorched_N.png"))
    Image.new("RGB", (w, w), (255, 200, 0)).save(os.path.join(OUT, "T_Kit_InsigniaScorched_ORM.png"))
    # a faint ember only in the deepest burns (where the mark meets the plating), else black
    burn = ImageChops.multiply(ImageChops.invert(edge), A).point(lambda v: max(0, min(255, (v - 120) * 3)))
    Image.merge("RGB", [burn, burn.point(lambda v: int(v * 0.35)), burn.point(lambda v: int(v * 0.1))]).save(
        os.path.join(OUT, "T_Kit_InsigniaScorched_E.png"))


drag_mark()
scorch_lines()
light_pool()
insignia_scorched()
print("wrote T_Kit_DragMark_* and T_Kit_ScorchLines_*")
