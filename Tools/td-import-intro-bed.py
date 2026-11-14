# Triad Drift - #146: import the approved opening bed and bind it to DA_UI_Style.IntroOpeningBed. Owner: Teammate 3.
# Run headless, once, with the extracted export's path in TD_BED_SRC:
#   $env:TD_BED_SRC = "<path>\MUS_IntroAmbient_HerosWake_01.wav"
#   powershell -File Tools\td-python.ps1 -Script Tools\td-import-intro-bed.py
# Refuses unless the source's SHA256 is the selected Candidate01 export (issue #146, comment 6884748676). Imports it once
# at /Game/TriadDrift/Audio/IntroAmbient01/MUS_IntroAmbient_HerosWake_01 (a 20 s seamless loop: looping on; the music
# sound class the Settings music slider controls). Then sets ONLY IntroOpeningBed on the live style asset (the one
# UTDStatics::LoadPresentation("DA_UI_Style") resolves to: /Game/TriadDrift/Presentation/DA_UI_Style), reading every
# other UTDUIStyle property before and after and refusing to save if any of them changed.

import hashlib
import os
import re
import unreal

SHA256 = "0633e5eb56a3978e329c985a00e01cf8279b49d19ed4d3e5d3fee55979da72c8"
DEST_DIR = "/Game/TriadDrift/Audio/IntroAmbient01"
NAME = "MUS_IntroAmbient_HerosWake_01"
SOUND_CLASS = "/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.SC_MenuTheme_Music"
STYLE = "/Game/TriadDrift/Presentation/DA_UI_Style"
FIELD = "intro_opening_bed"

src = os.environ.get("TD_BED_SRC", "")
if not os.path.isfile(src):
    raise RuntimeError("TDPY: TD_BED_SRC is not a file: %r" % src)
digest = hashlib.sha256(open(src, "rb").read()).hexdigest()
if digest != SHA256:
    raise RuntimeError("TDPY: SHA256 mismatch: %s (expected %s)" % (digest, SHA256))

# 1. The sound.
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
wave.set_editor_property("looping", True)
sound_class = unreal.EditorAssetLibrary.load_asset(SOUND_CLASS)
if sound_class:
    wave.set_editor_property("sound_class_object", sound_class)
unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
print("TDPY: imported %s (sha256 %s), %.2f s, looping %s, class %s" % (path, digest, wave.get_editor_property("duration"),
      wave.get_editor_property("looping"), sound_class.get_path_name() if sound_class else "none"))

# 2. The binding: one field, every other property read back unchanged.
style = unreal.EditorAssetLibrary.load_asset(STYLE)
if not style:
    raise RuntimeError("TDPY: style asset missing: " + STYLE)
print("TDPY: style asset %s (class %s)" % (style.get_path_name(), style.get_class().get_name()))
header = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),
                      "Source/unrealGameJam/TriadDrift/Core/TDPresentation.h")
text = open(header, encoding="utf-8").read()
block = text[text.index("class UNREALGAMEJAM_API UTDUIStyle"):]
block = block[:block.index("\n};")]
names = re.findall(r"UPROPERTY\([^)]*\)\s*\n?\s*[\w<>:, ]+?\s+(\w+)\s*(?:=[^;]*)?;", block)


def snake(n):
    n = re.sub(r"^b(?=[A-Z])", "", n)
    return re.sub(r"(?<!^)(?=[A-Z])", "_", n).lower()


def snapshot():
    out = {}
    for n in names:
        try:
            out[n] = str(style.get_editor_property(snake(n)))
        except Exception as e:  # a property Python cannot read is compared by its error text
            out[n] = "unreadable: %s" % e
    return out


before = snapshot()
print("TDPY: %d style properties read" % len(before))
style.set_editor_property(FIELD, wave)
after = snapshot()
changed = [n for n in names if before[n] != after[n] and n != "IntroOpeningBed"]
if changed:
    raise RuntimeError("TDPY: refusing to save: other properties changed: %s" % changed)
if "IntroOpeningBed" not in names or after["IntroOpeningBed"] == before["IntroOpeningBed"]:
    raise RuntimeError("TDPY: the field did not take: %s" % after.get("IntroOpeningBed"))
unreal.EditorAssetLibrary.save_asset(STYLE, only_if_is_dirty=False)
reread = unreal.EditorAssetLibrary.load_asset(STYLE).get_editor_property(FIELD)
print("TDPY: %s.IntroOpeningBed = %s (was %s); %d other properties unchanged" % (STYLE, reread, before["IntroOpeningBed"],
      len(names) - 1))
print("TDPY: OK")
