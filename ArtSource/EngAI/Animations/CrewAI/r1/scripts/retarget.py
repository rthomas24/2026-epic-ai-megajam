# SMPL-H (HY-Motion) -> SKEL_NullCrew / SKEL_Warden (UE5 Manny names) death-clip retarget + cleanup, Blender 4.5.3 headless.
# Based on ArtSource/EngAI/Animations/CrewTrooper/r1/scripts/retarget.py (PR #86); additions for the enemy deaths r1:
# target mesh per character, playback speed (weight), travel scale, yaw ramp during the fall, end hold (no leg kicks),
# floor clamp (no part of the mesh more than FLOOR_CLAMP below Z=0), UnitScaleFactor 1.0 export option.
import bpy, sys, math, json, os
from mathutils import Vector, Quaternion, Matrix
args = sys.argv[sys.argv.index('--')+1:]
SRC, OUTFBX, CFG = args[0], args[1], json.loads(args[2])
TGT = CFG['tgt']; TMESH = CFG['tmesh']
LOOP = CFG.get('loop', False); ROOTMO = CFG.get('rootmotion', False); GRIP = CFG.get('grip', 1.0)
TRIM = CFG.get('trim')  # [first,last] 0-based source frames to keep
FPS = 30
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene; sc.render.fps = FPS
# ---------- target ----------
bpy.ops.import_scene.fbx(filepath=TGT)
tgt = bpy.data.objects['root']; tmesh = bpy.data.objects[TMESH]
for o in list(bpy.data.objects):
    if o.name.startswith('SOCKET_'): bpy.data.objects.remove(o)
mw = tmesh.matrix_world.copy(); tmesh.parent = None; tmesh.matrix_world = mw
bpy.ops.object.select_all(action='DESELECT'); tgt.select_set(True); tmesh.select_set(True)
bpy.context.view_layer.objects.active = tgt
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
tmesh.parent = tgt; tmesh.matrix_parent_inverse = Matrix()
for m in tmesh.modifiers:
    if m.type == 'ARMATURE': m.object = tgt
# ---------- source ----------
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=SRC)
newobjs = set(bpy.data.objects) - before
src = [o for o in newobjs if o.type == 'ARMATURE'][0]
for o in newobjs:
    if o.type == 'MESH': o.hide_render = True; o.hide_viewport = True
act = src.animation_data.action
F0, F1 = [int(round(x)) for x in act.frame_range]
SPEED = CFG.get('speed', 1.0)
if TRIM: F0, F1 = F0 + TRIM[0], F0 + TRIM[1]
frames = []
t = float(F0)
while t <= F1 + 1e-6:
    frames.append(t); t += SPEED
N = len(frames)

MAP = {'Pelvis':'pelvis','Spine1':'spine_02','Spine2':'spine_03','Spine3':'spine_05','Neck':'neck_01','Head':'head',
 'L_Collar':'clavicle_l','L_Shoulder':'upperarm_l','L_Elbow':'lowerarm_l','L_Wrist':'hand_l',
 'R_Collar':'clavicle_r','R_Shoulder':'upperarm_r','R_Elbow':'lowerarm_r','R_Wrist':'hand_r',
 'L_Hip':'thigh_l','L_Knee':'calf_l','L_Ankle':'foot_l','L_Foot':'ball_l',
 'R_Hip':'thigh_r','R_Knee':'calf_r','R_Ankle':'foot_r','R_Foot':'ball_r'}
# direction child pairs (source child, target child)
DIRC = {'Pelvis':('Spine1','spine_01'),'Spine1':('Spine2','spine_03'),'Spine2':('Spine3','spine_04'),'Spine3':('Neck','neck_01'),
 'Neck':('Head','head'),'L_Collar':('L_Shoulder','upperarm_l'),'L_Shoulder':('L_Elbow','lowerarm_l'),'L_Elbow':('L_Wrist','hand_l'),
 'L_Wrist':('L_Middle1','middle_01_l'),'R_Collar':('R_Shoulder','upperarm_r'),'R_Shoulder':('R_Elbow','lowerarm_r'),
 'R_Elbow':('R_Wrist','hand_r'),'R_Wrist':('R_Middle1','middle_01_r'),'L_Hip':('L_Knee','calf_l'),'L_Knee':('L_Ankle','foot_l'),
 'L_Ankle':('L_Foot','ball_l'),'R_Hip':('R_Knee','calf_r'),'R_Knee':('R_Ankle','foot_r'),'R_Ankle':('R_Foot','ball_r')}

def wq(M): return M.to_3x3().normalized().to_quaternion()
TB = tgt.data.bones
order = []  # parents first
def walk(b):
    order.append(b.name)
    for c in b.children: walk(c)
