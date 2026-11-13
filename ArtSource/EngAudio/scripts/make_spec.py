"""Triad Drift - turn build/manifest.json into the specs the Unreal steps read.

    python ArtSource/EngAudio/scripts/make_spec.py

Writes build/spec_build_NN.json for the ue-game-audio skill's ue_audio_build.py (mix assets, wave imports,
MetaSound one-shots: random take, +/-0.7 semitone, +/-1.5 dB) and build/spec_wire.json for ue_wire.py
(the fallback data assets and their Sound slots).
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
MIX = "/Game/TriadDrift/Gameplay/Audio/Mix"
FX_FOLDER = "/Game/TriadDrift/Gameplay/Characters/FX"
# Nick's existing music class (read-only reference): the Settings music slider already scales it, so the
# fallback music follows that slider with no code change.
MUSIC_CLASS = "/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music"

ATT = {  # mix-standard.md section 5: full level inside, silent beyond, low-pass at the far end, non-spatialised inside
    "Space_Near": (2500, 20000, 4000, 2500), "Space_Medium": (3000, 60000, 2500, 2500), "Space_Far": (8000, 200000, 1500, 2500),
    "Ground_Near": (300, 3000, 3000, 150), "Ground_Medium": (600, 8000, 2500, 150)}
CON = {  # max voices, rule, retrigger seconds
    "Weapon": (6, "StopQuietest", 0.03), "EnemyWeapon": (6, "StopQuietest", 0.03), "Impact": (8, "StopQuietest", 0.02),
    "Explosion": (4, "StopQuietest", 0.05), "Loop": (12, "StopFarthestThenOldest", 0.0), "Footstep": (4, "StopOldest", 0.05),
    "Power": (4, "StopOldest", 0.05), "Ship": (6, "StopQuietest", 0.03), "Blade": (4, "StopOldest", 0.03), "UITick": (3, "StopOldest", 0.04)}
CLASSES = ["SFX", "UI", "Cine", "Ambience"]
SIGNATURE = ("Cine", "UI")   # signature sounds: no random pitch or level

# Presentation names with no asset anywhere yet: created as engineering fallbacks (class by name)
CREATE = {"TDWeaponFX": ["DA_PhasePulseLasers_FX", "DA_SplitterBolt_FX", "DA_NeedleCannons_FX", "DA_SpikeRockets_FX", "DA_BreakerBlaster_FX",
                         "DA_FlakBurst_FX", "DA_MissileSwarm_FX", "DA_Enemy_PulseGuns_FX", "DA_Enemy_HeavyBolt_FX", "DA_Enemy_HomingMissile_FX",
                         "DA_Enemy_SalvoMissile_FX", "DA_Enemy_PlasmaMines_FX", "DA_Enemy_PointDefense_FX"],
          "TDAbilityFX": ["DA_Ability_Cloak_FX", "DA_Ability_EMPPulse_FX", "DA_Ability_GravityWell_FX"],
          "TDTravelFX": ["DA_Travel_FX"], "TDBladeFX": ["DA_Blade_FX"], "TDUIStyle": ["DA_UI_Style"]}
SOFT_SOUND_FIELDS = ("EngineLoop", "CombatMusic", "IntroMusic")   # plain TSoftObjectPtr<USoundBase>, not an FTDFXCue


def class_path(name):
    return MUSIC_CLASS if name == "Music" else "%s/SC_TDE_%s" % (MIX, name)


def sound_block(asset):
    block = {"soundClassObject": class_path(asset["cls"])}
    if asset["att"]:
        block["attenuationSettings"] = "%s/ATT_TDE_%s" % (MIX, asset["att"])
    if asset["con"]:
        block["concurrencySet"] = ["%s/SCON_TDE_%s" % (MIX, asset["con"])]
    return block


def main():
    with open(os.path.join(ROOT, "build", "manifest.json"), encoding="utf-8") as handle:
        manifest = json.load(handle)
    mix = [{"type": "SoundClass", "path": "%s/SC_TDE_Master" % MIX}]
    for name in CLASSES:
        props = {"parentClass": "%s/SC_TDE_Master" % MIX}
        if name in ("UI", "Cine"):
            props["properties.bIsUISound"] = True     # still plays while the game is paused
        mix.append({"type": "SoundClass", "path": "%s/SC_TDE_%s" % (MIX, name), "properties": props})
    for name, (inner, outer, lpf, flat) in ATT.items():
        mix.append({"type": "SoundAttenuation", "path": "%s/ATT_TDE_%s" % (MIX, name), "properties": {
            "attenuation.attenuationShapeExtents": {"x": inner, "y": 0, "z": 0}, "attenuation.falloffDistance": outer - inner,
            "attenuation.nonSpatializedRadiusStart": flat, "attenuation.bAttenuateWithLPF": True,
            "attenuation.lPFRadiusMin": inner, "attenuation.lPFRadiusMax": outer,
            "attenuation.lPFFrequencyAtMin": 20000, "attenuation.lPFFrequencyAtMax": lpf}})
    for name, (count, rule, retrigger) in CON.items():
        mix.append({"type": "SoundConcurrency", "path": "%s/SCON_TDE_%s" % (MIX, name), "properties": {
            "concurrency.maxCount": count, "concurrency.resolutionRule": rule, "concurrency.retriggerTime": retrigger,
            "concurrency.voiceStealReleaseTime": 0.03, "concurrency.bLimitToOwner": False}})

    wave_props = {}
    for asset in manifest["assets"]:
        if asset["kind"] == "wave":   # a single take is played as the Sound Wave itself and carries everything
            props = sound_block(asset)
            props["volume"] = round(10 ** (asset["volume_db"] / 20.0), 4)
            props["bLooping"] = bool(asset["looping"])
            wave_props[asset["path"]] = props
    imports = [{"file": wave["file"], "path": wave["path"], "properties": wave_props.get(wave["path"])} for wave in manifest["waves"]]
    oneshots = []
    for asset in manifest["assets"]:
        if asset["kind"] == "metasound":
            signature = asset["cls"] in SIGNATURE
            oneshots.append({"path": asset["path"], "format": "Mono", "sounds": asset["waves"], "volume_db": asset["volume_db"],
                             "volume_spread_db": 0 if signature else 1.5, "pitch_semitones": 0, "pitch_spread": 0 if signature else 0.7,
                             "sound": sound_block(asset)})

    specs = [{"mix": mix}]
    for i in range(0, len(imports), 40):
        specs.append({"import": imports[i:i + 40]})
    for i in range(0, len(oneshots), 12):
        specs.append({"oneshots": oneshots[i:i + 12]})
    for old in os.listdir(os.path.join(ROOT, "build")):
        if old.startswith("spec_build_"):
            os.remove(os.path.join(ROOT, "build", old))
    for i, spec in enumerate(specs):
        with open(os.path.join(ROOT, "build", "spec_build_%02d.json" % i), "w", encoding="utf-8") as handle:
            json.dump(spec, handle, indent=1)

    slots = []
    for asset in manifest["assets"]:
        for slot in asset["slots"]:
            data_asset, field = slot.split(".")
            slots.append({"asset": data_asset, "field": field, "soft": field in SOFT_SOUND_FIELDS, "sound": asset["path"]})
    wire = {"folder": FX_FOLDER, "create": CREATE, "slots": slots,
            # DA_Blade_FX did not exist: gameplay then used this colour. A new asset would default to white, so keep the look.
            "keep": [{"asset": "DA_Blade_FX", "field": "BladeColor", "color": [0.1, 0.65, 1.0, 1.0]}]}
    with open(os.path.join(ROOT, "build", "spec_wire.json"), "w", encoding="utf-8") as handle:
        json.dump(wire, handle, indent=1)
    print("%d build specs (%d mix assets, %d waves, %d MetaSound one-shots); %d slots on %d data assets" % (
        len(specs), len(mix), len(imports), len(oneshots), len(slots), len({s["asset"] for s in slots})))


if __name__ == "__main__":
    main()
