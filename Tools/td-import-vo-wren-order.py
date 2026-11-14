# Triad Drift - import the selected dry master for C8_Wren_Order (#135). Owner: Teammate 3.
# Run headless, once:
#   powershell -File Tools\td-python.ps1 -Script Tools\td-import-vo-wren-order.py
# Refuses unless the source SHA256 is exactly take 03 (dry), selected by the Lead of Nicholas (#135). Imports it once
# into the engineering audio folder (always cooked), sound class SC_TDE_Cine, no radio effect, and saves. Story: the
# line C8_Wren_Order ("Take out the reactor. Keep the route to the hangar clear.") falls back to this asset when the
# presentation transmissions asset has no sound for it. The Dont take 04 master is not touched.

import hashlib
import os
import unreal

SOURCE = "ArtSource/EngAI/Voice/Wren/present_order_r1/VO_PresentWren_Order_03_dry.wav"
SHA256 = "e5be90ba51c35c1580e76ad5f012224036983dea4d285d3a83082196c4fc0e24"
DEST_DIR = "/Game/TriadDrift/Gameplay/Audio/VO"
NAME = "SW_VO_PresentWren_Order_03"
SOUND_CLASS = "/Game/TriadDrift/Gameplay/Audio/Mix/SC_TDE_Cine.SC_TDE_Cine"

src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), SOURCE)
if not os.path.isfile(src):
    raise RuntimeError("TDPY: source missing (is PR #78 on main?): " + src)
digest = hashlib.sha256(open(src, "rb").read()).hexdigest()
if digest != SHA256:
    raise RuntimeError("TDPY: SHA256 mismatch: %s (expected %s)" % (digest, SHA256))
path = DEST_DIR + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(path):
    raise RuntimeError("TDPY: %s already exists; not importing twice" % path)

task = unreal.AssetImportTask()
task.filename = src
task.destination_path = DEST_DIR
task.destination_name = NAME
task.automated = True
task.replace_existing = False
task.save = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

wave = unreal.EditorAssetLibrary.load_asset(path)
if not wave:
    raise RuntimeError("TDPY: import failed")
sound_class = unreal.EditorAssetLibrary.load_asset(SOUND_CLASS)
if sound_class:
    wave.set_editor_property("sound_class_object", sound_class)
unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
print("TDPY: imported %s from %s (sha256 %s), %.2f s" % (path, SOURCE, digest, wave.get_editor_property("duration")))
print("TDPY: OK")
