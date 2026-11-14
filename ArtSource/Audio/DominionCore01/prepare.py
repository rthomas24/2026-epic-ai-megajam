"""Preserve Nicholas's original core selection and prepare its complete import master."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import wave

EXPECTED_SOURCE = '4bec6bc4b5921ff65449701ddfb2560ed15e2b100f9c22798ad252bacc49307f'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--ffmpeg', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    original = root / 'source' / 'Cathedral Pulse.mp3'
    master = root / 'export' / 'MUS_DominionCore_CathedralPulse_Full_01.wav'
    manifest = root / 'manifest.json'
    assert args.source.is_file() and digest(args.source) == EXPECTED_SOURCE
    assert not original.exists() and not master.exists() and not manifest.exists(), 'Preserve previous outputs'
    original.parent.mkdir(parents=True, exist_ok=True)
    master.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.source, original)
    command = [args.ffmpeg, '-hide_banner', '-nostdin', '-n', '-i', str(original),
               '-map', '0:a:0', '-vn', '-map_metadata', '-1',
               '-af', 'aresample=48000:dither_method=triangular', '-ac', '2',
               '-c:a', 'pcm_s16le', '-bitexact', str(master)]
    completed = subprocess.run(command, capture_output=True, text=True, timeout=60)
    if completed.returncode:
        raise RuntimeError(completed.stderr)
    with wave.open(str(master), 'rb') as audio:
        frames, rate, channels, width = audio.getnframes(), audio.getframerate(), audio.getnchannels(), audio.getsampwidth()
        pcm_hash = hashlib.sha256(audio.readframes(frames)).hexdigest()
    assert rate == 48000 and channels == 2 and width == 2
    assert digest(original) == digest(args.source) == EXPECTED_SOURCE
    data = {
        'stage': 'source prepared; not imported, connected, loop-approved or gameplay verified',
        'selection': 'Original Cathedral Pulse for the core chamber; not Cathedral Pulse (1) used for credits',
        'source': {'path': original.relative_to(root).as_posix(), 'sha256': EXPECTED_SOURCE, 'bytes': original.stat().st_size},
        'master': {'path': master.relative_to(root).as_posix(), 'sha256': digest(master), 'pcm_sha256': pcm_hash,
                   'frames': frames, 'sample_rate': rate, 'channels': channels, 'bits_per_sample': width * 8,
                   'duration_seconds': frames / rate},
        'processing': {'trim': False, 'loop': False, 'crossfade': False, 'gain_db': 0,
                       'description': 'Full audio stream decoded to stereo 48 kHz PCM16 with triangular dither; embedded cover image excluded.',
                       'ffmpeg_version': subprocess.check_output([args.ffmpeg, '-version'], text=True).splitlines()[0]},
        'pending': ['native import and reload', 'core entry/exit timing agreement', 'loop decision if needed',
                    'actual gameplay listening approval', 'cook and Shipping validation']
    }
    manifest.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(data, indent=2))

if __name__ == '__main__':
    main()
