from PIL import Image, ImageDraw, ImageFont
import json
V=json.load(open('/tmp/wd/final/verify.json'))
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',26)
f2=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
f3=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',17)
clips=[('A_Warden_Fire_01','Fire 01 - braced aim, cannon kick (fireC seed 2301)','Projectile spawn (all fan bolts)'),
       ('A_Warden_Fire_02','Fire 02 - raise and aim, authored recoil (fireE seed 2502)','Projectile spawn (all fan bolts)'),
       ('A_Warden_Summon_01','Summon 01 - gather, arms-up roar (sumB seed 3201)','Summon spawn (grunts appear)'),
       ('A_Warden_Summon_02','Summon 02 - fist to the ground (sumA seed 3101)','Summon spawn (grunts appear)')]
W,H=420,480; cols=9; head=90; rowh=H+70
sheet=Image.new('RGB',(W*cols,head+rowh*len(clips)),(18,19,22)); d=ImageDraw.Draw(sheet)
d.text((20,15),'Warden fire + summon r1  |  HY-Motion 1.0 via fal  ->  retargeted to the in-game SK_Warden (crew skeleton), Blender 4.5.3  |  30 fps, in place, feet IK-locked',font=F,fill=(235,235,235))
d.text((20,52),'Frames 1-based. Release (code: BurstWindup 0.5 s, then DoBoltFan / DoSummon) = frame 16 = 0.500 s in every clip. Blue cylinder = 180 cm figure. Last column: side view at the release frame.',font=f2,fill=(190,190,190))
for r,(n,title,ev) in enumerate(clips):
    N=V[n]['frames']; y0=head+r*rowh
    fr=[1,6,11,14,16,19,25,N]
    if N-fr[-2]<4: fr[-2]=fr[-2]-4
    d.text((10,y0+8),f'{n}  -  {title}  -  {N} frames / {N/30:.2f} s  -  {ev} at frame 16 (0.50 s)',font=f2,fill=(240,200,120))
    for c,f in enumerate(fr):
        im=Image.open(f'/tmp/wd/rend/{n}_q34_{f:03d}.png').convert('RGB'); x=c*W; y=y0+40
        sheet.paste(im,(x,y))
        lab=f'f{f}  {(f-1)/30:.2f}s'
        if f==16:
            d.rectangle([x+2,y+2,x+W-3,y+H-3],outline=(255,80,40),width=5); lab+='  RELEASE'
        elif f<16: lab+='  windup'
        else: lab+='  recovery'
        d.text((x+10,y+8),lab,font=f3,fill=(255,255,255))
        if c==0 and r==0: d.text((x+W-150,y+H-40),'180 cm',font=f3,fill=(120,190,255))
    im=Image.open(f'/tmp/wd/rend/{n}_side_016.png').convert('RGB'); x=8*W; y=y0+40
    sheet.paste(im,(x,y)); d.rectangle([x+2,y+2,x+W-3,y+H-3],outline=(255,80,40),width=5); d.text((x+10,y+8),'side f16 RELEASE',font=f3,fill=(255,255,255))
sheet.save('/tmp/wd/contact_sheet.jpg',quality=88)
sheet.resize((sheet.width//2,sheet.height//2)).save('/tmp/wd/contact_small.png')
print(sheet.size)
