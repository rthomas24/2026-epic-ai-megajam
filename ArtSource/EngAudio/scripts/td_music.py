"""Triad Drift - engineering fallback music (ENG-35). Original compositions, synthesised here note by note.

Both pieces are driven by recipes/music.json (tempo, key, chord roots, patterns) so they can be changed
without touching this file. Everything is added into the buffer modulo its length, so the combat loop's
tails wrap around its end and the loop point is exact.
"""
import numpy as np

import td_synth as ts

SR = ts.SR


def hz(midi):
    return 440.0 * 2 ** ((midi - 69) / 12.0)


class Track:
    def __init__(self, seconds, loop):
        self.n = int(round(seconds * SR))
        self.loop = loop
        self.buf = np.zeros((self.n, 2))

    def add(self, sig, at_seconds, gain=1.0, pan=0.0):
        """pan -1 (left) .. +1 (right), equal power."""
        start = int(round(at_seconds * SR))
        left = gain * np.cos((pan + 1) * np.pi / 4)
        right = gain * np.sin((pan + 1) * np.pi / 4)
        if self.loop:
            idx = (start + np.arange(len(sig))) % self.n
            np.add.at(self.buf[:, 0], idx, sig * left)
            np.add.at(self.buf[:, 1], idx, sig * right)
        else:
            end = min(self.n, start + len(sig))
            if end > start:
                self.buf[start:end, 0] += sig[:end - start] * left
                self.buf[start:end, 1] += sig[:end - start] * right


_cache = {}


def cached(key, make):
    if key not in _cache:
        _cache[key] = make()
    return _cache[key]


def kick():
    rng = np.random.default_rng(1)
    body = ts.layer_tone({"f0": 150, "f1": 44, "curve": 0.45, "dur": 0.3, "a": 0.001, "d": 0.28}, rng)
    click = ts.layer_noise({"lo0": 2000, "hi0": 8000, "dur": 0.008, "d": 0.007}, rng) * 0.4
    body[:len(click)] += click
    return ts.drive(body, 0.6)


def snare():
    rng = np.random.default_rng(2)
    noise = ts.layer_noise({"lo0": 180, "hi0": 7000, "dur": 0.2, "d": 0.18}, rng)
    tone = ts.layer_tone({"f0": 220, "f1": 170, "dur": 0.1, "d": 0.09}, rng) * 0.6
    noise[:len(tone)] += tone
    return ts.reverb(noise, 0.25, 0.25, 400, 7000, seed=3)


def hat(length=0.04, seed=4):
    rng = np.random.default_rng(seed)
    return ts.layer_noise({"lo0": 7000, "hi0": 15000, "dur": length, "d": length * 0.9}, rng)


def bass_note(midi, dur):
    rng = np.random.default_rng(5)
    f = hz(midi)
    sig = ts.layer_tone({"wave": "saw", "f0": f, "dur": dur, "a": 0.004, "h": dur * 0.5, "d": dur * 0.5, "unison": 2, "detune": 8}, rng)
    sub = ts.layer_tone({"wave": "sine", "f0": f, "dur": dur, "a": 0.004, "h": dur * 0.5, "d": dur * 0.5}, rng)
    return ts.drive(ts.fft_filter(sig, None, 700, 3) + 0.7 * sub, 0.5)


def pluck(midi, dur=0.22, bright=3500, seed=6):
    rng = np.random.default_rng(seed)
    sig = ts.layer_tone({"wave": "saw", "f0": hz(midi), "dur": dur, "a": 0.002, "d": dur, "unison": 2, "detune": 10}, rng)
    return ts.fft_filter(sig, 150, bright, 2)


def pad_chord(midis, dur, seed):
    rng = np.random.default_rng(seed)
    out = None
    for m in midis:
        sig = ts.layer_tone({"wave": "saw", "f0": hz(m), "dur": dur, "a": min(0.6, dur * 0.25), "h": dur * 0.55, "d": dur * 0.3,
                             "unison": 3, "detune": 16}, rng)
        out = sig if out is None else out + sig
    return ts.fft_filter(out, 180, 1600, 2) / len(midis)


