# Null crew own skeleton, rebuild: import the CENTIMETRE re-exports (ArtSource/EngCharacters/Crew/export/<A>/SK_<A>.fbx,
# made by b_cm_export.py; the original metre exports gave a root bone at scale 100 that collapses every animation) as
# SK_NullTrooper / SK_NullGrunt / SK_NullHeavy at their old paths. The Trooper import creates the skeleton -> renamed
# Crew/SKEL_NullCrew; Grunt and Heavy reuse it. PA_NullCrew is generated. Material instances MI_<A>, sockets Muzzle@hand_r
# and HeadGlow@head (offsets measured on the old meshes) are set, and the DA_Crew_*_Visuals re-pointed.
# Precondition: the old SK_<A>.uasset, SKEL_NullCrew, PA_NullCrew were removed on disk (nothing loaded = nothing to unload).
# Saves ONLY inside Characters/{NullTrooper,NullGrunt,NullHeavy,Crew}.
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CREWSRC = os.path.abspath(os.path.join(HERE, "..", "..", "export"))
CH = "/Game/TriadDrift/Gameplay/Characters"
CREW = CH + "/Crew"
SKEL = CREW + "/SKEL_NullCrew"
PA = CREW + "/PA_NullCrew"
ASSETS = [("NullTrooper", "DA_Crew_Trooper_Visuals"), ("NullGrunt", "DA_Crew_Grunt_Visuals"), ("NullHeavy", "DA_Crew_Heavy_Visuals")]
SOCKETS = [("HeadGlow", "head", unreal.Vector(7.9249, 14.3369, 0.0), unreal.Rotator(roll=-90.0, pitch=0.0, yaw=90.0)),
           ("Muzzle", "hand_r", unreal.Vector(-19.8861, -2.1012, 0.3548), unreal.Rotator(roll=69.4269, pitch=1.0163, yaw=-173.9680))]
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
tools = unreal.AssetToolsHelpers.get_asset_tools()
problems = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def fail(m):
    problems.append(m)
    log("FAILED: " + m)


def pkg(o):
    return o.get_path_name().split(".")[0]


def rename(obj, target):
    if pkg(obj) == target:
        return obj
    if not EAL.rename_asset(pkg(obj), target):
        fail("rename %s -> %s" % (pkg(obj), target))
        return obj
    return EAL.load_asset(target)


def import_mesh(fbx, folder, name, skeleton):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("import_morph_targets", False)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("use_t0_as_ref_pose", False)
    d.set_editor_property("preserve_smoothing_groups", True)
    d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fbx.replace("\\", "/"))
    t.set_editor_property("destination_path", folder)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("import %s produced %s" % (name, sorted(str(p) for p in t.get_editor_property("imported_object_paths"))))
    m = EAL.load_asset(folder + "/" + name)
    return m if isinstance(m, unreal.SkeletalMesh) else None


def ref_report(tag, skel):
    p = APE.get_reference_pose(skel)
    n = APE.get_bone_names(p)
    rl = APE.get_bone_pose(p, "root", unreal.AnimPoseSpaces.LOCAL)
    pl = APE.get_bone_pose(p, "pelvis", unreal.AnimPoseSpaces.LOCAL)
    hw = APE.get_bone_pose(p, "head", unreal.AnimPoseSpaces.WORLD)
    log("%s bones %d root scale (%.3f,%.3f,%.3f) pelvis local t (%.2f,%.2f,%.2f) head world z %.2f" % (
        tag, len(n), rl.scale3d.x, rl.scale3d.y, rl.scale3d.z, pl.translation.x, pl.translation.y, pl.translation.z, hw.translation.z))
    return rl.scale3d.x


