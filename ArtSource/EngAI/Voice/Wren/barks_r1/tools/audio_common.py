"""Triad Drift - shared helpers for the ue-game-audio scripts (Python 3 standard library + ffmpeg/ffprobe).

Holds the category targets (the mix standard in references/mix-standard.md, as data), the measuring
code used by audio_report.py, and small ffmpeg wrappers used by audio_prep.py and audio_layer.py.
"""
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys

SAMPLE_RATE = 48000
AUDIO_EXTENSIONS = (".wav", ".flac", ".ogg", ".mp3", ".aif", ".aiff", ".opus", ".m4a")
UNGATED_MAX_SECONDS = 20.0   # below this the K-weighted ungated loudness is computed in Python
GATED_MIN_SECONDS = 3.0      # below this, gated integrated LUFS is not trusted (BS.1770 uses 400 ms blocks)

# Source-file targets per category. Peaks are true peak (dBTP). "loudness" is integrated LUFS for files
# of 3 s or more and K-weighted ungated loudness below that. None = not checked.
#   norm: how audio_prep.py sets the level ("peak" = true peak to norm_target, "loudness" = LUFS to norm_target)
TARGETS = {
    # 3D one-shots: weapons, impacts, explosions, blade, footsteps, enemy and crew sounds
    "sfx":      dict(channels=(1,), norm="peak", norm_target=-1.0, tp_min=-3.0, tp_max=-0.9, loud_min=None, loud_max=None,
                     lead_max=0.005, tail_max=0.050, loop=False, dur_min=0.03, dur_max=8.0, fade_in_ms=1, fade_out_ms=30),
    # 2D one-shots: HUD and menu sounds
    "ui":       dict(channels=(1, 2), norm="peak", norm_target=-3.0, tp_min=-5.0, tp_max=-2.9, loud_min=None, loud_max=None,
                     lead_max=0.005, tail_max=0.050, loop=False, dur_min=0.02, dur_max=3.0, fade_in_ms=1, fade_out_ms=20),
    # 2D cinematic one-shots: switch cut, "CUT!", slate, slow-motion in/out, hyperdrive drop-out, stingers
    "sting":    dict(channels=(1, 2), norm="peak", norm_target=-1.0, tp_min=-3.0, tp_max=-0.9, loud_min=None, loud_max=None,
                     lead_max=0.010, tail_max=0.100, loop=False, dur_min=0.05, dur_max=12.0, fade_in_ms=1, fade_out_ms=60),
    # 3D loops: engines, boost, drift, shield dome, portal idle, charge, low-hull alarm
    "loop":     dict(channels=(1,), norm="loudness", norm_target=-23.0, tp_min=None, tp_max=-3.0, loud_min=-24.5, loud_max=-21.5,
                     lead_max=0.001, tail_max=0.001, loop=True, dur_min=2.0, dur_max=30.0, fade_in_ms=0, fade_out_ms=0),
    # 2D beds: space room tone, planet ambience, hyperdrive tunnel
    "ambience": dict(channels=(2,), norm="loudness", norm_target=-30.0, tp_min=None, tp_max=-6.0, loud_min=-32.0, loud_max=-28.0,
                     lead_max=0.001, tail_max=0.001, loop=True, dur_min=20.0, dur_max=180.0, fade_in_ms=0, fade_out_ms=0),
    # Music stems and tracks (each stem; check the sum of all stems with audio_layer.py: -16 LUFS +/- 2)
    "music":    dict(channels=(2,), norm="peak", norm_target=-1.0, tp_min=None, tp_max=-0.9, loud_min=None, loud_max=-14.0,
                     lead_max=None, tail_max=None, loop=False, dur_min=5.0, dur_max=600.0, fade_in_ms=0, fade_out_ms=0),
    # Voice lines (radio barks)
    "vo":       dict(channels=(1,), norm="loudness", norm_target=-19.0, tp_min=None, tp_max=-1.0, loud_min=-21.0, loud_max=-17.0,
                     lead_max=0.050, tail_max=0.150, loop=False, dur_min=0.2, dur_max=20.0, fade_in_ms=3, fade_out_ms=30),
}
DC_OFFSET_MAX = 0.002          # linear, about -54 dBFS
SEAM_JUMP_MAX_DB = -40.0       # loop seam: size of the sample step from the last sample back to the first
SEAM_LEVEL_MAX_DB = 1.5        # loop seam: RMS difference between the first and the last 50 ms
TAKE_SPREAD_MAX_LU = 2.0       # takes of one sound must sit within this loudness spread


class AudioError(Exception):
    pass


def require_tools():
    for tool in ("ffmpeg", "ffprobe"):
        if not shutil.which(tool):
            raise AudioError("%s is not on PATH" % tool)


