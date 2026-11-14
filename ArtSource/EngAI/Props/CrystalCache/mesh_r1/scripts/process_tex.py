
import sys, numpy as np
from PIL import Image
from pathlib import Path

args = [a for a in sys.argv[1:] if not a.startswith("-")]
CRYSTAL_ONLY = "--crystal-only" in sys.argv
tex = Path(args[0])

def srgb_to_lin(a):
    return np.where(a<=0.04045, a/12.92, ((a+0.055)/1.055)**2.4)
def lin_to_srgb(a):
    a=np.clip(a,0,None)
    return np.where(a<=0.0031308, a*12.92, 1.055*np.power(a,1/2.4)-0.055)

if not CRYSTAL_ONLY:
    bc = np.asarray(Image.open(tex/"T_CrystalCacheCradle_BC_raw.png").convert("RGB")).astype(np.float32)/255
    emi = np.asarray(Image.open(tex/"T_CrystalCacheCradle_E_raw.png").convert("RGB")).astype(np.float32)/255
    rough = np.asarray(Image.open(tex/"T_CrystalCacheCradle_R_raw.png").convert("L")).astype(np.float32)/255
    metal = np.asarray(Image.open(tex/"T_CrystalCacheCradle_M_raw.png").convert("L")).astype(np.float32)/255
    ao = np.asarray(Image.open(tex/"T_CrystalCacheCradle_AO_raw.png").convert("L")).astype(np.float32)/255
    nrm = np.asarray(Image.open(tex/"T_CrystalCacheCradle_N_ogl.png").convert("RGB")).astype(np.float32)

    lin = srgb_to_lin(bc)
    srgb_lum = 0.2126*bc[...,0]*255+0.7152*bc[...,1]*255+0.0722*bc[...,2]*255
    used = srgb_lum > 3.0  # ignore empty UV padding
    lum = 0.2126*lin[...,0]+0.7152*lin[...,1]+0.0722*lin[...,2]
    tgt = ((68/255+0.055)/1.055)**2.4
    med = float(np.median(lum[used])) if used.any() else float(np.median(lum))
    k = min(6.0, tgt/max(med, 1e-4))
    lin2 = lin.copy()
    lin2[used] = lin[used]*k
    lin2 = lin2/(1+np.maximum(lin2-0.5,0)*1.5)
    lin2 = np.clip(lin2, 0, 0.82)
    e_lum = np.max(emi, axis=-1)
    mask = e_lum > (18/255)
    lin2[mask] = lin[mask]
    sr = np.clip(lin_to_srgb(lin2),0,1)
    # keep padding black
    sr[~used & ~mask] = 0
    Image.fromarray((sr*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCradle_BC.png")
    after = 0.2126*sr[...,0]*255+0.7152*sr[...,1]*255+0.0722*sr[...,2]*255
    print("cradle BC lift gain", round(k,2),
          "used p50 before", round(float(np.median(srgb_lum[used])),1),
          "after", round(float(np.median(after[used])),1),
          "used frac", round(float(used.mean()),3))

    e2 = emi.copy()
    e2[e_lum <= (18/255)] = 0
    Image.fromarray((np.clip(e2,0,1)*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCradle_E.png")


    # If a data bake landed black (legacy Non-Color bug), synthesize usable defaults.
    if rough.max() == 0:
        print("WARN roughness bake empty; defaulting to 0.50")
        rough = np.where(used if "used" in dir() else (bc.max(axis=-1)>3/255), 0.50, 0.0)
    if metal.max() == 0:
        print("WARN metallic bake empty; defaulting to 0.70 on used texels")
        metal = np.where(srgb_lum>3.0, 0.70, 0.0)
    if ao.max() == 0:
        print("WARN AO bake empty; defaulting to 1")
        ao = np.where(srgb_lum>3.0, 1.0, 0.0)
    if nrm.max() == 0:
        print("WARN normal bake empty; defaulting to flat OpenGL (128,128,255)")
        nrm = np.zeros_like(nrm)
        nrm[...,0] = 128; nrm[...,1] = 128; nrm[...,2] = 255
        nrm = nrm.astype(np.float32)


    m2 = np.clip(0.28 + metal*3.2, 0.0, 0.92)
    m2 = np.clip(m2 * (0.85 + 0.20*ao), 0.0, 0.92)
    paint = (metal < 0.04) & (rough > 0.85)
    m2 = np.where(paint, np.clip(metal*2.0+0.08,0,0.35), m2)
    m2 = np.where(mask, np.minimum(m2, 0.15), m2)
    r2 = np.clip(0.18 + rough*0.50, 0.22, 0.86)
    r2 = np.clip(r2 + (1-ao)*0.10, 0.20, 0.90)
    r2 = np.clip(r2 - 0.06*m2, 0.18, 0.90)
    orm = np.stack([np.clip(ao,0,1), r2, m2], axis=-1)
    Image.fromarray((orm*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCradle_ORM.png")

    n2 = nrm.copy()
    n2[...,1] = 255.0 - nrm[...,1]
    Image.fromarray(np.clip(n2,0,255).astype(np.uint8)).save(tex/"T_CrystalCacheCradle_N.png")


cbc = np.asarray(Image.open(tex/"T_CrystalCacheCrystal_BC_raw.png").convert("RGB")).astype(np.float32)/255
cemi = np.asarray(Image.open(tex/"T_CrystalCacheCrystal_E_raw.png").convert("RGB")).astype(np.float32)/255
crm_ = np.asarray(Image.open(tex/"T_CrystalCacheCrystal_RM_raw.png").convert("RGB")).astype(np.float32)/255
cao = np.asarray(Image.open(tex/"T_CrystalCacheCrystal_AO_raw.png").convert("L")).astype(np.float32)/255
cn = np.asarray(Image.open(tex/"T_CrystalCacheCrystal_N_ogl.png").convert("RGB")).astype(np.float32)

lin = srgb_to_lin(cbc)
lum = 0.2126*lin[...,0]+0.7152*lin[...,1]+0.0722*lin[...,2]
dark = lum < srgb_to_lin(np.array(0.16))
lin = lin.copy()
lin[dark] = np.clip(lin[dark]*1.8, 0, 0.4)
cbc2 = np.clip(lin_to_srgb(lin),0,1)
Image.fromarray((cbc2*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCrystal_BC.png")

e_lum = np.max(cemi, axis=-1)
cemi2 = cemi.copy()
cemi2[e_lum <= (10/255)] = 0
Image.fromarray((np.clip(cemi2,0,1)*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCrystal_E.png")

orm = np.stack([np.clip(cao,0,1), np.clip(crm_[...,0], 0.06, 0.90), np.clip(crm_[...,1], 0, 0.25)], axis=-1)
Image.fromarray((orm*255+0.5).astype(np.uint8)).save(tex/"T_CrystalCacheCrystal_ORM.png")
n2 = cn.copy(); n2[...,1] = 255.0 - cn[...,1]
Image.fromarray(np.clip(n2,0,255).astype(np.uint8)).save(tex/"T_CrystalCacheCrystal_N.png")
print("textures processed")
