# Crew anims step 3: delete the Crew/Anims/Tmp working copies once nothing references them.
import unreal

EAL = unreal.EditorAssetLibrary
TMP = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/Tmp"
ok = True
for p in EAL.list_assets(TMP, recursive=True):
    pkg = p.split(".")[0]
    refs = EAL.find_package_referencers_for_asset(pkg, False)
    unreal.log("TDPY: %s referencers %s" % (pkg, list(refs)))
    if refs:
        ok = False
        continue
    unreal.log("TDPY: delete %s %s" % (pkg, EAL.delete_asset(pkg)))
if ok:
    unreal.log("TDPY: delete dir %s" % EAL.delete_directory(TMP))
d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
unreal.log("TDPY: dirty packages: %s" % [x.get_name() for x in d])
unreal.log("TDPY: OK")
