# SPDX-License-Identifier: MIT
"""
bake_texture_set.py - bake Blender (procedural or layered) materials into an Unreal-ready texture set.

Outputs (PNG, named for Unreal):
    T_<Asset>_BC.png    base colour            sRGB
    T_<Asset>_N.png     tangent normal map     linear, DirectX (green flipped) by default
    T_<Asset>_ORM.png   R=AO G=Roughness B=Metallic   linear (packed)
    T_<Asset>_E.png     emissive colour (strength normalised to 1; real strength returned) sRGB

Why bake "via emission": Cycles' DIFFUSE colour pass multiplies base colour by (1 - metallic),
so metallic hulls bake almost black. Routing each Principled BSDF input (Base Color, Metallic,
Roughness, Emission Color) through a temporary Emission shader and baking EMIT captures the
actual input values. NORMAL and AO use Cycles' native bake types.

Inside Blender (MCP `execute_blender_code` / `execute_blender_code_for_cli`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-texturing/scripts/bake_texture_set.py")
    result = m["main"](target="SM_Asteroid_A", out_dir=r"<repo>/ArtSource/Environment/Asteroids/A/export",
                       size=1024, build_preview_material=True)

High-to-low (e.g. decimated AI mesh): pass source_objects=["HighPoly"], cage_extrusion=0.02.
Requires a UV map on the target. Uses Cycles; device="GPU" is faster if configured.
"""

import json
import os
import sys
import time

import bpy
import numpy as np

SUFFIX = {"BC": "BC", "N": "N", "ORM": "ORM", "E": "E"}


def _principled(mat):
    """Return (bsdf, output) for a material whose surface is driven by a Principled BSDF."""
    if not mat or not mat.node_tree:
        return None, None
    nt = mat.node_tree
    out = nt.get_output_node("ALL") or nt.get_output_node("CYCLES") or next(
        (n for n in nt.nodes if n.type == "OUTPUT_MATERIAL"), None)
    bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
    return bsdf, out


class _EmissionRoute:
    """Temporarily route one Principled input (or a constant) into the material output via Emission."""

    def __init__(self, mats, input_name, strength_override=None, scale=1.0):
        self.mats, self.input_name, self.strength_override, self.scale = mats, input_name, strength_override, scale
        self.saved = []

    def __enter__(self):
        for mat in self.mats:
            bsdf, out = _principled(mat)
            if bsdf is None or out is None:
                continue
            nt = mat.node_tree
            surf = out.inputs["Surface"]
            prev = surf.links[0].from_socket if surf.is_linked else None
            em = nt.nodes.new("ShaderNodeEmission")
            em.inputs["Strength"].default_value = 1.0
            sock = bsdf.inputs[self.input_name]
            if sock.is_linked:
                nt.links.new(sock.links[0].from_socket, em.inputs["Color"])
            else:
                v = sock.default_value
                if hasattr(v, "__len__"):
                    em.inputs["Color"].default_value = (v[0], v[1], v[2], 1.0)
                else:
                    f = float(v) * self.scale
                    em.inputs["Color"].default_value = (f, f, f, 1.0)
            nt.links.new(em.outputs["Emission"], surf)
            self.saved.append((nt, out, prev, em))
        return self

    def __exit__(self, *exc):
        for nt, out, prev, em in self.saved:
            nt.nodes.remove(em)
            if prev is not None:
                nt.links.new(prev, out.inputs["Surface"])


def _set_active_image(mats, image):
    nodes = []
    for mat in mats:
        nt = mat.node_tree
        node = nt.nodes.new("ShaderNodeTexImage")
        node.image = image
        for n in nt.nodes:
            n.select = False
        node.select = True
        nt.nodes.active = node
        nodes.append((nt, node))
    return nodes


def _clear_nodes(nodes):
    for nt, node in nodes:
        nt.nodes.remove(node)


