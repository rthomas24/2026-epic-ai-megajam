# Triad Drift - import the engineering-made prop kits (ArtSource/EngProps/**) into Unreal. Lead-owned.
# Run headless:  powershell -File Tools\td-python.ps1 -Script Tools\td-import-props.py
#
# For every kit below it imports each asset folder (SM_*.fbx + T_*.png + *.manifest.json, written by
# the blender-unreal-export skill) into Content/TriadDrift/Gameplay/Props/<Kit>, using that skill's
# own import script with three project-specific changes:
#   - the textured master material is OURS: /Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured
#     (created here if missing; usable on instanced static meshes, which is how gameplay scatters props)
#   - the Interchange pipeline asset is saved under /Game/TriadDrift/Gameplay/Pipelines
#   - the FBX path comes from the asset folder, not from the absolute path stored in the manifest
#     (manifests are written on the machine and worktree that exported them)
# Nothing is written outside /Game/TriadDrift/Gameplay. Idempotent: re-running replaces the assets.
# Claim the Props folders in CLAIMS.md before running.

import glob
import json
import os

import unreal

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKILL_SCRIPT = os.path.join(REPO, ".claude", "skills", "blender-unreal-export", "scripts", "ue_import_assets.py")
MASTER_TEXTURED = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
PIPELINE = "/Game/TriadDrift/Gameplay/Pipelines/PL_TD_StaticMesh"

KITS = [
    ("ArtSource/EngProps/SpaceRocks", "/Game/TriadDrift/Gameplay/Props/Space"),
    ("ArtSource/EngProps/Verdance", "/Game/TriadDrift/Gameplay/Props/Verdance"),
    ("ArtSource/EngProps/Cinder", "/Game/TriadDrift/Gameplay/Props/Cinder"),
]


def log(message):
    unreal.log("TDPY: " + message)