def run(args, binary=False):
    """Runs ffmpeg / ffprobe. Returns (returncode, stdout, stderr text)."""
    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out = process.stdout if binary else process.stdout.decode("utf-8", errors="replace")
    return process.returncode, out, process.stderr.decode("utf-8", errors="replace")


def ffmpeg(args):
    code, _, err = run(["ffmpeg", "-hide_banner", "-nostdin", "-y", "-v", "error"] + args)
    if code != 0:
        raise AudioError("ffmpeg failed: %s\n%s" % (" ".join(args), err.strip()[-1500:]))


def list_audio(paths):
    """Expands files and folders (recursive) into a sorted list of audio files."""
    found = []
    for path in paths:
        if os.path.isdir(path):
            for root, _, names in os.walk(path):
                found += [os.path.join(root, n) for n in names if n.lower().endswith(AUDIO_EXTENSIONS)]
        elif os.path.isfile(path):
            found.append(path)
        else:
            raise AudioError("not found: %s" % path)
    return sorted(found)


def probe(path):
    code, out, err = run(["ffprobe", "-v", "error", "-select_streams", "a:0", "-show_entries",
                          "stream=codec_name,sample_rate,channels,bits_per_sample,bits_per_raw_sample,duration:format=duration",
                          "-of", "json", path])
    if code != 0:
        raise AudioError("ffprobe failed on %s: %s" % (path, err.strip()[-400:]))
    data = json.loads(out)
    if not data.get("streams"):
        raise AudioError("no audio stream in %s" % path)
    stream = data["streams"][0]
    duration = stream.get("duration") or data.get("format", {}).get("duration") or 0
    bits = int(stream.get("bits_per_sample") or 0) or int(stream.get("bits_per_raw_sample") or 0)
    return dict(codec=stream.get("codec_name"), sample_rate=int(stream.get("sample_rate") or 0),
                channels=int(stream.get("channels") or 0), bits=bits, duration=float(duration))


def decode_f32(path, start=None, length=None, mono=False):
    """Decodes (part of) a file to float samples at 48 kHz. Returns (channels, [per-channel lists])."""
    args = ["ffmpeg", "-hide_banner", "-nostdin", "-v", "error"]
    if start is not None:
        args += ["-ss", "%.6f" % start]
    args += ["-i", path]
    if length is not None:
        args += ["-t", "%.6f" % length]
    info = probe(path)
    channels = 1 if mono else info["channels"]
    args += ["-ac", str(channels), "-ar", str(SAMPLE_RATE), "-f", "f32le", "-"]
    code, raw, err = run(args, binary=True)
    if code != 0:
        raise AudioError("decode failed on %s: %s" % (path, err.strip()[-400:]))
    count = len(raw) // 4
    samples = struct.unpack("<%df" % count, raw[:count * 4])
    return channels, [list(samples[c::channels]) for c in range(channels)]


def db(value):
    return 20.0 * math.log10(value) if value > 0 else -math.inf


def k_weighted_loudness(channel_samples):
    """Ungated K-weighted loudness (LUFS) of whole channels at 48 kHz: BS.1770 filters, no gating."""
    total = 0.0
    for samples in channel_samples:
        if not samples:
            continue
        # stage 1: high shelf; stage 2: high-pass (ITU-R BS.1770 coefficients for 48 kHz)
        x1 = x2 = y1 = y2 = 0.0
        u1 = u2 = v1 = v2 = 0.0
        acc = 0.0
        for x in samples:
            y = 1.53512485958697 * x - 2.69169618940638 * x1 + 1.19839281085285 * x2 + 1.69065929318241 * y1 - 0.73248077421585 * y2
            x2, x1, y2, y1 = x1, x, y1, y
            v = y - 2.0 * u1 + u2 + 1.99004745483398 * v1 - 0.99007225036621 * v2
            u2, u1, v2, v1 = u1, y, v1, v
            acc += v * v
        total += acc / len(samples)
    return -0.691 + 10.0 * math.log10(total) if total > 0 else -math.inf


def seam_metrics(path, duration):
    """For loops: the sample step across the loop point, and the level difference between both ends."""
    window = min(0.05, duration / 4.0)
    _, head = decode_f32(path, start=0.0, length=window, mono=True)
    _, tail = decode_f32(path, start=max(0.0, duration - window), mono=True)
    head, tail = head[0], tail[0]
    if not head or not tail:
        return None, None
    jump = db(abs(head[0] - tail[-1]))
    rms = lambda block: math.sqrt(sum(s * s for s in block) / len(block))
    level = abs(db(rms(head)) - db(rms(tail))) if rms(head) > 0 and rms(tail) > 0 else math.inf
    return jump, level


