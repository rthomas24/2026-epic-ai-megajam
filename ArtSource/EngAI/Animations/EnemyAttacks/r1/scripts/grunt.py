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

def yaw_of(v): return math.atan2(v.y, v.x)
def blend_pose(A_, B_, w):
    out = {}
    for n in A_:
        la, qa, _ = A_[n].decompose(); lb, qb, _ = B_[n].decompose()
        out[n] = Matrix.Translation(la.lerp(lb, w)) @ qa.slerp(qb, w).to_matrix().to_4x4()
    return out
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
# ==== Grunt charge: cyclic upper body from the source + procedural grounded legs ====
N = int(args.get('N', 16)); A0 = float(args['a']); CYC = float(args['cyc']); KS = CYC / N
SPAN = float(args.get('span', 86.0)); TD = {'l': float(args.get('td_l', 0.0)), 'r': float(args.get('td_r', 8.0))}
STANCE = float(args.get('stance', 8.5)); LIFT = float(args.get('lift', 12.0))
V = SPAN * FPS / STANCE                     # cm/s, ground speed implied by a world-locked stance foot
def center(Sd):
    p = Sd['Pelvis'].translation.copy(); p.z = 0
    T = Matrix.Translation(-p); return {n: T @ m for n, m in Sd.items()}
# travel direction over the window -> rotate onto -Y
Sa = sample_src(A0 - CYC); Sb = sample_src(A0 + CYC)
trav = Sb['Pelvis'].translation - Sa['Pelvis'].translation; trav.z = 0
rotz = (-math.pi/2) - math.atan2(trav.y, trav.x)
RZ = Matrix.Rotation(rotz, 4, 'Z')
samples = []
for i in range(N):
    w = i / N
    P1 = center(sample_src(A0 + i*KS)); P2 = center(sample_src(A0 - CYC + i*KS))
    B = blend_pose(P1, P2, w)
    samples.append({n: RZ @ m for n, m in B.items()})
frames = []
for i in range(N):
    S = samples[i]; rot = {}
    for t, s in MAP.items():
        rot[t] = rot3(S[s]) @ rot3(s_rest[s]).inverted() @ A[t] @ rot3(t_rest[t])
    frames.append({'rot': rot, 'pel': S['Pelvis'].translation * s_scale})
# ---- authored shoulder drop (right shoulder low and forward) + optional extra lean
SUB3 = ['spine_03','spine_05','neck_01','head','clavicle_l','upperarm_l','lowerarm_l','hand_l','clavicle_r','upperarm_r','lowerarm_r','hand_r']
SUB5 = SUB3[1:]
HEAD = ['neck_01','head']
fwd = Vector((0,-1,0))
roll = math.radians(args.get('roll', 10.0)); twist = math.radians(args.get('twist', 10.0)); lean = math.radians(args.get('lean', 0.0))
q_roll3 = Quaternion(fwd, roll*0.5); q_roll5 = Quaternion(fwd, roll*0.5)
q_tw = Quaternion(Vector((0,0,1)), twist)
q_lean = Quaternion(Vector((1,0,0)), -lean)   # tips the torso forward (toward -Y)
for fr in frames:
    R = fr['rot']
    for n in SUB3: R[n] = q_lean @ q_tw @ q_roll3 @ R[n]
    for n in SUB5: R[n] = q_roll5 @ R[n]
    qh = Quaternion(fwd, -roll*0.6)
    for n in HEAD: R[n] = qh @ R[n]
# ---- fist
FIST = args.get('fist', 70.0)
def curl_axis(side, f):
    d = (thead(f'{f}_02_{side}') - thead(f'{f}_01_{side}')).normalized()
    across = (thead(f'pinky_01_{side}') - thead(f'index_01_{side}')).normalized()
    return d.cross(across).normalized()
# ---- pelvis: source height (scaled) + small authored sway; centred XY
for fr in frames: fr['basis'] = {}
Ws = [fk(fr) for fr in frames]
restA = {s: thead(f'foot_{s}') for s in 'lr'}; restB = {s: thead(f'ball_{s}') for s in 'lr'}
FLOOR_A = {s: restA[s].z for s in 'lr'}
off_ab = {s: restA[s] - restB[s] for s in 'lr'}      # ankle relative to ball at rest
TRACK = {s: restB[s].x * args.get('track', 0.8) for s in 'lr'}
YOFF = args.get('yoff', -4.0)                        # stance centre (cm, -Y = forward)
def phase_pos(s, t):
    """ball position + foot pitch (deg, + = heel up) for side s at continuous frame t (in place coords)"""
    u = (t - TD[s]) % N
    if u <= STANCE:
        y = YOFF - SPAN/2 + V * u / FPS; z = restB[s].z
        k = u / STANCE; th = 0.0
        if k > 0.7: th = 28.0 * ((k - 0.7) / 0.3) ** 1.5     # heel-off, pivot on the ball
        return Vector((TRACK[s], y, z)), th, True
    sw = N - STANCE; g = (u - STANCE) / sw
    e = g*g*(3-2*g)
    y = YOFF + SPAN/2 - SPAN * e
    z = restB[s].z + LIFT * math.sin(math.pi * g) ** 1.2
    th = 28.0 * (1 - g) ** 2 - 6.0 * math.sin(math.pi * min(1, g * 1.15)) * g
    return Vector((TRACK[s], y, z)), th, False
