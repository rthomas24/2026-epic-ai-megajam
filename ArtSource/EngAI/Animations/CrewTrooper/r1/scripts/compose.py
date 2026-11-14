import sys, os, glob
from PIL import Image, ImageDraw, ImageFont
d, title, out = sys.argv[1], sys.argv[2], sys.argv[3]
fr = sorted(glob.glob(os.path.join(d,'front_*.png'))); sd = sorted(glob.glob(os.path.join(d,'side_*.png')))
w,h = Image.open(fr[0]).size; s = 0.6
W = int(w*s); H = int(h*s)
im = Image.new('RGB', (W*len(fr), H*2+40), (20,20,24))
dr = ImageDraw.Draw(im)
try: font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 20); fs = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 13)
except: font = fs = None
dr.text((8,8), title, fill=(240,240,240), font=font)
for i,(a,b) in enumerate(zip(fr,sd)):
    A = Image.open(a).convert('RGB').resize((W,H)); B = Image.open(b).convert('RGB').resize((W,H))
    im.paste(A,(i*W,40)); im.paste(B,(i*W,40+H))
    dr.text((i*W+4,44), 'f'+os.path.basename(a)[6:9], fill=(255,255,255), font=fs)
im.save(out, quality=88)
