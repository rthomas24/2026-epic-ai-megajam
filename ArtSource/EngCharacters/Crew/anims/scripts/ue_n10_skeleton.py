# Null crew own skeleton, step 1: re-import the three crew FBX exports onto a NEW shared skeleton SKEL_NullCrew
# (created by the Trooper import, reused by Grunt and Heavy), under temporary names, then replace the old mesh assets
# (bound to the template SK_Mannequin) in place: delete old, rename new to the old path/name, restore material
# instances and sockets, assign PA_NullCrew, re-point the DA_Crew_*_Visuals data assets.
# Saves ONLY inside Characters/{NullTrooper,NullGrunt,NullHeavy,Crew}.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Crew\anims\scripts\ue_n10_skeleton.py
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", ".."))
EXP = os.path.join(REPO, "ArtSource", "EngCharacters", "NullTrooper", "export")
CH = "/Game/TriadDrift/Gameplay/Characters"
CREW = CH + "/Crew"
SKEL = CREW + "/SKEL_NullCrew"
PA = CREW + "/PA_NullCrew"
ASSETS = [  # (asset, folder, fbx, data asset)
    ("NullTrooper", CH + "/NullTrooper", os.path.join(EXP, "SK_NullTrooper.fbx"), "DA_Crew_Trooper_Visuals"),
    ("NullGrunt", CH + "/NullGrunt", os.path.join(EXP, "NullGrunt", "SK_NullGrunt.fbx"), "DA_Crew_Grunt_Visuals"),
    ("NullHeavy", CH + "/NullHeavy", os.path.join(EXP, "NullHeavy", "SK_NullHeavy.fbx"), "DA_Crew_Heavy_Visuals"),
]
ALLOWED = tuple(CH + "/" + f + "/" for f in ("NullTrooper", "NullGrunt", "NullHeavy", "Crew"))
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
problems = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def fail(m):
    problems.append(m)
    log("FAILED: " + m)


def pkg(obj):
    return obj.get_path_name().split(".")[0]


def own_sockets(mesh):
    out = []
    for i in range(mesh.num_sockets()):
        s = mesh.get_socket_by_index(i)
        out.append(dict(name=str(s.get_editor_property("socket_name")), bone=str(s.get_editor_property("bone_name")),
                        loc=s.get_editor_property("relative_location"), rot=s.get_editor_property("relative_rotation"),
                        scale=s.get_editor_property("relative_scale"), mine=(s.get_outer() == mesh)))
    return out


def fmt_sockets(ss):
    return ["%s@%s (%.1f,%.1f,%.1f) r(%.1f,%.1f,%.1f)%s" % (s["name"], s["bone"], s["loc"].x, s["loc"].y, s["loc"].z,
            s["rot"].pitch, s["rot"].yaw, s["rot"].roll, "" if s["mine"] else " [skeleton]") for s in ss]


def bone_count(skel):
    return len(unreal.AnimPoseExtensions.get_bone_names(unreal.AnimPoseExtensions.get_reference_pose(skel)))


def rename(obj, target):
    cur = pkg(obj)
    if cur == target:
        return obj
    if EAL.does_asset_exist(target):
        log("delete existing %s %s" % (target, EAL.delete_asset(target)))
    if not EAL.rename_asset(cur, target):
        fail("rename %s -> %s" % (cur, target))
        return obj
    return EAL.load_asset(target)


def import_mesh(fbx, folder, name, skeleton, make_pa):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", make_pa)
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


