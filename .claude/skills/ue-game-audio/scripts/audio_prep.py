#!/usr/bin/env python3
"""Triad Drift - turn source audio into import-ready takes: trim, fade, 48 kHz / 16-bit WAV, category level.

    python audio_prep.py picks/*.wav --category sfx --name SFX_Weapon_Lancer_Cannon --out export/
    python audio_prep.py engine_raw.wav --category loop --name SFX_Engine_Lancer_Loop --out export/ --loop-crossfade 0.5
    python audio_prep.py stem_*.wav --category music --name MUS_Combat --out export/ --keep-names

For every input, in order:
  1. channels to what the category needs (mono for 3D sounds), 15 Hz high-pass to remove DC (not for music);
  2. one-shots: leading silence cut at --trim-db (peak detection), trailing silence cut, optional --max-duration;
  3. short fade in / fade out (category defaults, override with --fade-in-ms / --fade-out-ms);
  4. loops with --loop-crossfade S: the file is split in the middle, the halves are swapped and the new
     inner seam is cross-faded with equal-power curves, so the loop point is the original, continuous middle;
  5. resample to 48 kHz; ONE linear gain to the category target (true peak or loudness; never a
     dynamic normaliser, so the sound's own dynamics stay); 16-bit PCM with triangular dither;
  6. write <name>_01.wav, <name>_02.wav ... plus <name>.prep.json (source of each take, gain applied),
     then measure the results against the category (same check as audio_report.py).

Exit code: 0 = every take passes its category, 1 = at least one fails, 2 = usage / tool error.
Sources are never modified.
"""
import argparse
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402


def stage_shape(source, target_path, category, args, in_channels):
    """Channels, DC removal, trim, max duration, fades -> float WAV at 48 kHz."""
    target = ac.TARGETS[category]
    filters = []
    oneshot = not target["loop"] and category != "music"
    if category != "music":
        filters.append("highpass=f=15:poles=2")
    if oneshot and not args.no_trim:
        filters.append("silenceremove=start_periods=1:start_threshold=%gdB:detection=peak" % args.trim_db)
        filters.append("areverse,silenceremove=start_periods=1:start_threshold=%gdB:detection=peak,areverse" % args.tail_db)
    if args.max_duration:
        filters.append("atrim=end=%g" % args.max_duration)
    fade_in = target["fade_in_ms"] if args.fade_in_ms is None else args.fade_in_ms
    fade_out = target["fade_out_ms"] if args.fade_out_ms is None else args.fade_out_ms
    if fade_in > 0:
        filters.append("afade=t=in:d=%g:curve=tri" % (fade_in / 1000.0))
    if fade_out > 0:
        filters.append("areverse,afade=t=in:d=%g:curve=qsin,areverse" % (fade_out / 1000.0))
    wanted = target["channels"]
    channels = in_channels if in_channels in wanted else wanted[0]
    command = ["-i", source, "-map", "0:a:0", "-ac", str(channels), "-ar", str(ac.SAMPLE_RATE)]
    if filters:
        command += ["-af", ",".join(filters)]
    ac.ffmpeg(command + ["-c:a", "pcm_f32le", target_path])


def stage_loop(source, target_path, crossfade):
    """Swap the halves and cross-fade the new inner seam: the loop point becomes the original middle."""
    duration = ac.probe(source)["duration"]
    if crossfade * 2.5 >= duration:
        raise ac.AudioError("--loop-crossfade %g s is too long for a %.2f s file" % (crossfade, duration))
    half = duration / 2.0
    graph = ("[0:a]asplit=2[x][y];"
             "[x]atrim=start=%f,asetpts=PTS-STARTPTS[second];"
             "[y]atrim=end=%f,asetpts=PTS-STARTPTS[first];"
             "[second][first]acrossfade=d=%f:c1=qsin:c2=qsin[out]" % (half, half, crossfade))
    ac.ffmpeg(["-i", source, "-filter_complex", graph, "-map", "[out]", "-c:a", "pcm_f32le", target_path])