def _new_image(name, size, non_color):
    old = bpy.data.images.get(name)
    if old is not None:
        bpy.data.images.remove(old)
    img = bpy.data.images.new(name, width=size, height=size, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    img.generated_color = (0.5, 0.5, 1.0, 1.0) if non_color else (0.0, 0.0, 0.0, 1.0)
    return img


def _pixels(img):
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    return px.reshape(h, w, 4)


def _save(img, path):
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    return path


def main(target, out_dir, asset_name=None, size=1024, maps=("BC", "N", "ORM", "E"), samples=16,
         ao_samples=64, margin=16, source_objects=None, cage_extrusion=0.02, max_ray_distance=0.0,
         normal_directx=True, device="CPU", build_preview_material=False, skip_empty_emissive=True):
    t0 = time.time()
    scene = bpy.context.scene
    view_layer = bpy.context.view_layer
    obj = bpy.data.objects.get(target)
    if obj is None or obj.type != "MESH":
        raise ValueError("Target {!r} is not a mesh object".format(target))
    if not obj.data.uv_layers:
        raise ValueError("{} has no UV map; unwrap before baking".format(target))
    sources = [bpy.data.objects[n] for n in (source_objects or [])]
    selected_to_active = bool(sources)
    material_owner = sources if selected_to_active else [obj]
    mats = []
    for o in material_owner:
        for slot in o.material_slots:
            if slot.material and slot.material not in mats:
                mats.append(slot.material)
    target_mats = [s.material for s in obj.material_slots if s.material]
    if not target_mats:
        raise ValueError("{} needs at least one material slot to receive the bake".format(target))
    warnings = [m.name + ": no Principled BSDF found; colour maps for it will be wrong"
                for m in mats if _principled(m)[0] is None]

    asset = asset_name or (obj.name[3:] if obj.name.startswith("SM_") else obj.name)
    out_dir = os.path.abspath(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    state = {
        "engine": scene.render.engine,
        "samples": scene.cycles.samples,
        "device": scene.cycles.device,
        "active": view_layer.objects.active,
        "selected": [o for o in view_layer.objects if o.select_get()],
    }
    # Other objects in the file would occlude the AO bake (and show up in reflections of
    # selected-to-active rays): hide everything except the target and its sources while baking.
    keep = {obj.name} | {s.name for s in sources}
    hidden = [o for o in scene.objects if o.name not in keep and not o.hide_render]
    results, files = {}, {}
    try:
        for o in hidden:
            o.hide_render = True
        scene.render.engine = "CYCLES"
        scene.cycles.device = device
        bake = scene.render.bake
        bake.margin = margin
        try:
            bake.margin_type = "EXTEND"
        except Exception:
            pass
        bake.use_selected_to_active = selected_to_active
        bake.cage_extrusion = cage_extrusion
        bake.max_ray_distance = max_ray_distance
        bake.target = "IMAGE_TEXTURES"
        for o in view_layer.objects:
            o.select_set(False)
        for o in sources:
            o.select_set(True)
        obj.select_set(True)
        view_layer.objects.active = obj

        def run_bake(kind, image, route=None, pass_filter=None, smp=samples):
            scene.cycles.samples = smp
            nodes = _set_active_image(target_mats, image)
            try:
                kwargs = {"type": kind, "margin": margin, "use_selected_to_active": selected_to_active,
                          "cage_extrusion": cage_extrusion, "max_ray_distance": max_ray_distance}
                if pass_filter:
                    kwargs["pass_filter"] = pass_filter
                if route is not None:
                    with route:
                        bpy.ops.object.bake(**kwargs)
                else:
                    bpy.ops.object.bake(**kwargs)
            finally:
                _clear_nodes(nodes)

        if "BC" in maps:
            img = _new_image("T_{}_BC".format(asset), size, non_color=False)
            run_bake("EMIT", img, _EmissionRoute(mats, "Base Color"), smp=1)
            files["BC"] = _save(img, os.path.join(out_dir, img.name + ".png"))
            results["BC"] = img
        if "N" in maps:
            img = _new_image("T_{}_N".format(asset), size, non_color=True)
            bake.normal_space = "TANGENT"
            run_bake("NORMAL", img, smp=max(4, samples // 2))
            if normal_directx:
                px = _pixels(img)
                px[:, :, 1] = 1.0 - px[:, :, 1]
                img.pixels.foreach_set(px.ravel())
            files["N"] = _save(img, os.path.join(out_dir, img.name + ".png"))
            results["N"] = img
        if "ORM" in maps:
            ao = _new_image("__bake_ao", size, non_color=True)
            rough = _new_image("__bake_rough", size, non_color=True)
            metal = _new_image("__bake_metal", size, non_color=True)
            run_bake("AO", ao, smp=ao_samples)
            run_bake("EMIT", rough, _EmissionRoute(mats, "Roughness"), smp=1)
            run_bake("EMIT", metal, _EmissionRoute(mats, "Metallic"), smp=1)
            orm = _new_image("T_{}_ORM".format(asset), size, non_color=True)
            packed = _pixels(ao).copy()
            packed[:, :, 1] = _pixels(rough)[:, :, 0]
            packed[:, :, 2] = _pixels(metal)[:, :, 0]
            packed[:, :, 3] = 1.0
            orm.pixels.foreach_set(packed.ravel())
            files["ORM"] = _save(orm, os.path.join(out_dir, orm.name + ".png"))
            results["ORM"] = orm
            for tmp in (ao, rough, metal):
                bpy.data.images.remove(tmp)
        emission_strength = 0.0
        for m in mats:
            bsdf, _out = _principled(m)
            if bsdf is not None:
                emission_strength = max(emission_strength, float(bsdf.inputs["Emission Strength"].default_value))
        if "E" in maps:
            if emission_strength <= 0.0 and skip_empty_emissive:
                results["E_skipped"] = "No emission in source materials"
            else:
                img = _new_image("T_{}_E".format(asset), size, non_color=False)
                run_bake("EMIT", img, _EmissionRoute(mats, "Emission Color"), smp=1)
                px = _pixels(img)
                if skip_empty_emissive and float(px[:, :, :3].max()) < 1e-3:
                    results["E_skipped"] = "Emissive map was black"
                    bpy.data.images.remove(img)
                else:
                    files["E"] = _save(img, os.path.join(out_dir, img.name + ".png"))
                    results["E"] = img

        preview = None
        if build_preview_material:
            preview = _build_preview_material(asset, results, normal_directx, emission_strength)
            obj.data.materials.clear()
            obj.data.materials.append(preview)
    finally:
        for o in hidden:
            o.hide_render = False
        scene.render.engine = state["engine"]
        scene.cycles.samples = state["samples"]
        scene.cycles.device = state["device"]
        for o in view_layer.objects:
            o.select_set(False)
        for o in state["selected"]:
            if o.name in view_layer.objects:
                o.select_set(True)
        view_layer.objects.active = state["active"]

    return {
        "status": "ok",
        "asset": asset,
        "files": files,
        "size": size,
        "normal_convention": "DirectX (green flipped)" if normal_directx else "OpenGL",
        "emission_strength": emission_strength,
        "preview_material": preview.name if build_preview_material and preview else None,
        "notes": [results[k] for k in results if k.endswith("_skipped")],
        "warnings": warnings,
        "seconds": round(time.time() - t0, 2),
    }


def _build_preview_material(asset, imgs, normal_directx, emission_strength):
    """Image-based material equivalent to what Unreal will render (for review renders)."""
    name = "M_{}_Baked".format(asset)
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    try:
        mat.use_nodes = True
    except Exception:
        pass
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    def tex(img):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = img
        return n

    if "BC" in imgs:
        nt.links.new(tex(imgs["BC"]).outputs["Color"], bsdf.inputs["Base Color"])
    if "ORM" in imgs:
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(tex(imgs["ORM"]).outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
        nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    if "N" in imgs:
        src = tex(imgs["N"]).outputs["Color"]
        if normal_directx:  # flip green back for Blender's OpenGL convention
            sep = nt.nodes.new("ShaderNodeSeparateColor")
            inv = nt.nodes.new("ShaderNodeMath")
            inv.operation = "SUBTRACT"
            inv.inputs[0].default_value = 1.0
            comb = nt.nodes.new("ShaderNodeCombineColor")
            nt.links.new(src, sep.inputs["Color"])
            nt.links.new(sep.outputs["Red"], comb.inputs["Red"])
            nt.links.new(sep.outputs["Green"], inv.inputs[1])
            nt.links.new(inv.outputs["Value"], comb.inputs["Green"])
            nt.links.new(sep.outputs["Blue"], comb.inputs["Blue"])
            src = comb.outputs["Color"]
        nmap = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(src, nmap.inputs["Color"])
        nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    if "E" in imgs:
        nt.links.new(tex(imgs["E"]).outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    return mat


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Bake an Unreal-ready texture set")
    p.add_argument("--target", required=True)
    p.add_argument("--out-dir", required=True)
    p.add_argument("--size", type=int, default=1024)
    p.add_argument("--asset-name")
    p.add_argument("--sources", nargs="*")
    p.add_argument("--device", default="CPU")
    p.add_argument("--save-blend", action="store_true", help="save the .blend after baking")
    a = p.parse_args(argv)
    res = main(target=a.target, out_dir=a.out_dir, size=a.size, asset_name=a.asset_name,
               source_objects=a.sources, device=a.device)
    if a.save_blend:
        bpy.ops.wm.save_mainfile()
    print("__RESULT__" + json.dumps(res))


if __name__ == "__main__":
    _cli()
