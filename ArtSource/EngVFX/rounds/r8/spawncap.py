#!/usr/bin/env python3
"""Round 10: spawn one system ahead of the flown ship and photograph it (PIE on $TD_PORT must be running).
usage: python spawncap.py <tag> <system> <dist_m> <view> [scale=1] [yaw=0] [params json] [frames=6] [slomo=1] [fov=50]
  view: side   = fixed scene camera dist_m to the right of the subject (same distance as the player's view, so
                 Engine.Owner.LODDistance matches), one frame every 0.1 s, a strip
        chase  = the player's own viewport (play.py cap), one frame
  yaw: added to the ship's yaw (180 = +X of the system points back at the camera).
  params: {"Pulse": 1.0, "Length": 20000, "Color": [1, 0.6, 0.2]} (lists of 3/4 are colours)."""
import os, sys, json, time, subprocess
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'scripts'))
import rig
from rig import py, con, capture
a = sys.argv
tag_, name, dist, view = a[1], a[2], float(a[3]), a[4]
scale = float(a[5]) if len(a) > 5 else 1.0
yaw = float(a[6]) if len(a) > 6 else 0.0
params = json.loads(a[7]) if len(a) > 7 else {}
frames = int(a[8]) if len(a) > 8 else 6
slomo = a[9] if len(a) > 9 else '1'
fov = float(a[10]) if len(a) > 10 else 50.0
OUT = os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', 'spawn')
os.makedirs(OUT, exist_ok=True)
D = dist * 100.0
cam = py('''
o = pawn.get_actor_location() + pawn.get_actor_forward_vector() * %r
c = o + pawn.get_actor_right_vector() * %r + unreal.Vector(0, 0, %r)
rr = unreal.MathLibrary.find_look_at_rotation(c, o)
json.dumps({'loc': vec(c), 'rot': {'pitch': rr.pitch, 'yaw': rr.yaw, 'roll': rr.roll}})''' % (D, D, D * 0.08))
con('slomo ' + slomo)
py('''
clear()
o = pawn.get_actor_location() + pawn.get_actor_forward_vector() * %r
r = pawn.get_actor_rotation(); r = unreal.Rotator(r.roll, r.pitch, r.yaw + %r)
c = tag(unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, unreal.load_asset(SYS + %r), o, r, unreal.Vector(%r, %r, %r), True, True, unreal.NCPoolMethod.NONE, True))
for k, v in %r.items():
    if isinstance(v, list):
        c.set_variable_linear_color(k, unreal.LinearColor(*(v + [1.0])[:4]))
    else:
        c.set_variable_float(k, float(v))
"spawned"''' % (D, yaw, name, scale, scale, scale, params))
time.sleep(0.6)
if view == 'side':
    files = []
    for i in range(frames):
        f = os.path.join(OUT, '%s_%02d.png' % (tag_, i))
        capture(cam, f, 960, 540, fov)
        files.append(f)
        time.sleep(0.1)
    sel = files[:6]
    grid = '3x2' if len(sel) == 6 else '%dx1' % len(sel)
    args = ['ffmpeg', '-y', '-loglevel', 'error'] + sum([['-i', x] for x in sel], [])
    subprocess.check_call(args + ['-filter_complex', ''.join('[%d]' % k for k in range(len(sel))) + 'xstack=inputs=%d:grid=%s' % (len(sel), grid) if len(sel) > 1 else 'null',
                                  os.path.join(OUT, tag_ + '.png')])
else:
    subprocess.check_call([sys.executable, os.path.join(rig.ROOT, 'ArtSource', 'EngVFX', 'scripts', 'play.py'), os.environ.get('TD_PORT', '21031'),
                           os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', 'spawn'), 'cap:' + tag_], stdout=subprocess.DEVNULL)
con('slomo 1')
print(os.path.join(OUT, tag_ + '.png'))
