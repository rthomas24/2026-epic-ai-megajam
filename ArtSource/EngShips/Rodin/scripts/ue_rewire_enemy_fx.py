# SPDX-License-Identifier: MIT
# Triad Drift - re-apply the effects round-5 wiring on the five enemy visuals (the ship import's versions of those
# data assets were kept in the merge, so the effects wiring on them was lost). Runs ONLY the enemy-visuals part of
# ArtSource/EngVFX/scripts/wire_r5.py (VFX and VFXScale fields; it refuses to save on any Sound difference) and
# checks that Mesh and MaterialOverrides are the same before and after.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Rodin\scripts\ue_rewire_enemy_fx.py
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(HERE))))
WIRE = os.path.join(REPO, "ArtSource", "EngVFX", "scripts", "wire_r5.py")
CH = "/Game/TriadDrift/Gameplay/Characters/"
PLAN = {
    "NullFleet/DA_Enemy_Fighter_Visuals": {"engine_trail": ("NS_TD_Engine_Enemy", 1.0)},
    "NullFleet/DA_Enemy_Interceptor_Visuals": {"engine_trail": ("NS_TD_Engine_Enemy", 1.0)},
    "NullFleet/DA_Enemy_Gunship_Visuals": {"engine_trail": ("NS_TD_Engine_Enemy", 2.0)},
    "NullFleet/DA_Enemy_Bomber_Visuals": {"engine_trail": ("NS_TD_Engine_Enemy", 1.5)},
    "NullFleet/DA_Enemy_Carrier_Visuals": {"engine_trail": ("NS_TD_Engine_Enemy", 5.0), "explosion": (None, 2.5)},
}


def log(msg):
    unreal.log("TDPY: " + msg)


def look(rel):
    da = unreal.load_asset(CH + rel)
    out = {"mesh": str(da.get_editor_property("mesh")).split("'")[1] if da else None,
           "overrides": [str(m).split("'")[1] if "'" in str(m) else str(m) for m in da.get_editor_property("material_overrides")]}
    for prop in ("bay_mesh", "turret_mesh"):
        try:
            out[prop] = str(da.get_editor_property(prop))
        except Exception:
            pass
    return out


def cue(rel, field):
    c = unreal.load_asset(CH + rel).get_editor_property(field)
    return "%s x%.2f" % (str(c.get_editor_property("vfx")).split(".")[-1].rstrip("'>"), c.get_editor_property("vfx_scale").x)


before = {rel: look(rel) for rel in PLAN}
for rel, fields in PLAN.items():
    log("BEFORE %s: %s | %s" % (rel, before[rel], {f: cue(rel, f) for f in fields}))
with open(WIRE, encoding="utf-8") as fh:
    src = fh.read()
body, _, last = src.rstrip().rpartition("\n")          # the script ends with a bare expression: its log
env = {"unreal": unreal, "PLAN": PLAN, "SAVE": True}
exec(compile(body, WIRE, "exec"), env)
lines = env["log"]
for line in lines:
    log("wire_r5: " + line)
changed = sum(1 for l in env["dump"] if l.endswith("CHANGED"))
log("sound fields read %d, changed %d" % (len(env["dump"]), changed))
ok = changed == 0 and not any(l.startswith(("MISSING", "NO SYSTEM", "FAIL", "SOUND CHANGED", "SAVE FAILED")) for l in lines)
for rel, fields in PLAN.items():
    after = look(rel)
    same = after == before[rel]
    ok &= same
    log("AFTER %s: mesh / overrides %s | %s" % (rel, "SAME" if same else "CHANGED %s" % after, {f: cue(rel, f) for f in fields}))
log("OK" if ok else "FAILED: see the lines above")
