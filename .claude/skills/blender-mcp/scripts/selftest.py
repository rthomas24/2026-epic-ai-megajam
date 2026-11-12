# SPDX-License-Identifier: MIT
"""
selftest.py - prove this machine's Blender + the repo's blender-* skill scripts work end to end.

HEADLESS ONLY (it starts from an empty scene). Run from the repo root:

    "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --factory-startup \
        --python .claude/skills/blender-mcp/scripts/selftest.py -- --out-dir %TEMP%/blender_selftest

Builds a procedural asteroid and a small hard-surface prop, runs the health report, the floating-part
check and a review render, bakes a texture set, exports both assets to FBX, re-imports one and checks
names/scale/orientation. Prints one line `__RESULT__{json}` with a pass/fail per step. Takes ~15 s.
"""

import json
import os
import runpy
import sys
import tempfile
import time
import traceback

import bpy

SKILLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def _load(skill, script):
    return runpy.run_path(os.path.join(SKILLS, skill, "scripts", script))


def main(out_dir):
    if not bpy.app.background:
        raise RuntimeError("selftest.py clears the scene - run it headless (blender --background), never in a live session")
    os.makedirs(out_dir, exist_ok=True)
    steps, t0 = {}, time.time()

    def step(name, fn):
        try:
            steps[name] = {"ok": True, "detail": fn()}
        except Exception as ex:  # keep going so one failure doesn't hide the rest
            steps[name] = {"ok": False, "error": repr(ex), "trace": traceback.format_exc()[-1500:]}

    bpy.ops.wm.read_factory_settings(use_empty=True)
    rocks = _load("blender-game-asset", "procedural_rocks.py")
    hs = _load("blender-game-asset", "hardsurface.py")
    report = _load("blender-mcp", "asset_report.py")
    check = _load("blender-mcp", "assembly_check.py")
    review = _load("blender-mcp", "review_renders.py")
    bake = _load("blender-texturing", "bake_texture_set.py")
    export = _load("blender-unreal-export", "export_for_unreal.py")

    step("build_asteroid", lambda: rocks["make_asteroid"]("SM_SelfTest_Rock", radius=2.0, seed=2))

    def build_prop():
        paint = hs["paint"]("M_SelfTest_Paint", (0.0, 0.55, 0.9))
        trim = hs["paint"]("M_SelfTest_Trim", (0.05, 0.05, 0.06), metallic=0.8)
        body = hs["loft"]("SM_SelfTest_Prop", [
            {"y": -1.0, "w": 0.0, "h": 0.0}, {"y": -0.8, "w": 0.6, "h": 0.4},
            {"y": 0.0, "w": 0.9, "h": 0.6, "n": 3.5, "groove": 0.008, "groove_width": 0.02},
            {"y": 0.8, "w": 0.7, "h": 0.45, "n": 3.5}], collection="SelfTest", location=(6, 0, 0))
        body.data.materials.append(trim)
        hs["add_bevel"](body, width=0.004)
        hs["add_weighted_normal"](body)
        hs["plates"](body, normal=(0, 0, 1), min_dot=0.5, material=paint)
        hole = hs["cylinder"]("Hole", radius=0.12, depth=0.6, axis="Z", location=(6, 0.4, 0.2), collection="SelfTest")
        hs["boolean"](body, hole)
        hole.parent = body
        hole.matrix_parent_inverse = body.matrix_world.inverted()
        return {"object": body.name}

    step("build_prop", build_prop)
    step("asset_report", lambda: report["main"](targets=["SM_SelfTest_Rock", "SM_SelfTest_Prop"])["totals"])
    step("assembly_check", lambda: {"ok": check["main"](targets=["SM_SelfTest_Prop"])["ok"]})
    step("review_render", lambda: review["main"](targets=["SM_SelfTest_Prop"], out_dir=out_dir, label="selftest",
                                                 mode="clay", views=["front_3q", "top", "right"])["sheet"])
    step("bake", lambda: bake["main"](target="SM_SelfTest_Rock", out_dir=out_dir, size=256,
                                      build_preview_material=True)["files"])
    step("export_rock", lambda: export["main"](root="SM_SelfTest_Rock", out_dir=out_dir)["status"])
    step("export_prop", lambda: export["main"](root="SM_SelfTest_Prop", out_dir=out_dir)["status"])

    def ai_cleanup():
        # Pretend the rock came from an AI generator: export it as GLB, then run the cleanup skill on it
        # (also proves cleanup_ai_mesh.py finds the blender-texturing bake script by its default path).
        rock = bpy.data.objects["SM_SelfTest_Rock"]
        vl = bpy.context.view_layer
        for o in vl.objects:
            o.select_set(False)
        rock.select_set(True)
        vl.objects.active = rock
        glb = os.path.join(out_dir, "fake_generator_rock.glb")
        bpy.ops.export_scene.gltf(filepath=glb, export_format="GLB", use_selection=True)
        clean = _load("blender-ai-mesh-cleanup", "cleanup_ai_mesh.py")
        r = clean["main"](path=glb, name="SM_SelfTest_AIRock", target_size_cm=150, target_tris=1500,
                          out_dir=os.path.join(out_dir, "ai_cleanup"), bake_size=256)
        if r["tris_out"] > 1500 or not r["bake"]["files"]:
            raise AssertionError("cleanup result unexpected: {}".format(r))
        return {"tris_in": r["tris_in"], "tris_out": r["tris_out"], "size_cm": r["size_cm"],
                "baked": sorted(r["bake"]["files"])}

    step("ai_mesh_cleanup", ai_cleanup)

    def reimport():
        fbx = os.path.join(out_dir, "SM_SelfTest_Prop.fbx")
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=fbx)
        names = sorted(o.name for o in bpy.data.objects)
        sm = bpy.data.objects["SM_SelfTest_Prop"]
        xs = [v.co.x for v in sm.data.vertices]
        ys = [v.co.y for v in sm.data.vertices]
        if not any(n.startswith("UCX_SM_SelfTest_Prop_") for n in names):
            raise AssertionError("collision missing: {}".format(names))
        if (max(xs) - min(xs)) < (max(ys) - min(ys)):
            raise AssertionError("prop is not front-to-+X after export")
        length = max(xs) - min(xs)
        if not 1.6 < length < 2.0:  # built 1.8 m long: catches scale/geometry regressions
            raise AssertionError("exported prop is {:.3f} m long, expected ~1.8 m".format(length))
        return {"objects": names, "length_m": round(length, 3)}

    step("fbx_round_trip", reimport)
    ok = all(s["ok"] for s in steps.values())
    return {"status": "ok" if ok else "failed", "blender": bpy.app.version_string, "out_dir": out_dir,
            "steps": steps, "seconds": round(time.time() - t0, 1)}


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = argv[argv.index("--out-dir") + 1] if "--out-dir" in argv else os.path.join(tempfile.gettempdir(), "blender_selftest")
    print("__RESULT__" + json.dumps(main(out)))
