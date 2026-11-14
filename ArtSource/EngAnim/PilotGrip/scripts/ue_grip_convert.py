# Triad Drift - turn the Blender grip solve (grip_blender.json, Blender armature space) into Unreal data for SKEL_Pilot:
# per-bone LOCAL transforms for A_Pilot_Grip_R and the BladeSocket transform on hand_r (grip_pose.json).
# python ue_grip_convert.py <grip_blender.json> <hand_ref.json (inspect_pilot_hand.py)> <out grip_pose.json>
#
# SK_Pilot.fbx came from Blender with centimetres baked in, so Unreal component space = Blender armature space with Y
# negated (checked: hand_r Blender (-30.2, -3.7, 91.3) = Unreal (-30.2, 3.7, 91.3)). A rotation conjugated by that
# mirror keeps W and Y and negates X and Z. Each finger bone keeps its reference local translation (the solve only
# turns bones about their own heads). Animation builder (eng/anim), Nov 13.
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tdq  # noqa: E402

PARENT = {}
for f in ("index", "middle", "ring", "pinky", "thumb"):
    PARENT["%s_01_r" % f] = "hand_r"
    PARENT["%s_02_r" % f] = "%s_01_r" % f
    PARENT["%s_03_r" % f] = "%s_02_r" % f


def mirror_v(v):
    return (v[0], -v[1], v[2])


def mirror_q_wxyz(q):
    """Blender (w, x, y, z) -> Unreal FQuat (x, y, z, w) of the mirrored rotation."""
    w, x, y, z = q
    return (-x, y, -z, w)


def main():
    gb = json.load(open(sys.argv[1]))
    ref = json.load(open(sys.argv[2]))
    out_path = sys.argv[3]
    bones = ref["bones"]

    def cs(name):
        b = bones[name]["cs"]
        return tuple(b["loc"]), tuple(b["quat"])

    # check the mirror on the rest heads
    worst = 0.0
    for n, h in gb["rest_heads"].items():
        if n in bones:
            worst = max(worst, tdq.length(tdq.sub(mirror_v(h), cs(n)[0])))
    print("rest head mismatch Blender->Unreal: %.3f cm" % worst)
    if worst > 0.5:
        raise SystemExit("skeletons do not line up")

    # cumulative delta rotations (Blender FK: R_b = R_parent * q_b), mirrored into Unreal
    cum = {"hand_r": (0.0, 0.0, 0.0, 1.0)}
    order = [b for f in ("index", "middle", "ring", "pinky", "thumb") for b in ("%s_01_r" % f, "%s_02_r" % f, "%s_03_r" % f)]
    for b in order:
        q = mirror_q_wxyz(gb["bones"][b]["quat_wxyz"])
        cum[b] = tdq.qnorm(tdq.qmul(cum[PARENT[b]], q))
    posed_cs = {"hand_r": cs("hand_r")[1]}
    out = {"bones": {}}
    for b in order:
        posed_cs[b] = tdq.qnorm(tdq.qmul(cum[b], cs(b)[1]))
        local_q = tdq.qnorm(tdq.qmul(tdq.qinv(posed_cs[PARENT[b]]), posed_cs[b]))
        loc = bones[b]["local"]["loc"]
        ref_q = bones[b]["local"]["quat"]
        out["bones"][b] = {"loc": loc, "quat": list(local_q), "deg_from_ref": round(tdq.qangle(local_q, ref_q), 1)}
        print("%-12s %5.1f deg from its reference" % (b, out["bones"][b]["deg_from_ref"]))

    # BladeSocket on hand_r: the hilt pivot (grip centre), X along the blade, Z (activation slide) toward the thumb
    hl, hq = cs("hand_r")
    C = mirror_v(gb["hilt"]["centre"])
    X = tdq.norm(mirror_v(gb["hilt"]["axis"]))
    Zr = mirror_v(gb["hilt"]["top"])
    Z = tdq.norm(tdq.sub(Zr, tdq.mul(X, tdq.dot(Zr, X))))
    Y = tdq.cross(Z, X)
    hilt_q = tdq.qfrom_basis(X, Y, Z)
    rel_loc = tdq.qrot(tdq.qinv(hq), tdq.sub(C, hl))
    rel_q = tdq.qnorm(tdq.qmul(tdq.qinv(hq), hilt_q))
    pitch, yaw, roll = tdq.to_rotator(rel_q)
    out["socket"] = {"bone": "hand_r", "loc": list(rel_loc), "quat": list(rel_q), "rot": [pitch, yaw, roll]}
    out["hilt_cs"] = {"loc": list(C), "x": list(X), "z": list(Z)}
    print("BladeSocket on hand_r: loc (%.2f, %.2f, %.2f) rot P %.1f Y %.1f R %.1f" % (rel_loc + (pitch, yaw, roll)))
    json.dump(out, open(out_path, "w"), indent=1)


if __name__ == "__main__":
    main()
