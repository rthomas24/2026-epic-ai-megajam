# SPDX-License-Identifier: MIT
# Read-only probe for the enemy-engine audio intake (#151): the player ships' engine loop settings, the two enemy
# visuals data assets' EngineTrail cue and their other fields, and which headless cue-building APIs exist. Saves nothing.
import json

import unreal

eas = unreal.EditorAssetLibrary
out = {}


def name(o):
    return o.get_path_name() if o else None


for p in ("/Game/TriadDrift/Gameplay/Audio/Engines/SW_Engine_Rift_01", "/Game/TriadDrift/Gameplay/Audio/Engines/SW_Engine_Lancer_01"):
    w = eas.load_asset(p)
    row = {}
    for prop in ("looping", "attenuation_settings", "sound_class_object", "concurrency_set", "volume", "pitch", "virtualization_mode"):
        try:
            v = w.get_editor_property(prop)
            row[prop] = [name(x) for x in v] if isinstance(v, (list, unreal.Array)) else (name(v) if isinstance(v, unreal.Object) else str(v))
        except Exception as e:
            row[prop] = "n/a (%s)" % e
    out[p] = row
for p in ("/Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_Fighter_Visuals", "/Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_Interceptor_Visuals"):
    da = eas.load_asset(p)
    row = {"class": da.get_class().get_name()}
    try:
        et = da.get_editor_property("engine_trail")
        row["engine_trail"] = {k: str(et.get_editor_property(k)) for k in ("sound", "niagara") if hasattr(et, "get_editor_property")}
    except Exception as e:
        row["engine_trail"] = "n/a (%s)" % e
    try:
        row["export_text_len"] = len(unreal.EditorAssetLibrary.get_metadata_tag_values(da) or {})
    except Exception:
        pass
    out[p] = row
out["apis"] = {k: hasattr(unreal, k) for k in ("SoundCue", "SoundNodeRandom", "SoundNodeWavePlayer", "SoundNodeModulator",
                                                "MetaSoundBuilderSubsystem", "MetaSoundEditorSubsystem", "SoundCueGraphNode")}
unreal.log("TDPY: PROBE " + json.dumps(out))
unreal.log("TDPY: OK")
