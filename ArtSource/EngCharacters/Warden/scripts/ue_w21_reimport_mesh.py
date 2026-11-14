# Warden step 2b: re-import only the skeletal mesh (export/SK_Warden.fbx) over SK_Warden, after a rebuild of the mesh
# (Nov 14: the shield rigid on lowerarm_l instead of spine_05). The mesh as it was is kept first as SK_Warden_ShieldSpine.
# Nothing else is touched: SKEL_Warden, PA_Warden, textures and material instances stay as they are.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Warden\scripts\ue_w21_reimport_mesh.py
# Animation builder (eng/anim), Nov 14.
import os

HERE = os.path.dirname(os.path.abspath(__file__))
_src = open(os.path.join(HERE, "ue_w20_import.py"), encoding="utf-8-sig").read()
exec(_src[:_src.index("def main():")])   # constants, import_mesh(), log()

KEEP = DEST + "/SK_Warden_ShieldSpine"


def remesh():
    skel = EAL.load_asset(DEST + "/SKEL_Warden")
    old = EAL.load_asset(DEST + "/SK_Warden")
    if skel is None or old is None:
        log("FAILED: no SKEL_Warden / SK_Warden"); return
    if not EAL.does_asset_exist(KEEP):
        kept = EAL.duplicate_asset(DEST + "/SK_Warden", KEEP)
        log("kept the old mesh as %s: %s" % (KEEP, kept is not None))
        EAL.save_asset(KEEP, only_if_is_dirty=False)
    for n_ in [str(old.get_socket_by_index(i).get_editor_property("socket_name")) for i in range(old.num_sockets())
               if old.get_socket_by_index(i).get_outer() == old]:
        old.remove_socket(n_)
    mesh = import_mesh(skel)
    if mesh is None:
        log("FAILED: SK_Warden not imported"); return
    if mesh.get_editor_property("skeleton") != skel:
        problems.append("mesh skeleton is %s" % mesh.get_editor_property("skeleton").get_path_name())
    mats = list(mesh.get_editor_property("materials"))
    for m in mats:
        slot = str(m.get_editor_property("material_slot_name"))
        mi = EAL.load_asset(DEST + "/" + MI_NAME.get(slot, "?"))
        if mi is None:
            problems.append("no material for slot " + slot); continue
        m.set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", mats)
    pa = EAL.load_asset(DEST + "/PA_Warden")
    if pa is not None:
        mesh.set_editor_property("physics_asset", pa)
    EAL.save_asset(DEST + "/SK_Warden", only_if_is_dirty=False)
    mesh = EAL.load_asset(DEST + "/SK_Warden")
    b = mesh.get_bounds()
    socks = ["%s@%s" % (mesh.get_socket_by_index(i).get_editor_property("socket_name"), mesh.get_socket_by_index(i).get_editor_property("bone_name")) for i in range(mesh.num_sockets())]
    log("SK_Warden: height %.1f cm, sockets %s, slots %s, physics asset %s" % (b.origin.z + b.box_extent.z, socks,
        [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name() if m.get_editor_property("material_interface") else None)
         for m in mesh.get_editor_property("materials")], pa.get_name() if pa else None))
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in reg.get_assets_by_path(DEST, recursive=False):
        if str(data.asset_class_path.asset_name) == "ObjectRedirector":
            log("removing redirector %s: %s" % (data.package_name, EAL.delete_asset(str(data.package_name))))
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


remesh()