def measure(path, silence_db=-60.0):
    """All measurements for one file, as a dict (None where something could not be measured)."""
    info = probe(path)
    chain = ("silencedetect=n=%gdB:d=0.005,"
             "astats=measure_perchannel=none:measure_overall=DC_offset+Peak_level+RMS_level+Flat_factor+Peak_count+Number_of_samples,"
             "ebur128=peak=true:framelog=verbose" % silence_db)
    code, _, err = run(["ffmpeg", "-hide_banner", "-nostdin", "-nostats", "-i", path, "-map", "0:a:0", "-af", chain, "-f", "null", "-"])
    if code != 0:
        raise AudioError("measure failed on %s: %s" % (path, err.strip()[-600:]))

    def grab(pattern, text=err):
        match = None
        for match in re.finditer(pattern, text):
            pass  # keep the last occurrence (the summary)
        if not match:
            return None
        try:
            return float(match.group(1))
        except ValueError:
            return None  # "-inf", "nan"

    duration = info["duration"]
    samples = grab(r"Number of samples:\s*([-\d.]+)")
    if samples and info["sample_rate"]:
        duration = samples / info["sample_rate"]
    summary = err[err.rfind("Summary:"):] if "Summary:" in err else ""
    integrated = grab(r"I:\s*(-?[\d.]+) LUFS", summary)
    lra = grab(r"LRA:\s*(-?[\d.]+) LU", summary)
    true_peak = grab(r"True peak:\s*Peak:\s*(-?[\d.]+|-inf) dBFS", summary)
    sample_peak = grab(r"Peak level dB:\s*(-?[\d.]+|-inf)")
    rms = grab(r"RMS level dB:\s*(-?[\d.]+|-inf)")
    dc = grab(r"DC offset:\s*(-?[\d.]+)")
    flat = grab(r"Flat factor:\s*(-?[\d.]+)")
    peak_count = grab(r"Peak count:\s*(-?[\d.]+)")

    starts = [float(m) for m in re.findall(r"silence_start:\s*(-?[\d.]+)", err)]
    ends = [float(m) for m in re.findall(r"silence_end:\s*(-?[\d.]+)", err)]
    lead = ends[0] if starts and ends and starts[0] <= 0.002 else 0.0
    tail = 0.0
    if starts and (len(starts) > len(ends) or (ends and ends[-1] >= duration - 0.002)):
        tail = max(0.0, duration - starts[-1])
    silent = sample_peak is None or (len(starts) == 1 and starts[0] <= 0.002 and tail >= duration - 0.01)
    if silent:
        lead, tail = duration, duration

    ungated = None
    if not silent and duration <= UNGATED_MAX_SECONDS:
        _, channels = decode_f32(path)
        ungated = k_weighted_loudness(channels)
        if math.isinf(ungated):
            ungated = None
    gated_ok = integrated is not None and integrated > -69.9 and duration >= GATED_MIN_SECONDS
    loudness, loudness_kind = (integrated, "integrated") if gated_ok else (ungated, "ungated")

    clipped = bool(sample_peak is not None and sample_peak >= -0.05 and (flat or 0) > 0) or bool(true_peak is not None and true_peak > 0.0)
    seam_jump = seam_level = None
    result = dict(
        file=path, codec=info["codec"], sample_rate=info["sample_rate"], channels=info["channels"], bits=info["bits"],
        duration=round(duration, 4), loudness_lufs=None if loudness is None else round(loudness, 2), loudness_kind=loudness_kind,
        integrated_lufs=integrated if gated_ok else None, ungated_lufs=None if ungated is None else round(ungated, 2),
        lra_lu=lra if gated_ok else None, true_peak_dbtp=true_peak, sample_peak_dbfs=sample_peak, rms_dbfs=rms,
        lead_silence_s=round(lead, 4), tail_silence_s=round(tail, 4), dc_offset=dc, flat_factor=flat,
        peak_count=None if peak_count is None else int(peak_count), clipped=clipped, silent=silent,
        seam_jump_db=seam_jump, seam_level_db=seam_level)
    return result


def add_seam(result):
    """Fills the loop-seam fields (a second, short decode); only needed for loop categories."""
    if result["silent"] or result["duration"] < 0.2:
        return result
    jump, level = seam_metrics(result["file"], result["duration"])
    result["seam_jump_db"] = None if jump is None or math.isinf(jump) else round(jump, 1)
    if jump is not None and math.isinf(jump):
        result["seam_jump_db"] = -160.0
    result["seam_level_db"] = None if level is None or math.isinf(level) else round(level, 2)
    return result