pitch_axis = Vector((1,0,0))
def ankle_from(s, ball, th):
    q = Quaternion(pitch_axis, math.radians(th))
    return ball + q @ off_ab[s], q
L1 = {s: (thead(f'calf_{s}') - thead(f'thigh_{s}')).length for s in 'lr'}
L2 = {s: (thead(f'foot_{s}') - thead(f'calf_{s}')).length for s in 'lr'}
# pelvis height so the legs reach: lower where needed (max 96% extension), then cyclic smoothing
pz = []
src_pz = [fr['pel'].z for fr in frames]
PZ_OFF = args.get('pz_off', 0.0)
for i, fr in enumerate(frames):
    fr['pel'] = Vector((args.get('sway', 1.5) * math.sin(2*math.pi*(i - TD['l'])/N), 0.0, src_pz[i] + PZ_OFF))
    fr['basis'] = {}; W = fk(fr)
    need = fr['pel'].z
    for s in 'lr':
        b, th, _ = phase_pos(s, i); a_, _ = ankle_from(s, b, th)
        H = W[f'thigh_{s}'].translation; d = (a_ - H); lim = 0.96 * (L1[s] + L2[s])
        if d.length > lim:
            horiz = Vector((d.x, d.y, 0)).length
            dz_ok = math.sqrt(max(lim*lim - horiz*horiz, 0))
            need = min(need, fr['pel'].z - ((H.z - a_.z) - dz_ok))
    pz.append(need)
sm = [sum(pz[(i+j) % N] for j in (-2,-1,0,1,2)) / 5 for i in range(N)]
sm = [min(a, b + 0.5) for a, b in zip(sm, pz)]   # never above what the reach allows
for i, fr in enumerate(frames): fr['pel'].z = sm[i]
# ---- legs: two-bone IK onto the procedural ankle targets
contact_log = {'l': [], 'r': []}
for i, fr in enumerate(frames):
    fr['basis'] = {}; W = fk(fr)
    for s in 'lr':
        th_, ca_, fo_ = f'thigh_{s}', f'calf_{s}', f'foot_{s}'
        b, th, inst = phase_pos(s, i); T, qp = ankle_from(s, b, th)
        contact_log[s].append(bool(inst))
        H = W[th_].translation; K = W[ca_].translation; Acur = W[fo_].translation
        l1 = (K - H).length; l2 = (Acur - K).length
        dvec = T - H; dl = min(dvec.length, (l1 + l2) * 0.999); ax = dvec.normalized(); T2 = H + ax * dl
        a_ = (l1*l1 - l2*l2 + dl*dl) / (2*dl); h_ = math.sqrt(max(l1*l1 - a_*a_, 0))
        pole = Vector((0, -1, 0.15)); pole = pole - ax * pole.dot(ax)
        Kn = H + ax * a_ + pole.normalized() * h_
        q1 = (K - H).normalized().rotation_difference((Kn - H).normalized())
        Wth = Matrix.Translation(H) @ (q1 @ W[th_].to_quaternion()).to_matrix().to_4x4()
        cdir = q1 @ (Acur - K)
        q2 = cdir.normalized().rotation_difference((T2 - Kn).normalized())
        Wca = Matrix.Translation(Kn) @ (q2 @ q1 @ W[ca_].to_quaternion()).to_matrix().to_4x4()
        qf = qp @ t_rest[fo_].to_quaternion()
        Wfo = Matrix.Translation(T2) @ qf.to_matrix().to_4x4()
        pth = W[TB[th_].parent.name]
        fr['basis'][th_] = (pth @ lrest[th_]).inverted() @ Wth
        fr['basis'][ca_] = (Wth @ lrest[ca_]).inverted() @ Wca
        fr['basis'][fo_] = (Wca @ lrest[fo_]).inverted() @ Wfo
        bb = f'ball_{s}'
        Wb0 = Wfo @ lrest[bb]
        flat = 1.0 if inst else 0.5
        qb = Wb0.to_quaternion().slerp(t_rest[bb].to_quaternion(), flat)
        fr['basis'][bb] = Wb0.inverted() @ (Matrix.Translation(Wb0.translation) @ qb.to_matrix().to_4x4())
