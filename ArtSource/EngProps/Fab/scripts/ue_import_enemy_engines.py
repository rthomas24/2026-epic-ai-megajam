# SPDX-License-Identifier: MIT
# Triad Drift - #151 enemy engine loops (Kenney Sci-Fi Sounds, CC0; prepared by Nick's side as EnemyEngines Candidate01),
# headless (Tools\td-python.ps1). TD_EE_DRY=1 reports without saving.
#   1. imports the six takes as SW_EnemyEngine_{Fighter,Interceptor}_Loop_0{1,2,3} in Gameplay/Audio/Engines/EnemyEngines01/:
#      looping, volume 0.28, the player engines' presets unedited (ATT_TDE_Space_Near, SC_TDE_SFX, SCON_TDE_Loop),
#      virtualization Restart;
#   2. builds CUE_EnemyEngine_<Type>: Modulator (pitch 0.96-1.04, volume -1.5..+1.5 dB) -> Random (no immediate repeat)
#      -> three looping wave players; if the cue graph cannot be set from Python the take 01 wave is bound instead;
#   3. binds only EngineTrail.Sound on DA_Enemy_Fighter_Visuals / DA_Enemy_Interceptor_Visuals: the full text export
#      before and after must differ only in that field, or nothing is saved.
import difflib
import json
import os
import re

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(os.path.dirname(HERE), "EnemyEngines01", "source")
OUT = os.path.join(os.path.dirname(HERE), "EnemyEngines01")
DEST = "/Game/TriadDrift/Gameplay/Audio/Engines/EnemyEngines01"
MIX = "/Game/TriadDrift/Gameplay/Audio/Mix/"
DAS = {"Fighter": "/Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_Fighter_Visuals",
       "Interceptor": "/Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_Interceptor_Visuals"}
DRY = os.environ.get("TD_EE_DRY", "") == "1"
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
fails, notes, report = [], [], {}


def log(m):
    unreal.log("TDPY: " + m)


att = eas.load_asset(MIX + "ATT_TDE_Space_Near")
cls = eas.load_asset(MIX + "SC_TDE_SFX")
con = eas.load_asset(MIX + "SCON_TDE_Loop")
if not (att and cls and con):
    fails.append("presets missing")


def export_text(obj, tag):
    path = os.path.join(OUT, "readback", "%s_%s.copy" % (obj.get_name(), tag))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    task = unreal.AssetExportTask()
    task.set_editor_property("object", obj)
    task.set_editor_property("filename", path)
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    ok = unreal.Exporter.run_asset_export_task(task)
    if not ok or not os.path.isfile(path):
        return None
    return open(path, encoding="utf-8", errors="replace").read()


# ---- 1. waves
if not eas.does_directory_exist(DEST):
    eas.make_directory(DEST)
waves = {}
tasks = []
for t in ("Fighter", "Interceptor"):
    for k in ("01", "02", "03"):
        task = unreal.AssetImportTask()
        for a, v in (("filename", os.path.join(SRC, "SW_EnemyEngine_%s_Loop_%s.wav" % (t, k))), ("destination_path", DEST),
                     ("automated", True), ("replace_existing", True), ("save", False)):
            task.set_editor_property(a, v)
        tasks.append(task)
tools.import_asset_tasks(tasks)
for t in ("Fighter", "Interceptor"):
    waves[t] = []
    for k in ("01", "02", "03"):
        w = eas.load_asset("%s/SW_EnemyEngine_%s_Loop_%s" % (DEST, t, k))
        if w is None:
            fails.append("wave %s %s" % (t, k))
            continue
        w.set_editor_property("looping", True)
        w.set_editor_property("volume", 0.28)
        w.set_editor_property("attenuation_settings", att)
        w.set_editor_property("sound_class_object", cls)
        w.set_editor_property("concurrency_set", [con])
        w.set_editor_property("virtualization_mode", unreal.VirtualizationMode.RESTART)
        if not DRY:
            eas.save_loaded_asset(w, False)
        waves[t].append(w)
    report[t] = {"waves": [w.get_name() for w in waves[t]], "seconds": [round(w.get_editor_property("duration"), 3) for w in waves[t]]}