def check(result, category):
    """Returns the list of failures of one measured file against a category ([] = pass)."""
    target = TARGETS[category]
    fails = []
    if result["silent"]:
        return ["file is silent"]
    if result["sample_rate"] != SAMPLE_RATE:
        fails.append("sample rate %d (want %d)" % (result["sample_rate"], SAMPLE_RATE))
    if result["channels"] not in target["channels"]:
        fails.append("%d channel(s) (want %s)" % (result["channels"], " or ".join(str(c) for c in target["channels"])))
    if result["clipped"]:
        fails.append("clipping suspected (peak %.2f dBFS, flat factor %s)" % (result["sample_peak_dbfs"], result["flat_factor"]))
    peak = result["true_peak_dbtp"]
    if peak is not None:
        if target["tp_max"] is not None and peak > target["tp_max"] + 0.05:
            fails.append("true peak %.1f dBTP above %.1f" % (peak, target["tp_max"]))
        if target["tp_min"] is not None and peak < target["tp_min"] - 0.05:
            fails.append("true peak %.1f dBTP below %.1f (not normalised)" % (peak, target["tp_min"]))
    loud = result["loudness_lufs"]
    if loud is not None:
        if target["loud_min"] is not None and loud < target["loud_min"]:
            fails.append("loudness %.1f LUFS below %.1f" % (loud, target["loud_min"]))
        if target["loud_max"] is not None and loud > target["loud_max"]:
            fails.append("loudness %.1f LUFS above %.1f" % (loud, target["loud_max"]))
    elif target["loud_min"] is not None or target["loud_max"] is not None:
        fails.append("loudness could not be measured")
    if target["lead_max"] is not None and result["lead_silence_s"] > target["lead_max"] + 0.0005:
        fails.append("leading silence %.0f ms (max %.0f)" % (result["lead_silence_s"] * 1000, target["lead_max"] * 1000))
    if target["tail_max"] is not None and result["tail_silence_s"] > target["tail_max"] + 0.0005:
        fails.append("trailing silence %.0f ms (max %.0f)" % (result["tail_silence_s"] * 1000, target["tail_max"] * 1000))
    if result["dc_offset"] is not None and abs(result["dc_offset"]) > DC_OFFSET_MAX:
        fails.append("DC offset %.4f (max %.4f)" % (result["dc_offset"], DC_OFFSET_MAX))
    if result["duration"] < target["dur_min"]:
        fails.append("duration %.2f s below %.2f" % (result["duration"], target["dur_min"]))
    if result["duration"] > target["dur_max"]:
        fails.append("duration %.2f s above %.2f" % (result["duration"], target["dur_max"]))
    if target["loop"]:
        if result["seam_jump_db"] is not None and result["seam_jump_db"] > SEAM_JUMP_MAX_DB:
            fails.append("loop seam step %.1f dBFS (max %.1f): will click" % (result["seam_jump_db"], SEAM_JUMP_MAX_DB))
        if result["seam_level_db"] is not None and result["seam_level_db"] > SEAM_LEVEL_MAX_DB:
            fails.append("loop ends differ by %.1f dB (max %.1f): audible bump" % (result["seam_level_db"], SEAM_LEVEL_MAX_DB))
    return fails


def level_gain(result, category, override=None):
    """Linear gain in dB that brings a measured file to its category target, kept under the peak ceiling."""
    target = TARGETS[category]
    goal = target["norm_target"] if override is None else override
    peak = result["true_peak_dbtp"]
    if peak is None or math.isinf(peak):
        raise AudioError("%s: no peak measured (silent?)" % result["file"])
    note = ""
    if target["norm"] == "peak":
        gain = goal - peak
    else:
        if result["loudness_lufs"] is None:
            raise AudioError("%s: loudness could not be measured" % result["file"])
        gain = goal - result["loudness_lufs"]
        ceiling = target["tp_max"] if target["tp_max"] is not None else -1.0
        if peak + gain > ceiling:
            short = peak + gain - ceiling
            gain -= short
            note = "peak-limited: %.1f LU under the loudness target (too dynamic; compress or accept)" % short
    return gain, note


def write_final(source, destination, gain_db, trim_tail_db=None, fade_out_ms=0):
    """Applies one linear gain (then an optional tail trim and fade out) and writes 48 kHz 16-bit PCM
    with triangular dither."""
    chain = ["volume=%.3fdB" % gain_db]
    if trim_tail_db is not None:
        chain.append("areverse,silenceremove=start_periods=1:start_threshold=%gdB:detection=peak,areverse" % trim_tail_db)
    if fade_out_ms and fade_out_ms > 0:
        chain.append("areverse,afade=t=in:d=%f:curve=qsin,areverse" % (fade_out_ms / 1000.0))
    chain.append("aresample=osf=s16:dither_method=triangular")
    ffmpeg(["-i", source, "-af", ",".join(chain), "-ar", str(SAMPLE_RATE), "-c:a", "pcm_s16le", destination])


def fmt(value, pattern="%.1f"):
    return "-" if value is None else pattern % value


def fail(message):
    print("ERROR: %s" % message, file=sys.stderr)
    sys.exit(2)