def ensure_master_material():
    """Lit material driven by a baked set: BaseColorTex, ORMTex (AO / roughness / metallic), NormalTex, EmissiveTex * EmissiveStrength."""
    assets = unreal.EditorAssetLibrary
    lib = unreal.MaterialEditingLibrary
    if assets.does_asset_exist(MASTER_TEXTURED):
        return True
    folder, name = MASTER_TEXTURED.rsplit("/", 1)
    if not assets.does_directory_exist(folder):
        assets.make_directory(folder)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        log("FAILED: could not create " + MASTER_TEXTURED)
        return False
    for flag in ("used_with_instanced_static_meshes", "used_with_nanite", "used_with_static_lighting"):
        try:
            material.set_editor_property(flag, True)
        except Exception as error:
            log("note: could not set %s (%s)" % (flag, error))

    def sample(param, default_path, sampler, x, y):
        node = lib.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        node.set_editor_property("parameter_name", param)
        texture = assets.load_asset(default_path)
        if texture is not None:
            node.set_editor_property("texture", texture)
        node.set_editor_property("sampler_type", sampler)
        return node

    st = unreal.MaterialSamplerType
    base = sample("BaseColorTex", "/Engine/EngineResources/DefaultTexture", st.SAMPLERTYPE_COLOR, -700, -300)
    orm = sample("ORMTex", "/Engine/EngineResources/DefaultTexture", st.SAMPLERTYPE_MASKS, -700, 0)
    normal = sample("NormalTex", "/Engine/EngineMaterials/DefaultNormal", st.SAMPLERTYPE_NORMAL, -700, 300)
    emissive = sample("EmissiveTex", "/Engine/EngineResources/Black", st.SAMPLERTYPE_COLOR, -700, 600)
    strength = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, 850)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", 0.0)
    multiply = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 650)

    mp = unreal.MaterialProperty
    ok = True
    ok &= lib.connect_material_property(base, "RGB", mp.MP_BASE_COLOR)
    ok &= lib.connect_material_property(orm, "R", mp.MP_AMBIENT_OCCLUSION)
    ok &= lib.connect_material_property(orm, "G", mp.MP_ROUGHNESS)
    ok &= lib.connect_material_property(orm, "B", mp.MP_METALLIC)
    ok &= lib.connect_material_property(normal, "RGB", mp.MP_NORMAL)
    ok &= lib.connect_material_expressions(emissive, "RGB", multiply, "A")
    ok &= lib.connect_material_expressions(strength, "", multiply, "B")
    ok &= lib.connect_material_property(multiply, "", mp.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: could not wire " + MASTER_TEXTURED)
        return False
    try:
        lib.recompile_material(material)
    except RuntimeError:
        log("note: no renderer here, the material's shaders compile on first load")
    if not assets.save_asset(MASTER_TEXTURED, only_if_is_dirty=False):
        log("FAILED: could not save " + MASTER_TEXTURED)
        return False
    log("created " + MASTER_TEXTURED)
    return True


def load_skill():
    scope = {"__name__": "ue_import_assets"}
    with open(SKILL_SCRIPT, encoding="utf-8") as handle:
        exec(compile(handle.read(), SKILL_SCRIPT, "exec"), scope)
    scope["MASTER_TEXTURED"] = MASTER_TEXTURED
    scope["PIPELINE"] = PIPELINE
    return scope


def import_kit(skill, source_rel, dest):
    source = os.path.join(REPO, *source_rel.split("/"))
    if not os.path.isdir(source):
        log("skipped %s (not in this checkout yet)" % source_rel)
        return 0, 0
    assets = unreal.EditorAssetLibrary
    if not assets.does_directory_exist(dest):
        assets.make_directory(dest)
    good = bad = 0
    shared_textures = os.path.join(source, "textures")
    for manifest_path in sorted(glob.glob(os.path.join(source, "*", "*.manifest.json"))):
        folder = os.path.dirname(manifest_path)
        with open(manifest_path, encoding="utf-8") as handle:
            manifest = json.load(handle)
        local_fbx = os.path.join(folder, manifest["asset"] + ".fbx")
        if not os.path.isfile(local_fbx):
            log("FAILED %s: no FBX beside the manifest" % manifest["asset"])
            bad += 1
            continue
        # Import from a temporary manifest that points at this checkout's FBX.
        patched = dict(manifest)
        patched["fbx"] = local_fbx
        temp_dir = os.path.join(REPO, "Saved", "TDImport", manifest["asset"])
        os.makedirs(temp_dir, exist_ok=True)
        for old in glob.glob(os.path.join(temp_dir, "*")):
            os.remove(old)
        with open(os.path.join(temp_dir, manifest["asset"] + ".manifest.json"), "w", encoding="utf-8") as handle:
            json.dump(patched, handle)
        # The skill's script looks for T_<Asset>_*.png beside the manifest: link the real ones by copying paths.
        short = manifest["asset"][3:] if manifest["asset"].startswith("SM_") else manifest["asset"]
        texture_files = sorted(glob.glob(os.path.join(folder, "T_%s_*.png" % short)))
        if not texture_files and os.path.isdir(shared_textures):
            texture_files = [os.path.join(shared_textures, t) for t in manifest.get("textures", [])
                             if os.path.isfile(os.path.join(shared_textures, t))]
        report = []
        textures = skill["import_textures"](texture_files, dest, report)
        how = skill["import_mesh"](local_fbx, dest, report)
        mesh = skill["eas"].load_asset("%s/%s" % (dest, manifest["asset"]))
        if mesh is None:
            log("FAILED %s: mesh not found after import (%s). %s" % (manifest["asset"], how, "; ".join(report)))
            bad += 1
            continue
        try:
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            report.append("lightmap toggle: %s" % error)
        # Shared atlases are named after the kit, not the asset: map them by suffix.
        by_suffix = dict(textures)
        materials = skill["apply_materials"](mesh, patched, by_suffix, dest, report)
        skill["eas"].save_loaded_asset(mesh)
        bounds = mesh.get_bounds()
        log("%s: %s, textures %s, materials %s, collision hulls %d, sphere radius %.1f cm%s" % (
            manifest["asset"], how, sorted(textures), materials,
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_convex_collision_count(mesh),
            bounds.sphere_radius, ("  NOTES: " + "; ".join(report)) if report else ""))
        good += 1
    return good, bad


def main():
    if not ensure_master_material():
        return
    skill = load_skill()
    total_good = total_bad = 0
    for source_rel, dest in KITS:
        good, bad = import_kit(skill, source_rel, dest)
        total_good += good
        total_bad += bad
    log("imported %d asset(s), %d failed" % (total_good, total_bad))
    if total_good > 0 and total_bad == 0:
        log("OK")
    else:
        log("FAILED: see the lines above")


main()
