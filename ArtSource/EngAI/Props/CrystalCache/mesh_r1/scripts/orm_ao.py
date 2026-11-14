import sys, numpy as np
from PIL import Image
sys.path.insert(0,'/tmp/cc-finish'); 
exec(open('/tmp/cc-finish/fill_lift.py').read().split('bc_in, e_in')[0])  # s2l/l2s/pushpull
name, ao_png = sys.argv[1], sys.argv[2]
D='/tmp/wt-crystal-cache-r1/ArtSource/EngAI/Props/CrystalCache/mesh_r1/'
valid=np.asarray(Image.open(f'/tmp/cc-finish/texbak/T_CrystalCache{name}_BC.png').convert('RGB')).sum(-1)>0
orm=np.asarray(Image.open(f'/tmp/cc-finish/texbak/T_CrystalCache{name}_ORM.png').convert('RGB')).astype(np.float32)/255
ao=np.asarray(Image.open(ao_png).convert('L')).astype(np.float32)/255
orm[...,0]=ao
orm=pushpull(orm, valid)
Image.fromarray((np.clip(orm,0,1)*255+0.5).astype(np.uint8)).save(D+f'T_CrystalCache{name}_ORM.png')
print(name,'covered AO p25/50/75', np.round(np.percentile(ao[valid],[25,50,75]),2), 'ORM whole medians', np.round(np.median(orm.reshape(-1,3),0),2))
