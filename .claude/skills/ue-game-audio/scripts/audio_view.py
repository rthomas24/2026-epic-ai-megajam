#!/usr/bin/env python3
"""Triad Drift - write a picture of a sound (spectrogram over waveform) so an agent can look at it.

    python audio_view.py export/SFX_Weapon_Lancer_Cannon_01.wav                 # -> ..._01.png next to it
    python audio_view.py export/ --out review/                                  # every file in a folder
    python audio_view.py a.wav b.wav --sheet review/compare.png                 # one contact sheet, same scale

Top: spectrogram, frequency on a log axis (low at the bottom), level as colour over a 72 dB range.
Bottom: waveform, linear amplitude. Time runs left to right across the whole file; with --sheet every
row is padded to the longest file, so lengths can be compared by eye.

What a picture can show: where the energy is (sub / low-mid / presence / air), how fast the attack
is, how long the tail rings, clicks at the edges, a loop whose two ends do not match, takes that are
copies of each other. What it cannot show: whether the sound is good. Read the PNG with your image
viewer (or MediaToolset.ReadImageFile through Unreal's MCP server).
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402


def picture_graph(index, width, height, pad_seconds=None):
    """Filter graph for one input: spectrogram stacked on a waveform. Returns (graph, output label)."""
    spec_h = int(height * 0.72) // 2 * 2
    wave_h = height - spec_h
    source = "[%d:a]aformat=channel_layouts=mono" % index
    if pad_seconds:
        source += ",apad=whole_dur=%f" % pad_seconds
    graph = ("%s,asplit=2[sa%d][wa%d];"
             "[sa%d]showspectrumpic=s=%dx%d:legend=0:scale=log:fscale=log:drange=72:color=intensity[sp%d];"
             "[wa%d]showwavespic=s=%dx%d:colors=0x66ccff:scale=lin[wv%d];"
             "[sp%d][wv%d]vstack=inputs=2[pic%d]" % (source, index, index, index, width, spec_h, index,
                                                      index, width, wave_h, index, index, index, index))
    return graph, "[pic%d]" % index


def main():
    parser = argparse.ArgumentParser(description="Write spectrogram + waveform PNGs of audio files.")
    parser.add_argument("paths", nargs="+", help="audio files or folders")
    parser.add_argument("--out", help="folder for the PNGs (default: next to each file)")
    parser.add_argument("--sheet", metavar="PNG", help="write ONE image with all files stacked, on a shared time scale")
    parser.add_argument("--width", type=int, default=1200)
    parser.add_argument("--height", type=int, default=420, help="height per file")
    args = parser.parse_args()
    try:
        ac.require_tools()
        files = ac.list_audio(args.paths)
        if not files:
            ac.fail("no audio files found")
        written = []
        if args.sheet:
            if len(files) > 12:
                raise ac.AudioError("a sheet holds at most 12 files (got %d)" % len(files))
            longest = max(ac.probe(path)["duration"] for path in files)
            inputs, graphs, labels = [], [], []
            for index, path in enumerate(files):
                inputs += ["-i", path]
                graph, label = picture_graph(index, args.width, args.height, longest)
                graphs.append(graph)
                labels.append(label)
            if len(labels) > 1:
                graphs.append("%svstack=inputs=%d[sheet]" % ("".join(labels), len(labels)))
                final = "[sheet]"
            else:
                final = labels[0]
            os.makedirs(os.path.dirname(os.path.abspath(args.sheet)), exist_ok=True)
            ac.ffmpeg(inputs + ["-filter_complex", ";".join(graphs), "-map", final, "-frames:v", "1", args.sheet])
            written.append(args.sheet)
            print("rows, top to bottom (each %.2f s wide):" % longest)
            for path in files:
                print("  %s" % os.path.basename(path))
        else:
            if args.out:
                os.makedirs(args.out, exist_ok=True)
            for path in files:
                target = os.path.splitext(os.path.basename(path))[0] + ".png"
                target = os.path.join(args.out or os.path.dirname(os.path.abspath(path)), target)
                graph, label = picture_graph(0, args.width, args.height)
                ac.ffmpeg(["-i", path, "-filter_complex", graph, "-map", label, "-frames:v", "1", target])
                written.append(target)
    except ac.AudioError as error:
        ac.fail(str(error))
    for path in written:
        print("wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
