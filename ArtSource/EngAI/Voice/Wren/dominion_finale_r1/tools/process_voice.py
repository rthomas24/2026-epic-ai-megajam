#!/usr/bin/env python3
"""Build dry / radio / echo Wren takes from raw xAI TTS wavs.

Dry edit matches present_order_r1 (PR #93): 50 ms head, 100 ms tail, 3 ms linear
fade-in, 20 ms cos^2 fade-out, DC removal, one gain to -19 LUFS, TP capped at
-1.5 dBTP, 48 kHz mono 24-bit.

Radio chain matches audition_r1 / take 04 (PR #78) except the mid-word static
tail, which belonged to the interrupted "Don't—" cut. Full lines keep:
band-limit 300 Hz–3.4 kHz, tanh(2.2) saturation, carrier hiss at -38 dBFS,
250 Hz high-pass, DC removal, short end fade, then -19 LUFS.

Echo chain (FutureWren Echo recordings only): dry plus a 95 ms delayed copy at
-7 dB, low-passed at 3.4 kHz, then -19 LUFS.
"""
from __future__ import annotations

import argparse
import json
import os
import sys

import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audio_common as ac  # noqa: E402

SR = 48000
TARGET_LUFS = -19.0
TP_MAX = -1.5
HEAD_S = 0.050
TAIL_S = 0.100
FADE_IN_S = 0.003
FADE_OUT_S = 0.020
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
        spoken = np.where(db >= db.max() - 20)[0]
    start = max(0.0, times[spoken[0]] - (0.010 / 2))
    end = min(len(x) / SR, times[spoken[-1]] + (0.010 / 2))
    return start, end


def mid_pause_ms(x, start, end):
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


def build_dry(x, head=HEAD_S, tail=TAIL_S):
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
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    sf.write(path, np.clip(y, -1.0, 1.0), SR, subtype="PCM_24")


def gain_to_target(path):
    m = ac.measure(path)
    g = TARGET_LUFS - m["loudness_lufs"]
    g = min(g, TP_MAX - m["true_peak_dbtp"])
    return g, m


def apply_gain(path, y):
    g, _ = gain_to_target(path)
    y = y * (10 ** (g / 20.0))
    write24(path, y)
    after = ac.measure(path)
    return y, g, after


def radio(y, seed):
    """Take 04 radio chain without the interrupted-line static tail."""
    rng = np.random.default_rng(seed)
    hp = butter(4, 300, "hp", fs=SR, output="sos")
    lp = butter(6, 3400, "lp", fs=SR, output="sos")
    v = sosfilt(lp, sosfilt(hp, y))
    peak = np.abs(v).max()
    if peak > 0:
        v = v / peak
    drive = 2.2
    v = np.tanh(drive * v) / np.tanh(drive)
    v = sosfilt(lp, v)
    noise = sosfilt(lp, sosfilt(hp, rng.standard_normal(len(v))))
    npeak = np.abs(noise).max()
    if npeak > 0:
        noise = noise / npeak
    bed = noise * 10 ** (-38 / 20)
    out = v + bed
    out = sosfilt(butter(2, 250, "hp", fs=SR, output="sos"), out)
    out -= out.mean()
    fo = int(0.012 * SR)
    fi = int(0.003 * SR)
    if fo > 0:
        out[-fo:] *= np.linspace(1, 0, fo)
    if fi > 0:
        out[:fi] *= np.linspace(0, 1, fi)
    return out


def echo(y):
    delay = int(0.095 * SR)
    lp = butter(6, 3400, "lp", fs=SR, output="sos")
    delayed = np.concatenate([np.zeros(delay), y])
    dry = np.concatenate([y, np.zeros(delay)])
    wet = sosfilt(lp, delayed) * (10 ** (-7 / 20))
    out = dry + wet
    fo = int(0.020 * SR)
    if fo > 0:
        out[-fo:] *= np.cos(np.linspace(0.0, np.pi / 2, fo)) ** 2
    out -= out.mean()
    return out


def seed_for(stem):
    # stable integer seed from the stem (take number included)
    import hashlib
    h = hashlib.sha1(stem.encode()).hexdigest()
    return int(h[:8], 16)


def process_one(raw_path, out_dir, stem, treatment):
    x = load_mono(raw_path)
    y, start, end = build_dry(x)
    pause = mid_pause_ms(x, start, end)
    dry_path = os.path.join(out_dir, f"{stem}_dry.wav")
    write24(dry_path, y)
    y, g, after = apply_gain(dry_path, y)
    rec = dict(
        stem=stem,
        raw=os.path.basename(raw_path),
        treatment=treatment,
        raw_speech_start_s=round(start, 4),
        raw_speech_end_s=round(end, 4),
        mid_pause_ms=pause,
        dry=os.path.basename(dry_path),
        dry_gain_db=round(g, 2),
        dry_duration_s=after["duration"],
        dry_loudness_lufs=after["loudness_lufs"],
        dry_true_peak_dbtp=after["true_peak_dbtp"],
        dry_lead_ms=round(after["lead_silence_s"] * 1000, 1),
        dry_tail_ms=round(after["tail_silence_s"] * 1000, 1),
    )
    if "radio" in treatment:
        r = radio(y, seed_for(stem))
        rp = os.path.join(out_dir, f"{stem}_radio.wav")
        write24(rp, r)
        r, g2, after2 = apply_gain(rp, r)
        rec.update(
            radio=os.path.basename(rp),
            radio_gain_db=round(g2, 2),
            radio_duration_s=after2["duration"],
            radio_loudness_lufs=after2["loudness_lufs"],
            radio_true_peak_dbtp=after2["true_peak_dbtp"],
        )
    if "echo" in treatment:
        e = echo(y)
        ep = os.path.join(out_dir, f"{stem}_echo.wav")
        write24(ep, e)
        e, g3, after3 = apply_gain(ep, e)
        rec.update(
            echo=os.path.basename(ep),
            echo_gain_db=round(g3, 2),
            echo_duration_s=after3["duration"],
            echo_loudness_lufs=after3["loudness_lufs"],
            echo_true_peak_dbtp=after3["true_peak_dbtp"],
        )
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", required=True, help="catalog jobs.json")
    ap.add_argument("--raw-dir", required=True)
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--batch", default=None)
    args = ap.parse_args()
    jobs = json.load(open(args.jobs))
    os.makedirs(args.out_dir, exist_ok=True)
    log = []
    missing = []
    for j in jobs:
        if args.batch and j["batch"] != args.batch:
            continue
        raw = os.path.join(args.raw_dir, j["raw_name"])
        if not os.path.exists(raw):
            missing.append(j["raw_name"])
            continue
        rec = process_one(raw, args.out_dir, j["stem"], j["treatment"])
        rec.update(line_id=j["line_id"], variant=j.get("variant"), take=j["take"], batch=j["batch"])
        log.append(rec)
        print(json.dumps({k: rec[k] for k in ("stem", "dry_loudness_lufs", "dry_duration_s")}))
    json.dump(log, open(os.path.join(args.out_dir, "build_log.json"), "w"), indent=2)
    print(f"wrote {len(log)} takes, missing raw={len(missing)}")
    if missing:
        json.dump(missing, open(os.path.join(args.out_dir, "missing_raw.json"), "w"), indent=2)


if __name__ == "__main__":
    main()