def main():
    for a, da in ASSETS:
        if EAL.does_asset_exist("%s/%s/SK_%s" % (CH, a, a)):
            fail("old SK_%s still present: remove it on disk first" % a)
    for p in (SKEL, PA):
        if EAL.does_asset_exist(p):
            fail(p + " still present: remove it on disk first")
    if problems:
        log("FAILED: %s" % problems); return
    skel = None
    meshes = {}
    for a, da in ASSETS:
        folder = CH + "/" + a
        m = import_mesh(os.path.join(CREWSRC, a, "SK_%s.fbx" % a), folder, "SK_" + a, skel)
        if m is None:
            fail("import " + a); return
        if skel is None:
            sk = m.get_editor_property("skeleton")
            if sk is None or pkg(sk).startswith("/Game/Characters/"):
                fail("no new skeleton (%s)" % (pkg(sk) if sk else None)); return
            skel = rename(sk, SKEL)
        meshes[a] = m
    skel = EAL.load_asset(SKEL)
    if abs(ref_report("SKEL_NullCrew", skel) - 1.0) > 1e-3:
        fail("root bone scale is not 1")
    pa = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).create_physics_asset(meshes["NullTrooper"])
    if pa is None:
        fail("physics asset not created"); return
    pa = rename(pa, PA)
    EAL.save_asset(PA, only_if_is_dirty=False)
    for a, da in ASSETS:
        folder = CH + "/" + a
        m = meshes[a]
        mi = EAL.load_asset(folder + "/MI_" + a)
        mats = list(m.get_editor_property("materials"))
        for x in mats:
            x.set_editor_property("material_interface", mi)
        m.set_editor_property("materials", mats)
        m.set_editor_property("physics_asset", pa)
        have = {str(m.get_socket_by_index(i).get_editor_property("socket_name")) for i in range(m.num_sockets())}
        # sockets come from the SOCKET_ empties in the cm FBX (socket fields are read-only from Python): check them
        for name, bone, loc, rot in SOCKETS:
            s = m.find_socket(name) if name in have else None
            if s is None:
                fail("%s: socket %s missing" % (a, name)); continue
            l = s.get_editor_property("relative_location"); r = s.get_editor_property("relative_rotation")
            dl = (l - loc).length()
            dr = max(abs(r.pitch - rot.pitch), abs(r.yaw - rot.yaw), abs(r.roll - rot.roll))
            if str(s.get_editor_property("bone_name")) != bone or dl > 0.05 or dr > 0.1:
                fail("%s: socket %s on %s off by %.3f cm / %.3f deg" % (a, name, s.get_editor_property("bone_name"), dl, dr))
        EAL.save_asset(pkg(m), only_if_is_dirty=False)
        d = EAL.load_asset(folder + "/" + da)
        d.set_editor_property("skeletal_mesh", m)
        EAL.save_asset(folder + "/" + da, only_if_is_dirty=False)
    EAL.save_asset(SKEL, only_if_is_dirty=False)
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for f in ("NullTrooper", "NullGrunt", "NullHeavy", "Crew"):
        for data in reg.get_assets_by_path(CH + "/" + f, recursive=False):
            if str(data.asset_class_path.asset_name) == "ObjectRedirector":
                log("removing redirector %s: %s" % (data.package_name, EAL.delete_asset(str(data.package_name))))
    for a, da in ASSETS:
        folder = CH + "/" + a
        m = EAL.load_asset(folder + "/SK_" + a)
        d = EAL.load_asset(folder + "/" + da)
        dm = d.get_editor_property("skeletal_mesh")
        socks = []
        for i in range(m.num_sockets()):
            s = m.get_socket_by_index(i)
            l = s.get_editor_property("relative_location"); r = s.get_editor_property("relative_rotation")
            socks.append("%s@%s (%.1f,%.1f,%.1f) r(%.1f,%.1f,%.1f)" % (s.get_editor_property("socket_name"), s.get_editor_property("bone_name"),
                                                                     l.x, l.y, l.z, r.pitch, r.yaw, r.roll))
        b = m.get_bounds()
        log("FINAL %s skel=%s pa=%s mats=%s sockets=%s z %.1f..%.1f | %s -> %s same=%s" % (
            pkg(m), pkg(m.get_editor_property("skeleton")), pkg(m.get_editor_property("physics_asset")),
            [pkg(x.get_editor_property("material_interface")) for x in m.get_editor_property("materials")],
            socks, b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z, da, pkg(dm) if dm else None, dm == m))
        if dm != m or m.get_editor_property("skeleton") != skel:
            fail(a + " not resolved")
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()