# ---- 2. cue (falls back to take 01)
def build_cue(t):
    name = "CUE_EnemyEngine_" + t
    path = "%s/%s" % (DEST, name)
    try:
        cue = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(name, DEST, unreal.SoundCue, unreal.SoundCueFactoryNew())
        players = []
        for w in waves[t]:
            p = unreal.new_object(unreal.SoundNodeWavePlayer, outer=cue)
            p.set_editor_property("sound_wave", w)
            p.set_editor_property("looping", True)
            players.append(p)
        rnd = unreal.new_object(unreal.SoundNodeRandom, outer=cue)
        rnd.set_editor_property("weights", [1.0] * len(players))
        rnd.set_editor_property("randomize_without_replacement", True)
        rnd.set_editor_property("child_nodes", players)
        mod = unreal.new_object(unreal.SoundNodeModulator, outer=cue)
        for a, v in (("pitch_min", 0.96), ("pitch_max", 1.04), ("volume_min", 0.841), ("volume_max", 1.189)):
            mod.set_editor_property(a, v)
        mod.set_editor_property("child_nodes", [rnd])
        cue.set_editor_property("first_node", mod)
        cue.set_editor_property("attenuation_settings", att)
        cue.set_editor_property("sound_class_object", cls)
        cue.set_editor_property("concurrency_set", [con])
        # read the graph back: first node -> modulator -> random with three looping players
        fn = cue.get_editor_property("first_node")
        kids = fn.get_editor_property("child_nodes")[0].get_editor_property("child_nodes") if fn else []
        if not fn or len(kids) != 3:
            raise RuntimeError("graph did not read back (%s, %d players)" % (fn, len(kids)))
        if not DRY:
            eas.save_loaded_asset(cue, False)
        return cue, "cue (modulator -> random -> 3 looping takes)"
    except Exception as e:
        notes.append("%s: cue graph not settable headless (%s); bound take 01" % (t, e))
        if eas.does_asset_exist(path):
            eas.delete_asset(path)
        return waves[t][0], "fallback: take 01 wave"


bound = {}
for t in ("Fighter", "Interceptor"):
    if waves.get(t):
        snd, how = build_cue(t)
        bound[t] = snd
        report[t]["bound"] = snd.get_path_name()
        report[t]["how"] = how

# ---- 3. bind EngineTrail.Sound with a full-text readback
for t, path in DAS.items():
    da = eas.load_asset(path)
    before = export_text(da, "before")
    trail = da.get_editor_property("engine_trail")
    old = trail.get_editor_property("sound")
    trail.set_editor_property("sound", bound[t])
    da.set_editor_property("engine_trail", trail)
    after = export_text(da, "after")
    if before is None or after is None:
        fails.append("%s: text export failed" % t)
        continue
    diff = [l for l in difflib.unified_diff(before.splitlines(), after.splitlines(), lineterm="", n=0)
            if l[:1] in "+-" and not l.startswith(("+++", "---"))]
    # everything except the Sound value inside EngineTrail must be identical: strip that one value and compare
    # (an empty Sound is not exported at all, so the whole "Sound=...," entry is removed before comparing)
    strip = lambda s: re.sub(r'(EngineTrail=\(.*?)Sound=("[^"]*"|[^,)]*),?', r"\1", s)
    other = [l for l in difflib.unified_diff([strip(x) for x in before.splitlines()], [strip(x) for x in after.splitlines()],
                                             lineterm="", n=0) if l[:1] in "+-" and not l.startswith(("+++", "---"))]
    report[t]["old_sound"] = str(old)
    report[t]["diff"] = diff
    if other:
        fails.append("%s: other fields changed: %s" % (t, other[:4]))
        continue
    if not DRY:
        if not eas.save_loaded_asset(da, False):
            fails.append("save " + t)
log("REPORT " + json.dumps(report))
for n in notes:
    log("NOTE " + n)
log("OK" if not fails else "FAILED %s" % fails)