for b in TB:
    if b.parent is None: walk(b)
restW = {b.name: (tgt.matrix_world @ b.matrix_local) for b in TB}  # unit scale after apply
restQ = {n: wq(M) for n, M in restW.items()}
restH = {n: M.to_translation() for n, M in restW.items()}
relRest = {}
for b in TB:
    if b.parent: relRest[b.name] = restW[b.parent.name].inverted() @ restW[b.name]
SB = src.data.bones
srestW = {b.name: src.matrix_world @ b.matrix_local for b in SB}
srestQ = {n: wq(M) for n, M in srestW.items()}
srestH = {n: M.to_translation() for n, M in srestW.items()}
OFF = {}
for s, t in MAP.items():
    if s in DIRC:
        sc_, tc = DIRC[s]
        ds = srestH[sc_] - srestH[s]; dt = restH[tc] - restH[t]
        A = dt.rotation_difference(ds)
    else:
        A = Quaternion()
    OFF[t] = (s, srestQ[s].inverted() @ A @ restQ[t])
# leg scale
k = ((restH['thigh_l']-restH['calf_l']).length + (restH['calf_l']-restH['foot_l']).length) / \
    ((srestH['L_Hip']-srestH['L_Knee']).length + (srestH['L_Knee']-srestH['L_Ankle']).length)
# ---------- finger grip (rest-space curl) ----------
FING = {}
def chain(side):
    return {f: [f'{f}_0{i}_{side}' for i in (1,2,3)] for f in ('index','middle','ring','pinky','thumb')}
for side in ('l','r'):
    ch = chain(side)
    m = ch['middle']
    d = restH[m[1]] - restH[m[0]]
    tip = restH[m[2]] - restH[m[0]]
    n = (tip - d.normalized()*tip.dot(d.normalized())).normalized()
    for f, bones in ch.items():
        if f == 'thumb':
            ang = [15, 25, 20]
        elif f == 'index' and side == 'r' and not CFG.get('fist'):
            ang = [20, 35, 20]   # trigger finger
        else:
            ang = [60, 75, 45]
        dd = restH[bones[1]] - restH[bones[0]]
        ax = dd.cross(n).normalized()
        cum = 0
        for b, a in zip(bones, ang):
            cum += math.radians(a) * GRIP
            FING[b] = (side, Quaternion(ax, cum) @ restQ[b])
HAND = {'l': 'hand_l', 'r': 'hand_r'}
# ---------- sample source ----------
S = []
for f in frames:
    sc.frame_set(int(math.floor(f)), subframe=f-math.floor(f))
    S.append({b.name: (src.matrix_world @ src.pose.bones[b.name].matrix) for b in SB})
p0 = S[0]['Pelvis'].to_translation()
def desired(i):
    rot = {}
    for t, (s, O) in OFF.items():
        rot[t] = wq(S[i][s]) @ O
    p = (S[i]['Pelvis'].to_translation() - Vector((p0.x, p0.y, 0))) * k
    p.x *= TRAVEL; p.y *= TRAVEL
    if YAW:
        a_, b_, deg = YAW
        t_ = 0.0 if i <= a_ else 1.0 if i >= b_ else (i-a_)/(b_-a_)
        t_ = t_*t_*(3-2*t_)
        q = Quaternion(Vector((0,0,1)), math.radians(deg*t_))
        rot = {n: q @ r for n, r in rot.items()}
        p = q @ p
    return rot, p
TRAVEL = CFG.get('travel', 1.0); YAW = CFG.get('yaw')  # yaw = [from_frame, to_frame, degrees] (output frames, 0-based)
D = [desired(i) for i in range(N)]
# end hold: from frame h the body eases (over HB frames) into the frame-h pose and stays (kills end-of-clip leg kicks)
HOLD = CFG.get('hold'); HB = CFG.get('hold_blend', 8)
if HOLD is not None:
    rH, pH = D[HOLD]
    for j in range(HOLD, N):
        w = min(1.0, (j-HOLD+1)/HB); w = w*w*(3-2*w)
        rot, p = D[j]
        D[j] = ({n: rot[n].slerp(rH[n], w) for n in rot}, p.lerp(pH, w))
