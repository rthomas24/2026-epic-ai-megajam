# Triad Drift - probe for the ART-08 audit: pilot interact / board / exit clips (length, which hand reaches, how far),
# and whether the three player ship meshes have separable landing gear (sections, material slots, sockets).
import math
import unreal
X = unreal.AnimPoseExtensions
W = unreal.AnimPoseSpaces.WORLD
O = unreal.AnimPoseEvaluationOptions()
def log(m): unreal.log("TDPY: " + str(m))
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/"
for name in ("A_Pilot_Interact_Console", "A_Pilot_Interact_Draw", "A_Pilot_Interact_Shoulder", "A_Pilot_Board_Ship", "A_Pilot_Exit_Ship"):
    seq = unreal.load_asset(P + name)
    if not seq:
        log(name + " missing"); continue
    n = int(round(seq.get_play_length() * 30)) + 1
    out = []
    for h in ("hand_l", "hand_r"):
        best = (-1e9, 0, 0)
        for i in range(n):
            p = X.get_anim_pose_at_time(seq, min(seq.get_play_length(), i / 30.0), O)
            v = X.get_bone_pose(p, h, W).translation; c = X.get_bone_pose(p, "pelvis", W).translation
            if v.y - c.y > best[0]:
                best = (v.y - c.y, i / 30.0, v.z)
        out.append("%s furthest forward %.0f cm at %.2f s (height %.0f cm)" % (h, best[0], best[1], best[2]))
    p0 = X.get_anim_pose_at_time(seq, 0.0, O); p1 = X.get_anim_pose_at_time(seq, seq.get_play_length(), O)
    r0, r1 = X.get_bone_pose(p0, "root", W).translation, X.get_bone_pose(p1, "root", W).translation
    log("%s: %.2f s; %s; root travel %.0f cm" % (name, seq.get_play_length(), "; ".join(out), math.hypot(r1.x - r0.x, r1.y - r0.y)))
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for d in reg.get_assets_by_path("/Game/TriadDrift", recursive=True):
    n = str(d.asset_name)
    cls = str(d.asset_class_path.asset_name)
    if cls in ("StaticMesh", "SkeletalMesh") and any(k in n for k in ("Rift", "Lancer", "Bastion")) and "Null" not in n:
        a = d.get_asset()
        try:
            if cls == "StaticMesh":
                slots = [str(m.material_slot_name) for m in a.static_materials]
                socks = [str(s.socket_name) for s in a.get_sockets_by_tag("")] if hasattr(a, "get_sockets_by_tag") else []
                log("%s %s: sections %d, slots %s, tris %d" % (cls, d.package_name, a.get_num_sections(0), slots, a.get_num_triangles(0)))
            else:
                log("%s %s: bones %d" % (cls, d.package_name, len(X.get_bone_names(X.get_reference_pose(a.get_editor_property("skeleton"))))))
        except Exception as e:
            log("%s %s: %s" % (cls, d.package_name, e))
log("OK")
