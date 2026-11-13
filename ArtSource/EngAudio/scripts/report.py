"""Triad Drift - check every final take against the mix standard, with the skill's own checks.

    python ArtSource/EngAudio/scripts/report.py

The same measurements and pass / fail rules as .claude/skills/ue-game-audio/scripts/audio_report.py
(audio_common.measure / check), run per sound with its own category, plus the takes-within-2-LU rule.
Writes reports/levels.txt and reports/levels.json. Exit 0 = everything passes.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPO = os.path.normpath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, os.path.join(REPO, ".claude", "skills", "ue-game-audio", "scripts"))
import audio_common as ac  # noqa: E402


def main():
    with open(os.path.join(ROOT, "build", "manifest.json"), encoding="utf-8") as handle:
        manifest = json.load(handle)
    by_sound = {}
    for wave in manifest["waves"]:
        sound = os.path.basename(wave["file"])[4:-7]
        by_sound.setdefault((sound, wave["cat"]), []).append(wave["file"])
    lines, rows, failures, size = [], [], 0, 0
    lines.append("%-34s %-8s %6s %2s %7s %6s %6s %6s %6s %6s  %s" % ("file", "cat", "dur s", "ch", "LUFS", "TP", "lead", "tail", "seam", "ends", "result"))
    for (sound, cat), files in sorted(by_sound.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        louds = []
        for path in sorted(files):
            result = ac.measure(path)
            if ac.TARGETS[cat]["loop"]:
                ac.add_seam(result)
            fails = ac.check(result, cat)
            size += os.path.getsize(path)
            if result["loudness_lufs"] is not None:
                louds.append(result["loudness_lufs"])
            failures += 1 if fails else 0
            result["fails"], result["category"] = fails, cat
            rows.append(result)
            lines.append("%-34s %-8s %6.2f %2d %7s %6s %4.0fms %4.0fms %6s %6s  %s" % (
                os.path.basename(path)[:34], cat, result["duration"], result["channels"], ac.fmt(result["loudness_lufs"]),
                ac.fmt(result["true_peak_dbtp"]), result["lead_silence_s"] * 1000, result["tail_silence_s"] * 1000,
                ac.fmt(result["seam_jump_db"]), ac.fmt(result["seam_level_db"]), "PASS" if not fails else "FAIL: " + "; ".join(fails)))
        if len(louds) >= 2 and max(louds) - min(louds) > ac.TAKE_SPREAD_MAX_LU:
            failures += 1
            lines.append("FAIL (takes) %s: spread %.1f LU (max %g)" % (sound, max(louds) - min(louds), ac.TAKE_SPREAD_MAX_LU))
    lines.append("")
    lines.append("%d file(s), %d failure(s), %.1f MB of WAV" % (len(rows), failures, size / 1e6))
    os.makedirs(os.path.join(ROOT, "reports"), exist_ok=True)
    with open(os.path.join(ROOT, "reports", "levels.txt"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    with open(os.path.join(ROOT, "reports", "levels.json"), "w", encoding="utf-8") as handle:
        json.dump({"files": rows, "failures": failures}, handle, indent=1)
    print("\n".join(line for line in lines if "FAIL" in line))
    print(lines[-1])
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
