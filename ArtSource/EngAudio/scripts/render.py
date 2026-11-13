"""Triad Drift - render every recipe to raw 32-bit float takes (step 1 of 2; step 2 is level.py).

    "C:/Program Files/Blender Foundation/Blender 5.2/5.2/python/bin/python.exe" ArtSource/EngAudio/scripts/render.py --raw <folder> [--only REGEX]

Needs numpy (Blender's bundled Python has it). The same recipe and take number always give the same file.
"""
import argparse
import os
import re
import sys
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import td_music  # noqa: E402
import td_synth as ts  # noqa: E402

PREFIX = {"sfx": "SFX", "ui": "SFX", "sting": "SFX", "loop": "SFX", "ambience": "AMB", "music": "MUS"}


def take_name(entry, take):
    return "%s_%s_%02d.wav" % (PREFIX[entry["cat"]], entry["name"], take)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", required=True, help="folder for the raw float takes")
    parser.add_argument("--only", help="regular expression on the sound name")
    args = parser.parse_args()
    sounds = ts.load_recipes(os.path.join(HERE, "..", "recipes"))
    count = 0
    for name, entry in sounds.items():
        if "alias" in entry or (args.only and not re.search(args.only, name)):
            continue
        started = time.time()
        base = zlib.crc32(name.encode("utf-8")) % 100000
        kind = entry.get("kind", "shot")
        takes = int(entry.get("takes", 1))
        for take in range(1, takes + 1):
            seed = base * 10 + take
            if kind == "shot":
                audio = ts.render_oneshot(entry, seed)
            elif kind == "loop":
                audio = ts.render_loop(entry, seed)
            else:
                audio = td_music.PIECES[entry["piece"]](entry["spec"])
            ts.write_f32(os.path.join(args.raw, entry["group"], take_name(entry, take)), audio)
            count += 1
        print("%-30s %d take(s) %.1fs" % (name, takes, time.time() - started), flush=True)
    print("rendered %d file(s)" % count)


if __name__ == "__main__":
    main()
