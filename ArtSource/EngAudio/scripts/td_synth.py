"""Triad Drift - engineering fallback sound set (ENG-35): the synthesiser.

Every sound in this set is ORIGINAL: computed here from oscillators, noise and filters. No samples, no
recordings, no third-party audio. numpy only (Blender's bundled Python has it):

    "C:/Program Files/Blender Foundation/Blender 5.2/5.2/python/bin/python.exe" render.py

A sound is a JSON recipe (recipes/*.json): a list of layers, each a small generator with an envelope,
plus a post chain. A number may be written {"r": [lo, hi]}: a fresh uniform random value per take, so
takes differ while the same seed always renders the same file.

Layer types
  tone   swept oscillator        f0 f1 curve wave(sine|saw|square|tri) fm_ratio fm_index unison detune vib_rate vib_depth
  noise  band-limited noise      lo0 hi0 lo1 hi1 (the band glides from 0 to 1)
  res    swept narrow-band noise f0 f1 bw (a noisy whistle: wind, plasma, whoosh)
Common: dur a(attack s) h(hold s) d(decay to -60 dB, s) ec(envelope curve) gain start drive lp hp trem_rate trem_depth
Loop layers (periodic by construction, so the loop point is exact)
  harm   harmonic drone          f0 n rolloff odd voices detune am_rate am_depth
  pnoise periodic band noise     lo hi tilt am_rate am_depth
  pulse  repeated tone bursts    f0 f1 wave every len (alarms, beacons)
"""
import json
import math
import os
import struct

import numpy as np

SR = 48000


# ---------------------------------------------------------------- helpers
def val(v, rng):
    """A recipe number: plain, or {"r": [lo, hi]} = uniform random per take."""
    if isinstance(v, dict) and "r" in v:
        lo, hi = v["r"]
        return float(rng.uniform(lo, hi))
    return v


def resolve(d, rng):
    return {k: (val(v, rng) if not isinstance(v, (list, str)) else v) for k, v in d.items()}


def fft_filter(x, lo=None, hi=None, order=4, circ=False):
    """Zero-phase band filter with Butterworth-shaped magnitude (done in the frequency domain).
    One-shots are padded with silence first, so the filter's ringing cannot wrap from the start to the end;
    circ=True filters one whole period of a loop, which must wrap."""
    if lo is None and hi is None:
        return x
    if not circ:
        pad = 4800
        return fft_filter(np.concatenate([np.zeros(pad), x, np.zeros(pad)]), lo, hi, order, True)[pad:pad + len(x)]
    n = len(x)
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(n, 1.0 / SR)
    f[0] = 1e-6
    mag = np.ones_like(f)
    if lo:
        mag *= 1.0 / np.sqrt(1.0 + (lo / f) ** (2 * order))
    if hi:
        mag *= 1.0 / np.sqrt(1.0 + (f / hi) ** (2 * order))
    return np.fft.irfft(spec * mag, n)


def envelope(n, a=0.002, h=0.0, d=0.2, ec=1.0):
    """Linear attack, hold, exponential decay reaching -60 dB after d seconds (ec bends the decay)."""
    t = np.arange(n) / SR
    env = np.ones(n)
    a = max(a, 1.0 / SR)
    env = np.minimum(env, t / a)
    td = np.maximum(0.0, t - a - h)
    d = max(d, 1e-3)
    env *= np.exp(-6.908 * (td / d) ** ec)
    return env


def sweep(f0, f1, n, curve=1.0):
    """Instantaneous frequency gliding exponentially from f0 to f1 (curve < 1 = fast at first)."""
    u = (np.arange(n) / max(1, n - 1)) ** curve
    return f0 * (f1 / f0) ** u


