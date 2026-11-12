"""Original Hero refinement: tiled multiscale gas/dust authoring + angular stars.

Uses numpy from Blender's bundled Python, without importing bpy or starting Blender.
NASA imagery is reference only; no image files are downloaded or sampled.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time
import zlib
import numpy as np

HERE = Path(__file__).resolve().parent
TAU = np.float32(math.tau)


def smooth(a, b, value):
    t = np.clip((value-a)/(b-a), 0, 1)
    return t*t*(3-2*t)


def noise(x, y, z, seed=0):
    """3D value noise, smooth interpolation; integer hash from frozen generator."""
    ix, iy, iz = np.floor(x).astype(np.int32), np.floor(y).astype(np.int32), np.floor(z).astype(np.int32)
    fx, fy, fz = x-ix.astype(np.float32), y-iy.astype(np.float32), z-iz.astype(np.float32)
    fx, fy, fz = (v*v*v*(v*(v*6-15)+10) for v in (fx, fy, fz))
    def h(a,b,c):
        with np.errstate(over='ignore'):
            n = np.uint32(a)*np.uint32(73856093) ^ np.uint32(b)*np.uint32(19349663) ^ np.uint32(c)*np.uint32(83492791) ^ np.uint32(90210+seed*31337)
            n = (n ^ (n >> 13))*np.uint32(1274126177)
            return ((n ^ (n >> 16)) & np.uint32(65535)).astype(np.float32)/np.float32(65535)
    def plane(k):
        a = h(ix,iy,iz+k)*(1-fx)+h(ix+1,iy,iz+k)*fx
        b = h(ix,iy+1,iz+k)*(1-fx)+h(ix+1,iy+1,iz+k)*fx
        return a*(1-fy)+b*fy
    return (plane(0)*(1-fz)+plane(1)*fz).astype(np.float32)


def fbm(x, y, z, octaves=7, seed=0, roughness=0.52):
    total = np.zeros(np.broadcast_shapes(np.shape(x),np.shape(y),np.shape(z)), np.float32)
    amplitude, norm = 1.0, 0.0
    for i in range(octaves):
        total += amplitude*noise(x,y,z,seed+i*7)
        norm += amplitude
        # Non-axis-aligned octave rotation prevents a visible lattice/striping.
        x,y,z = (1.63*x+1.14*y+0.51*z+7.2,
                 -1.17*x+1.56*y+0.61*z-3.1,
                 0.42*x-0.67*y+1.94*z+11.8)
        amplitude *= roughness
    return total/norm


def sphere(u, v):
    lon,lat = u*TAU,(0.5-v)*np.float32(math.pi)
    c = np.cos(lat)
    return c*np.cos(lon), c*np.sin(lon), np.sin(lat)


def layers(u, v):
    x,y,z = sphere(u,v)
    broad = noise(x*2+12,y*2-3,z*2+8)
    # Preserve the original Hero's broad curved banks and dark combat corridor.
    band = z+0.28*x-0.16*np.sin(y*4)+(broad-0.5)*0.44
    envelope = np.exp(-((band-0.21)/0.25)**2)
    secondary = np.exp(-((band+0.25)/0.13)**2)
    wx = fbm(x*3.1+8,y*3.1-2,z*3.1+13,4,11)-0.5
    wy = fbm(x*3.1-9,y*3.1+17,z*3.1+6,4,23)-0.5
    wz = fbm(x*3.1+19,y*3.1+7,z*3.1-11,4,41)-0.5
    px,py,pz = x*6.0+wx*0.95,y*6.0+wy*0.95,z*6.0+wz*0.95
    cloud = fbm(px,py,pz,8,71,0.56)
    turbulence = fbm(px*1.7+5,py*0.65-8,pz*2.3+6,7,131,0.55)
    filament_field = fbm(px*0.42-3,py*2.1+9,pz*2.7-12,7,199,0.58)
    filaments = np.exp(-((filament_field-0.52)/0.023)**2)
    filaments *= smooth(0.35,0.61,turbulence)
    density = envelope*smooth(0.29,0.70,cloud)*(0.32+1.7*turbulence**2)
    # Broken, fine emission ridges; not uniform glow around every dark object.
    fine=fbm(px*3.4+11,py*3.4-19,pz*3.4+17,4,283,0.58)
    emission = density*(0.35+1.1*cloud)*(0.5+1.1*fine) + envelope*filaments*0.06
    emission += secondary*filaments*(0.035+cloud*0.09)
    dust_field = fbm(px*0.74+21,py*1.2-6,pz*1.85+3,7,307,0.57)
    dust_threads = np.exp(-((dust_field-0.53)/0.026)**2)
    dust_threads *= smooth(0.50,0.68,turbulence)
    dust_clumps = smooth(0.43,0.68,fbm(px*0.63-17,py*0.9+3,pz*0.9+8,5,397))
    dust_tau = envelope*(dust_threads*0.35+dust_clumps*3.2)*smooth(0.30,0.57,cloud)
    # Cool scattered light from a localized source, attenuated by irregular gas.
    light = 0.22+0.78*np.exp(-((x+0.1)**2+(y-0.66)**2+(z-0.35)**2)/0.9)
    shadow = np.exp(-density*0.8-dust_tau*0.44)
    illumination = (0.32+light)*shadow
    hue = np.clip(0.5+x*0.45+y*0.30,0,1)
    cyan = np.stack(np.broadcast_arrays(0.12+0*hue,0.46+0*hue,0.62+0*hue),axis=-1)
    violet = np.stack(np.broadcast_arrays(0.38+0*hue,0.16+0*hue,0.57+0*hue),axis=-1)
    color = cyan*hue[...,None]+violet*(1-hue[...,None])
    # Linear radiance art values, kept modest; sparse rims carry the highlights.
    far_density=envelope*smooth(0.28,0.70,fbm(px*.72+29,py*.72-14,pz*.72+37,5,449))
    far_light=0.28+0.4*light
    far_radiance=color*((1-np.exp(-far_density*.65))*far_light*.32)[...,None]
    back_rgb = far_radiance*np.exp(-density*.75-dust_tau*.4)[...,None]+color*(emission*illumination*1.05)[...,None]
    back_rgb += np.array([0.085,0.093,0.11],np.float32)*(density*light*cloud)[...,None]
    back_rgb += np.array([0.001,0.0015,0.003],np.float32)
    star_transmission = np.exp(-far_density*.35-density*0.65-dust_tau*0.5)
    # A separate foreground depth slab contributes extinction and weak scattering.
    front_shape = np.exp(-((band-0.01+(dust_field-0.5)*0.18)/0.15)**2)
    front_density = front_shape*(dust_threads*0.4+dust_clumps*1.5)*smooth(0.32,0.60,turbulence)
    front_a = 1-np.exp(-front_density*1.6)
    rim = front_shape*filaments*light*0.03
    front_rgb = color*(0.008+rim)[...,None] + np.array([0.004,0.0045,0.006],np.float32)
    return np.concatenate((back_rgb,star_transmission[...,None]),-1), np.concatenate((front_rgb,front_a[...,None]),-1)


def linear_to_srgb(value):
    return np.where(value<=0.0031308,value*12.92,1.055*np.maximum(value,0)**(1/2.4)-0.055)


class PngWriter:
    def __init__(self,path,width,height):
        if path.exists():
            raise RuntimeError('Refusing existing output: '+str(path))
        self.f=path.open('wb');self.f.write(b'\x89PNG\r\n\x1a\n');self.z=zlib.compressobj(6)
        self.chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0));self.chunk(b'sRGB',b'\0')
    def chunk(self,kind,data):
        self.f.write(struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff))
    def rows(self,rgba):
        rgb=np.clip(linear_to_srgb(rgba[...,:3]),0,1)
        encoded=np.concatenate((rgb,np.clip(rgba[...,3:],0,1)),-1)
        encoded=np.round(encoded*255).astype(np.uint8)
        for row in encoded:
            data=self.z.compress(b'\0'+row.tobytes())
            if data:self.chunk(b'IDAT',data)
    def close(self):
        self.chunk(b'IDAT',self.z.flush());self.chunk(b'IEND',b'');self.f.close()


def bake(width,out):
    height=width//2
    back=PngWriter(out/'T_HeroM02_Back.png',width,height)
    front=PngWriter(out/'T_HeroM02_Front.png',width,height)
    u=(np.arange(width,dtype=np.float32)+0.5)[None,:]/width
    for row in range(0,height,64):
        v=(np.arange(row,min(height,row+64),dtype=np.float32)+0.5)[:,None]/height
        b,f=layers(u,v);back.rows(b);front.rows(f)
        if row%512==0:print(f'Gas/dust {row}/{height}',flush=True)
    back.close();front.close()


def camera_study(path):
    """Authoring-only approximation, not a native Unreal verification image."""
    width,height=1280,720
    yaw,pitch=math.radians(125),math.radians(10)
    f=np.array([math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch)])
    r=np.array([-math.sin(yaw),math.cos(yaw),0])
    t=np.cross(f,r)
    sx=((np.arange(width)+.5)/width*2-1)[None,:,None]
    sy=(1-(np.arange(height)+.5)/height*2)[:,None,None]*(height/width)
    d=f+r*sx+t*sy;d/=np.linalg.norm(d,axis=-1,keepdims=True)
    u=np.mod(np.arctan2(d[...,1],d[...,0])/math.tau,1).astype(np.float32)
    v=(np.arccos(d[...,2])/math.pi).astype(np.float32)
    back,front=layers(u,v)
    rgb=back[...,:3]*(1-front[...,3:])+front[...,:3]*front[...,3:]
    rgb=np.clip((rgb*(2.51*rgb+.03))/(rgb*(2.43*rgb+.59)+.14),0,1)
    writer=PngWriter(path,width,height);writer.rows(np.concatenate((rgb,np.ones((height,width,1))),axis=-1));writer.close()


def temperature_rgb(kelvin):
    t=kelvin/100.0
    red=255 if t<=66 else 329.698727446*(t-60)**-0.1332047592
    green=99.4708025861*math.log(t)-161.1195681661 if t<=66 else 288.1221695283*(t-60)**-0.0755148492
    blue=255 if t>=66 else (0 if t<=19 else 138.5177312231*math.log(t-10)-305.0447927307)
    rgb=np.clip(np.array([red,green,blue])/255,0,1)
    return np.where(rgb<=0.04045,rgb/12.92,((rgb+0.055)/1.055)**2.4)


def stars(width,out,count=10500):
    """Long-lat HDR source with angular-distance kernels, converted to a UE cube.

    Longitude stamps widen with latitude to preserve circular angular shapes;
    sampling the imported cube has no equirectangular zenith singularity.
    """
    height=width//2
    pixels=np.zeros((height,width,3),np.float32)
    rng=np.random.default_rng(8314117)
    for i in range(count):
        lon=rng.uniform(0,math.tau)
        z=rng.uniform(-1,1)
        if i%4==0:
            z=np.clip(rng.normal(0.04,0.18),-.98,.98)
        lat=math.asin(z)
        # One deliberately near-zenith test star is part of the original design.
        if i==0:lon,lat=0.2,math.radians(88.0)
        radius=math.radians(rng.uniform(0.025,0.065))
        bright=0.018+float(rng.random()**5)*1.65
        if i<18:radius=math.radians(rng.uniform(0.07,0.11));bright=rng.uniform(2.0,4.5)
        color=temperature_rgb(float(rng.uniform(3400,10500)))
        cx,cy=lon/math.tau*width,(.5-lat/math.pi)*height
        ry=max(2,math.ceil(radius*3/math.pi*height))
        rx=min(width//2,max(2,math.ceil(radius*3/(math.tau*max(.015,math.cos(lat)))*width)))
        xx=np.arange(math.floor(cx)-rx,math.floor(cx)+rx+1)
        yy=np.arange(max(0,math.floor(cy)-ry),min(height,math.floor(cy)+ry+1))
        ll=(xx+.5)/width*math.tau;aa=(.5-(yy+.5)/height)*math.pi
        # Stable haversine distance keeps very small stars numerically round.
        hav=np.sin((aa[:,None]-lat)/2)**2+np.cos(aa[:,None])*math.cos(lat)*np.sin((ll[None,:]-lon)/2)**2
        angle2=4*hav
        kernel=np.exp(-angle2/(2*radius*radius))*bright
        pixels[yy[:,None],xx[None,:]%width,:]+=kernel[...,None]*color
    path=out/'T_HeroM02_Stars.hdr'
    if path.exists():raise RuntimeError('Refusing existing stars')
    with path.open('wb') as f:
        f.write(f'#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y {height} +X {width}\n'.encode())
        for row in pixels:
            maximum=row.max(axis=1);mantissa,exponent=np.frexp(maximum)
            scale=np.where(maximum>1e-30,mantissa*256/np.maximum(maximum,1e-30),0)
            rgbe=np.zeros((width,4),np.uint8)
            rgbe[:,:3]=np.minimum(255,row*scale[:,None]).astype(np.uint8)
            rgbe[:,3]=np.where(maximum>1e-30,exponent+128,0).astype(np.uint8)
            f.write(bytes((2,2,width>>8,width&255)))
            for channel in range(4):
                values=rgbe[:,channel]
                edges=np.r_[0,np.flatnonzero(values[1:]!=values[:-1])+1,width]
                literal=bytearray()
                def flush():
                    while literal:
                        part=literal[:128];del literal[:128]
                        f.write(bytes((len(part),))+part)
                for start,end in zip(edges[:-1],edges[1:]):
                    length=int(end-start)
                    if length>=4:
                        flush()
                        while length:
                            size=min(length,127);f.write(bytes((128+size,int(values[start]))));length-=size
                    else:
                        literal.extend(values[start:end].tobytes())
                        if len(literal)>=128:flush()
                flush()
    return {'star_count':count,'seed':8314117,'hdr_size':[width,height],
            'angular_sigma_deg':[.025,.11],'temperature_kelvin':[3400,10500]}


def validate():
    u=np.array([[0,.17,.41,.78,1]],np.float32)
    for v in (0.,.3,.5,1.):
        b,f=layers(u,np.array([[v]],np.float32))
        assert np.isfinite(b).all() and np.isfinite(f).all()
        for slab in (b,f):
            assert np.allclose(slab[:,0],slab[:,-1],atol=2e-4)
            assert np.min(slab)>=0 and np.max(slab[...,3])<=1
            if v in (0.,1.):assert np.max(np.abs(slab-slab[:,:1]))<2e-4
    return 'finite values, longitude continuity and pole continuity PASS'


def main():
    p=argparse.ArgumentParser();p.add_argument('--width',type=int,choices=(1024,2048,4096,8192),default=2048)
    p.add_argument('--output',type=Path,default=HERE/'export');p.add_argument('--stars-width',type=int,default=4096)
    p.add_argument('--skip-stars',action='store_true');p.add_argument('--camera-study',action='store_true');args=p.parse_args()
    print(validate(),flush=True);args.output.mkdir(parents=True,exist_ok=True);start=time.perf_counter()
    bake(args.width,args.output)
    if args.camera_study:camera_study(args.output/'AuthoringCameraStudy.png')
    star_info=None if args.skip_stars else stars(args.stars_width,args.output)
    manifest={'gas_resolution':[args.width,args.width//2],'stars':star_info,'generation_seconds':time.perf_counter()-start,
              'author':'Nicholas Tosti / OpenAI Codex Environment & VFX Agent','downloaded_texture_inputs':[],
              'reference_only':['https://science.nasa.gov/asset/webb/cosmic-cliffs-in-the-carina-nebula-nircam-image/',
               'https://science.nasa.gov/missions/hubble/ghostly-cats-head-nebula-lurks-within-sight-of-nasas-hubble-webb/'],
              'files':{x.name:{'bytes':x.stat().st_size,'sha256':hashlib.sha256(x.read_bytes()).hexdigest()} for x in args.output.iterdir() if x.is_file()}}
    (args.output/'authoring.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2),flush=True)


if __name__=='__main__':main()