def lead_note(midi, dur):
    rng = np.random.default_rng(7)
    sig = ts.layer_tone({"wave": "square", "f0": hz(midi), "dur": dur + 0.12, "a": 0.008, "h": dur * 0.7, "d": dur * 0.3 + 0.12,
                         "vib_rate": 5.5, "vib_depth": 0.006, "unison": 2, "detune": 9}, rng)
    return ts.fft_filter(sig, 200, 4200, 2)


def riser(dur, seed=8):
    rng = np.random.default_rng(seed)
    return ts.layer_noise({"lo0": 300, "hi0": 2000, "lo1": 2500, "hi1": 12000, "dur": dur, "a": dur * 0.95, "d": dur * 0.05}, rng)


def master(x, loop):
    """Gentle glue: band limit, soft saturation; the levelling pass sets the final gain."""
    for c in range(2):
        x[:, c] = ts.fft_filter(x[:, c], 28, 16000, 2, circ=loop)
    x = x / (np.percentile(np.abs(x), 99.9) + 1e-9) * 0.7
    return np.tanh(x * 1.25) / np.tanh(1.25)


def combat(spec):
    bpm = spec["bpm"]
    beat = 60.0 / bpm
    bars = spec["bars"]
    six = beat / 4
    track = Track(bars * 4 * beat, loop=True)
    roots = spec["roots"]            # one MIDI root per 2 bars, cycled
    chords = spec["chords"]          # semitone offsets per 2-bar chord, cycled
    section = lambda bar: bar // (bars // 4)   # 4 sections: 0 = lean, 1 = +snare/pad, 2 = +lead, 3 = full

    k, s = cached("kick", kick), cached("snare", snare)
    h, oh = cached("hat", hat), cached("ohat", lambda: hat(0.18, 9))
    for bar in range(bars):
        sec = section(bar)
        t0 = bar * 4 * beat
        ci = (bar // 2) % len(roots)
        root, chord = roots[ci], chords[ci % len(chords)]
        last = bar == bars - 1
        for b in range(4):
            track.add(k, t0 + b * beat, 1.0)
            if sec >= 1 and b in (1, 3):
                track.add(s, t0 + b * beat, 0.55)
            track.add(h, t0 + b * beat + beat / 2, 0.22, 0.25)
            if sec == 3 or (sec == 2 and bar % 2 == 1):
                for q16 in (1, 3):
                    track.add(h, t0 + b * beat + q16 * six, 0.12, -0.2)
        if bar % 2 == 1:
            track.add(oh, t0 + 3 * beat + beat / 2, 0.2, 0.25)
        if last:  # fill: sixteenth snares rising into the loop point
            for i in range(8):
                track.add(s, t0 + 2 * beat + i * six, 0.2 + 0.05 * i)
            track.add(cached("riser", lambda: riser(2 * beat)), t0 + 2 * beat, 0.35)
        # bass: off-beat sixteenths (the kick owns the beat), octave jump on the last sixteenth
        for st in range(16):
            if st % 4 == 0:
                continue
            note = root + (12 if st % 4 == 3 and sec >= 1 else 0)
            if spec["bass_walk"] and bar % 2 == 1 and st >= 12:
                note = root + spec["bass_walk"][st - 12]
            track.add(cached(("bass", note), lambda note=note: bass_note(note, six * 0.95)), t0 + st * six, 0.55)
        # arp: sixteenth chord tones over two octaves, ping-ponging, with a dotted-eighth echo
        pattern = spec["arp"]
        for st in range(16):
            if sec == 0 and bar < 4 and st % 2 == 1:
                continue  # thin at the very start
            deg = pattern[st % len(pattern)]
            note = root + 24 + chord[deg % len(chord)] + 12 * (deg // len(chord))
            p = cached(("pluck", note), lambda note=note: pluck(note))
            g = 0.2 + 0.05 * sec
            pan = -0.6 if st % 2 == 0 else 0.6
            track.add(p, t0 + st * six, g, pan)
            track.add(p, t0 + st * six + 3 * six, g * 0.4, -pan)
            track.add(p, t0 + st * six + 6 * six, g * 0.18, pan)
        # pad: one chord per 2 bars, pumping against the kick
        if sec >= 1 and bar % 2 == 0:
            dur = 8 * beat
            midis = [root + 12 + c for c in chord] + [root + 24 + chord[0]]
            pad = cached(("pad", ci, sec >= 3), lambda: pad_chord(midis, dur, 20 + ci)).copy()
            t = np.arange(len(pad)) / SR
            pad *= 1.0 - 0.55 * np.exp(-((t % beat) / (beat * 0.35)))
            track.add(pad, t0, 0.5, -0.35)
            track.add(np.roll(pad, 311), t0, 0.5, 0.35)
        # lead: a 2-bar phrase, sections 2 and 3 (the answer phrase an octave up in the last section)
        if sec >= 2 and bar % 2 == 0:
            phrase = spec["lead"][(bar // 2) % len(spec["lead"])]
            for step, midi, length in phrase:
                note = midi + (12 if sec == 3 and (bar // 2) % 2 == 1 else 0)
                sig = cached(("lead", note, length), lambda note=note, length=length: lead_note(note, length * six))
                track.add(sig, t0 + step * six, 0.3)
                track.add(sig, t0 + step * six + 3 * six, 0.1, 0.5)
                track.add(sig, t0 + step * six + 6 * six, 0.05, -0.5)
    return master(track.buf, True)


def intro(spec):
    seconds = spec["seconds"]
    hit = spec["hit_at"]
    track = Track(seconds, loop=False)
    rng = np.random.default_rng(31)
    root = spec["root"]
    # a drone that opens slowly, root + fifth + octave
    for i, (m, g) in enumerate(((root, 1.0), (root + 7, 0.6), (root + 12, 0.5), (root + 19, 0.25))):
        dur = hit + 1.0
        sig = ts.layer_tone({"wave": "saw", "f0": hz(m), "dur": dur, "a": dur * 0.8, "h": 0.5, "d": 0.5, "unison": 5, "detune": 18}, rng)
        sig = ts.fft_filter(sig, 60, 900 + 500 * i, 2)
        track.add(sig, 0.0, 0.35 * g, -0.4 + 0.27 * i)
    # low pulse, half time, entering quietly
    beat = 60.0 / spec["bpm"]
    k = kick()
    t = spec["pulse_from"]
    while t < hit - 0.2:
        u = (t - spec["pulse_from"]) / (hit - spec["pulse_from"])
        track.add(k, t, 0.25 + 0.5 * u)
        if u > 0.5:
            track.add(k, t + beat * 0.75, 0.15 + 0.3 * u)
        t += beat * (2 if u < 0.7 else 1)
    # rising arpeggio
    arp = spec["arp"]
    t = spec["arp_from"]
    i = 0
    while t < hit - 0.1:
        u = (t - spec["arp_from"]) / (hit - spec["arp_from"])
        note = root + 24 + arp[i % len(arp)] + (12 if u > 0.6 and i % 2 else 0)
        p = pluck(note, 0.35, 2500 + 3000 * u, seed=40)
        track.add(p, t, 0.1 + 0.2 * u, -0.5 if i % 2 else 0.5)
        track.add(p, t + beat * 0.75, 0.05 + 0.08 * u, 0.5 if i % 2 else -0.5)
        t += beat / 2
        i += 1
    # swell and the hit that hands over to wave 1
    r = riser(6.0, 41)
    track.add(r, hit - 6.0, 0.4)
    boom = ts.layer_tone({"f0": 90, "f1": 30, "dur": 2.5, "d": 2.4}, rng) + ts.layer_noise({"lo0": 100, "hi0": 4000, "lo1": 40, "hi1": 400, "dur": 2.5, "d": 2.4}, rng) * 0.7
    track.add(ts.drive(boom, 0.5), hit, 1.0)
    chord = pad_chord([root + 12, root + 19, root + 24, root + 27], seconds - hit, 42)
    track.add(chord * ts.envelope(len(chord), 0.01, 0.0, seconds - hit - 0.05), hit, 0.8, -0.3)
    track.add(np.roll(chord, 257) * ts.envelope(len(chord), 0.01, 0.0, seconds - hit - 0.05), hit, 0.8, 0.3)
    x = track.buf
    for c in range(2):
        x[:, c] = ts.reverb(x[:, c], 2.2, 0.25, 150, 6000, seed=50 + c)[:track.n]
    fade = int(0.5 * SR)
    x[-fade:] *= np.linspace(1.0, 0.0, fade)[:, None] ** 2
    return master(x, False)


PIECES = {"combat": combat, "intro": intro}
