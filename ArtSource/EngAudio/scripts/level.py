"""Triad Drift - level the raw takes to the mix standard and write the final 48 kHz 16-bit files (step 2 of 2).

    python ArtSource/EngAudio/scripts/level.py --raw <folder> [--only REGEX]

Uses the ue-game-audio skill's own measuring and writing code (ffmpeg), so the files are levelled exactly the way
audio_report.py then checks them: one linear gain per file, true peak for one-shots, loudness for loops and beds.
Also writes build/manifest.json: every wave, sound asset and cue slot, for the Unreal import (ue_build.py).
Plain Python 3 + ffmpeg; no numpy.
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPO = os.path.normpath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, os.path.join(REPO, ".claude", "skills", "ue-game-audio", "scripts"))
import audio_common as ac  # noqa: E402

PREFIX = {"sfx": "SFX", "ui": "SFX", "sting": "SFX", "loop": "SFX", "ambience": "AMB", "music": "MUS"}
GOAL = {"sfx": -1.2, "sting": -1.2, "ui": -3.2}   # a little under the ceiling: dither must not push a take over it
GAME = "/Game/TriadDrift/Gameplay/Audio"


def load():
    sounds = {}
    folder = os.path.join(ROOT, "recipes")
    for name in sorted(os.listdir(folder)):
        if name.endswith(".json"):
            with open(os.path.join(folder, name), encoding="utf-8") as handle:
                for entry in json.load(handle)["sounds"]:
                    sounds[entry["name"]] = entry
    return sounds


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", required=True)
    parser.add_argument("--only")
    args = parser.parse_args()
    sounds = load()
    notes = []
    for name, entry in sounds.items():
        if "alias" in entry or (args.only and not re.search(args.only, name)):
            continue
        cat = entry["cat"]
        plan = []
        for take in range(1, int(entry.get("takes", 1)) + 1):
            file_name = "%s_%s_%02d.wav" % (PREFIX[cat], name, take)
            source = os.path.join(args.raw, entry["group"], file_name)
            target = os.path.join(ROOT, "wav", entry["group"], file_name)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            result = ac.measure(source)
            if cat == "music":   # peak to -1.2 dBTP, but never louder than -15 LUFS (the standard allows -14 at most)
                gain = min(-1.2 - result["true_peak_dbtp"], -15.0 - result["loudness_lufs"])
                note = ""
            else:
                gain, note = ac.level_gain(result, cat, GOAL.get(cat))
            plan.append([source, target, gain, result["loudness_lufs"]])
            if note:
                notes.append("%s: %s" % (file_name, note))
        # takes of one sound: peak-normalised takes can differ in loudness; pull the louder ones down so the
        # set sits within about 1 LU (never more than 1.6 dB, which keeps the true peak inside the category window)
        if len(plan) > 1 and ac.TARGETS[cat]["norm"] == "peak":
            louds = [item[3] + item[2] for item in plan]
            floor = min(louds) + 1.0
            for item, loud in zip(plan, louds):
                item[2] -= min(1.6, max(0.0, loud - floor))
        for source, target, gain, _ in plan:
            ac.write_final(source, target, gain)
        print("levelled %s" % name, flush=True)
    for note in notes:
        print("NOTE " + note)

    # ---- manifest for Unreal
    waves, assets = [], []
    for name, entry in sounds.items():
        base = sounds[entry["alias"]] if "alias" in entry else entry
        cat = base["cat"]
        takes = int(base.get("takes", 1))
        folder = "%s/%s" % (GAME, base["group"])
        take_assets = ["%s/SW_%s_%02d" % (folder, base["name"], t) for t in range(1, takes + 1)]
        if "alias" not in entry:
            for t in range(1, takes + 1):
                waves.append({"file": "wav/%s/%s_%s_%02d.wav" % (entry["group"], PREFIX[cat], name, t),   # relative to ArtSource/EngAudio
                              "path": take_assets[t - 1], "cat": cat})
        ue = entry["ue"]
        one_shot = cat in ("sfx", "ui", "sting")
        multi = one_shot and (takes > 1 or "alias" in entry)
        # in-game level (mix-standard.md section 2): one-shots -6 + table value, loops and beds the table value itself
        volume_db = (-6.0 + ue["db"]) if one_shot else float(ue["db"])
        assets.append({
            "name": name, "cat": cat, "kind": "metasound" if multi else "wave",
            "path": ("%s/%s/MSS_%s" % (GAME, entry["group"], name)) if multi else take_assets[0],
            "waves": take_assets, "volume_db": volume_db, "looping": not one_shot and cat != "music" or name == "Music_Combat",
            "att": ue.get("att"), "con": ue.get("con"), "cls": ue.get("cls", "SFX"),
            "slots": entry.get("slots", []), "note": entry.get("note", base.get("note", ""))})
    os.makedirs(os.path.join(ROOT, "build"), exist_ok=True)
    with open(os.path.join(ROOT, "build", "manifest.json"), "w", encoding="utf-8") as handle:
        json.dump({"waves": waves, "assets": assets}, handle, indent=1)
    print("manifest: %d waves, %d sound assets" % (len(waves), len(assets)))


if __name__ == "__main__":
    main()