# wrap: append frame N == frame 0 (closed loop like the trooper loops)
import copy
frames.append({'rot': dict(frames[0]['rot']), 'pel': frames[0]['pel'].copy(), 'basis': dict(frames[0]['basis'])})
NOUT = N + 1
lock_info = {'V_cm_s': V, 'N': N, 'stance': STANCE, 'span': SPAN, 'contacts': {s: ''.join('1' if c else '0' for c in contact_log[s]) for s in 'lr'}}

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
# ---- hinge fix: elbows/knees bend as hinges (no roll between the upper and lower limb); the forearm roll is
#      spread over the lower-arm twist bones and the wrist, so the elbow skin does not candy-wrap.
def _twist_angle(q, axis):
    v = Vector((q.x, q.y, q.z)); p = axis * v.dot(axis)
    t = Quaternion((q.w, p.x, p.y, p.z));
    if t.magnitude < 1e-8: return 0.0
    t.normalize(); a = 2*math.atan2(Vector((t.x,t.y,t.z)).dot(axis), t.w)
    return (a + math.pi) % (2*math.pi) - math.pi
HINGES = [('upperarm_l','lowerarm_l','hand_l',['lowerarm_twist_01_l','lowerarm_twist_02_l']),
          ('upperarm_r','lowerarm_r','hand_r',['lowerarm_twist_01_r','lowerarm_twist_02_r'])]
if args.get('hinge_legs', True):
    HINGES += [('thigh_l','calf_l','foot_l',[]),('thigh_r','calf_r','foot_r',[])]
if args.get('hinge', True):
    hinge_log = []
    for fr in frames:
        W = fk(fr)
        for up, lo, end, tws in HINGES:
            if lo not in TB or end not in TB: continue
            Wlo_old = W[lo]; Wend_old = W[end]
            base = W[up] @ lrest[lo]                       # lower limb at rest relative to the current upper limb
            d_rest = (base @ lrest[end]).translation - base.translation
            d_now = Wend_old.translation - Wlo_old.translation
            sw = d_rest.normalized().rotation_difference(d_now.normalized())
            q_new = sw @ base.to_quaternion()
            Wlo_new = Matrix.Translation(base.translation) @ q_new.to_matrix().to_4x4()
            fr['basis'][lo] = base.inverted() @ Wlo_new
            # remaining roll (old vs new about the limb axis)
            qd = Wlo_old.to_quaternion() @ q_new.inverted()
            ax = d_now.normalized(); ang = _twist_angle(qd, ax)
            hinge_log.append(abs(math.degrees(ang)))
            for k, tw in enumerate(tws):
                if tw not in TB: continue
                frac = (k + 1) / (len(tws) + 1)
                qt = Quaternion(ax, ang * frac) @ q_new
                Wtw_base = Wlo_new @ lrest[tw]
                fr['basis'][tw] = Wtw_base.inverted() @ (Matrix.Translation(Wtw_base.translation) @ (qt @ q_new.inverted() @ Wtw_base.to_quaternion()).to_matrix().to_4x4())
            # keep the end bone's world rotation (hand / foot) exactly as before
            We_base = Wlo_new @ lrest[end]
            fr['basis'][end] = We_base.inverted() @ (Matrix.Translation(We_base.translation) @ Wend_old.to_quaternion().to_matrix().to_4x4())
            W = fk(fr)
    print('HINGE max roll moved %.0f deg' % (max(hinge_log) if hinge_log else 0))

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
# ---- root-motion variant: the same loop with the travel on the armature object (= root bone in UE)
RM = args.get('out_rm')
if RM:
    for i in range(NOUT):
        tgt.location = Vector((0, -V * i / FPS, 0)); tgt.keyframe_insert('location', frame=i)
    for fc in tgt.animation_data.action.fcurves:
        for kp in fc.keyframe_points: kp.interpolation = 'LINEAR'
    bpy.ops.export_scene.fbx(filepath=RM, use_selection=True, object_types={'ARMATURE','MESH'}, add_leaf_bones=False,
        bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0.0, bake_anim_step=1.0, mesh_smooth_type='FACE', use_tspace=True, use_armature_deform_only=False,
        primary_bone_axis='Y', secondary_bone_axis='X', axis_forward='-Z', axis_up='Y', apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE', global_scale=1.0, use_triangles=True)
    print('RM_OK', RM, V)
