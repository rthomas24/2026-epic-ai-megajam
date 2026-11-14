# Triad Drift - import Nicholas's chosen master for the shared "Don't-" (N-097). Owner: Teammate 3.
# Run headless, only once the file is on origin/main (PR #78):
#   powershell -File Tools\td-python.ps1 -Script Tools\td-import-vo-dont.py
# Refuses unless the source's SHA256 is exactly the selected take 04 radio master. Imports it once into the engineering
# audio folder (always cooked), sound class SC_TDE_Cine, and saves. Story: SharedSounds[Dont] falls back to this asset
# (UTDStorySubsystem, used by O_Unknown_Dont and F_FutureWren_Dont) when Nick's DA_Story_Transmissions has no entry.

import hashlib
import os
import unreal

SOURCE = "ArtSource/EngAI/Voice/Wren/audition_r1/VO_FutureWren_Dont_04_radio.wav"
SHA256 = "e5491da76ab449fe3d2701e98a0b0f11e7a529c2f2becb040c9f56bf89abc19a"
DEST_DIR = "/Game/TriadDrift/Gameplay/Audio/VO"
NAME = "SW_VO_FutureWren_Dont_04"
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
