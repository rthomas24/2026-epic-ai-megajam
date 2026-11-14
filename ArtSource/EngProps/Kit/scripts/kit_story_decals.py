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


drag_mark()
scorch_lines()
print("wrote T_Kit_DragMark_* and T_Kit_ScorchLines_*")
