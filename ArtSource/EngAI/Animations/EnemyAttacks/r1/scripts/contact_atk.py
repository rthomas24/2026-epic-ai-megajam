from PIL import Image, ImageDraw, ImageFont
import json
V=json.load(open('/tmp/wd_atk/final/verify.json'))
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',26)
f2=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
f3=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',17)
R='/tmp/wd_atk/rend2'
clips=[('A_NullGrunt_Charge','Grunt charge loop - grounded run, lean ~41 deg, right shoulder dropped/leading (gruntD seed 4801 upper body + procedural legs)',[1,3,5,7,9,11,13,15,17],None,('side',9)),
       ('A_NullHeavy_ShieldBash','Heavy shield bash - coil, left-shoulder shove with a short step (heavyB seed 5201)',[1,8,14,18,22,28,38,48,56],22,('side',22)),
       ('A_Warden_GroundSlam','Warden two-handed ground slam - arms overhead, hammer down into a crouch, rise (slamC seed 6302)',[1,12,25,34,37,40,46,55,63],40,('side',40))]
W,H=360,480; cols=10; head=90; rowh=H+70
sheet=Image.new('RGB',(W*cols,head+rowh*len(clips)),(18,19,22)); d=ImageDraw.Draw(sheet)
d.text((20,15),'Enemy attacks r1  |  HY-Motion 1.0 via fal -> retargeted to SK_NullGrunt / SK_NullHeavy / SK_Warden (crew skeleton), Blender 4.5.3  |  30 fps, in place, feet IK-locked',font=F,fill=(235,235,235))
d.text((20,52),'Frames 1-based (frame N = (N-1)/30 s). Orange box = impact frame matching the code timers (Heavy MeleeWindup 0.7 s -> f22; Warden ChargeWindup 0.8 s + ChargeDuration 0.5 s -> f40). Blue cylinder = 180 cm figure.',font=f2,fill=(190,190,190))
for r,(n,title,fr,imp,side) in enumerate(clips):
    N=V[n]['frames']; y0=head+r*rowh
    extra=f'impact frame {imp} ({(imp-1)/30:.3f} s)' if imp else f'loop: frame 17 = frame 1, contact {V[n]["contact_line"]}'
    d.text((10,y0+8),f'{n}  -  {title}  -  {N} frames / {(N-1)/30:.2f} s  -  {extra}',font=f2,fill=(240,200,120))
    for c,f in enumerate(fr):
        im=Image.open(f'{R}/{n}_q34_{f:03d}.png').convert('RGB'); x=c*W; y=y0+40
        sheet.paste(im,(x,y)); lab=f'f{f}  {(f-1)/30:.2f}s'
        if imp:
            if f==imp: d.rectangle([x+2,y+2,x+W-3,y+H-3],outline=(255,80,40),width=5); lab+='  IMPACT'
            elif f<imp: lab+='  anticipation'
            else: lab+='  recovery'
        else: lab+='  '+{'l':'L foot down','r':'R foot down','B':'both down'}[V[n]['contact_line'][f-1]]
        d.text((x+10,y+8),lab,font=f3,fill=(255,255,255))
        if c==0: d.text((x+W-120,y+H-40),'180 cm',font=f3,fill=(120,190,255))
    v,f=side; im=Image.open(f'{R}/{n}_{v}_{f:03d}.png').convert('RGB'); x=9*W; y=y0+40
    sheet.paste(im,(x,y))
    if imp: d.rectangle([x+2,y+2,x+W-3,y+H-3],outline=(255,80,40),width=5)
    d.text((x+10,y+8),f'side f{f}'+('  IMPACT' if imp else ''),font=f3,fill=(255,255,255))
    # per-clip strip
    strip=sheet.crop((0,y0,W*cols,y0+rowh)); strip.save(f'/tmp/wd_atk/previews_{n}.png')
sheet.save('/tmp/wd_atk/contact_sheet.jpg',quality=88)
sheet.resize((sheet.width//2,sheet.height//2)).save('/tmp/wd_atk/contact_small.png')
print(sheet.size)
