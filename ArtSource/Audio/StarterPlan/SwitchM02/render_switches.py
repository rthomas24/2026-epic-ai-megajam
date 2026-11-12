"""Original Triad Drift switch candidates. Python 3.11+ and NumPy only.

Run from any directory; writes only beside this script. No sampled recordings,
network, plugin instruments, browser oscillators, or third-party audio content.
Seeded stochastic excitation, damped inharmonic modes, granular friction,
pressure/air layers, and decorrelated finite room reflections.
"""
from pathlib import Path
import hashlib
import json
import math
import platform
import wave
import numpy as np

ROOT = Path(__file__).resolve().parent
SR = 48000
SEED = 261112
RNG = np.random.default_rng(SEED)


def seconds(duration):
    return np.arange(round(duration * SR), dtype=np.float64) / SR


def rms(x):
    return float(np.sqrt(np.mean(np.square(x))))


def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))


def spectral_noise(duration, bands):
    """Finite stochastic pressure texture; bands=(Hz, octave width, gain)."""
    n = round(duration * SR)
    f = np.fft.rfftfreq(n, 1 / SR)
    spectrum = np.zeros_like(f)
    for hz, octaves, gain in bands:
        spectrum += gain * np.exp(-0.5 * (np.log2(np.maximum(f, 1) / hz) / octaves) ** 2)
    spectrum *= (1 - np.exp(-(f / 30) ** 4)) * np.exp(-(f / 14500) ** 8)
    z = RNG.normal(size=len(f)) + 1j * RNG.normal(size=len(f))
    z[0] = 0
    x = np.fft.irfft(z * spectrum, n=n)
    return x / max(rms(x), 1e-9)


def envelope(t, attack, decay, power=1):
    return (-np.expm1(-t / attack)) ** power * np.exp(-t / decay)


def modal(duration, frequencies, decay, attack=.0015, drift=0):
    """Damped shell modes with irregular initial phase and micro detuning.

    Frequencies are deliberately inharmonic: this is a struck physical object,
    not a pitched activation melody. Broad excitation is mixed separately.
    """
    t = seconds(duration)
    x = np.zeros(len(t))
    for i, frequency in enumerate(frequencies):
        d = decay * RNG.uniform(.65, 1.25) / (1 + i * .055)
        hz = frequency * RNG.uniform(.985, 1.015)
        phase = 2 * np.pi * (hz * t + drift * hz * .05 * (-np.expm1(-t / .05)))
        x += np.sin(phase + RNG.uniform(-.15, .15)) * np.exp(-t / d) / (1 + i * .28)
    x *= -np.expm1(-t / attack)
    return x / max(float(np.max(np.abs(x))), 1e-9)


def shell(duration, low, mass, brightness=1):
    t = seconds(duration)
    body = modal(duration, [low * k for k in [1, 1.47, 2.31, 3.82, 5.13, 7.62, 10.4]], mass)
    thud = spectral_noise(duration, [(low * 1.4, .75, 1), (380, 1, .45)])
    snap = spectral_noise(duration, [(1600, .7, 1), (4900, .55, .8)])
    return .78 * body + .32 * thud * envelope(t, .0006, .048) + .14 * brightness * snap * envelope(t, .00025, .011)


def air(duration, center, attack, decay, weight=1):
    t = seconds(duration)
    x = spectral_noise(duration, [(center, 1.0, 1), (center * 3.1, .6, .25)])
    slow = spectral_noise(duration, [(9, 1, 1), (22, .5, .4)])
    modulation = np.clip(1 + .17 * slow, .35, 1.6)
    return weight * x * modulation * envelope(t, attack, decay)


def friction(duration, count, band, decay):
    """Non-periodic micro contacts, e.g. rail/armature/plate movement."""
    out = np.zeros(round(duration * SR))
    for onset in np.sort(RNG.uniform(.002, duration * .64, count)):
        grain_dur = RNG.uniform(.008, .031)
        t = seconds(grain_dur)
        grain = spectral_noise(grain_dur, [(band * RNG.uniform(.7, 1.4), .6, 1)])
        grain *= np.sin(np.pi * np.arange(len(t)) / max(len(t) - 1, 1)) ** 2
        start = round(onset * SR)
        end = min(start + len(grain), len(out))
        out[start:end] += grain[:end-start] * RNG.uniform(.09, .24) * np.exp(-onset / decay)
    return out


def place(bus, source, at=0, gain=1, pan=0):
    start = round(at * SR)
    end = min(start + len(source), len(bus))
    if end <= start:
        return
    p = np.broadcast_to(pan, (len(source),))[:end-start]
    # Near-centre constant power placement; low end is monoed in mastering.
    bus[start:end, 0] += gain * source[:end-start] * np.sqrt((1-p) / 2)
    bus[start:end, 1] += gain * source[:end-start] * np.sqrt((1+p) / 2)