def main():
    if not os.path.isfile(ASSETS[0][2]):
        fail("missing " + ASSETS[0][2]); return
    old = {}
    for a, folder, fbx, da in ASSETS:
        p = folder + "/SK_" + a
        m = EAL.load_asset(p)
        if m is None:
            fail("old mesh missing " + p); return
        sk = m.get_editor_property("skeleton")
        mats = [(str(x.get_editor_property("material_slot_name")), x.get_editor_property("material_interface")) for x in m.get_editor_property("materials")]
        socks = own_sockets(m)
        refs = list(EAL.find_package_referencers_for_asset(p, False))
        old[a] = dict(mats=mats, socks=socks, skel=sk.get_path_name() if sk else None)
        log("OLD %s skel=%s skelbones=%d pa=%s mats=%s sockets=%s referencers=%s" % (
            p, old[a]["skel"], bone_count(sk) if sk else -1, m.get_editor_property("physics_asset"),
            [(s, mi.get_name() if mi else None) for s, mi in mats], fmt_sockets(socks), refs))
        if SKEL in (old[a]["skel"] or ""):
            log("%s is already on SKEL_NullCrew: nothing to replace" % a)

    skel = EAL.load_asset(SKEL) if EAL.does_asset_exist(SKEL) else None
    news = {}
    for i, (a, folder, fbx, da) in enumerate(ASSETS):
        tmpname = "SK_%s_NC" % a
        make_pa = (skel is None and not EAL.does_asset_exist(PA))
        m = import_mesh(fbx, folder, tmpname, skel, make_pa)
        if m is None:
            fail("import " + tmpname); return
        sk = m.get_editor_property("skeleton")
        log("new %s skeleton %s pa %s" % (tmpname, sk.get_path_name() if sk else None, m.get_editor_property("physics_asset")))
        if skel is None:
            if sk is None or sk.get_path_name().startswith("/Game/Characters/"):
                fail("Trooper import did not create a new skeleton (%s)" % (sk.get_path_name() if sk else None)); return
            skel = rename(sk, SKEL)
            pa = m.get_editor_property("physics_asset")
            if pa is not None:
                rename(pa, PA)
        news[a] = m
    skel = EAL.load_asset(SKEL)
    pa = EAL.load_asset(PA) if EAL.does_asset_exist(PA) else None
    if pa is None:
        try:
            pa = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).create_physics_asset(news["NullTrooper"])
            pa = rename(pa, PA) if pa else None
        except Exception as e:
            log("create_physics_asset error %s" % e)
    if pa is None:
        fail("no PA_NullCrew"); return
    log("SKEL_NullCrew bones %d; PA %s" % (bone_count(skel), pa.get_path_name()))

    for a, folder, fbx, da in ASSETS:
        m = news[a]
        if m.get_editor_property("skeleton") != skel:
            fail("%s not on SKEL_NullCrew: %s" % (a, m.get_editor_property("skeleton").get_path_name()))
        # materials: same instance per slot (by slot name, else by index)
        omats = old[a]["mats"]
        mats = list(m.get_editor_property("materials"))
        for j, x in enumerate(mats):
            sname = str(x.get_editor_property("material_slot_name"))
            mi = next((o for s, o in omats if s == sname), omats[min(j, len(omats) - 1)][1])
            x.set_editor_property("material_interface", mi)
        m.set_editor_property("materials", mats)
        m.set_editor_property("physics_asset", pa)
        # sockets: re-add any of the old mesh's own sockets the import dropped
        have = {s["name"] for s in own_sockets(m)}
        for s in old[a]["socks"]:
            if s["mine"] and s["name"] not in have:
                ns = unreal.SkeletalMeshSocket(outer=m)
                ns.set_editor_property("socket_name", s["name"])
                ns.set_editor_property("bone_name", s["bone"])
                ns.set_editor_property("relative_location", s["loc"])
                ns.set_editor_property("relative_rotation", s["rot"])
                ns.set_editor_property("relative_scale", s["scale"])
                m.add_socket(ns, False)
                log("%s re-added socket %s on %s" % (a, s["name"], s["bone"]))
        log("NEWMESH %s sockets %s" % (a, fmt_sockets(own_sockets(m))))
        EAL.save_asset(pkg(m), only_if_is_dirty=False)

    # replace in place: delete old, rename new to old name, re-point data asset
    for a, folder, fbx, da in ASSETS:
        target = folder + "/SK_" + a
        m = news[a]
        log("delete old %s %s" % (target, EAL.delete_asset(target)))
        m = rename(m, target)
        news[a] = m
        EAL.save_asset(target, only_if_is_dirty=False)
        dap = folder + "/" + da
        d = EAL.load_asset(dap)
        d.set_editor_property("skeletal_mesh", m)
        EAL.save_asset(dap, only_if_is_dirty=False)
    for p in (SKEL, PA):
        EAL.save_asset(p, only_if_is_dirty=False)

    # redirectors left by renames inside our folders
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for f in ALLOWED:
        for data in reg.get_assets_by_path(f.rstrip("/"), recursive=False):
            if str(data.asset_class_path.asset_name) == "ObjectRedirector":
                log("removing redirector %s: %s" % (data.package_name, EAL.delete_asset(str(data.package_name))))

    # report
    for a, folder, fbx, da in ASSETS:
        m = EAL.load_asset(folder + "/SK_" + a)
        d = EAL.load_asset(folder + "/" + da)
        dm = d.get_editor_property("skeletal_mesh")
        b = m.get_bounds()
        log("FINAL %s skel=%s pa=%s mats=%s sockets=%s bounds z %.1f..%.1f | %s.skeletal_mesh=%s same=%s" % (
            m.get_path_name(), m.get_editor_property("skeleton").get_path_name(), m.get_editor_property("physics_asset").get_path_name(),
            [(str(x.get_editor_property("material_slot_name")), x.get_editor_property("material_interface").get_path_name()) for x in m.get_editor_property("materials")],
            fmt_sockets(own_sockets(m)), b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z, da, dm.get_path_name() if dm else None, dm == m))
        if dm != m:
            fail("%s does not resolve to the new mesh" % da)
    try:
        log("PA bodies %d" % len(pa.get_editor_property("skeletal_body_setups")))
    except Exception as e:
        log("PA bodies n/a %s" % e)
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    for f in ALLOWED:
        log("assets %s: %s" % (f, sorted(str(x).split(".")[0].split("/")[-1] for x in EAL.list_assets(f.rstrip("/"), recursive=False))))
    log("FAILED: %s" % problems if problems else "OK")


main()
