#!/usr/bin/env python3
"""Build present-Wren C8_Wren_Order dry takes from raw xAI TTS wavs.

Trim to the spoken region, leave a 50 ms head (project vo lead_max) and a 100 ms tail,
remove DC, apply
short fades, and apply one linear gain to the project's vo target of -19 LUFS.
Writes 48 kHz mono 24-bit WAV. Deterministic given the raw files.
"""
from __future__ import annotations

import argparse
import json
import os
import sys

import numpy as np
import soundfile as sf

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402

SR = 48000
TARGET_LUFS = -19.0
TP_MAX = -1.5  # same cap used for audition_r1; vo category allows up to -1.0 dBTP
HEAD_S = 0.050
TAIL_S = 0.100
FADE_IN_S = 0.003
FADE_OUT_S = 0.020
# Speech bounds: first/last time the 10 ms RMS envelope is above this (dBFS).
# Low enough to keep consonants, high enough to ignore dither / hiss.
ONSET_DBFS = -45.0


def load_mono(path):
    x, sr = sf.read(path, always_2d=False)
    if sr != SR:
        raise SystemExit(f"{path}: expected {SR} Hz, got {sr}")
    if getattr(x, "ndim", 1) > 1:
        x = x.mean(axis=1)
    return x.astype(np.float64)


def rms_db(x, win=int(0.010 * SR), hop=int(0.002 * SR)):
    if len(x) < win:
        r = np.array([np.sqrt(np.mean(x * x))])
        times = np.array([0.0])
        return times, 20 * np.log10(r + 1e-12)
    n = 1 + (len(x) - win) // hop
    r = np.empty(n)
    for i in range(n):
        s = x[i * hop : i * hop + win]
        r[i] = np.sqrt(np.mean(s * s))
    times = (np.arange(n) * hop + win / 2) / SR
    return times, 20 * np.log10(r + 1e-12)


def speech_bounds(x):
    times, db = rms_db(x)
    spoken = np.where(db >= ONSET_DBFS)[0]
    if spoken.size == 0:
        # fallback: 20 dB below peak
        spoken = np.where(db >= db.max() - 20)[0]
    start = max(0.0, times[spoken[0]] - (0.010 / 2))
    end = min(len(x) / SR, times[spoken[-1]] + (0.010 / 2))
    return start, end


def mid_pause_ms(x, start, end):
    """Longest interior dip below onset threshold, in ms, if >= 80 ms."""
    a, b = int(start * SR), int(end * SR)
    times, db = rms_db(x[a:b])
    if times.size == 0:
        return 0.0
    below = db < ONSET_DBFS
    longest = 0
    run = 0
    hop_s = 0.002
    for v in below:
        if v:
            run += 1
            longest = max(longest, run)
        else:
            run = 0
    dur = longest * hop_s
    return round(dur * 1000, 1) if dur >= 0.08 else 0.0


def build_take(x, head=HEAD_S, tail=TAIL_S):
    x = x - np.mean(x)
    start, end = speech_bounds(x)
    a = int(round(start * SR))
    b = int(round(end * SR))
    y = x[a:b].copy()
    y = np.concatenate([np.zeros(int(round(head * SR))), y, np.zeros(int(round(tail * SR)))])
    fi = int(round(FADE_IN_S * SR))
    fo = int(round(FADE_OUT_S * SR))
    if fi > 0:
        y[:fi] *= np.linspace(0.0, 1.0, fi)
    if fo > 0:
        y[-fo:] *= np.cos(np.linspace(0.0, np.pi / 2, fo)) ** 2
    y = y - np.mean(y)
    return y, start, end


def write24(path, y):
    sf.write(path, np.clip(y, -1.0, 1.0), SR, subtype="PCM_24")


def gain_to_target(path):
    m = ac.measure(path)
    g = TARGET_LUFS - m["loudness_lufs"]
    g = min(g, TP_MAX - m["true_peak_dbtp"])
    return g, m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw-dir", required=True)
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--map", required=True, help="JSON list of {take, raw, out}")
    args = ap.parse_args()
    os.makedirs(args.out_dir, exist_ok=True)
    mapping = json.load(open(args.map))
    log = []
    for item in mapping:
        raw = os.path.join(args.raw_dir, item["raw"])
        out = os.path.join(args.out_dir, item["out"])
        x = load_mono(raw)
        y, start, end = build_take(x)
        pause = mid_pause_ms(x, start, end)
        write24(out, y)
        g, before = gain_to_target(out)
        y = y * (10 ** (g / 20.0))
        write24(out, y)
        after = ac.measure(out)
        rec = dict(
            take=item["take"],
            raw=item["raw"],
            out=item["out"],
            raw_speech_start_s=round(start, 4),
            raw_speech_end_s=round(end, 4),
            mid_pause_ms=pause,
            gain_db=round(g, 2),
            duration_s=after["duration"],
            loudness_lufs=after["loudness_lufs"],
            loudness_kind=after.get("loudness_kind"),
            true_peak_dbtp=after["true_peak_dbtp"],
            lead_silence_ms=round(after["lead_silence_s"] * 1000, 1),
            tail_silence_ms=round(after["tail_silence_s"] * 1000, 1),
        )
        log.append(rec)
        print(json.dumps(rec))
    json.dump(log, open(os.path.join(args.out_dir, "build_log.json"), "w"), indent=2)
    print("wrote", os.path.join(args.out_dir, "build_log.json"))


if __name__ == "__main__":
    main()
