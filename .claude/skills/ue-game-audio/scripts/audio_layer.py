#!/usr/bin/env python3
"""Triad Drift - build one sound from a JSON recipe of layers, as numbered takes.

    python audio_layer.py ArtSource/Audio/SFX/Weapons/Lancer_Cannon/recipe.json --out ArtSource/Audio/SFX/Weapons/Lancer_Cannon/export

The recipe is the reviewable, repeatable description of a designed sound (the recorded-material
version of a synthesis recipe): which files, where each starts, how loud, how pitched, how filtered.
Same recipe + same seed + same sources = the same files.

    {
      "name": "SFX_Weapon_Lancer_Cannon",      output files are <name>_01.wav ...
      "category": "sfx",                       level target and checks (audio_report.py --targets)
      "takes": 4,                              how many variations to render
      "seed": 7,                               change to get a different set of variations
      "channels": 1,                           optional; default = the category's first allowed count
      "layers": [
        {
          "role": "transient",                 free text for the reader: transient | body | tail | sweetener ...
          "file": "src/snap.wav",              or "files": ["a.wav", "b.wav"] : takes rotate through them
          "start_ms": 0,                       where the layer starts in the result
          "gain_db": -3,
          "pitch_semitones": 5,                tape-style: pitch and length change together
          "trim_start_ms": 0, "duration_ms": 120,   optional: use only this part of the source
          "highpass_hz": 800, "lowpass_hz": 0,      0 or absent = off (12 dB/octave each)
          "fade_in_ms": 0, "fade_out_ms": 40,
          "reverse": false,
          "vary": {"pitch_semitones": 0.5, "gain_db": 1.0, "start_ms": 4}   random +/- range per take
        }
      ],
      "master": {"highpass_hz": 30, "lowpass_hz": 0, "fade_out_ms": 30, "max_duration_ms": 0, "keep_tail": false}
    }

File paths are relative to the recipe. Layers are summed without any automatic gain change; the sum
is then brought to the category target with one linear gain (see audio_prep.py), the dead tail below
-58 dBFS is cut (one-shots only; "keep_tail": true keeps it), the master fade out is applied, and the
result is written as 48 kHz / 16-bit WAV. A <name>.layers.json next to the takes records the exact values used per take.

Exit code: 0 = every take passes its category, 1 = at least one fails, 2 = usage / recipe / tool error.
"""
import argparse
import json
import os
import random
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402


def layer_chain(layer, values, layout):
    """The ffmpeg filter chain of one layer, with this take's resolved values."""
    parts = ["aresample=%d" % ac.SAMPLE_RATE, "aformat=sample_fmts=fltp:channel_layouts=%s" % layout]
    trim_start = float(layer.get("trim_start_ms", 0)) / 1000.0
    duration = float(layer.get("duration_ms", 0)) / 1000.0
    if trim_start > 0 or duration > 0:
        trim = "atrim=start=%f" % trim_start
        if duration > 0:
            trim += ":duration=%f" % duration
        parts += [trim, "asetpts=PTS-STARTPTS"]
    if layer.get("reverse"):
        parts.append("areverse")
    if abs(values["pitch_semitones"]) > 1e-6:
        ratio = 2.0 ** (values["pitch_semitones"] / 12.0)
        parts += ["asetrate=%d" % round(ac.SAMPLE_RATE * ratio), "aresample=%d" % ac.SAMPLE_RATE]
    if float(layer.get("highpass_hz", 0)) > 0:
        parts.append("highpass=f=%g:poles=2" % float(layer["highpass_hz"]))
    if float(layer.get("lowpass_hz", 0)) > 0:
        parts.append("lowpass=f=%g:poles=2" % float(layer["lowpass_hz"]))
    if float(layer.get("fade_in_ms", 0)) > 0:
        parts.append("afade=t=in:d=%f:curve=tri" % (float(layer["fade_in_ms"]) / 1000.0))
    if float(layer.get("fade_out_ms", 0)) > 0:
        parts.append("areverse,afade=t=in:d=%f:curve=qsin,areverse" % (float(layer["fade_out_ms"]) / 1000.0))
    parts.append("volume=%.3fdB" % values["gain_db"])
    if values["start_ms"] > 0:
        parts.append("adelay=%d:all=1" % round(values["start_ms"]))
    return ",".join(parts)


def resolve(layer, index, take, rng):
    """Picks this take's file and random offsets for one layer."""
    files = layer.get("files") or ([layer["file"]] if layer.get("file") else [])
    if not files:
        raise ac.AudioError("layer %d has no file" % (index + 1))
    vary = layer.get("vary", {})
    spread = lambda key: rng.uniform(-float(vary.get(key, 0)), float(vary.get(key, 0)))
    return dict(
        file=files[(take + index) % len(files)],
        pitch_semitones=float(layer.get("pitch_semitones", 0)) + spread("pitch_semitones"),
        gain_db=float(layer.get("gain_db", 0)) + spread("gain_db"),
        start_ms=max(0.0, float(layer.get("start_ms", 0)) + spread("start_ms")))