def main():
    parser = argparse.ArgumentParser(description="Trim, fade, resample to 48 kHz / 16-bit and level audio to a category target.")
    parser.add_argument("inputs", nargs="+", help="source files or folders")
    parser.add_argument("--category", required=True, choices=sorted(ac.TARGETS))
    parser.add_argument("--out", required=True, help="output folder (created)")
    parser.add_argument("--name", help="base name: outputs are <name>_01.wav ... (required unless --keep-names)")
    parser.add_argument("--keep-names", action="store_true", help="keep each source's own file name (music stems)")
    parser.add_argument("--start-index", type=int, default=1)
    parser.add_argument("--target", type=float, help="override the category's level target (dBTP or LUFS)")
    parser.add_argument("--no-trim", action="store_true", help="keep leading and trailing silence")
    parser.add_argument("--trim-db", type=float, default=-50.0, help="leading-silence threshold, dBFS peak (default -50)")
    parser.add_argument("--tail-db", type=float, default=-60.0, help="trailing-silence threshold, dBFS peak (default -60)")
    parser.add_argument("--max-duration", type=float, help="cut to this many seconds before the fade out")
    parser.add_argument("--fade-in-ms", type=float)
    parser.add_argument("--fade-out-ms", type=float)
    parser.add_argument("--loop-crossfade", type=float, metavar="S", help="make a seamless loop with an S-second cross-fade")
    args = parser.parse_args()
    if not args.name and not args.keep_names:
        parser.error("--name is required unless --keep-names is given")

    try:
        ac.require_tools()
        sources = ac.list_audio(args.inputs)
        if not sources:
            ac.fail("no audio files found")
        os.makedirs(args.out, exist_ok=True)
        rows, log = [], []
        with tempfile.TemporaryDirectory(prefix="td_audio_prep_") as work:
            for index, source in enumerate(sources):
                before = ac.measure(source)
                if before["silent"]:
                    raise ac.AudioError("%s is silent" % source)
                shaped = os.path.join(work, "shape_%03d.wav" % index)
                stage_shape(source, shaped, args.category, args, before["channels"])
                if args.loop_crossfade:
                    looped = os.path.join(work, "loop_%03d.wav" % index)
                    stage_loop(shaped, looped, args.loop_crossfade)
                    shaped = looped
                middle = ac.measure(shaped)
                if middle["silent"] or middle["duration"] <= 0:
                    raise ac.AudioError("%s: nothing left after trimming (raise --trim-db?)" % source)
                gain, note = ac.level_gain(middle, args.category, args.target)
                if args.keep_names:
                    out_name = os.path.splitext(os.path.basename(source))[0] + ".wav"
                else:
                    out_name = "%s_%02d.wav" % (args.name, args.start_index + index)
                out_path = os.path.join(args.out, out_name)
                ac.write_final(shaped, out_path, gain)
                after = ac.measure(out_path)
                if ac.TARGETS[args.category]["loop"]:
                    ac.add_seam(after)
                fails = ac.check(after, args.category)
                rows.append((source, before, after, gain, note, fails))
                log.append(dict(source=os.path.abspath(source), output=out_name, gain_db=round(gain, 2), note=note,
                                before=dict(duration=before["duration"], sample_rate=before["sample_rate"], channels=before["channels"],
                                            true_peak_dbtp=before["true_peak_dbtp"], loudness_lufs=before["loudness_lufs"]),
                                after=after, fails=fails))
    except ac.AudioError as error:
        ac.fail(str(error))

    print("%-28s -> %-34s %8s %8s %7s %7s %7s  %s" % ("source", "output", "dur s", "gain dB", "TP", "LUFS", "tail", "result"))
    for source, before, after, gain, note, fails in rows:
        verdict = "PASS" if not fails else "FAIL: " + "; ".join(fails)
        if note:
            verdict += "  [" + note + "]"
        print("%-28s -> %-34s %8s %+8.1f %7s %7s %6.0fms  %s" % (
            os.path.basename(source)[:28], os.path.basename(after["file"])[:34], "%.2f>%.2f" % (before["duration"], after["duration"]),
            gain, ac.fmt(after["true_peak_dbtp"]), ac.fmt(after["loudness_lufs"]), after["tail_silence_s"] * 1000, verdict))

    group_fail = ""
    louds = [after["loudness_lufs"] for _, _, after, _, _, _ in rows if after["loudness_lufs"] is not None]
    if not args.keep_names and len(louds) >= 2 and max(louds) - min(louds) > ac.TAKE_SPREAD_MAX_LU:
        group_fail = "takes spread %.1f LU (max %g): they will not sound like one sound; re-pick or level by loudness" % (
            max(louds) - min(louds), ac.TAKE_SPREAD_MAX_LU)
        print("FAIL (group): " + group_fail)

    sidecar = os.path.join(args.out, (args.name or "prep") + ".prep.json")
    with open(sidecar, "w", encoding="utf-8") as handle:
        json.dump(dict(category=args.category, name=args.name, takes=log, group_fail=group_fail), handle, indent=1)
    failed = sum(1 for row in rows if row[5]) + (1 if group_fail else 0)
    print("\n%d take(s) written to %s, %d failure(s) against '%s'. Log: %s" % (len(rows), args.out, failed, args.category, sidecar))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
