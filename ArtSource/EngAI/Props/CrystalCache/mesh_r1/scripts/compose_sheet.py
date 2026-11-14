
#!/usr/bin/env python3
"""Compose the #53 contact sheet from rendered panels + the modelling reference."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

RENDERS = Path("/tmp/crystal-cache-build/renders")
REF = Path("/tmp/wt-crystal-cache-r1/ArtSource/EngAI/Props/CrystalCache/mesh_r1/refs/Crystal_Cache_Modeling_Reference.png")
MESH = Path("/tmp/wt-crystal-cache-r1/ArtSource/EngAI/Props/CrystalCache/mesh_r1")
OUT = Path("/tmp/wt-crystal-cache-r1/ArtSource/EngAI/Props/CrystalCache/mesh_r1/contact_sheet.png")

W, H = 3840, 2400
BG = (18, 20, 24)
BAR = (12, 14, 18)
ACCENT = (255, 140, 40)
FG = (236, 238, 242)
MUTED = (160, 166, 176)
CYAN = (80, 210, 230)

def font(size, bold=False):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    ]
    for p in candidates:
        if Path(p).exists():
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()

def fit(im, box, fill=(28, 30, 34)):
    bw, bh = box
    canvas = Image.new("RGB", (bw, bh), fill)
    im = im.convert("RGB")
    s = min(bw / im.width, bh / im.height)
    nw, nh = max(1, int(im.width * s)), max(1, int(im.height * s))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    canvas.paste(im, ((bw - nw)//2, (bh - nh)//2))
    return canvas

def panel(dst, draw, img, x, y, w, h, title, subtitle=""):
    # frame
    draw.rectangle([x, y, x+w, y+h], fill=(28, 30, 34), outline=(48, 52, 60), width=2)
    cap_h = 42
    body = fit(img, (w-4, h-4-cap_h))
    dst.paste(body, (x+2, y+2))
    draw.rectangle([x, y+h-cap_h, x+w, y+h], fill=BAR)
    draw.rectangle([x, y+h-cap_h, x+6, y+h], fill=ACCENT)
    draw.text((x+16, y+h-cap_h+8), title, font=font(20, True), fill=FG)
    if subtitle:
        tw = draw.textlength(title, font=font(20, True))
        draw.text((x+24+tw, y+h-cap_h+11), subtitle, font=font(16), fill=MUTED)

def main():
    dst = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(dst)
    # header
    draw.rectangle([0, 0, W, 110], fill=BAR)
    draw.rectangle([0, 110, W, 114], fill=ACCENT)
    draw.text((48, 22), "CRYSTAL CACHE  &  HERO CRYSTAL", font=font(42, True), fill=FG)
    draw.text((48, 70), "Issue #53  ·  mesh_r1  ·  Blender 4.5.3 LTS  ·  R-042 / R-043  ·  two meshes, SOCKET_Crystal, UCX boxes",
              font=font(20), fill=MUTED)
    draw.text((W-48, 28), "TRIAD DRIFT", font=font(22, True), fill=ACCENT, anchor="ra")
    draw.text((W-48, 70), "ArtSource/EngAI/Props/CrystalCache/mesh_r1", font=font(16), fill=MUTED, anchor="ra")

    # footer
    footer_y = H - 86
    draw.rectangle([0, footer_y, W, H], fill=BAR)
    draw.rectangle([0, footer_y, W, footer_y+3], fill=ACCENT)
    foot = ("KEY SPECS   Cradle  360 × 200 × 180 cm   ·   Hero cluster  140 cm   ·   180 cm pilot   ·   "
            "Crystal: Route C procedural hex facets, 1,384 tris   ·   Cradle: Rodin + Blender polish, 50,376 tris   ·   live Bevel + WeightedNormal on both   ·   SOCKET_Crystal (0, 0, 30.05 cm)")
    draw.text((48, footer_y+18), foot, font=font(18), fill=FG)
    draw.text((48, footer_y+50), "Origin at ground-base centre  ·  +Z up  ·  +X forward (emblem)  ·  UnitScaleFactor 100  ·  2K PBR  BC / N (DirectX 255-G) / ORM / E",
              font=font(16), fill=MUTED)

    # grid 3x3 under header, above footer
    top, bottom = 130, footer_y - 16
    left, right = 24, W-24
    gap = 16
    cols, rows = 4, 3
    cell_w = (right-left - gap*(cols-1)) // cols
    cell_h = (bottom-top - gap*(rows-1)) // rows

    def cell(c, r):
        return left + c*(cell_w+gap), top + r*(cell_h+gap), cell_w, cell_h

    ref = Image.open(REF)
    # use the full board as the concept panel
    def maps(prefix):
        tiles=[Image.open(MESH/f"{prefix}_{k}.png").convert("RGB").resize((512,512)) for k in ("BC","N","ORM","E")]
        m=Image.new("RGB",(1024,1024))
        for i,t in enumerate(tiles): m.paste(t,((i%2)*512,(i//2)*512))
        return m
    files = {
        (0,0): (ref, "01  #53 MODELLING REFERENCE", "3.6 x 2.0 x 1.8 m cradle, 1.4 m cluster"),
        (1,0): (Image.open(RENDERS/"assembled_hero.png"), "02  ASSEMBLED  +  180 cm PILOT", "Neutral studio HDRI"),
        (2,0): (Image.open(RENDERS/"cradle_alone.png"), "03  SM_CrystalCacheCradle", "Rodin cage, Blender polish"),
        (3,0): (Image.open(RENDERS/"crystal_alone.png"), "04  SM_CrystalCacheCrystal", "Route C hex facets, cyan + amber"),
        (0,1): (Image.open(RENDERS/"closeup.png"), "05  CLOSE-UP", "Bevel + WN, baked DirectX N"),
        (1,1): (Image.open(RENDERS/"collision_socket.png"), "06  SOCKET  &  UCX", "SOCKET_Crystal z 30 cm, UCX box + hull"),
        (2,1): (Image.open(RENDERS/"front_ortho.png"), "07  FRONT ORTHO (+X)  +  PILOT", "Emblem faces +X"),
        (3,1): (Image.open(RENDERS/"end_ortho.png"), "08  END ORTHO  +  PILOT", "Octagon end, crystal seated"),
        (0,2): (Image.open(RENDERS/"clay.png"), "09  CLAY", "Form without textures"),
        (1,2): (Image.open(RENDERS/"clay_wire.png"), "10  CLAY  +  HARD EDGES", "Feature edges >= 28 / 20 deg"),
        (2,2): (maps("T_CrystalCacheCradle"), "11  CRADLE 2K MAPS", "BC / N / ORM / E"),
        (3,2): (maps("T_CrystalCacheCrystal"), "12  CRYSTAL 2K MAPS", "BC / N / ORM / E"),
    }
    for (c,r), (im, title, sub) in files.items():
        x,y,w,h = cell(c,r)
        panel(dst, draw, im, x, y, w, h, title, sub)

    dst.save(OUT, optimize=True)
    # also write a local copy
    dst.save(RENDERS/"contact_sheet.png")
    print("wrote", OUT, dst.size)

if __name__ == "__main__":
    main()