def main():
    parser = argparse.ArgumentParser(description="Build one sound from a JSON recipe of layers, as numbered takes.")
    parser.add_argument("recipe", help="recipe JSON file")
    parser.add_argument("--out", required=True, help="output folder (created)")
    parser.add_argument("--takes", type=int, help="override the recipe's number of takes")
    parser.add_argument("--seed", type=int, help="override the recipe's seed")
    args = parser.parse_args()

    try:
        ac.require_tools()
        with open(args.recipe, encoding="utf-8") as handle:
            recipe = json.load(handle)
        base = os.path.dirname(os.path.abspath(args.recipe))
        name, category = recipe["name"], recipe["category"]
        if category not in ac.TARGETS:
            raise ac.AudioError("unknown category %r" % category)
        layers = recipe.get("layers") or []
        if not layers:
            raise ac.AudioError("the recipe has no layers")
        takes = args.takes or int(recipe.get("takes", 1))
        seed = int(recipe.get("seed", 1) if args.seed is None else args.seed)
        channels = int(recipe.get("channels", ac.TARGETS[category]["channels"][0]))
        layout = "mono" if channels == 1 else "stereo"
        master = recipe.get("master", {})
        os.makedirs(args.out, exist_ok=True)
        rows, log = [], []
        with tempfile.TemporaryDirectory(prefix="td_audio_layer_") as work:
            for take in range(takes):
                rng = random.Random(seed * 1000 + take)
                inputs, graph, labels, used = [], [], [], []
                for index, layer in enumerate(layers):
                    values = resolve(layer, index, take, rng)
                    path = values["file"] if os.path.isabs(values["file"]) else os.path.join(base, values["file"])
                    if not os.path.isfile(path):
                        raise ac.AudioError("layer %d: file not found: %s" % (index + 1, path))
                    inputs += ["-i", path]
                    graph.append("[%d:a]%s[l%d]" % (index, layer_chain(layer, values, layout), index))
                    labels.append("[l%d]" % index)
                    used.append(dict(role=layer.get("role", ""), file=values["file"], start_ms=round(values["start_ms"], 1),
                                     gain_db=round(values["gain_db"], 2), pitch_semitones=round(values["pitch_semitones"], 2)))
                post = []
                if float(master.get("highpass_hz", 0)) > 0:
                    post.append("highpass=f=%g:poles=2" % float(master["highpass_hz"]))
                if float(master.get("lowpass_hz", 0)) > 0:
                    post.append("lowpass=f=%g:poles=2" % float(master["lowpass_hz"]))
                if float(master.get("max_duration_ms", 0)) > 0:
                    post.append("atrim=end=%f" % (float(master["max_duration_ms"]) / 1000.0))
                mix = "amix=inputs=%d:normalize=0:duration=longest" % len(labels) if len(labels) > 1 else "anull"
                graph.append("%s%s[out]" % ("".join(labels), ",".join([mix] + post)))
                summed = os.path.join(work, "sum_%02d.wav" % take)
                ac.ffmpeg(inputs + ["-filter_complex", ";".join(graph), "-map", "[out]", "-ac", str(channels),
                                    "-ar", str(ac.SAMPLE_RATE), "-c:a", "pcm_f32le", summed])
                measured = ac.measure(summed)
                if measured["silent"]:
                    raise ac.AudioError("take %d is silent: check the layers" % (take + 1))
                gain, note = ac.level_gain(measured, category)
                out_path = os.path.join(args.out, "%s_%02d.wav" % (name, take + 1))
                keep_tail = ac.TARGETS[category]["loop"] or category == "music" or bool(master.get("keep_tail"))
                ac.write_final(summed, out_path, gain, trim_tail_db=None if keep_tail else -58.0,
                               fade_out_ms=float(master.get("fade_out_ms", 0)))
                after = ac.measure(out_path)
                if ac.TARGETS[category]["loop"]:
                    ac.add_seam(after)
                fails = ac.check(after, category)
                rows.append((after, gain, note, fails, used))
                log.append(dict(take=take + 1, output=os.path.basename(out_path), sum_true_peak_dbtp=measured["true_peak_dbtp"],
                                gain_db=round(gain, 2), note=note, layers=used, after=after, fails=fails))
    except (ac.AudioError, KeyError, ValueError) as error:
        ac.fail("%s: %s" % (type(error).__name__, error))

    for after, gain, note, fails, used in rows:
        verdict = "PASS" if not fails else "FAIL: " + "; ".join(fails)
        print("%s  %.3f s  TP %s  %s LUFS  gain %+.1f dB  %s%s" % (
            os.path.basename(after["file"]), after["duration"], ac.fmt(after["true_peak_dbtp"]), ac.fmt(after["loudness_lufs"]),
            gain, verdict, ("  [" + note + "]") if note else ""))
        for layer in used:
            print("    %-10s %-28s start %6.1f ms  %+5.1f dB  %+5.2f st" % (
                layer["role"][:10], os.path.basename(layer["file"])[:28], layer["start_ms"], layer["gain_db"], layer["pitch_semitones"]))

    group_fail = ""
    louds = [row[0]["loudness_lufs"] for row in rows if row[0]["loudness_lufs"] is not None]
    if len(louds) >= 2 and max(louds) - min(louds) > ac.TAKE_SPREAD_MAX_LU:
        group_fail = "takes spread %.1f LU (max %g): reduce the vary ranges" % (max(louds) - min(louds), ac.TAKE_SPREAD_MAX_LU)
        print("FAIL (group): " + group_fail)
    sidecar = os.path.join(args.out, name + ".layers.json")
    with open(sidecar, "w", encoding="utf-8") as handle:
        json.dump(dict(recipe=os.path.abspath(args.recipe), seed=seed, category=category, takes=log, group_fail=group_fail), handle, indent=1)
    failed = sum(1 for row in rows if row[3]) + (1 if group_fail else 0)
    print("\n%d take(s) written to %s, %d failure(s) against '%s'. Log: %s" % (len(rows), args.out, failed, category, sidecar))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
