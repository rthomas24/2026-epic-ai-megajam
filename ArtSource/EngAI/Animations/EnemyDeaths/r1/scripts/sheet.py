import json, os, glob
from PIL import Image, ImageDraw, ImageFont
V = json.load(open('final/verify.json'))
F = lambda s: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf' % s, 1) 
fb = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 22)
fm = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 15)
fs = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 12)
order = [f'A_{c}_Death_{d}' for c in ('Grunt','Heavy','Warden') for d in ('Front','Back','Left','Right')]
fall = {'Front':'shot from the front, falls onto its back','Back':'shot from behind, falls face down','Left':'shot from its left, falls to its right','Right':'shot from its right, falls to its left'}
s = 0.42; W0, H0 = 360, 480; w, h = int(W0*s), int(H0*s)
cols = 9; rowh = h + 34
im = Image.new('RGB', (cols*w + 10, 96 + len(order)*rowh + 30), (22,22,26))
d = ImageDraw.Draw(im)
d.text((10,10), 'Enemy death clips r1 - Grunt, Heavy (SKEL_NullCrew), Warden (SKEL_Warden)', fill=(240,240,240), font=fb)
d.text((10,42), 'Per row: 8 evenly spaced frames, front-right view (character faces -Y), then the last frame from the side. Orange column = 180 cm. Grid = 50 cm. Camera follows the pelvis.', fill=(200,200,200), font=fm)
d.text((10,64), 'Names follow ATDCrewCharacter::FallbackDeathAnim: the side the killing shot came from. Root motion on the root bone (smoothed pelvis XY). Workbench preview, not an in-game capture.', fill=(200,200,200), font=fm)
y = 96
for n in order:
    v = V[n+'.fbx']; c, dd = n.split('_')[1], n.split('_')[3]
    d.text((10, y+4), f"{n}   {v['frames']} f / {v['seconds']:.2f} s @30   root motion {v['root_end_dist_cm']:.0f} cm   still from f{v['still_from_frame']-1}   ({fall[dd]})", fill=(255,210,120), font=fm)
    fr = sorted(glob.glob(f'prev/{n}/front_*.png')); sd = sorted(glob.glob(f'prev/{n}/side_*.png'))
    for i, f in enumerate(fr + sd[-1:]):
        t = Image.open(f).convert('RGB').resize((w, h)); im.paste(t, (5 + i*w, y+26))
        lab = ('f' + os.path.basename(f)[-7:-4]) if i < len(fr) else 'end, side'
        d.text((8 + i*w, y+28), lab, fill=(255,255,255), font=fs)
    y += rowh
im.save('contact_sheet.jpg', quality=86)
im.resize((im.width//2, im.height//2), Image.LANCZOS).save('preview.png')
print(im.size)