def reflections(dry, wet, decay):
    """Sparse early reflections into a dense, dark finite diffusion tail.

    No external impulse response. Different L/R taps avoid metallic flutter;
    no polarity tricks. No undamped or indefinite feedback.
    """
    n = len(dry)
    out = dry.copy()
    mono = dry.mean(axis=1)
    # Reverb send removes low bass and extreme treble.
    f = np.fft.rfftfreq(n, 1/SR)
    send = np.fft.irfft(np.fft.rfft(mono) * (1-np.exp(-(f/220)**4)) * np.exp(-(f/5200)**2), n=n)
    for channel in range(2):
        ir = np.zeros(round(.72*SR))
        for delay, amplitude in [(0.019, .6), (.031, .42), (.047, .28)]:
            ir[round((delay + channel*.0017)*SR)] = amplitude
        for delay in RNG.uniform(.05, .7, 340):
            ir[round(delay*SR)] += RNG.uniform(-1, 1) * np.exp(-delay/decay) * .075
        size = 1 << (n + len(ir) - 2).bit_length()
        tail = np.fft.irfft(np.fft.rfft(send, size) * np.fft.rfft(ir, size), size)[:n]
        out[:,channel] += wet * tail
    return out


def make_rift():
    duration = 1.62
    out = np.zeros((round(duration*SR), 2))
    # Immediate cut: pressure implosion, followed by a wide dimensional wake.
    place(out, shell(.82, 61, .19, .35), .002, 1.15)
    place(out, air(.72, 220, .003, .16), 0, .43)
    t = seconds(.82)
    tear = air(.82, 1350, .008, .19)
    tear += .18 * friction(.82, 18, 3300, .2)
    place(out, tear, .012, .43, .72*np.tanh((t-.15)*9))
    # A short reversed stochastic breath pulls into a second compression point.
    suction = air(.19, 650, .002, .045)[::-1].copy()
    suction[-240:] *= np.linspace(1, 0, 240)
    place(out, suction, .018, .18, -.2)
    place(out, modal(.75, [89, 137, 226, 371, 597, 941], .13, .006, -.065), .11, .18)
    # Spatial trace is a quiet textured side layer, rather than a musical arpeggio.
    place(out, air(.8, 3800, .021, .13), .075, .10, -.6)
    return reflections(out, .28, .16)


def make_lancer():
    duration = 1.28
    out = np.zeros((round(duration*SR), 2))
    place(out, shell(.49, 128, .067, .85), .001, .78)
    place(out, shell(.42, 197, .045, .8), .043, .40, -.16)
    place(out, friction(.37, 23, 2200, .12), 0, .8, .2)
    # Broad turbine pressure builds rapidly; no bare rising oscillator.
    t = seconds(.70)
    turbine = air(.70, 470, .009, .14)
    turbine += .19 * spectral_noise(.70, [(1400, .35, 1), (2800, .4, .4)]) * envelope(t, .014, .12)
    turbine = np.tanh(1.18*turbine) / 1.18
    place(out, turbine, .028, .62, .46*np.tanh((t-.12)*12))
    place(out, modal(.57, [74, 116, 171, 274, 449, 713], .095, .003, .07), .029, .53)
    place(out, air(.42, 4700, .003, .055), .014, .16, -.25)
    # Positive stop/armature detent after the surge, still inside the HUD beat.
    place(out, shell(.30, 291, .032, .6), .125, .15, .12)
    return reflections(out, .15, .105)


def make_bastion():
    duration = 1.78
    out = np.zeros((round(duration*SR), 2))
    place(out, shell(.98, 46, .22, .65), .002, 1.20)
    # Offset heavy clamps, not evenly quantized percussion.
    place(out, shell(.69, 103, .125, .68), .069, .61, -.22)
    place(out, shell(.58, 164, .09, .7), .112, .32, .22)
    place(out, friction(.40, 16, 830, .13), .008, .78, -.1)
    place(out, air(.66, 185, .005, .11), .019, .39)
    place(out, modal(.94, [58, 87, 143, 233, 379, 612], .17, .013, -.025), .037, .43)
    place(out, air(.58, 1700, .014, .09), .082, .15, .20)
    # Damped plate rattle/pressure release makes the weight tangible.
    place(out, friction(.53, 14, 1680, .16), .128, .47, .25)
    return reflections(out, .23, .19)


def fft_filter(x, response):
    return np.fft.irfft(np.fft.rfft(x, axis=0) * response[:,None], n=len(x), axis=0)


def biquad(x, b, a):
    y = np.empty_like(x)
    z1 = np.zeros(x.shape[1]); z2 = z1.copy()
    for i in range(len(x)):
        sample = x[i]
        value = b[0]*sample + z1
        z1 = b[1]*sample - a[1]*value + z2
        z2 = b[2]*sample - a[2]*value
        y[i] = value
    return y


