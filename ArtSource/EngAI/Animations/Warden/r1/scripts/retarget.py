# Retarget HY-Motion (SMPL-H) clip onto the in-game Warden skeleton (crew bone names), with
# time warp (release at a fixed frame), foot-lock two-bone IK, closed left fist, in-place root.
import bpy, sys, json, math
from mathutils import Vector, Matrix, Quaternion
args = json.loads(sys.argv[sys.argv.index('--')+1])
SRC, TGT, OUT, BLEND = args['src'], args['tgt'], args['out'], args.get('blend')
FPS = 30
sc = bpy.context.scene
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.scale_length = 0.01
sc.render.fps = FPS
bpy.ops.import_scene.fbx(filepath=TGT, use_anim=False, automatic_bone_orientation=False, ignore_leaf_bones=False)
tgt = bpy.data.objects['root']; body = [o for o in bpy.data.objects if o.type=='MESH'][0]
pre = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=SRC)
srcobjs = [o for o in bpy.data.objects if o not in pre]
src = [o for o in srcobjs if o.type=='ARMATURE'][0]
sact = src.animation_data.action
sf0, sf1 = [int(x) for x in sact.frame_range]

MAP = {'pelvis':'Pelvis','spine_01':'Spine1','spine_03':'Spine2','spine_05':'Spine3','neck_01':'Neck','head':'Head',
 'clavicle_l':'L_Collar','upperarm_l':'L_Shoulder','lowerarm_l':'L_Elbow','hand_l':'L_Wrist',
 'clavicle_r':'R_Collar','upperarm_r':'R_Shoulder','lowerarm_r':'R_Elbow','hand_r':'R_Wrist',
 'thigh_l':'L_Hip','calf_l':'L_Knee','foot_l':'L_Ankle','ball_l':'L_Foot',
 'thigh_r':'R_Hip','calf_r':'R_Knee','foot_r':'R_Ankle','ball_r':'R_Foot'}
# limb direction pairs (target child joint, source child joint); None = world up
DIR = {'pelvis':('spine_01','Spine1'),'spine_01':('spine_03','Spine2'),'spine_03':('spine_05','Spine3'),
 'spine_05':('neck_01','Neck'),'neck_01':('head','Head'),'head':None,
 'clavicle_l':('upperarm_l','L_Shoulder'),'upperarm_l':('lowerarm_l','L_Elbow'),'lowerarm_l':('hand_l','L_Wrist'),'hand_l':('middle_01_l','L_Middle1'),
 'clavicle_r':('upperarm_r','R_Shoulder'),'upperarm_r':('lowerarm_r','R_Elbow'),'lowerarm_r':('hand_r','R_Wrist'),'hand_r':('middle_01_r','R_Middle1'),
 'thigh_l':('calf_l','L_Knee'),'calf_l':('foot_l','L_Ankle'),'foot_l':('ball_l','L_Foot'),'ball_l':'foot_l',
 'thigh_r':('calf_r','R_Knee'),'calf_r':('foot_r','R_Ankle'),'foot_r':('ball_r','R_Foot'),'ball_r':'foot_r'}

TB = tgt.data.bones; SB = src.data.bones
tmw = tgt.matrix_world.copy(); tmw_inv = tmw.inverted()
smw = src.matrix_world.copy()
def rot3(m): return m.to_quaternion()
# rest data in world (cm)
t_rest = {b.name: tmw @ b.matrix_local for b in TB}
s_rest = {b.name: smw @ b.matrix_local for b in SB}
def thead(n): return t_rest[n].translation
def shead(n): return s_rest[n].translation
A = {}
for t, s in MAP.items():
    d = DIR[t]
    if isinstance(d, str): A[t] = A[d]; continue
    if d is None: A[t] = Quaternion(); continue
    ut = (thead(d[0]) - thead(t)).normalized(); us = (shead(d[1]) - shead(s)).normalized()
    A[t] = ut.rotation_difference(us)
