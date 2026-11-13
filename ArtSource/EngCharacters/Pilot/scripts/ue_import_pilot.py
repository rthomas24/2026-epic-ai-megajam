# Pilot step 2 (Unreal, headless): import export/SK_Pilot.fbx, the six A_Pilot_*.fbx and T_Pilot_{BC,N,ORM}.png into
# /Game/TriadDrift/Gameplay/Characters/Pilot with its OWN skeleton SKEL_Pilot, build M_Pilot / MI_Pilot, create
# DA_Pilot_Visuals (UTDPilotVisuals) and print the verification numbers. Idempotent. Nothing outside that folder is saved.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Pilot\scripts\ue_import_pilot.py
import json
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
EXPORT = os.path.join(ROOT, "export")
DEST = "/Game/TriadDrift/Gameplay/Characters/Pilot"
ANIMS = ["A_Pilot_Run", "A_Pilot_Walk", "A_Pilot_CasualWalk", "A_Pilot_Idle02", "A_Pilot_Idle3", "A_Pilot_Idle6"]
IDLE = "A_Pilot_Idle3"          # neutral stand, hands at the sides, feet planted, loops within 0.1 cm (see SOURCE.md)
EYE_TARGET_CM = 164.0           # gameplay eye height for the 180 cm capsule
EYE_BONES = ("Bone_026", "Bone_028")   # the rig's two eye bones (157.7 cm in the source)

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def fail(m):
    problems.append(m)
    log("FAILED: " + m)


def fwd(p):
    return os.path.join(EXPORT, p).replace("\\", "/")


def import_textures():
    out = {}
    tasks = []
    for suffix in ("BC", "N", "ORM"):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", fwd("T_Pilot_%s.png" % suffix))
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM"):
        tex = eal.load_asset("%s/T_Pilot_%s" % (DEST, suffix))
        if tex is None:
            fail("texture %s not imported" % suffix)
            return None
        if suffix == "N":
            # T_Pilot_N.png was written by p10_build.py with green already inverted (DirectX): no flip here
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", False)
        elif suffix == "ORM":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        else:
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            tex.set_editor_property("srgb", True)
        eal.save_loaded_asset(tex)
        out[suffix] = tex
        log("texture %s: %dx%d srgb=%s compression=%s flip_green=%s" % (
            tex.get_name(), tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), tex.get_editor_property("srgb"),
            tex.get_editor_property("compression_settings"), tex.get_editor_property("flip_green_channel")))
    return out