def osc(phase, wave, finst):
    """Band-limited-enough oscillators: saw / square / tri are sums of harmonics kept under 18 kHz."""
    if wave == "sine":
        return np.sin(phase)
    out = np.zeros_like(phase)
    kmax = 28
    for k in range(1, kmax + 1):
        if wave in ("square", "tri") and k % 2 == 0:
            continue
        mask = (k * finst) < 18000.0
        if not mask.any():
            break
        if wave == "saw":
            amp = 1.0 / k
        elif wave == "square":
            amp = 1.0 / k
        else:
            amp = ((-1) ** ((k - 1) // 2)) / (k * k)
        out += amp * np.sin(k * phase) * mask
    return out * (0.6 if wave != "tri" else 0.8)


def drive(x, amount):
    if not amount:
        return x
    g = 1.0 + amount
    return np.tanh(x * g) / math.tanh(g)


# ---------------------------------------------------------------- one-shot layers
def layer_tone(p, rng):
    n = int(p.get("dur", 0.3) * SR)
    f = sweep(p["f0"], p.get("f1", p["f0"]), n, p.get("curve", 1.0))
    if p.get("vib_rate"):
        f = f * (1.0 + p.get("vib_depth", 0.02) * np.sin(2 * np.pi * p["vib_rate"] * np.arange(n) / SR + rng.uniform(0, 6.28)))
    env = envelope(n, p.get("a", 0.002), p.get("h", 0.0), p.get("d", p.get("dur", 0.3)), p.get("ec", 1.0))
    voices = int(p.get("unison", 1))
    out = np.zeros(n)
    for v in range(voices):
        det = 0.0 if voices == 1 else (v / (voices - 1) - 0.5) * 2 * p.get("detune", 12.0)
        fv = f * 2 ** (det / 1200.0)
        phase = 2 * np.pi * np.cumsum(fv) / SR + rng.uniform(0, 6.28)
        if p.get("fm_ratio"):
            mod = np.sin(phase * p["fm_ratio"])
            phase = phase + p.get("fm_index", 1.0) * env ** p.get("fm_env", 1.0) * mod
        out += osc(phase, p.get("wave", "sine"), fv)
    return out / voices * env


def layer_noise(p, rng):
    n = int(p.get("dur", 0.3) * SR)
    white = rng.standard_normal(n)
    lo0, hi0 = p.get("lo0", 200), p.get("hi0", 8000)
    lo1, hi1 = p.get("lo1", lo0), p.get("hi1", hi0)
    if lo0 == lo1 and hi0 == hi1:
        out = fft_filter(white, lo0, hi0, p.get("order", 3))
    else:  # gliding band: cross-fade a few statically filtered copies
        steps = 6
        out = np.zeros(n)
        u = (np.arange(n) / max(1, n - 1)) ** p.get("curve", 1.0) * (steps - 1)
        for s in range(steps):
            k = s / (steps - 1)
            lo = lo0 * (lo1 / lo0) ** k
            hi = hi0 * (hi1 / hi0) ** k
            w = np.clip(1.0 - np.abs(u - s), 0.0, 1.0)
            out += fft_filter(white, lo, hi, p.get("order", 3)) * w * (math.sqrt(hi0 - lo0) / math.sqrt(max(50.0, hi - lo)))
    out /= (np.sqrt(np.mean(out ** 2)) + 1e-9) * 3.0
    return out * envelope(n, p.get("a", 0.001), p.get("h", 0.0), p.get("d", p.get("dur", 0.3)), p.get("ec", 1.0))


def layer_res(p, rng):
    n = int(p.get("dur", 0.4) * SR)
    f = sweep(p["f0"], p.get("f1", p["f0"]), n, p.get("curve", 1.0))
    jitter = fft_filter(rng.standard_normal(n), None, p.get("jit_hz", 400.0), 2)
    jitter /= np.std(jitter) + 1e-9
    f = f * (1.0 + p.get("bw", 0.15) * jitter)
    phase = 2 * np.pi * np.cumsum(f) / SR
    out = osc(phase, p.get("wave", "sine"), np.abs(f))
    return out * envelope(n, p.get("a", 0.01), p.get("h", 0.0), p.get("d", p.get("dur", 0.4)), p.get("ec", 1.0))


LAYERS = {"tone": layer_tone, "noise": layer_noise, "res": layer_res}


def reverb(x, time=0.6, mix=0.2, lo=200, hi=6000, predelay=0.008, seed=7, circular=False):
    """A plain diffuse tail: convolution with exponentially decaying filtered noise."""
    rng = np.random.default_rng(seed)
    m = int(time * SR)
    ir = rng.standard_normal(m) * np.exp(-6.908 * np.arange(m) / m)
    ir = fft_filter(ir, lo, hi, 2)
    ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
    pre = int(predelay * SR)
    if circular:
        n = len(x)
        irp = np.zeros(n)
        idx = (np.arange(m) + pre) % n
        np.add.at(irp, idx, ir)
        wet = np.fft.irfft(np.fft.rfft(x) * np.fft.rfft(irp), n)
        return x + mix * wet
    n = len(x) + m + pre
    size = 1 << (n - 1).bit_length()
    wet = np.fft.irfft(np.fft.rfft(x, size) * np.fft.rfft(ir, size), size)[:n]
    dry = np.zeros(n)
    dry[:len(x)] = x
    wet = np.roll(wet, pre)
    wet[:pre] = 0
    return dry + mix * wet


def post_chain(x, post, rng, circular=False):
    post = resolve(post or {}, rng)
    if post.get("drive"):
        x = drive(x / (np.max(np.abs(x)) + 1e-9), post["drive"])
    if post.get("hp") or post.get("lp"):
        x = fft_filter(x, post.get("hp"), post.get("lp"), post.get("order", 3), circ=circular)
    if post.get("reverb"):
        r = post["reverb"]
        x = reverb(x, r.get("time", 0.6), r.get("mix", 0.2), r.get("lo", 200), r.get("hi", 6000), r.get("predelay", 0.008),
                   seed=r.get("seed", 7), circular=circular)
    return x


def render_oneshot(recipe, seed):
    """Mixes the layers of a one-shot recipe; returns mono float samples, trimmed and faded."""
    rng = np.random.default_rng(seed)
    pitch = val(recipe.get("pitch", 1.0), rng)
    stretch = val(recipe.get("stretch", 1.0), rng)
    parts = []
    for layer in recipe["layers"]:
        p = resolve(layer, rng)
        for key in ("f0", "f1", "lo0", "hi0", "lo1", "hi1"):
            if key in p:
                p[key] = min(20000.0, p[key] * pitch)
        for key in ("dur", "d", "start", "h"):
            if key in p:
                p[key] = p[key] * stretch
        sig = LAYERS[p["type"]](p, rng)
        if p.get("trem_rate"):
            sig = sig * (1.0 - p.get("trem_depth", 0.5) * (0.5 + 0.5 * np.sin(2 * np.pi * p["trem_rate"] * np.arange(len(sig)) / SR)))
        if p.get("drive"):
            sig = drive(sig / (np.max(np.abs(sig)) + 1e-9), p["drive"])
        if p.get("hp") or p.get("lp"):
            sig = fft_filter(sig, p.get("hp"), p.get("lp"), 3)
        if p.get("rev"):
            sig = sig[::-1].copy()
        parts.append((int(p.get("start", 0.0) * SR), sig * p.get("gain", 1.0)))
    n = max(s + len(x) for s, x in parts)
    out = np.zeros(n)
    for s, x in parts:
        out[s:s + len(x)] += x
    out = post_chain(out, recipe.get("post"), rng)
    return finish_oneshot(out, recipe.get("fade_out", 0.03), max_dur=recipe.get("max_dur"))


def finish_oneshot(x, fade_out=0.03, tail_db=-50.0, max_dur=None):
    """Trim to where the sound's level (10 ms RMS) last sits above tail_db, fade both ends, remove the DC left over."""
    x = x - np.mean(x)
    x = fft_filter(x, 20.0, None, 2)
    x = x / (np.max(np.abs(x)) + 1e-12)
    w = int(0.01 * SR)
    rms = np.sqrt(np.convolve(x ** 2, np.ones(w) / w, mode="same"))
    loud = np.nonzero(rms > 10 ** (tail_db / 20.0))[0]
    lead = np.nonzero(np.abs(x) > 10 ** (-45 / 20.0))[0]
    start = max(0, int(lead[0]) - 24) if len(lead) else 0
    end = min(len(x), int(loud[-1]) + w) if len(loud) else len(x)
    if max_dur:
        end = min(end, start + int(max_dur * SR))
    x = x[start:end].copy()
    window = np.ones(len(x))
    fi = min(len(x), 48)
    window[:fi] *= np.linspace(0.0, 1.0, fi)
    fo = min(len(x) // 2, int(min(fade_out, 0.1) * SR))
    window[-fo:] *= np.cos(np.linspace(0.0, np.pi / 2, fo)) ** 2
    x *= window
    x -= window * (np.sum(x) / np.sum(window))   # what is left of the mean, removed without a step at either end
    return x / (np.max(np.abs(x)) + 1e-12)


# ---------------------------------------------------------------- loops (periodic by construction)
def q(freq, dur):
    """Nearest frequency with a whole number of cycles in the loop."""
    return max(1, round(freq * dur)) / dur


def loop_harm(p, n, dur, rng):
    t = np.arange(n) / SR
    out = np.zeros(n)
    voices = int(p.get("voices", 1))
    for v in range(voices):
        det = 0.0 if voices == 1 else (v / (voices - 1) - 0.5) * 2 * p.get("detune", 8.0)
        f0 = p["f0"] * 2 ** (det / 1200.0)
        for k in range(1, int(p.get("n", 8)) + 1):
            if p.get("odd") and k % 2 == 0:
                continue
            fk = q(f0 * k, dur)
            if fk > 16000:
                break
            out += np.sin(2 * np.pi * fk * t + rng.uniform(0, 6.28)) / (k ** p.get("rolloff", 1.2))
    return out / voices


def loop_pnoise(p, n, dur, rng):
    f = np.fft.rfftfreq(n, 1.0 / SR)
    f[0] = 1e-6
    lo, hi = p.get("lo", 100), p.get("hi", 6000)
    mag = 1.0 / np.sqrt(1.0 + (lo / f) ** 6) / np.sqrt(1.0 + (f / hi) ** 6) * (f / 1000.0) ** p.get("tilt", -0.5)
    spec = mag * np.exp(1j * rng.uniform(0, 2 * np.pi, len(f)))
    out = np.fft.irfft(spec, n)
    return out / (np.std(out) + 1e-9) * 0.3


def loop_pulse(p, n, dur, rng):
    out = np.zeros(n)
    every = dur / max(1, round(dur / p.get("every", 0.5)))
    m = int(p.get("len", 0.12) * SR)
    burst = layer_tone({"f0": p["f0"], "f1": p.get("f1", p["f0"]), "dur": p.get("len", 0.12), "a": 0.004, "h": p.get("len", 0.12) * 0.5,
                        "d": p.get("len", 0.12) * 0.5, "wave": p.get("wave", "sine"), "fm_ratio": p.get("fm_ratio"), "fm_index": p.get("fm_index", 1.0)},
                       np.random.default_rng(3))
    for i in range(int(round(dur / every))):
        s = int(i * every * SR) + int(p.get("offset", 0.0) * SR)
        idx = (s + np.arange(m)) % n
        out[idx] += burst
    return out


LOOPS = {"harm": loop_harm, "pnoise": loop_pnoise, "pulse": loop_pulse}


def render_loop_channel(recipe, seed):
    rng = np.random.default_rng(seed)
    dur = recipe["dur"]
    n = int(round(dur * SR))
    t = np.arange(n) / SR
    out = np.zeros(n)
    for layer in recipe["layers"]:
        p = resolve(layer, rng)
        sig = LOOPS[p["type"]](p, n, dur, rng)
        if p.get("am_rate"):
            rate = q(p["am_rate"], dur)
            sig = sig * (1.0 - p.get("am_depth", 0.3) * (0.5 + 0.5 * np.sin(2 * np.pi * rate * t + rng.uniform(0, 6.28))))
        if p.get("am2_rate"):
            rate = q(p["am2_rate"], dur)
            sig = sig * (1.0 - p.get("am2_depth", 0.3) * (0.5 + 0.5 * np.sin(2 * np.pi * rate * t + rng.uniform(0, 6.28))))
        if p.get("drive"):
            sig = drive(sig / (np.max(np.abs(sig)) + 1e-9), p["drive"])
        if p.get("hp") or p.get("lp"):
            sig = fft_filter(sig, p.get("hp"), p.get("lp"), 3, circ=True)  # filtering a whole period stays periodic
        out += sig * p.get("gain", 1.0)
    for ev in recipe.get("events", []):  # one-shot recipes scattered through the loop, wrapped around its end
        erng = np.random.default_rng(seed * 131 + 17)
        count = int(ev.get("count", 4))
        for i in range(count):
            shot = render_oneshot(ev["recipe"], int(erng.integers(1, 1 << 30)))
            at = int(((i + erng.uniform(0.15, 0.85)) / count) * n)
            g = ev.get("gain", 0.3) * erng.uniform(0.5, 1.0) * (erng.uniform(0.3, 1.0) if recipe.get("stereo") else 1.0)
            idx = (at + np.arange(len(shot))) % n
            np.add.at(out, idx, shot * g)
    out = post_chain(out, recipe.get("post"), rng, circular=True)
    out -= np.mean(out)
    return out


def best_rotation(x):
    """Rotate a periodic loop so the file starts where the sample step is smallest and both ends are equally loud."""
    mono = x if x.ndim == 1 else x.mean(axis=1)
    n = len(mono)
    step = np.abs(mono - np.roll(mono, 1))
    w = int(0.05 * SR)
    sq = np.concatenate([mono, mono[:w]]) ** 2
    c = np.concatenate([[0.0], np.cumsum(sq)])
    rms_fwd = np.sqrt((c[w:w + n] - c[:n]) / w)
    rms_back = np.roll(rms_fwd, w)
    level = np.abs(20 * np.log10((rms_fwd + 1e-9) / (rms_back + 1e-9)))
    score = step / (np.max(step) + 1e-12) + (level > 0.7) * 10.0 + (rms_fwd < 0.3 * np.median(rms_fwd)) * 10.0
    k = int(np.argmin(score))
    return np.roll(x, -k, axis=0)


def render_loop(recipe, seed):
    if recipe.get("stereo"):
        left = render_loop_channel(recipe, seed)
        right = render_loop_channel(recipe, seed + 1000)
        width = recipe.get("width", 0.7)
        mid, side = (left + right) / 2, (left - right) / 2
        x = np.stack([mid + width * side, mid - width * side], axis=1)
    else:
        x = render_loop_channel(recipe, seed)
    x = best_rotation(x)
    return x / (np.max(np.abs(x)) + 1e-12) * 0.5


# ---------------------------------------------------------------- files
def write_f32(path, x):
    """32-bit float WAV (the levelling pass turns it into the final 16-bit file)."""
    x = np.asarray(x, dtype="<f4")
    channels = 1 if x.ndim == 1 else x.shape[1]
    data = x.tobytes()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE")
        handle.write(b"fmt " + struct.pack("<IHHIIHH", 16, 3, channels, SR, SR * channels * 4, channels * 4, 32))
        handle.write(b"data" + struct.pack("<I", len(data)) + data)


def load_recipes(folder):
    """All recipes/*.json merged; "like": "<name>" copies another recipe's sound fields first."""
    sounds = {}
    for name in sorted(os.listdir(folder)):
        if name.endswith(".json"):
            with open(os.path.join(folder, name), encoding="utf-8") as handle:
                for entry in json.load(handle)["sounds"]:
                    entry["file"] = name
                    sounds[entry["name"]] = entry
    for entry in sounds.values():
        if "like" in entry:
            base = sounds[entry["like"]]
            for key in ("layers", "post", "dur", "fade_out", "max_dur", "stereo", "events", "width"):
                if key in base and key not in entry:
                    entry[key] = base[key]
    return sounds