# limb settle: from frame a to b the limbs' local rotations (relative to their parent) ease into the reference
# pose's local rotations, so legs / arms fold into the final lying pose instead of flailing (kicks) during the fall.
SETS = CFG.get('settle')
SETS = SETS if isinstance(SETS, list) else ([SETS] if SETS else [])
for SET in SETS:
    CHAINS = {'legs': [('pelvis','thigh_l'),('thigh_l','calf_l'),('calf_l','foot_l'),('foot_l','ball_l'),
                       ('pelvis','thigh_r'),('thigh_r','calf_r'),('calf_r','foot_r'),('foot_r','ball_r')],
              'arms': [('spine_05','clavicle_l'),('clavicle_l','upperarm_l'),('upperarm_l','lowerarm_l'),('lowerarm_l','hand_l'),
                       ('spine_05','clavicle_r'),('clavicle_r','upperarm_r'),('upperarm_r','lowerarm_r'),('lowerarm_r','hand_r')]}
    a_, b_ = SET['from'], SET['to']; ref = SET.get('ref', len(D)-1); amt = SET.get('amount', 1.0)
    rR = D[ref][0]
    for j in range(a_, len(D)):
        w = 1.0 if j >= b_ else (j-a_)/max(1,(b_-a_)); w = w*w*(3-2*w)
        if SET.get('release'):
            r0, r1 = SET['release']
            if j >= r1: w = 0.0
            elif j > r0: u_ = (j-r0)/(r1-r0); w *= 1 - u_*u_*(3-2*u_)
        w *= amt
        rot, p = D[j]; rot = dict(rot)
        if 'core' in SET.get('parts', []):
            wc = w * SET.get('core_amount', 1.0)
            rot['pelvis'] = D[j][0]['pelvis'].slerp(rR['pelvis'], wc)
            for par, ch in (('pelvis','spine_02'),('spine_02','spine_03'),('spine_03','spine_05')):
                Lc = D[j][0][par].inverted() @ D[j][0][ch]; Lr = rR[par].inverted() @ rR[ch]
                rot[ch] = rot[par] @ Lc.slerp(Lr, wc)
            for par, ch in (('spine_05','neck_01'),('neck_01','head'),('spine_05','clavicle_l'),('spine_05','clavicle_r'),
                            ('clavicle_l','upperarm_l'),('clavicle_r','upperarm_r'),('upperarm_l','lowerarm_l'),('upperarm_r','lowerarm_r'),
                            ('lowerarm_l','hand_l'),('lowerarm_r','hand_r')):
                if 'arms' in SET['parts'] and par != 'spine_05' and ch not in ('neck_01','head'): continue
                rot[ch] = rot[par] @ (D[j][0][par].inverted() @ D[j][0][ch])
        for part in [p_ for p_ in SET.get('parts', ['legs']) if p_ != 'core']:
            for par, ch in CHAINS[part]:
                if par not in rot or ch not in rot: continue
                Lc = D[j][0][par].inverted() @ D[j][0][ch]
                Lr = rR[par].inverted() @ rR[ch]
                mix = SET.get('rest_mix', {}).get(part, 0.0)
                if mix: Lr = Lr.slerp(restQ[par].inverted() @ restQ[ch], mix)
                rot[ch] = rot[par] @ Lc.slerp(Lr, w)
        D[j] = (rot, p)
if CFG.get('end') is not None:
    D = D[:CFG['end']+1]; S = S[:CFG['end']+1]; N = len(D)
