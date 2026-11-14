import bpy,sys,math; sys.path.insert(0,'/tmp/rwork'); from rlib import *
a=sys.argv[sys.argv.index('--')+1:]; fbx,tdir,tag,views=a[0],a[1],a[2],a[3].split(',')
res=int(a[4]) if len(a)>4 else 800
sc=reset(); sc.render.resolution_x=res; sc.render.resolution_y=res
try: sc.eevee.taa_render_samples=64
except: pass
bpy.ops.import_scene.fbx(filepath=fbx)
o=[x for x in sc.objects if x.type=='MESH'][0]
for x in list(sc.objects):
    if x.type=='EMPTY': x.hide_render=True
tex=rift_mat('tex',tdir,estr=4.0); clay=rift_mat('clay','',clay=True)
V={'front':((8,0,0.1),(0,0,0.05),4.7,None),'rear':((-8,0,0.1),(-2,0,0.1),2.9,None),'side':((0,-8,0),(0,0,0),5.3,None),'top':((0,0,8),(0,0,0),5.3,None),
   'hero':((4.6,-4.3,2.6),(0,0,-0.1),None,42),'rearclose':((-5.0,-1.9,1.25),(-2.05,0,0.05),None,48),'ring':((-4.0,-1.25,1.55),(-1.72,0,0.5),None,62),
   'close3m':((1.55,-2.75,0.95),(0.45,0,0.05),None,32),'rear34':((-4.4,2.6,1.9),(-1.8,0,0.1),None,46)}
for mode in ('clay','tex'):
    o.data.materials.clear(); o.data.materials.append(clay if mode=='clay' else tex)
    for k in views:
        l,t,os_,lens=V[k]; cam(sc,l,t,ortho=os_,lens=lens or 50)
        render(sc,f'/tmp/rwork/sh/{tag}_{k}_{mode}.png')
