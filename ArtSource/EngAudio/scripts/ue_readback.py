"""Triad Drift - read back the pass-2 sound slots and the 2D / pause settings of the sounds in them (run in the editor
through the ue-game-audio skill's ue_run_python.py). Returns a dict; "errors" lists anything that is not as intended."""
OUT = {"slots": {}, "sounds": {}, "errors": []}
FX = "/Game/TriadDrift/Gameplay/Characters/FX/DA_UI_Style"
CUES = ["UIMove", "UIConfirm", "UIBack", "PauseIn", "PauseOut", "SlateClap", "CutSting", "SlowMoIn", "SlowMoOut", "ObjectiveComplete", "HitConfirm", "KillConfirm"]
SOFT = ["AmbienceSpace", "AmbienceVerdance", "AmbienceCinder", "CombatMusic", "IntroMusic", "MenuMusic"]
PAUSE_SAFE = ["UIMove", "UIConfirm", "UIBack", "PauseIn", "PauseOut", "SlateClap", "CutSting", "SlowMoIn", "SlowMoOut", "ObjectiveComplete"]


def describe(sound):
    cls = sound.get_editor_property("sound_class_object")
    props = cls.get_editor_property("properties") if cls else None
    return {"class": cls.get_name() if cls else None, "ui_sound": bool(props.get_editor_property("is_ui_sound")) if props else None,
            "attenuation": sound.get_editor_property("attenuation_settings").get_name() if sound.get_editor_property("attenuation_settings") else None,
            "looping": bool(sound.get_editor_property("looping")) if isinstance(sound, unreal.SoundWave) else None,
            "volume": round(sound.get_editor_property("volume"), 3) if isinstance(sound, unreal.SoundWave) else None}


style = unreal.load_asset(FX + ".DA_UI_Style")
for field in CUES + SOFT:
    value = style.get_editor_property(field)
    sound = value.get_editor_property("Sound") if field in CUES else value
    sound = unreal.load_asset(str(sound.get_path_name())) if sound and not isinstance(sound, unreal.SoundBase) else sound
    OUT["slots"]["DA_UI_Style." + field] = sound.get_name() if sound else None
    if not sound:
        OUT["errors"].append("empty: DA_UI_Style." + field)
        continue
    info = describe(sound)
    OUT["sounds"][sound.get_name()] = info
    if field in PAUSE_SAFE and (info["attenuation"] or not info["ui_sound"]):
        OUT["errors"].append("%s is not a 2D UI-class sound: %s" % (field, info))
for ship in ("Rift", "Lancer", "Bastion"):
    asset = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Squad/DA_%s_Visuals.DA_%s_Visuals" % (ship, ship))
    sound = asset.get_editor_property("LowHullLoop").get_editor_property("Sound")
    sound = unreal.load_asset(str(sound.get_path_name())) if sound and not isinstance(sound, unreal.SoundBase) else sound
    OUT["slots"]["DA_%s_Visuals.LowHullLoop" % ship] = sound.get_name() if sound else None
    if sound:
        OUT["sounds"][sound.get_name()] = describe(sound)
    else:
        OUT["errors"].append("empty: DA_%s_Visuals.LowHullLoop" % ship)
OUT
