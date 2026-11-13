# Fills gaps in the engineering fallback FX assets (sound fields only):
#  - creates DA_Ground_FX (UTDGroundFX) with the on-foot sounds the code used by path until now
#  - DA_BreakerBlaster_FX.Detonation.Sound (a splash weapon whose blast was silent)
#  - ImpactWorld.Sound on the three missile / mine weapons that lacked one
# Run headless: Tools\td-python.ps1 -Script ArtSource\EngAudio\scripts\ue_fill_fallback_gaps.py
import unreal

A = "/Game/TriadDrift/Gameplay/Audio/"
FX = "/Game/TriadDrift/Gameplay/Characters/FX/"
eal = unreal.EditorAssetLibrary


def snd(rel):
    path = A + rel
    if not eal.does_asset_exist(path):
        unreal.log("TDPY: MISSING sound " + path)
        return None
    return unreal.load_asset(path)


def set_cue_sound(asset, cue_name, sound, only_if_empty=True):
    cue = asset.get_editor_property(cue_name)
    cur = cue.get_editor_property("sound")
    cur_path = cur.get_path_name() if cur and hasattr(cur, "get_path_name") else str(cur)
    if only_if_empty and cur_path not in ("", "None"):
        unreal.log("TDPY: keep %s.%s = %s" % (asset.get_name(), cue_name, cur_path))
        return False
    cue.set_editor_property("sound", sound)
    asset.set_editor_property(cue_name, cue)
    unreal.log("TDPY: set %s.%s = %s" % (asset.get_name(), cue_name, sound.get_path_name()))
    return True


ok = True
# ---- DA_Ground_FX
ground_path = FX + "DA_Ground_FX"
cls = unreal.load_class(None, "/Script/unrealGameJam.TDGroundFX")
if cls is None:
    unreal.log("TDPY: FAIL no UTDGroundFX class")
    ok = False
else:
    if eal.does_asset_exist(ground_path):
        ground = unreal.load_asset(ground_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", cls)
        ground = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_Ground_FX", FX[:-1], cls, factory)
    slots = {
        "step_soil": "Foot/MSS_Foot_Step_Soil", "step_rock": "Foot/MSS_Foot_Step_Rock", "dash": "Foot/MSS_Foot_Dash", "jump": "Foot/MSS_Foot_Jump",
        "land": "Foot/MSS_Foot_Land", "hurt": "Foot/MSS_Player_Hurt", "crew_rifle_shot": "Crew/MSS_Crew_RifleShot", "drone_hum": "Crew/SW_Drone_Hum_01",
    }
    for slot, rel in slots.items():
        s = snd(rel)
        if s is None:
            ok = False
            continue
        set_cue_sound(ground, slot, s)
    eal.save_asset(ground_path, only_if_is_dirty=False)

# ---- weapon gaps
fills = [
    ("DA_BreakerBlaster_FX", "detonation", "Explosions/MSS_Detonation_Rocket"),
    ("DA_Enemy_HomingMissile_FX", "impact_world", "Impacts/MSS_Impact_World"),
    ("DA_Enemy_SalvoMissile_FX", "impact_world", "Impacts/MSS_Impact_World"),
    ("DA_Enemy_PlasmaMines_FX", "impact_world", "Impacts/MSS_Impact_World"),
]
for name, cue, rel in fills:
    asset = unreal.load_asset(FX + name)
    s = snd(rel)
    if asset is None or s is None:
        unreal.log("TDPY: FAIL " + name)
        ok = False
        continue
    try:
        if set_cue_sound(asset, cue, s):
            eal.save_asset(FX + name, only_if_is_dirty=False)
    except Exception as e:
        unreal.log("TDPY: FAIL %s.%s: %s" % (name, cue, e))
        ok = False

unreal.log("TDPY: OK" if ok else "TDPY: FAILED")
