# Read-only: print root and pelvis local transforms over time for some source sequences and their retargeted copies.
import unreal

EAL = unreal.EditorAssetLibrary
A = "/Game/Characters/Mannequins/Anims/"
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/"
PAIRS = [(A + "Death/MM_Death_Front_01", P + "A_Pilot_Death_Front"), (A + "Death/MM_Death_Back_01", P + "A_Pilot_Death_Back")]


def log(m):
    unreal.log("TDPY: " + str(m))


def dump(path):
    s = EAL.load_asset(path)
    n = unreal.AnimationLibrary.get_num_frames(s) + 1
    for f in range(0, n, 4):
        t = unreal.AnimationLibrary.get_bone_poses_for_frame(s, ["root", "pelvis", "spine_01"], f, False)
        r, p = t[0], t[1]
        log("%s f%02d root t=(%.1f %.1f %.1f) r=(%.1f %.1f %.1f) pelvis t=(%.1f %.1f %.1f) r=(%.1f %.1f %.1f)" % (
            path.split("/")[-1], f, r.translation.x, r.translation.y, r.translation.z,
            r.rotation.rotator().roll, r.rotation.rotator().pitch, r.rotation.rotator().yaw,
            p.translation.x, p.translation.y, p.translation.z,
            p.rotation.rotator().roll, p.rotation.rotator().pitch, p.rotation.rotator().yaw))


for a, b in PAIRS:
    dump(a)
    dump(b)
log("OK")