def ensure_master(tex):
    path = DEST + "/M_Pilot"
    if eal.does_asset_exist(path):
        # editing a material that already has instances crashes the headless editor: delete the .uasset to rebuild it
        log("master exists, kept: " + path)
        return eal.load_asset(path)
    mat = tools.create_asset("M_Pilot", DEST, unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_skeletal_mesh",):
        try:
            mat.set_editor_property(flag, True)
        except Exception as e:
            log("note: %s (%s)" % (flag, e))
    st = unreal.MaterialSamplerType

    def sample(param, texture, sampler, y):
        n = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, y)
        n.set_editor_property("parameter_name", param)
        n.set_editor_property("texture", texture)      # the default texture's type must match the sampler type
        n.set_editor_property("sampler_type", sampler)
        return n
    base = sample("BaseColorTex", tex["BC"], st.SAMPLERTYPE_COLOR, -300)
    orm = sample("ORMTex", tex["ORM"], st.SAMPLERTYPE_MASKS, 0)
    normal = sample("NormalTex", tex["N"], st.SAMPLERTYPE_NORMAL, 300)
    mp = unreal.MaterialProperty
    ok = True
    ok &= mel.connect_material_property(base, "RGB", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(orm, "G", mp.MP_ROUGHNESS)      # glTF metallic-roughness: G roughness, B metallic
    ok &= mel.connect_material_property(orm, "B", mp.MP_METALLIC)       # R is not occlusion in this file: left unconnected
    ok &= mel.connect_material_property(normal, "RGB", mp.MP_NORMAL)
    if not ok:
        fail("could not wire M_Pilot")
        return None
    try:
        mel.recompile_material(mat)
    except RuntimeError:
        log("note: no renderer here, shaders compile on first load")
    eal.save_asset(path, only_if_is_dirty=False)
    log("built " + path)
    return mat


def ensure_instance(master, tex):
    path = DEST + "/MI_Pilot"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "MI_Pilot", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    for param, key in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N")):
        mel.set_material_instance_texture_parameter_value(mi, param, tex[key])
    eal.save_asset(path, only_if_is_dirty=False)
    log("material instance %s parent %s" % (path, mi.get_editor_property("parent").get_path_name()))
    return mi


def rename_into_place(obj, wanted):
    """Give a freshly imported asset its final name inside DEST."""
    if obj is None:
        return None
    target = "%s/%s" % (DEST, wanted)
    current = obj.get_path_name().split(".")[0]
    if current == target:
        return obj
    if eal.does_asset_exist(target):
        eal.delete_asset(target)
    if not eal.rename_asset(current, target):
        fail("could not rename %s to %s" % (current, target))
        return obj
    return eal.load_asset(target)


def import_mesh():
    existing_skeleton = eal.load_asset(DEST + "/SKEL_Pilot") if eal.does_asset_exist(DEST + "/SKEL_Pilot") else None
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", existing_skeleton)      # None on the first run: the importer creates the skeleton
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", not eal.does_asset_exist(DEST + "/PA_Pilot"))
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("import_morph_targets", False)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("use_t0_as_ref_pose", False)
    d.set_editor_property("preserve_smoothing_groups", True)
    d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fwd("SK_Pilot.fbx"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", "SK_Pilot")
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("mesh import produced: %s" % sorted(str(p) for p in t.get_editor_property("imported_object_paths")))
    mesh = eal.load_asset(DEST + "/SK_Pilot")
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        fail("SK_Pilot not imported as a skeletal mesh")
        return None, None
    skeleton = rename_into_place(mesh.get_editor_property("skeleton"), "SKEL_Pilot")
    physics = mesh.get_editor_property("physics_asset")
    if physics is not None:
        physics = rename_into_place(physics, "PA_Pilot")
    elif eal.does_asset_exist(DEST + "/PA_Pilot"):
        physics = eal.load_asset(DEST + "/PA_Pilot")
        mesh.set_editor_property("physics_asset", physics)
    return mesh, skeleton


def import_anim(name, skeleton):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("create_physics_asset", False)
    a = ui.get_editor_property("anim_sequence_import_data")
    a.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    a.set_editor_property("import_bone_tracks", True)
    a.set_editor_property("remove_redundant_keys", False)
    a.set_editor_property("convert_scene", True)
    for prop, value in (("use_default_sample_rate", False), ("custom_sample_rate", 30), ("snap_to_closest_frame_boundary", True)):
        try:
            a.set_editor_property(prop, value)
        except Exception as e:
            log("note: anim option %s not set (%s)" % (prop, e))
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fwd(name + ".fbx"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    produced = [str(p) for p in t.get_editor_property("imported_object_paths")]
    seq = None
    for p in produced:
        obj = eal.load_asset(p.split(".")[0])
        if isinstance(obj, unreal.AnimSequence):
            seq = obj
    if seq is None and eal.does_asset_exist("%s/%s" % (DEST, name)):
        seq = eal.load_asset("%s/%s" % (DEST, name))
    if seq is None or not isinstance(seq, unreal.AnimSequence):
        fail("%s: no AnimSequence produced (%s)" % (name, produced))
        return None
    seq = rename_into_place(seq, name)
    try:
        seq.set_editor_property("enable_root_motion", False)
    except Exception as e:
        log("note: enable_root_motion (%s)" % e)
    eal.save_loaded_asset(seq)
    return seq


def vec(v):
    return (round(v.x, 2), round(v.y, 2), round(v.z, 2))


def world(pose, bone):
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)


def measure_skeleton(skeleton):
    """Reference pose in component space: bone count, where the feet / head / eyes are, which way it faces."""
    ape = unreal.AnimPoseExtensions
    pose = ape.get_reference_pose(skeleton)
    names = [str(n) for n in ape.get_bone_names(pose)]
    log("skeleton bones: %d, first %s, has root=%s" % (len(names), names[:3], "root" in names))
    log("extra (non-mannequin) bones: %s" % sorted(n for n in names if n.startswith("Bone_") or n.endswith("_end") or n == "headfront"))
    p = {n: world(pose, n).translation for n in ("root", "pelvis", "head", "foot_l", "ball_l", "foot_r", "hand_l", "hand_r") + EYE_BONES if n in names}
    log("ref pose (cm, component space): %s" % {k: vec(v) for k, v in p.items()})
    root_t = world(pose, names[0])
    log("root bone '%s' transform: loc %s rot %s scale %s" % (names[0], vec(root_t.translation),
        (round(root_t.rotation.rotator().roll, 2), round(root_t.rotation.rotator().pitch, 2), round(root_t.rotation.rotator().yaw, 2)), vec(root_t.scale3d)))
    toes = (p["ball_l"] - p["foot_l"])
    facing = math.degrees(math.atan2(toes.y, toes.x))            # yaw of the direction the toes point
    left = p["hand_l"] - p["hand_r"]
    log("facing: toes point along yaw %.1f deg (+X = 0, +Y = 90); left hand is on %s; MeshYaw to face +X = %.1f" % (
        facing, "+X" if left.x > abs(left.y) else "-X" if -left.x > abs(left.y) else "+Y" if left.y > 0 else "-Y", -round(facing / 90.0) * 90.0))
    eye = sum(p[b].z for b in EYE_BONES) / len(EYE_BONES)
    return names, -round(facing / 90.0) * 90.0, eye


def measure_anim(seq, names):
    ape = unreal.AnimPoseExtensions
    length = seq.get_play_length()
    opts = unreal.AnimPoseEvaluationOptions()
    try:
        frames = unreal.AnimationLibrary.get_num_frames(seq)
    except Exception:
        frames = -1
    n = 40
    poses = [ape.get_anim_pose_at_time(seq, length * i / n, opts) for i in range(n + 1)]
    first, last = poses[0], poses[-1]
    worst_cm = 0.0
    worst_deg = 0.0
    for b in names:
        a, c = world(first, b), world(last, b)
        worst_cm = max(worst_cm, (a.translation - c.translation).length())
        dot = abs(a.rotation.x * c.rotation.x + a.rotation.y * c.rotation.y + a.rotation.z * c.rotation.z + a.rotation.w * c.rotation.w)
        worst_deg = max(worst_deg, math.degrees(2 * math.acos(min(1.0, dot))))
    pel = [world(q, "pelvis").translation for q in poses]
    root = [world(q, names[0]).translation for q in poses]
    net = pel[-1] - pel[0]
    rng = lambda axis: max(getattr(v, axis) for v in pel) - min(getattr(v, axis) for v in pel)
    root_move = max((v - root[0]).length() for v in root)
    loops = worst_cm < 0.5 and worst_deg < 1.0
    in_place = abs(net.x) < 1.0 and abs(net.y) < 1.0 and root_move < 0.1
    sk = seq.get_editor_property("skeleton") if hasattr(seq, "get_editor_property") else None
    try:
        sk_name = seq.get_skeleton().get_name()
    except Exception:
        sk_name = sk.get_name() if sk else "?"
    log("%s: skeleton %s, %.4f s, %s frames, first-vs-last pose max %.2f cm / %.2f deg -> %s; pelvis net (%.2f, %.2f, %.2f) cm, "
        "range x %.1f y %.1f z %.1f, root bone moves %.3f cm -> %s" % (
            seq.get_name(), sk_name, length, frames, worst_cm, worst_deg, "LOOPS" if loops else "DOES NOT LOOP",
            net.x, net.y, net.z, rng("x"), rng("y"), rng("z"), root_move, "IN PLACE" if in_place else "TRAVELS"))
    return sk_name


def ensure_physics(mesh):
    """The FBX import made no physics asset here: ask the editor for the default one (a capsule per major bone)."""
    path = DEST + "/PA_Pilot"
    if mesh.get_editor_property("physics_asset") is not None:
        return mesh.get_editor_property("physics_asset")
    try:
        pa = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).create_physics_asset(mesh)
    except Exception as e:
        log("note: physics asset not created (%s)" % e)
        return None
    if pa is None:
        log("note: physics asset not created (the editor returned nothing)")
        return None
    pa = rename_into_place(pa, "PA_Pilot")
    mesh.set_editor_property("physics_asset", pa)
    eal.save_loaded_asset(pa)
    return pa


def export_back(mesh):
    """Write the imported mesh back out as FBX so Blender can count triangles and re-measure it (p30_check_roundtrip.py)."""
    out = os.path.join(REPO, "Saved", "Pilot", "SK_Pilot_back.fbx").replace("\\", "/")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    t = unreal.AssetExportTask()
    t.set_editor_property("object", mesh)
    t.set_editor_property("filename", out)
    t.set_editor_property("automated", True)
    t.set_editor_property("prompt", False)
    t.set_editor_property("replace_identical", True)
    t.set_editor_property("exporter", unreal.SkeletalMeshExporterFBX())
    opt = unreal.FbxExportOption()
    opt.set_editor_property("level_of_detail", False)
    opt.set_editor_property("collision", False)
    t.set_editor_property("options", opt)
    log("re-export to Saved/Pilot/SK_Pilot_back.fbx ok=%s" % unreal.Exporter.run_asset_export_task(t))


def main():
    if not eal.does_directory_exist(DEST):
        eal.make_directory(DEST)
    tex = import_textures()
    if not tex:
        return
    master = ensure_master(tex)
    if master is None:
        return
    mi = ensure_instance(master, tex)
    mesh, skeleton = import_mesh()
    if mesh is None or skeleton is None:
        return
    mats = list(mesh.get_editor_property("materials"))
    for m in mats:
        m.set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", mats)
    eal.save_loaded_asset(skeleton)
    physics = ensure_physics(mesh)
    eal.save_asset(DEST + "/SK_Pilot", only_if_is_dirty=False)

    seqs = {}
    for name in ANIMS:
        seq = import_anim(name, skeleton)
        if seq is not None:
            seqs[name] = seq
    eal.save_loaded_asset(skeleton)

    # ---- verification numbers ----
    mesh = eal.load_asset(DEST + "/SK_Pilot")
    skeleton = mesh.get_editor_property("skeleton")
    log("mesh %s skeleton %s physics %s" % (mesh.get_path_name(), skeleton.get_path_name(), physics.get_path_name() if physics else None))
    b = mesh.get_bounds()
    log("bounds origin (%.1f, %.1f, %.1f) extent (%.1f, %.1f, %.1f) cm -> size %.1f x %.1f x %.1f cm, z from %.1f to %.1f" % (
        b.origin.x, b.origin.y, b.origin.z, b.box_extent.x, b.box_extent.y, b.box_extent.z,
        b.box_extent.x * 2, b.box_extent.y * 2, b.box_extent.z * 2, b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z))
    log("material slots: %d %s" % (len(mesh.get_editor_property("materials")), [
        (str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name()
         if m.get_editor_property("material_interface") else None) for m in mesh.get_editor_property("materials")]))
    for label, fn_ in (("verts LOD0", lambda: unreal.EditorSkeletalMeshLibrary.get_num_verts(mesh, 0)),
                       ("lod count", lambda: unreal.EditorSkeletalMeshLibrary.get_lod_count(mesh))):
        try:
            log("%s: %s" % (label, fn_()))
        except Exception as e:
            log("note: %s unavailable (%s)" % (label, e))
    export_back(mesh)
    names, yaw, eye = measure_skeleton(skeleton)
    if physics is not None:
        try:
            log("physics asset %s: %d bodies" % (physics.get_name(), len(physics.get_editor_property("skeletal_body_setups"))))
        except Exception as e:
            log("note: physics body count unavailable (%s)" % e)
    height = b.origin.z + b.box_extent.z
    scale = round(EYE_TARGET_CM / eye, 2)
    log("eye bones at %.1f cm, top of helmet %.1f cm -> MeshScale %.2f gives eyes %.1f cm and height %.1f cm" % (eye, height, scale, eye * scale, height * scale))
    on_skeleton = 0
    for name in ANIMS:
        if name in seqs:
            if measure_anim(seqs[name], names) == "SKEL_Pilot":
                on_skeleton += 1
    log("animations on SKEL_Pilot: %d of %d" % (on_skeleton, len(ANIMS)))
    if on_skeleton != len(ANIMS):
        fail("not all six animations are on SKEL_Pilot")

    # ---- data asset for gameplay ----
    da_path = DEST + "/DA_Pilot_Visuals"
    cls = getattr(unreal, "TDPilotVisuals", None)
    if cls is None:
        fail("class TDPilotVisuals is not in this editor build (rebuild the module, then re-run)")
    else:
        try:
            da = eal.load_asset(da_path) if eal.does_asset_exist(da_path) else tools.create_asset("DA_Pilot_Visuals", DEST, cls, unreal.DataAssetFactory())
            da.set_editor_property("skeletal_mesh", mesh)
            da.set_editor_property("idle", seqs.get(IDLE))
            da.set_editor_property("walk", seqs.get("A_Pilot_Walk"))
            da.set_editor_property("run", seqs.get("A_Pilot_Run"))
            da.set_editor_property("mesh_yaw", yaw)
            da.set_editor_property("mesh_scale", scale)
            da.set_editor_property("head_bone", "head")
            eal.save_asset(da_path, only_if_is_dirty=False)
            log("data asset %s: SkeletalMesh=%s Idle=%s Walk=%s Run=%s MeshYaw=%.1f MeshScale=%.2f HeadBone=%s AnimBPClass=%s overrides=%d" % (
                da_path, da.get_editor_property("skeletal_mesh"), da.get_editor_property("idle"), da.get_editor_property("walk"),
                da.get_editor_property("run"), da.get_editor_property("mesh_yaw"), da.get_editor_property("mesh_scale"),
                da.get_editor_property("head_bone"), da.get_editor_property("anim_bp_class"), len(da.get_editor_property("material_overrides"))))
        except Exception as e:
            fail("data asset (%s)" % e)

    # redirectors left by the renames would be dead weight: remove them
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in registry.get_assets_by_path(DEST, recursive=True):
        if str(data.asset_class_path.asset_name) == "ObjectRedirector":
            log("removing redirector %s: %s" % (data.package_name, eal.delete_asset(str(data.package_name))))
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("assets: %s" % sorted(str(a) for a in eal.list_assets(DEST)))
    if problems:
        log("FAILED: %d problem(s): %s" % (len(problems), problems))
    else:
        log("OK")


main()
