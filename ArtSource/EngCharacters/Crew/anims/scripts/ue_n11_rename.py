# Null crew own skeleton, step 1b (fresh editor). ue_n10_skeleton.py made SKEL_NullCrew, PA_NullCrew and the three
# meshes as SK_<name>_NC, and pointed the DA_Crew_*_Visuals at them; deleting the loaded old meshes in-process failed
# (native references), so the old SK_<name>.uasset files (on SK_Mannequin) were removed on disk before this run
# (copies in Saved/NullCrewBackup). This renames SK_<name>_NC -> SK_<name>, re-points the data assets, removes
# the redirectors and reports. Saves only inside Characters/{NullTrooper,NullGrunt,NullHeavy,Crew}.
import unreal

CH = "/Game/TriadDrift/Gameplay/Characters"
SKEL = CH + "/Crew/SKEL_NullCrew"
PA = CH + "/Crew/PA_NullCrew"
ASSETS = [("NullTrooper", "DA_Crew_Trooper_Visuals"), ("NullGrunt", "DA_Crew_Grunt_Visuals"), ("NullHeavy", "DA_Crew_Heavy_Visuals")]
EAL = unreal.EditorAssetLibrary
problems = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def main():
    skel = EAL.load_asset(SKEL)
    pa = EAL.load_asset(PA)
    for a, da in ASSETS:
        folder = CH + "/" + a
        target = folder + "/SK_" + a
        tmp = target + "_NC"
        if EAL.does_asset_exist(target):
            m = EAL.load_asset(target)
            if m.get_editor_property("skeleton") != skel:
                problems.append("%s still exists on %s" % (target, m.get_editor_property("skeleton").get_path_name()))
                continue
        else:
            if not EAL.rename_asset(tmp, target):
                problems.append("rename " + tmp); continue
        m = EAL.load_asset(target)
        m.set_editor_property("physics_asset", pa)
        EAL.save_asset(target, only_if_is_dirty=False)
        d = EAL.load_asset(folder + "/" + da)
        d.set_editor_property("skeletal_mesh", m)
        EAL.save_asset(folder + "/" + da, only_if_is_dirty=False)
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for a, da in ASSETS:
        for data in reg.get_assets_by_path(CH + "/" + a, recursive=False):
            if str(data.asset_class_path.asset_name) == "ObjectRedirector":
                log("removing redirector %s: %s" % (data.package_name, EAL.delete_asset(str(data.package_name))))
    for a, da in ASSETS:
        folder = CH + "/" + a
        m = EAL.load_asset(folder + "/SK_" + a)
        d = EAL.load_asset(folder + "/" + da)
        dm = d.get_editor_property("skeletal_mesh")
        socks = ["%s@%s" % (m.get_socket_by_index(i).get_editor_property("socket_name"), m.get_socket_by_index(i).get_editor_property("bone_name"))
                 for i in range(m.num_sockets())]
        b = m.get_bounds()
        log("FINAL %s skel=%s pa=%s mats=%s sockets=%s z %.1f..%.1f | %s -> %s same=%s" % (
            m.get_path_name(), m.get_editor_property("skeleton").get_path_name(), m.get_editor_property("physics_asset").get_path_name(),
            [x.get_editor_property("material_interface").get_path_name() for x in m.get_editor_property("materials")],
            socks, b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z, da, dm.get_path_name() if dm else None, dm == m))
        if dm != m or m.get_editor_property("skeleton") != skel:
            problems.append(a)
        log("assets %s: %s" % (a, sorted(str(x).split(".")[-1] for x in EAL.list_assets(folder, recursive=False))))
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()