# leg scale
def L(h, a, b, names): return (h(names[1])-h(names[0])).length + (h(names[2])-h(names[1])).length
s_scale = ( L(thead,0,0,['thigh_l','calf_l','foot_l']) + L(thead,0,0,['thigh_r','calf_r','foot_r']) ) / \
          ( L(shead,0,0,['L_Hip','L_Knee','L_Ankle']) + L(shead,0,0,['R_Hip','R_Knee','R_Ankle']) )

# ---- time map
tw = args['timewarp']  # list of [out_frame, src_frame] knots (0-based, src frames 0-based)
NOUT = int(tw[-1][0]) + 1
def src_time(i):
    for (o0, s0), (o1, s1) in zip(tw, tw[1:]):
        if o0 <= i <= o1:
            u = (i - o0) / (o1 - o0) if o1 > o0 else 0
            if args.get('smooth', True):
                pass
            return s0 + (s1 - s0) * u
    return tw[-1][1]

order = []
def walk(b):
    order.append(b.name)
    for c in b.children: walk(c)
for b in TB:
    if b.parent is None: walk(b)
lrest = {b.name: (b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy() for b in TB}

def sample_src(t):
    f = sf0 + t
    fi = int(math.floor(f)); sc.frame_set(fi, subframe=f - fi)
    return {n: smw @ pb.matrix for n, pb in src.pose.bones.items()}

# ---- yaw normalisation (pelvis yaw held near a constant; keeps a fraction of the sway)
def yaw_of(v): return math.atan2(v.y, v.x)
def blend_pose(A_, B_, w):
    out = {}
    for n in A_:
        la, qa, _ = A_[n].decompose(); lb, qb, _ = B_[n].decompose()
        out[n] = Matrix.Translation(la.lerp(lb, w)) @ qa.slerp(qb, w).to_matrix().to_4x4()
    return out
pb_ = args.get('pre_blend')  # {'from': src frame, 'to': src frame, 'frames': n}
if pb_:
    PA = sample_src(pb_['from']); PB = sample_src(pb_['to'])
samples = []
for i in range(NOUT):
    if pb_ and i <= pb_['frames']:
        u = i / pb_['frames']; u = u*u*(3-2*u)
        samples.append(blend_pose(PA, PB, u))
    else:
        samples.append(sample_src(src_time(i)))
pel_yaw = [yaw_of(S['L_Hip'].translation - S['R_Hip'].translation) for S in samples]
# unwrap
for i in range(1, len(pel_yaw)):
    while pel_yaw[i] - pel_yaw[i-1] > math.pi: pel_yaw[i] -= 2*math.pi
    while pel_yaw[i] - pel_yaw[i-1] < -math.pi: pel_yaw[i] += 2*math.pi
REL = int(args.get('release_out', 15))
keep = args.get('yaw_keep', 0.3)
meany = sum(pel_yaw)/len(pel_yaw)
face = args.get('face', 'pelvis')
S = samples[REL]
if face == 'forearm_r':   # gun arm horizontal direction points forward (-Y) at release
    v = S['R_Wrist'].translation - S['R_Elbow'].translation; ref = math.atan2(v.y, v.x) - (-math.pi/2)
elif face == 'shoulders':
    v = S['L_Shoulder'].translation - S['R_Shoulder'].translation; ref = yaw_of(v)
else:
    ref = pel_yaw[REL]
ref += math.radians(args.get('face_offset', 0.0))
corr = []
YB = int(args.get('yaw_blend_in', 0))
for i in range(NOUT):
    y = pel_yaw[i]
    # the corrected pelvis yaw: (ref offset) + keep * (y - y_rel)
    want = (pel_yaw[REL] - ref) + keep * (y - pel_yaw[REL])
    if YB and i < YB:
        u = i / YB; u = u*u*(3-2*u)
        want = (1-u) * 0.0 + u * want
    corr.append(want - y)
def yawfix(Sd, c):
    piv = Sd['Pelvis'].translation.copy(); piv.z = 0
    R = Matrix.Translation(piv) @ Matrix.Rotation(c, 4, 'Z') @ Matrix.Translation(-piv)
    return {n: R @ m for n, m in Sd.items()}
samples = [yawfix(Sd, c) for Sd, c in zip(samples, corr)]
# ---- pass 1: world (armature-space == world; tgt matrix_world is identity-ish) target rotations + pelvis pos
frames = []
for i in range(NOUT):
    S = samples[i]
    rot = {}
    for t, s in MAP.items():
        qs = rot3(S[s]); qs0 = rot3(s_rest[s])
        rot[t] = qs @ qs0.inverted() @ A[t] @ rot3(t_rest[t])
    pel = S['Pelvis'].translation * s_scale
    frames.append({'rot': rot, 'pel': pel})
def fk(fr, basis_override=None):
    """compute world matrices for every target bone; returns dict"""
    W = {}
    for n in order:
        b = TB[n]
        if b.parent is None:
            base = tmw @ lrest[n]
        else:
            base = W[b.parent.name] @ lrest[n]
        if n in fr.get('basis', {}):
            W[n] = base @ fr['basis'][n]; continue
        if n in fr['rot']:
            q = fr['rot'][n]
            pos = fr['pel'] if n == 'pelvis' else base.translation
            W[n] = Matrix.Translation(pos) @ q.to_matrix().to_4x4()
        else:
            W[n] = base
        # store basis
        fr.setdefault('basis', {})[n] = base.inverted() @ W[n]
    return W

# ---- procedural recoil (authored on top): gun arm kicks up, chest rocks back, at the release frame
rc = args.get('recoil')
if rc:
    SUB_SP = ['spine_05','neck_01','head','clavicle_l','upperarm_l','lowerarm_l','hand_l','clavicle_r','upperarm_r','lowerarm_r','hand_r']
    SUB_ARM = ['upperarm_r','lowerarm_r','hand_r']
    for i, fr in enumerate(frames):
        k = i - REL
        if k < 0: continue
        if k <= rc.get('rise', 2): e = k / rc.get('rise', 2)
        else:
            u = min(1.0, (k - rc.get('rise', 2)) / rc.get('decay', 12)); e = (1 - u)**2
        if e <= 0: continue
        fa = (frames[i]['rot']['lowerarm_r'] @ Vector((0,1,0)))
        # horizontal forearm direction from the target rest-to-pose (bone Y axis may not be along the limb: use joints after fk)
        fr['basis'] = {}
        W = fk(fr)
        f = W['hand_r'].translation - W['lowerarm_r'].translation; f.z = 0
        if f.length < 1e-3: f = Vector((0,-1,0))
        f.normalize(); ax = f.cross(Vector((0,0,1))).normalized()
        q_sp = Quaternion(ax, math.radians(rc.get('spine', 7)) * e)
        q_arm = Quaternion(ax, math.radians(rc.get('arm', 20)) * e)
        for n in SUB_SP: fr['rot'][n] = q_sp @ fr['rot'][n]
        for n in SUB_ARM: fr['rot'][n] = q_arm @ fr['rot'][n]
        fr['pel'] = fr['pel'] + Vector((f.x, f.y, 0)) * (-rc.get('push', 3.0) * e)

# ---- aim level: bring the gun arm to a set pitch around the release (an aim offset baked in)
ap = args.get('aim_pitch')
if ap is not None:
    fr = frames[REL]; fr['basis'] = {}; W = fk(fr)
    f = W['hand_r'].translation - W['lowerarm_r'].translation; fl = Vector((f.x, f.y, 0)).length
    pitch_rel = math.degrees(math.atan2(f.z, fl))
    delta = ap - pitch_rel
    a0, a1, a2, a3 = args.get('aim_env', [0, REL-3, REL+10, NOUT-1])
    for i, fr in enumerate(frames):
        if i <= a0 or i >= a3: e = 0.0
        elif i < a1: u = (i-a0)/(a1-a0); e = u*u*(3-2*u)
        elif i <= a2: e = 1.0
        else: u = (i-a2)/(a3-a2); e = 1-u*u*(3-2*u)
        if e <= 0: continue
        fr['basis'] = {}; W = fk(fr)
        f = W['hand_r'].translation - W['lowerarm_r'].translation; f.z = 0
        if f.length < 1e-3: continue
        f.normalize(); ax = f.cross(Vector((0,0,1))).normalized()
        q = Quaternion(ax, math.radians(delta) * e)
        for n in ['upperarm_r','lowerarm_r','hand_r']: fr['rot'][n] = q @ fr['rot'][n]
    metrics_aim = {'pitch_raw_at_release': pitch_rel, 'set_to': ap}
# Fist: curl finger joints (target local), only l hand (r hand sits in the gun cuff)
FIST = args.get('fist', 75.0)
finger_ch = [(f, side) for f in ('index','middle','ring','pinky') for side in ('l','r')]
# curl axis per finger chain: perpendicular to finger direction and the knuckle line, sign chosen so tip moves toward palm (toward thumb side x down)
def curl_axis(side, f):
    hb = f'hand_{side}'
    d = (thead(f'{f}_02_{side}') - thead(f'{f}_01_{side}')).normalized()
    across = (thead(f'pinky_01_{side}') - thead(f'index_01_{side}')).normalized()
    ax = d.cross(across).normalized()
    return ax

# pass 1b: compute basis via fk once
Ws = []
for fr in frames:
    fr['basis'] = {}
    Ws.append(fk(fr))

# ---- placement: shift so frame-0 ankle midpoint = rest ankle midpoint XY; vertical so median min ankle Z = rest
restA = {s: thead(f'foot_{s}') for s in 'lr'}
mid_rest = (restA['l'] + restA['r']) / 2
mid0 = (Ws[0]['foot_l'].translation + Ws[0]['foot_r'].translation) / 2
minz = sorted(min(W['foot_l'].translation.z, W['foot_r'].translation.z) for W in Ws)
DZX = args.get('dz_extra', 0.0)
dz = restA['l'].z + DZX - minz[len(minz)//2]
shift = Vector((mid_rest.x - mid0.x, mid_rest.y - mid0.y, dz))
for fr in frames:
    fr['pel'] = fr['pel'] + shift
    fr['basis'] = {}
Ws = [fk(fr) for fr in frames]
# ---- in place: cancel the whole-body drift (smoothed feet midpoint), keep sway relative to the feet
if args.get('drift_fix', True):
    mids = [ (W['foot_l'].translation + W['foot_r'].translation)/2 for W in Ws ]
    k = args.get('drift_smooth', 5)
    sm = []
    for i in range(len(mids)):
        a, b = max(0, i-k), min(len(mids), i+k+1)
        v = sum((mids[j] for j in range(a, b)), Vector()) / (b - a); sm.append(v)
    for i, fr in enumerate(frames):
        d = sm[0] - sm[i]; d.z = 0
        fr['pel'] = fr['pel'] + d; fr['basis'] = {}
    Ws = [fk(fr) for fr in frames]

# ---- foot contacts and two-bone IK lock
legs = {'l': ('thigh_l','calf_l','foot_l'), 'r': ('thigh_r','calf_r','foot_r')}
lock_info = {}
for side, (th, ca, fo) in legs.items():
    P = [W[fo].translation.copy() for W in Ws]
    zmin = min(p.z for p in P)
    sp = [0.0] + [ (P[i]-P[i-1]).length * FPS for i in range(1, len(P)) ]
    contact = [ (P[i].z < zmin + args.get('contact_dz', 8.0)) and (sp[i] < args.get('contact_speed', 60.0)) for i in range(len(P)) ]
    if args.get('force_plant', {}).get(side):
        contact = [True]*len(P)
    if args.get('plant') and side in args['plant']:
        contact = [False]*len(P)
        for a_, b_ in args['plant'][side]:
            for k in range(a_, min(b_, len(P)-1)+1): contact[k] = True
    # segments
    segs = []; i = 0
    while i < len(P):
        if contact[i]:
            j = i
            while j+1 < len(P) and contact[j+1]: j += 1
            if j - i + 1 >= 3: segs.append((i, j))
            i = j + 1
        else: i += 1
    lock_info[side] = segs
    target = [None]*len(P); trot = [None]*len(P); wgt = [0.0]*len(P)
    for (a, b) in segs:
        mid = (a + b)//2
        pin = sum((P[k] for k in range(a, b+1)), Vector()) / (b - a + 1)
        pin.z = max(pin.z, restA[side].z + DZX)
        if args.get('pin_first', True): pin = Vector((P[a].x, P[a].y, max(P[a].z, restA[side].z + DZX)))
        prot = Ws[mid][fo].to_quaternion()
        for k in range(a, b+1):
            target[k] = pin; trot[k] = prot; wgt[k] = 1.0
        for e in range(1, 4):  # blend out edges
            for k, w in ((a - e, 1 - e/4), (b + e, 1 - e/4)):
                if 0 <= k < len(P) and wgt[k] < w:
                    target[k] = pin; trot[k] = prot; wgt[k] = w
    # clean steps between plants: arc from one pin to the next (replaces source foot drags)
    if args.get('clean_steps', True) and len(segs) > 1:
        pins = []
        for (a, b) in segs: pins.append((a, b, target[a], trot[a]))
        for (a0_, b0_, p0, r0), (a1_, b1_, p1, r1) in zip(pins, pins[1:]):
            n_ = a1_ - b0_
            for k in range(b0_+1, a1_):
                u = (k - b0_) / n_; us = u*u*(3-2*u)
                t = p0.lerp(p1, us); t = t + Vector((0, 0, args.get('step_lift', 10.0) * math.sin(math.pi*u)))
                target[k] = t; trot[k] = r0.slerp(r1, us); wgt[k] = 1.0
    for k, fr in enumerate(frames):
        if wgt[k] <= 0: continue
        W = fk(fr)  # current
        H = W[th].translation; K = W[ca].translation; Acur = W[fo].translation
        T = Acur.lerp(target[k], wgt[k])
        L1 = (K - H).length; L2 = (Acur - K).length
        dvec = T - H; dl = min(dvec.length, (L1 + L2) * 0.999); ax = dvec.normalized()
        T = H + ax * dl
        a_ = (L1*L1 - L2*L2 + dl*dl) / (2*dl); h_ = math.sqrt(max(L1*L1 - a_*a_, 0))
        pole = (K - H) - ax * (K - H).dot(ax)
        if pole.length < 1e-4: pole = Vector((0, -1, 0))
        Kn = H + ax * a_ + pole.normalized() * h_
        q1 = (K - H).normalized().rotation_difference((Kn - H).normalized())
        Wth = Matrix.Translation(H) @ (q1 @ W[th].to_quaternion()).to_matrix().to_4x4()
        # calf: after thigh rotation, its dir = q1*(A-K); rotate to (T-Kn)
        cdir = q1 @ (Acur - K)
        q2 = cdir.normalized().rotation_difference((T - Kn).normalized())
        Wca = Matrix.Translation(Kn) @ (q2 @ q1 @ W[ca].to_quaternion()).to_matrix().to_4x4()
        qf = W[fo].to_quaternion().slerp(trot[k], wgt[k])
        Wfo = Matrix.Translation(T) @ qf.to_matrix().to_4x4()
        # write basis
        pth = W[TB[th].parent.name]
        fr['basis'][th] = (pth @ lrest[th]).inverted() @ Wth
        fr['basis'][ca] = (Wth @ lrest[ca]).inverted() @ Wca
        fr['basis'][fo] = (Wca @ lrest[fo]).inverted() @ Wfo

# ---- fist on both hands (finger local curl about computed axis, in bone-local space)
for fr in frames:
    W = fk(fr)
    for side in 'lr':
        for f in ('index','middle','ring','pinky','thumb'):
            for j, amt in ((1, 0.9), (2, 1.0), (3, 0.8)):
                n = f'{f}_0{j}_{side}'
                if n not in TB: continue
                ang = math.radians(FIST * amt * (0.5 if f == 'thumb' else 1.0))
                axw = curl_axis(side, 'index' if f == 'thumb' else f) * args.get('curl_sign', 1.0)
                # world-space rotation about axw at the joint, converted to basis
                parent = TB[n].parent.name
                base = W[parent] @ lrest[n]
                cur = W[n]
                qw = Quaternion(axw, ang)
                # axis is defined in rest; carry it with the hand's current rotation
                hq = W[f'hand_{side}'].to_quaternion() @ t_rest[f'hand_{side}'].to_quaternion().inverted()
                qw = Quaternion(hq @ axw, ang)
                neww = Matrix.Translation(cur.translation) @ (qw @ cur.to_quaternion()).to_matrix().to_4x4()
                fr['basis'][n] = base.inverted() @ neww
                W = fk(fr)
# ---- ik bones follow
IKF = {'ik_foot_l':'foot_l','ik_foot_r':'foot_r','ik_hand_gun':'hand_r','ik_hand_l':'hand_l','ik_hand_r':'hand_r'}

# ---- write action
for o in srcobjs: bpy.data.objects.remove(o, do_unlink=True)
tgt.animation_data_create()
act = bpy.data.actions.new(args['name']); tgt.animation_data.action = act
for pb in tgt.pose.bones: pb.rotation_mode = 'QUATERNION'
sc.frame_start = 0; sc.frame_end = NOUT - 1
metrics = {'frames': NOUT, 'fps': FPS, 'scale': s_scale, 'contacts': lock_info}
foot_track = {'l': [], 'r': []}
for i, fr in enumerate(frames):
    W = fk(fr)
    for ikn, srcn in IKF.items():
        if ikn in TB:
            b = TB[ikn]; base = W[b.parent.name] @ lrest[ikn]
            want = W[srcn] @ (t_rest[srcn].inverted() @ t_rest[ikn])
            fr['basis'][ikn] = base.inverted() @ want
    for n in order:
        pb = tgt.pose.bones[n]; m = fr['basis'][n]
        loc, q, s = m.decompose()
        pb.location = loc if n == 'pelvis' or n.startswith('ik_') else Vector()
        pb.rotation_quaternion = q; pb.scale = Vector((1,1,1))
        pb.keyframe_insert('location', frame=i); pb.keyframe_insert('rotation_quaternion', frame=i)
    for s in 'lr': foot_track[s].append(list(W[f'foot_{s}'].translation))
metrics['foot_track'] = foot_track
# quaternion continuity
for fc in act.fcurves:
    for kp in fc.keyframe_points: kp.interpolation = 'LINEAR'
json.dump(metrics, open(OUT + '.json', 'w'))
if BLEND: bpy.ops.wm.save_as_mainfile(filepath=BLEND)
# export: armature + mesh, same settings as the SK_Warden build
for o in bpy.data.objects: o.select_set(False)
tgt.select_set(True); body.select_set(True); bpy.context.view_layer.objects.active = tgt
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={'ARMATURE','MESH'}, add_leaf_bones=False,
    bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True,
    bake_anim_simplify_factor=0.0, bake_anim_step=1.0, mesh_smooth_type='FACE', use_tspace=True, use_armature_deform_only=False,
    primary_bone_axis='Y', secondary_bone_axis='X', axis_forward='-Z', axis_up='Y', apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE', global_scale=1.0, use_triangles=True)
print('RT_OK', OUT, NOUT, s_scale, lock_info)
