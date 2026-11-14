#!/usr/bin/env python3
"""Triad Drift - measure audio files and check them against a category of the mix standard.

    python audio_report.py ArtSource/Audio/SFX/Weapons/Lancer_Cannon/export --category sfx
    python audio_report.py a.wav b.wav --category loop --json report.json
    python audio_report.py music_stems/ --category music --same-length
    python audio_report.py Saved/BouncedWavFiles/playtest.wav --mix          # a capture of the whole game mix
    python audio_report.py --targets                                          # print the category targets

Reports, per file: duration, sample rate, channels, loudness (integrated LUFS from 3 s up, K-weighted
ungated below that), loudness range, true peak, sample peak, leading / trailing silence, DC offset,
clipping, and for loop categories the loop-seam step and level difference. Prints a table; --json also
writes everything as JSON. Exit code: 0 = every file passes, 1 = at least one failure, 2 = usage / tool error.

Without --category nothing is judged (exit 0). Numbers are measurements, not a verdict on how it sounds.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402

MIX_TARGET_LUFS = (-20.0, -16.0)   # integrated loudness of a capture of the whole mix (references/mix-standard.md)
MIX_TRUE_PEAK_MAX = -1.0


def print_targets():
    print("%-9s %-8s %-18s %-16s %-16s %-9s %-9s %-5s %s" % (
        "category", "channels", "normalise", "true peak dBTP", "loudness LUFS", "lead ms", "tail ms", "loop", "duration s"))
    for name, t in ac.TARGETS.items():
        rng = lambda lo, hi: "%s..%s" % ("" if lo is None else "%g" % lo, "" if hi is None else "%g" % hi) if (lo is not None or hi is not None) else "-"
        print("%-9s %-8s %-18s %-16s %-16s %-9s %-9s %-5s %g..%g" % (
            name, "/".join(str(c) for c in t["channels"]), "%s %g" % (t["norm"], t["norm_target"]),
            rng(t["tp_min"], t["tp_max"]), rng(t["loud_min"], t["loud_max"]),
            "-" if t["lead_max"] is None else "%g" % (t["lead_max"] * 1000),
            "-" if t["tail_max"] is None else "%g" % (t["tail_max"] * 1000),
            "yes" if t["loop"] else "no", t["dur_min"], t["dur_max"]))
    print("\nAll categories: 48000 Hz, no clipping, |DC offset| <= %g, takes of one sound within %g LU." % (ac.DC_OFFSET_MAX, ac.TAKE_SPREAD_MAX_LU))
    print("Loops also: seam step <= %g dBFS, level difference between both ends <= %g dB." % (ac.SEAM_JUMP_MAX_DB, ac.SEAM_LEVEL_MAX_DB))
    print("--mix capture: integrated %g..%g LUFS, true peak <= %g dBTP." % (MIX_TARGET_LUFS[0], MIX_TARGET_LUFS[1], MIX_TRUE_PEAK_MAX))


def main():
    parser = argparse.ArgumentParser(description="Measure audio files and check them against the Triad Drift mix standard.")
    parser.add_argument("paths", nargs="*", help="audio files or folders (searched recursively)")
    parser.add_argument("--category", choices=sorted(ac.TARGETS), help="judge the files against this category")
    parser.add_argument("--mix", action="store_true", help="judge the files as captures of the whole game mix")
    parser.add_argument("--same-length", action="store_true", help="fail unless all files have the same length (music layers)")
    parser.add_argument("--takes", action="store_true", help="treat the files as takes of ONE sound: fail if their loudness spread is too wide")
    parser.add_argument("--json", metavar="FILE", help="also write the full results as JSON")
    parser.add_argument("--targets", action="store_true", help="print the category targets and exit")
    args = parser.parse_args()

    if args.targets:
        print_targets()
        return 0
    if not args.paths:
        parser.error("give at least one file or folder")
    try:
        ac.require_tools()
        files = ac.list_audio(args.paths)
        if not files:
            ac.fail("no audio files found")
        results = []
        for path in files:
            result = ac.measure(path)
            if args.category and ac.TARGETS[args.category]["loop"]:
                ac.add_seam(result)
            fails = []
            if args.category:
                fails = ac.check(result, args.category)
            if args.mix:
                loud, peak = result["integrated_lufs"], result["true_peak_dbtp"]
                if loud is None:
                    fails.append("capture too short for integrated loudness (need 3 s; use minutes of real play)")
                elif not MIX_TARGET_LUFS[0] <= loud <= MIX_TARGET_LUFS[1]:
                    fails.append("mix loudness %.1f LUFS outside %g..%g" % (loud, MIX_TARGET_LUFS[0], MIX_TARGET_LUFS[1]))
                if peak is not None and peak > MIX_TRUE_PEAK_MAX + 0.05:
                    fails.append("mix true peak %.1f dBTP above %g" % (peak, MIX_TRUE_PEAK_MAX))
                if result["clipped"]:
                    fails.append("clipping suspected")
            result["fails"] = fails
            results.append(result)
    except ac.AudioError as error:
        ac.fail(str(error))

    group_fails = []
    if args.same_length and len({round(r["duration"] * r["sample_rate"]) for r in results}) > 1:
        group_fails.append("files differ in length: " + ", ".join("%s=%.4fs" % (os.path.basename(r["file"]), r["duration"]) for r in results))
    if args.takes:
        louds = [r["loudness_lufs"] for r in results if r["loudness_lufs"] is not None]
        if len(louds) >= 2 and max(louds) - min(louds) > ac.TAKE_SPREAD_MAX_LU:
            group_fails.append("takes spread %.1f LU (max %g): level them" % (max(louds) - min(louds), ac.TAKE_SPREAD_MAX_LU))

    judged = bool(args.category or args.mix)
    header = "%-34s %7s %6s %2s %7s %-3s %5s %6s %6s %6s %6s %8s %6s %6s  %s" % (
        "file", "dur s", "rate", "ch", "LUFS", "", "LRA", "TP", "peak", "lead", "tail", "DC", "seam", "ends", "result")
    print(header)
    print("-" * len(header))
    for r in results:
        name = os.path.basename(r["file"])
        name = name if len(name) <= 34 else name[:31] + "..."
        verdict = "" if not judged else ("PASS" if not r["fails"] else "FAIL: " + "; ".join(r["fails"]))
        print("%-34s %7.3f %6d %2d %7s %-3s %5s %6s %6s %6s %6s %8s %6s %6s  %s" % (
            name, r["duration"], r["sample_rate"], r["channels"], ac.fmt(r["loudness_lufs"]),
            "int" if r["loudness_kind"] == "integrated" else "ung", ac.fmt(r["lra_lu"]), ac.fmt(r["true_peak_dbtp"]),
            ac.fmt(r["sample_peak_dbfs"]), "%.0fms" % (r["lead_silence_s"] * 1000), "%.0fms" % (r["tail_silence_s"] * 1000),
            ac.fmt(r["dc_offset"], "%.5f"), ac.fmt(r["seam_jump_db"]), ac.fmt(r["seam_level_db"]), verdict))
    for message in group_fails:
        print("FAIL (group): " + message)

    failed = sum(1 for r in results if r["fails"]) + len(group_fails)
    if judged or group_fails:
        print("\n%d file(s), %d failure(s)%s" % (len(results), failed, "" if not args.category else " against category '%s'" % args.category))
    print("LUFS: int = gated integrated (files of 3 s or more), ung = K-weighted ungated (short files). TP = true peak dBTP.")
    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(dict(category=args.category, mix=args.mix, files=results, group_fails=group_fails, failures=failed), handle, indent=1)
        print("JSON written to %s" % args.json)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
