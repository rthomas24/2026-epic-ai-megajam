# Triad Drift - make shippable copies of the procedural trees. Lead-owned.
# Run headless:  powershell -File Tools\td-python.ps1 -Script Tools\td-ship-trees.py
#
# The meshes the Procedural Vegetation Editor exports (Content/TriadDrift/Gameplay/Vegetation/**/SM_TDVeg_*)
# carry a link back to their PVE graph (asset user data of an editor-only class), so they cannot be cooked:
# "imports class /Script/ProceduralVegetation.ProceduralVegetationLink, which is in a module that is not
# available on the platform". This copies every tree mesh to /Game/TriadDrift/Gameplay/Props/Trees (a folder
# that is always cooked), removes that link from the copy and saves it. Materials and leaf meshes stay where
# they are and cook as dependencies. Gameplay loads trees from Props/Trees only.
# Idempotent: re-run after generating or changing trees. Claim Gameplay/Props/Trees in CLAIMS.md first.

import unreal

SOURCE = "/Game/TriadDrift/Gameplay/Vegetation"
DEST = "/Game/TriadDrift/Gameplay/Props/Trees"


def log(message):
    unreal.log("TDPY: " + message)


def main():
    assets = unreal.EditorAssetLibrary
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    if not assets.does_directory_exist(DEST):
        assets.make_directory(DEST)
    found = registry.get_assets_by_path(SOURCE, recursive=True)
    names = sorted(str(a.package_name) for a in found
                   if str(a.asset_name).startswith("SM_TDVeg_") and "_S" in str(a.asset_name)
                   and not str(a.asset_name).endswith("_Leaf") and "/Maps/" not in str(a.package_name))
    good = bad = 0
    for package in names:
        name = package.rsplit("/", 1)[-1]
        target = "%s/%s" % (DEST, name)
        if assets.does_asset_exist(target):
            assets.delete_asset(target)
        mesh = assets.duplicate_asset(package, target)
        if mesh is None or not isinstance(mesh, unreal.StaticMesh):
            log("FAILED %s: could not duplicate" % name)
            bad += 1
            continue
        try:
            data = list(mesh.get_editor_property("asset_user_data"))
        except Exception as error:
            log("FAILED %s: cannot read asset user data (%s)" % (name, error))
            bad += 1
            continue
        kept = [d for d in data if d is not None and "ProceduralVegetation" not in d.get_class().get_path_name()]
        removed = len(data) - len(kept)
        mesh.set_editor_property("asset_user_data", kept)
        if not assets.save_asset(target, only_if_is_dirty=False):
            log("FAILED %s: could not save" % name)
            bad += 1
            continue
        left = [d.get_class().get_name() for d in mesh.get_editor_property("asset_user_data") if d is not None]
        log("%s: removed %d link(s), user data left %s, %d triangles" % (
            name, removed, left, mesh.get_num_triangles(0)))
        good += 1
    log("copied %d tree(s), %d failed" % (good, bad))
    log("OK" if good > 0 and bad == 0 else "FAILED: see the lines above")


main()
