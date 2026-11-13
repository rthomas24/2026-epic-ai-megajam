# Animation audit step 0: list every AnimSequence / montage / blend space / AnimBP / retargeter in the project
# (ours and the template content) with length, frames, skeleton, additive type, root motion flag and root travel.
# Writes Saved/EngAnim/inventory.json. Saves nothing in Content.
import json, os
import unreal

EAL = unreal.EditorAssetLibrary
ROOTS = ["/Game/TriadDrift/Gameplay/Characters", "/Game/Characters", "/Game/Variant_Shooter", "/Game/Variant_Horror",
         "/Game/FirstPerson", "/Game/Weapons", "/Game/TriadDrift"]
OUTDIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    seen = set()
    rows = []
    for root in ROOTS:
        for d in reg.get_assets_by_path(root, recursive=True):
            pkg = str(d.package_name)
            if pkg in seen:
                continue
            cls = str(d.asset_class_path.asset_name)
            if cls not in ("AnimSequence", "AnimMontage", "BlendSpace", "BlendSpace1D", "AimOffsetBlendSpace", "AnimBlueprint",
                           "IKRetargeter", "IKRigDefinition", "Skeleton", "SkeletalMesh", "PoseAsset", "AnimComposite"):
                continue
            seen.add(pkg)
            row = {"path": pkg, "class": cls}
            if cls in ("AnimSequence", "AnimMontage"):
                a = EAL.load_asset(pkg)
                try:
                    row["len"] = round(a.get_play_length(), 4)
                    sk = a.get_editor_property("skeleton")
                    row["skel"] = sk.get_name() if sk else None
                    if cls == "AnimSequence":
                        nf = unreal.AnimationLibrary.get_num_frames(a)
                        row["frames"] = nf
                        row["additive"] = str(a.get_editor_property("additive_anim_type")).split(".")[-1].split(":")[0]
                        row["rootmotion"] = bool(a.get_editor_property("enable_root_motion"))
                        p0 = unreal.AnimationLibrary.get_bone_poses_for_frame(a, ["root"], 0, False)[0].translation
                        mx = 0.0
                        for f in range(0, nf + 1, max(1, nf // 12)):
                            p = unreal.AnimationLibrary.get_bone_poses_for_frame(a, ["root"], f, False)[0].translation
                            mx = max(mx, (p - p0).length())
                        pN = unreal.AnimationLibrary.get_bone_poses_for_frame(a, ["root"], nf, False)[0].translation
                        row["root_travel_max"] = round(mx, 1)
                        row["root_end"] = [round(pN.x - p0.x, 1), round(pN.y - p0.y, 1), round(pN.z - p0.z, 1)]
                        row["notifies"] = [str(n.notify_name) for n in a.get_editor_property("notifies")]
                except Exception as e:
                    row["err"] = str(e)
            elif cls == "SkeletalMesh":
                a = EAL.load_asset(pkg)
                sk = a.get_editor_property("skeleton")
                row["skel"] = sk.get_name() if sk else None
            rows.append(row)
    json.dump(rows, open(os.path.join(OUTDIR, "inventory.json"), "w"), indent=1)
    log("rows %d seqs %d" % (len(rows), sum(1 for r in rows if r["class"] == "AnimSequence")))
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [p.get_name() for p in d])
    log("OK")


main()
