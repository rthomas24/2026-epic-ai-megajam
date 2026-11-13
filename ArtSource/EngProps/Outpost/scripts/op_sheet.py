# Contact sheet from review/<prefix>_*.png (PIL, system Python).
import sys, glob
from PIL import Image
prefix, out = sys.argv[1], sys.argv[2]
order = sys.argv[3].split(",")
ims = [Image.open("%s_%s.png" % (prefix, n)).convert("RGB") for n in order]
w, h = ims[0].size
sheet = Image.new("RGB", (w * 3, h * ((len(ims) + 2) // 3)))
for i, im in enumerate(ims):
    sheet.paste(im.resize((w, h)), ((i % 3) * w, (i // 3) * h))
sheet.thumbnail((2400, 1800)); sheet.save(out, quality=88)