# loop closure on world rotations / pelvis
if LOOP:
    NB = min(12, N//3)
    r0, pp0 = D[0]; rL, ppL = D[-1]
    for j in range(N-NB, N):
        w = (j-(N-NB-1))/NB; w = w*w*(3-2*w)
        rot, p = D[j]
        for t in rot:
            delta = r0[t] @ rL[t].inverted()
            rot[t] = Quaternion().slerp(delta, w) @ rot[t]
        D[j] = (rot, p + (pp0-ppL)*w)

def fk(rot, ppos):
    W = {}
    for n in order:
        b = TB[n]
        if b.parent is None:
            if n == 'pelvis':
                W[n] = Matrix.Translation(ppos) @ rot['pelvis'].to_matrix().to_4x4()
            else:
                W[n] = restW[n].copy()
            continue
        M = W[b.parent.name] @ relRest[n]
        if n in rot:
            M = Matrix.Translation(M.to_translation()) @ rot[n].to_matrix().to_4x4()
        elif n in FING:
            side, q = FING[n]
            h = HAND[side]
            hq = wq(W[h]) @ restQ[h].inverted()
            M = Matrix.Translation(M.to_translation()) @ (hq @ q).to_matrix().to_4x4()
        W[n] = M
    for a_, b_ in (('ik_hand_gun','hand_r'),('ik_hand_r','hand_r'),('ik_hand_l','hand_l'),('ik_foot_l','foot_l'),('ik_foot_r','foot_r')):
        if a_ in W: W[a_] = W[b_].copy()
    return W

def key_all(Ws, roots):
    tgt.animation_data_create()
    a = bpy.data.actions.new(CFG['name']); tgt.animation_data.action = a
    for pb in tgt.pose.bones: pb.rotation_mode = 'QUATERNION'
    for i, W in enumerate(Ws):
        fr = i + 1
        R = roots[i]
        Rm = Matrix.Translation(R)
        for n in order:
            b = TB[n]; pb = tgt.pose.bones[n]
            Marm = Rm.inverted() @ W[n]
            if b.parent:
                Mp = Rm.inverted() @ W[b.parent.name]
                basis = (b.parent.matrix_local.inverted() @ b.matrix_local).inverted() @ Mp.inverted() @ Marm
            else:
                basis = b.matrix_local.inverted() @ Marm
            l, q, s = basis.decompose()
            pb.location = l; pb.rotation_quaternion = q; pb.scale = (1,1,1)
            pb.keyframe_insert('location', frame=fr, group=n); pb.keyframe_insert('rotation_quaternion', frame=fr, group=n)
            pb.keyframe_insert('scale', frame=fr, group=n)
        tgt.location = R
        tgt.keyframe_insert('location', frame=fr)
    # quaternion continuity
    for fc in a.fcurves:
        pass
    sc.frame_start = 1; sc.frame_end = len(Ws)

_gn = {g.index: g.name for g in tmesh.vertex_groups}
SOLE = [v.index for v in tmesh.data.vertices if v.groups and _gn[max(v.groups, key=lambda g: g.weight).group].startswith(('foot_','ball_'))]
def mesh_minz(sole=False):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = tmesh.evaluated_get(dg); mwm = ev.matrix_world
    if sole:
        vv = ev.data.vertices
        return min((mwm @ vv[i].co).z for i in SOLE)
    return min((mwm @ v.co).z for v in ev.data.vertices)


def two_bone(W, rot, side, T, names=None):
    th, ca, fo = names if names else ('thigh_'+side, 'calf_'+side, 'foot_'+side)
    H = W[th].to_translation(); K = W[ca].to_translation(); A = W[fo].to_translation()
    l1 = (K-H).length; l2 = (A-K).length
    d = max(min((T-H).length, l1+l2-1e-4), abs(l1-l2)+1e-4)
    u = (T-H).normalized()
    v = (K-H) - u*(K-H).dot(u)
    if v.length < 1e-6: v = W[th].to_3x3() @ Vector((0,0,1))
    v.normalize()
    ca_ = (l1*l1 + d*d - l2*l2)/(2*l1*d); ca_ = max(-1, min(1, ca_))
    K2 = H + l1*(ca_*u + math.sqrt(1-ca_*ca_)*v)
    T2 = H + u*d
    q1 = (K-H).rotation_difference(K2-H)
    q2 = (q1 @ (A-K)).rotation_difference(T2-K2)
    rot[th] = q1 @ rot[th]; rot[ca] = q2 @ q1 @ rot[ca]

# ---------- rifle hold (two-hand IK onto a virtual rifle carried by the chest) ----------
RIFLE = CFG.get('rifle')  # None or dict(mode=aim|ready|up|auto, lw=[[frame,w],...], rw=..., release=frame)
LEFTX = Vector((1,0,0)); UPZ = Vector((0,0,1)); FWD = Vector((0,-1,0))
MODES = {'aim':  dict(B=Vector((-0.13,-0.15,1.45)), pitch=0.0),
         'ready':dict(B=Vector((-0.12,-0.16,1.38)), pitch=-38.0),
         'up':   dict(B=Vector((-0.10,-0.20,1.20)), pitch=60.0)}
def hand_frame_rest(h):
    side = h[-1]
    m = ['middle_01_'+side,'middle_02_'+side,'middle_03_'+side]
    d = (restH['middle_01_'+side]-restH[h]).normalized()
    tip = restH[m[2]]-restH[m[0]]; dd = (restH[m[1]]-restH[m[0]]).normalized()
    n = (tip - dd*tip.dot(dd)); n = (n - d*n.dot(d)).normalized()
    return d, n
HF = {h: hand_frame_rest(h) for h in ('hand_l','hand_r')}
def frame_q(d, n):
    d = d.normalized(); n = (n - d*n.dot(d)).normalized(); c = d.cross(n)
    return Matrix((d, n, c)).transposed().to_quaternion()
HQ0 = {h: frame_q(*HF[h]) for h in HF}
def rifle_targets(mode_w):
    # mode_w: dict mode->weight ; returns (Rwrist, Lwrist, Rq, Lq) in rest-world (character facing -Y)
    B = Vector(); pitch = 0.0
    for m, w in mode_w.items(): B += MODES[m]['B']*w; pitch += MODES[m]['pitch']*w
    p = math.radians(pitch)
    u = (FWD*math.cos(p) + UPZ*math.sin(p)).normalized()
    up = (UPZ*math.cos(p) - FWD*math.sin(p)).normalized()
    Rw = B + u*0.16 - up*0.11 + LEFTX*0.005
    Lw = B + u*0.43 - up*0.07 + LEFTX*0.045
    rd = (u*0.35 - up*0.94).normalized(); rn = LEFTX
    ld = (u*0.55 - LEFTX*0.80 + up*0.15).normalized(); ln = up
    Rq = frame_q(rd, rn) @ HQ0['hand_r'].inverted() @ restQ['hand_r']
    Lq = frame_q(ld, ln) @ HQ0['hand_l'].inverted() @ restQ['hand_l']
    return Rw, Lw, Rq, Lq
def yaw_only(q):
    f = q @ Vector((0,-1,0)); f.z = 0
    if f.length < 1e-6: return Quaternion()
    return Vector((0,-1,0)).rotation_difference(f.normalized())
def curve(pts, i, default=1.0):
    if not pts: return default
    if i <= pts[0][0]: return pts[0][1]
    for (a,wa),(b,wb) in zip(pts, pts[1:]):
        if a <= i <= b:
            t = (i-a)/max(1,(b-a)); t = t*t*(3-2*t); return wa + (wb-wa)*t
    return pts[-1][1]
RIFLE_OFF = []
def apply_rifle(D):
    if not RIFLE: return D
    W0 = fk(*D[0]); C0 = W0['spine_05']
    base = Matrix.Translation(C0.to_translation()) @ yaw_only(wq(C0)).to_matrix().to_4x4() @ Matrix.Translation(-restH['spine_05'])
    # auto mode weight from source right wrist height
    wz = [S[i]['R_Wrist'].to_translation().z for i in range(N)]
    lo, hi = wz[0], max(wz)
    if RIFLE.get('release') == 'auto':
        pz = [D[i][1].z for i in range(N)]
        rf = next((i for i in range(N) if pz[i] < pz[0]*0.8), N-1)
        RIFLE['rw'] = [[max(0, rf-6), 1.0], [rf+6, 0.0]]
        RIFLE['lw'] = [[max(0, rf-8), 1.0], [rf+2, 0.0]]
        RIFLE['release_frame'] = rf
    out = []
    for i in range(N):
        rot, p = D[i]; rot = dict(rot)
        W = fk(rot, p)
        mode = RIFLE.get('mode')
        if RIFLE.get('modekeys'):
            mk = RIFLE['modekeys']; mw = {}
            if i <= mk[0][0]: mw = {mk[0][1]: 1.0}
            elif i >= mk[-1][0]: mw = {mk[-1][1]: 1.0}
            else:
                for (a_, ma), (b_, mb) in zip(mk, mk[1:]):
                    if a_ <= i <= b_:
                        t_ = (i-a_)/max(1, b_-a_); t_ = t_*t_*(3-2*t_)
                        mw = {ma: 1-t_}; mw[mb] = mw.get(mb, 0) + t_
                        break
        elif mode == 'auto':
            a = 0 if hi-lo < 0.05 else max(0, min(1, (wz[i]-lo)/(hi-lo)))
            a = a*a*(3-2*a); mw = {'ready':1-a, 'aim':a}
        elif isinstance(mode, dict): mw = mode
        else: mw = {mode: 1.0}
        Rw, Lw, Rq, Lq = rifle_targets(mw)
        B_ = Vector(); pitch_ = 0.0
        for m_, w_ in mw.items(): B_ += MODES[m_]['B']*w_; pitch_ += MODES[m_]['pitch']*w_
        pr = math.radians(pitch_)
        u_ = (FWD*math.cos(pr) + UPZ*math.sin(pr)).normalized(); up_ = (UPZ*math.cos(pr) - FWD*math.sin(pr)).normalized()
        RM = Matrix((u_, up_, u_.cross(up_))).transposed().to_4x4(); RM.translation = B_
        HM = Rq.to_matrix().to_4x4(); HM.translation = Rw
        RIFLE_OFF.append(HM.inverted() @ RM)
        dq = wq(W['spine_05']) @ wq(C0).inverted()
        fol = RIFLE.get('follow', 1.0)
        if fol < 1.0: dq = yaw_only(dq).slerp(dq, fol)
        CD = Matrix.Translation(W['spine_05'].to_translation()) @ dq.to_matrix().to_4x4() @ Matrix.Translation(-C0.to_translation()) @ base
        ke = kick_env(i)
        if ke:
            Bk = CD @ B_
            KM = Matrix.Translation(Bk) @ Matrix.Rotation(math.radians(5.0*ke), 4, (CD.to_3x3() @ u_.cross(up_)).normalized()) @ Matrix.Translation(-Bk)
            KM = Matrix.Translation((CD.to_3x3() @ u_).normalized() * (-0.035*ke)) @ KM
            CD = KM @ CD
        cq = wq(CD)
        for h, T, Q, key in (('r', CD @ Rw, cq @ Rq, 'rw'), ('l', CD @ Lw, cq @ Lq, 'lw')):
            w = curve(RIFLE.get(key), i)
            if w <= 0: continue
            names = ('upperarm_'+h, 'lowerarm_'+h, 'hand_'+h)
            cur = W[names[2]].to_translation()
            two_bone(W, rot, h, cur.lerp(T, w), names)
            rot[names[2]] = rot[names[2]].slerp(Q, w)
            W = fk(rot, p)
        out.append((rot, p))
    return out
LEAN = CFG.get('lean', 0.0)
KICKS = CFG.get('kicks', [])
def kick_env(i):
    e = 0.0
    for k0 in KICKS:
        d = i - k0
        if 0 <= d < 2: e += (d+1)/2.0
        elif d >= 2: e += math.exp(-(d-1)/2.2)
    return min(e, 1.4)
if LEAN or KICKS:
    up_names = set()
    def desc(b):
        up_names.add(b.name)
        for c in b.children: desc(c)
    desc(TB['spine_05'])
    for i in range(N):
        rot, p = D[i]
        for n in rot:
            fr = 1/3 if n == 'spine_02' else 2/3 if n == 'spine_03' else 1.0 if n in up_names else 0
            if fr: rot[n] = Quaternion(Vector((1,0,0)), math.radians(LEAN - 4.0*kick_env(i))*fr) @ rot[n]
# roll: after landing, roll the whole body about its own long axis (pelvis -> head, horizontal), pivot at the pelvis
ROLL = CFG.get('roll')  # [from, to, degrees]
if ROLL:
    a_, b_, deg = ROLL
    Wt = fk(*D[min(b_, len(D)-1)])
    ax = Wt['head'].to_translation() - Wt['pelvis'].to_translation(); ax.z = 0; ax.normalize()
    for j in range(a_, len(D)):
        t_ = 1.0 if j >= b_ else (j-a_)/max(1,(b_-a_)); t_ = t_*t_*(3-2*t_)
        q = Quaternion(ax, math.radians(deg*t_))
        rot, p = D[j]
        D[j] = ({n: q @ r for n, r in rot.items()}, p)
D = apply_rifle(D)
# pass A: FK, floor
WA = [fk(*D[i]) for i in range(N)]
key_all(WA, [Vector()]*N)
mz = []
for i in range(N):
    sc.frame_set(i+1); m_all = mesh_minz()
    if CFG.get('floor') == 'kneel' and i >= CFG.get('kneel_from', 0):
        m_all = 0.5*m_all + 0.5*mesh_minz(True)
    mz.append(m_all)
# smooth
corr = []
FLOOR_CLAMP = CFG.get('floor_clamp', 0.012)
for i in range(N):
    win = mz[max(0,i-2):i+3]; c_ = -sum(win)/len(win)
    c_ = max(c_, -mz[i] - FLOOR_CLAMP)          # never sink deeper than FLOOR_CLAMP
    if i >= CFG.get('ground_from', 10**6): c_ = -mz[i]  # lying: exact contact
    corr.append(c_)
D = [(r, p + Vector((0,0,corr[i]))) for i,(r,p) in enumerate(D)]
WB = [fk(*D[i]) for i in range(N)]
# contact detection on ankles (foot_l/r heads)
LOCK = {}
KS = restH['pelvis'].z/0.959
for side in ('l','r'):
    fb = 'foot_'+side
    pos = [W[fb].to_translation() for W in WB]
    ball = [W['ball_'+side].to_translation() for W in WB]
    con = []
    for i in range(N):
        a = pos[max(0,i-1)]; b = pos[min(N-1,i+1)]
        v = (Vector((b.x-a.x, b.y-a.y))).length / (2.0/FPS if 0<i<N-1 else 1.0/FPS)
        low = pos[i].z < restH[fb].z + 0.06*KS or ball[i].z < 0.05*KS
        stand = WB[i]['pelvis'].to_translation().z > 0.57*restH['pelvis'].z
        force = CFG.get('lockall') or ('lockfrom' in CFG and i >= CFG['lockfrom'])
        con.append(True if force else (low and stand and v < CFG.get('vthresh', 0.35)))
    # intervals
    iv = []; i = 0
    while i < N:
        if con[i]:
            j = i
            while j+1 < N and con[j+1]: j += 1
            if j - i + 1 >= 3: iv.append((i, j))
            i = j+1
        else: i += 1
    w = [0.0]*N; tgtp = [None]*N
    BL = 4
    for (i0, i1) in iv:
        seg = pos[i0:i1+1]
        xs = sorted(p.x for p in seg); ys = sorted(p.y for p in seg); zs = sorted(p.z for p in seg)
        P = Vector((xs[len(xs)//2], ys[len(ys)//2], zs[len(zs)//2]))
        if CFG.get('drop'):
            bz = sorted(b.z for b in ball[i0:i1+1])[ (i1-i0+1)//2 ]
            P.z -= max(0.0, min(bz - 0.015, P.z - restH[fb].z))
        for i in range(max(0, i0-BL), min(N, i1+BL+1)):
            if i < i0: ww = 1 - (i0-i)/(BL+1)
            elif i > i1: ww = 1 - (i-i1)/(BL+1)
            else: ww = 1.0
            if ww > w[i]: w[i] = ww; tgtp[i] = P
    LOCK[side] = (w, tgtp, iv)

# clean steps: between two plants (and from the last plant to where the body leaves its feet) the foot travels on a
# lifted arc instead of skimming the floor, so nothing slides while the character is still standing
ARC = {'l': {}, 'r': {}}
STEP_LIFT = CFG.get('step_lift', 0.07) * KS
for side in ('l','r'):
    w, tp, iv = LOCK[side]
    fb = 'foot_'+side
    pz_ = [W['pelvis'].to_translation().z for W in WB]
    for k_, (i0, i1) in enumerate(iv):
        if k_+1 < len(iv):
            j0 = iv[k_+1][0]; Pn = tp[j0]; trailing = False
        else:
            j0 = next((i for i in range(i1+1, N) if pz_[i] < 0.7*restH['pelvis'].z), None)
            if j0 is None: continue
            Pn = WB[j0][fb].to_translation(); trailing = True
        n_ = j0 - i1
        if n_ < 3 or n_ > 24: continue
        Pp = tp[i1]
        for i in range(i1+1, j0 + (0 if not trailing else 1)):
            s_ = (i-i1)/n_; e_ = s_*s_*(3-2*s_)
            T = Pp.lerp(Pn, e_); T.z += STEP_LIFT*math.sin(math.pi*s_)
            ARC[side][i] = (T, 1.0)
WF = []; RF = []
for i in range(N):
    rot, p = D[i]; rot = dict(rot)
    W = WB[i]
    for side in ('l','r'):
        w, tp, _ = LOCK[side]
        if i in ARC[side]:
            T, wa = ARC[side][i]
            two_bone(W, rot, side, T)
            W = fk(rot, p)
        elif w[i] > 0:
            cur = W['foot_'+side].to_translation()
            T = cur.lerp(tp[i], w[i])
            two_bone(W, rot, side, T)
            W = fk(rot, p)
    WF.append(fk(rot, p)); RF.append((rot, p))
# pass C: floor after the foot IK. Off the feet nothing may sink deeper than FLOOR_CLAMP; once the body is down
# (pelvis stays under 45 % of its standing height) it rests exactly on its lowest point.
key_all(WF, [Vector()]*N)
mz2 = []
for i in range(N):
    sc.frame_set(i+1); mz2.append(mesh_minz())
pz = [W['pelvis'].to_translation().z for W in WF]
hz = [W['head'].to_translation().z for W in WF]
down = next((i for i in range(N) if all(z < 0.6*restH['pelvis'].z for z in pz[i:]) and all(z < 0.35*restH['head'].z for z in hz[i:])), N)
dz = [0.0]*N
for i in range(N):
    if i >= down: dz[i] = -mz2[i]
    elif mz2[i] < -FLOOR_CLAMP: dz[i] = -mz2[i] - FLOOR_CLAMP
sm = []
for i in range(N):
    if i >= down:
        win = [dz[j] for j in range(max(down, i-1), min(N, i+2))]; v = sum(win)/len(win)
        v = max(v, -mz2[i] - FLOOR_CLAMP)
    else:
        win = [dz[j] for j in range(max(0, i-2), min(down, i+3))]; v = max(dz[i], sum(win)/len(win))
    sm.append(v)
if down < N:  # ease into exact contact over 4 frames before 'down'
    for i in range(max(0, down-4), down):
        if pz[i] < 0.57*restH['pelvis'].z:
            w = (i-(down-5))/5.0; sm[i] = max(sm[i] + (sm[down]-sm[i])*w, -mz2[i] - FLOOR_CLAMP)
WF = []
for i,(rot, p) in enumerate(RF):
    WF.append(fk(rot, p + Vector((0,0,sm[i]))))
res_down = down
# root motion: horizontal pelvis travel, smoothed, on the armature object (= root bone)
roots = [Vector()]*N
if ROOTMO:
    px = [W['pelvis'].to_translation() for W in WF]
    raw = [Vector((q.x - px[0].x, q.y - px[0].y, 0)) for q in px]
    sm = []
    for i in range(N):
        win = raw[max(0,i-4):i+5]; sm.append(sum(win, Vector())/len(win))
    sm = [v - sm[0] for v in sm]
    roots = sm
# convert to root-relative
key_all(WF, roots)
# metrics
res = {'name': CFG['name'], 'frames': N, 'seconds': round((N-1)/FPS, 3), 'scale_k': round(k,3)}
slide = {}
for side in ('l','r'):
    _, _, iv = LOCK[side]
    s = 0.0
    for (i0,i1) in iv:
        for i in range(i0, i1):
            sc.frame_set(i+1); a = (tgt.matrix_world @ tgt.pose.bones['foot_'+side].head)
            sc.frame_set(i+2); b = (tgt.matrix_world @ tgt.pose.bones['foot_'+side].head)
            s += (Vector((b.x-a.x, b.y-a.y))).length
    slide[side] = round(s*100, 2); res['contacts_'+side] = iv
res['slide_in_contact_cm'] = slide
mzs = []
for i in range(N):
    sc.frame_set(i+1); mzs.append(mesh_minz())
res['minz_cm'] = [round(min(mzs)*100,1), round(max(mzs)*100,1)]
res['down_frame'] = res_down + 1
res['minz_down_cm'] = [round(min(mzs[res_down:])*100,1), round(max(mzs[res_down:])*100,1)] if res_down < N else None
mx = 0.0
for i in range(max(1, N-15), N):
    sc.frame_set(i); A_ = {n: (tgt.matrix_world @ tgt.pose.bones[n].head).copy() for n in ('foot_l','foot_r','hand_l','hand_r','head')}
    sc.frame_set(i+1); mx = max(mx, max(((tgt.matrix_world @ tgt.pose.bones[n].head) - A_[n]).length for n in A_))
res['last_half_s_max_joint_speed_cms'] = round(mx*30*100,1)
res['root_end_cm'] = [round(roots[-1].x*100,1), round(roots[-1].y*100,1)]
sc.frame_set(N); res['pelvis_end_z_cm'] = round((tgt.matrix_world @ tgt.pose.bones['pelvis'].head).z*100,1)
if LOOP:
    sc.frame_set(1); A0 = {pb.name: pb.matrix.copy() for pb in tgt.pose.bones}
    sc.frame_set(N); err = max((A0[n].to_translation() - tgt.pose.bones[n].matrix.to_translation()).length for n in A0)
    res['loop_seam_max_bone_cm'] = round(err*100, 3)
if RIFLE_OFF:
    em = bpy.data.objects.new('RiflePreview', None); sc.collection.objects.link(em)
    em.rotation_mode = 'QUATERNION'
    for i in range(N):
        sc.frame_set(i+1)
        Hm = tgt.matrix_world @ tgt.pose.bones['hand_r'].matrix
        Hm = Matrix.Translation(Hm.to_translation()) @ wq(Hm).to_matrix().to_4x4()
        M = Hm @ RIFLE_OFF[i]
        em.location = M.to_translation(); em.rotation_quaternion = wq(M)
        em.keyframe_insert('location', frame=i+1); em.keyframe_insert('rotation_quaternion', frame=i+1)
# remove source
for o in newobjs: bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=OUTFBX.replace('.fbx', '.blend'))
# export armature only (scene on frame 1, so the file's default node pose is the clip's first, standing pose)
sc.frame_set(1)
bpy.ops.object.select_all(action='DESELECT'); tgt.select_set(True); bpy.context.view_layer.objects.active = tgt
USF1 = CFG.get('usf1', False)
if USF1: sc.unit_settings.scale_length = 0.01
# un-hide the target mesh so a mesh-included export previews on import
for o in bpy.data.objects:
    if o.type=='MESH' and o.name==TMESH:
        o.hide_render=False; o.hide_viewport=False
        o.select_set(True)
otypes={'ARMATURE','MESH'} if CFG.get('export_mesh', True) else {'ARMATURE'}
bpy.ops.export_scene.fbx(filepath=OUTFBX, use_selection=True, object_types=otypes, apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL', global_scale=100.0 if USF1 else 1.0, add_leaf_bones=False, bake_anim=True,
    bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False, bake_anim_use_all_actions=False,
    bake_anim_force_startend_keying=True, bake_anim_step=1.0, bake_anim_simplify_factor=0.0,
    use_armature_deform_only=False, axis_forward='-Z', axis_up='Y')
print('RESULT', json.dumps(res))