def loudness(x):
    """BS.1770 K-weighting + 400 ms blocks/100 ms hop + -70/-10 LU gates.

    Short SFX measurement is a matching reference, not a long-program target.
    """
    y = biquad(x, [1.53512485958697,-2.69169618940638,1.19839281085285], [1,-1.69065929318241,.73248077421585])
    y = biquad(y, [1,-2,1], [1,-1.99004745483398,.99007225036621])
    e = np.array([np.sum(np.mean(y[i:i+19200]**2, axis=0)) for i in range(0,len(y)-19200+1,4800)])
    l = -.691 + 10*np.log10(np.maximum(e,1e-24))
    e = e[l > -70]
    relative = -.691 + 10*np.log10(np.mean(e)) - 10
    e = e[-.691+10*np.log10(e) > relative]
    return float(-.691 + 10*np.log10(np.mean(e)))


def true_peak(x):
    # 4x FFT interpolation with long zero guard, preventing periodic wrap.
    pad = np.pad(x, ((4096,4096),(0,0)))
    spectrum = np.fft.rfft(pad, axis=0)
    spectrum[-1] *= .5
    expanded = np.fft.irfft(spectrum, n=4*len(pad), axis=0)*4
    return float(np.max(np.abs(expanded)))


def prepare(x):
    # Guard bass against stereo phase cancellation, suppress inaudible extremes.
    x -= x.mean(axis=0)
    f = np.fft.rfftfreq(len(x), 1/SR)
    low = np.exp(-(f/145)**4)
    bass = fft_filter(x, low)
    x += bass.mean(axis=1, keepdims=True) - bass
    x = fft_filter(x, (1-np.exp(-(f/28)**6))*np.exp(-(f/14500)**12))
    # Mild parallel body saturation before loudness matching, transient retained.
    x = .85*x + .15*np.tanh(1.3*x)/1.3
    # No abrupt edges or looping assumptions. Tail tapers to exact digital zero.
    x[:96] *= np.sin(np.linspace(0,np.pi/2,96))[:,None]**2
    x[-5760:] *= np.cos(np.linspace(0,np.pi/2,5760))[:,None]**2
    # Remove residual finite-window offset without creating edge discontinuities.
    dc_window = np.ones(len(x))
    dc_window[:96] = np.sin(np.linspace(0,np.pi/2,96))**2
    dc_window[-5760:] = np.cos(np.linspace(0,np.pi/2,5760))**2
    x -= dc_window[:,None] * x.sum(axis=0) / dc_window.sum()
    x[0] = x[-1] = 0
    return x


def write_pcm24(path, x):
    # Non-noise-shaped TPDF dither at one 24-bit LSB; preserve exact endpoints.
    dither = (RNG.random(x.shape)-RNG.random(x.shape)) / 8388608
    dither[0] = dither[-1] = 0
    ints = np.rint((x+dither)*8388608).astype(np.int32)
    assert np.max(ints) < 8388607 and np.min(ints) > -8388608
    packed = np.empty((ints.size,3),dtype=np.uint8)
    flat = ints.reshape(-1)
    packed[:,0] = flat & 255; packed[:,1] = (flat>>8)&255; packed[:,2] = (flat>>16)&255
    with wave.open(str(path),'wb') as wav:
        wav.setnchannels(2); wav.setsampwidth(3); wav.setframerate(SR)
        wav.writeframes(packed.tobytes())


def main():
    folder = ROOT/'audition'; folder.mkdir(exist_ok=True)
    designs = {'Rift':make_rift,'Lancer':make_lancer,'Bastion':make_bastion}
    mixes = {name:prepare(fn()) for name,fn in designs.items()}
    initial = {name:loudness(x) for name,x in mixes.items()}
    # Common K-weighted level constrained by the worst crest factor, not by a limiter.
    target = min(-17.0, *[initial[name] + db(10**(-3.0/20)/true_peak(x)) for name,x in mixes.items()])
    report = {'sample_rate':SR, 'bit_depth':24, 'channels':2, 'seed':SEED,
              'python':platform.python_version(), 'numpy':np.__version__,
              'level_reference':'BS.1770 gated short-SFX loudness; not perceptual approval',
              'target_lufs':round(target,3), 'listening_status':'Manual headphone/speaker review pending; no claim of auditory QA',
              'unreal_status':'Not imported or runtime-tested', 'files':[]}
    for name,x in mixes.items():
        x *= 10**((target-initial[name])/20)
        path = folder/f'SW_TD_{name}_Switch_M02.wav'
        write_pcm24(path,x)
        item = {'ship':name,'file':str(path.relative_to(ROOT)).replace('\\','/'),
                'duration_s':len(x)/SR, 'sample_peak_dbfs':round(db(np.max(np.abs(x))),3),
                'true_peak_dbfs_4x_fft':round(db(true_peak(x)),3),
                'short_sfx_lufs':round(loudness(x),3), 'rms_dbfs':round(db(rms(x)),3),
                'dc_offset':float(np.max(np.abs(x.mean(axis=0)))),
                'stereo_correlation':round(float(np.corrcoef(x.T)[0,1]),4),
                'mono_downmix_peak_dbfs':round(db(np.max(np.abs(x.mean(axis=1)))),3),
                'tail_100ms_rms_dbfs':round(db(rms(x[-4800:])),3),
                'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
        report['files'].append(item)
    (ROOT/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
