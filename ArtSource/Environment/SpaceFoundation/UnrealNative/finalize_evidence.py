"""Lightweight integrity/inventory check; no editor or asset mutation."""
import ast
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import time
import zlib

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
provenance = json.loads((HERE / 'provenance.json').read_text())
for name, expected in provenance['frozen_source_sha256'].items():
    assert hashlib.sha256((HERE.parent / name).read_bytes()).hexdigest() == expected, name
for source in HERE.glob('*.py'):
    ast.parse(source.read_text(), filename=str(source))
content = REPO / 'Content/TriadDrift/Environment/SpaceFoundation'
assets = sorted(p for p in content.rglob('*') if p.suffix in ('.uasset', '.umap'))
captures = [HERE / 'review' / (name + '.png') for name in (
    'Hero_Panorama', 'Combat_Panorama', 'Verdance_Panorama', 'Hero_LongitudeSeam', 'Hero_NorthPole')]
assert len(assets) == 12, len(assets)
assert all(p.stat().st_size > 1000 for p in assets + captures)


def validated_png_prefix(path):
    """Validate every chunk/CRC; return original bytes through a valid IEND."""
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', path
    offset, count, has_idat = 8, 0, False
    while offset + 12 <= len(data):
        length = struct.unpack_from('>I', data, offset)[0]
        end = offset + 12 + length
        assert end <= len(data), ('PNG chunk out of bounds', path, offset, length)
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from('>I', data, offset + 8 + length)[0]
        assert zlib.crc32(kind + payload) & 0xFFFFFFFF == crc, ('PNG CRC mismatch', path, kind)
        if count == 0:
            assert kind == b'IHDR' and length == 13
            assert struct.unpack_from('>II', payload) == (1280, 720)
        has_idat = has_idat or kind == b'IDAT'
        count += 1
        if kind == b'IEND':
            assert length == 0 and has_idat
            return data, data[:end], count
        offset = end
    raise RuntimeError('No valid PNG IEND: ' + str(path))


# Preflight all five files before changing any container. No image decoding,
# encoding or pixel edits: the complete PNG prefix remains byte-for-byte identical.
png_data = [(path, *validated_png_prefix(path)) for path in captures]
png_checks = []
for path, original, prefix, count in png_data:
    if len(original) != len(prefix):
        temporary = path.with_name(path.name + '.tailfix.tmp')
        assert not temporary.exists(), temporary
        temporary.write_bytes(prefix)
        temporary.replace(path)
    assert path.read_bytes() == prefix, ('PNG prefix changed', path)
    png_checks.append({'image': path.name, 'chunks_crc_valid': count,
                       'valid_png_bytes': len(prefix), 'removed_trailing_bytes': len(original) - len(prefix),
                       'pixel_payload_unchanged': True,
                       'valid_prefix_sha256': hashlib.sha256(prefix).hexdigest()})
(HERE / 'review' / 'png_integrity.json').write_text(json.dumps(png_checks, indent=2) + '\n')

def entry(path):
    return {'path': path.relative_to(REPO).as_posix(), 'bytes': path.stat().st_size,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

manifest_path = HERE / 'review' / 'delivery_manifest.json'
previous_manifest_sha = None
if manifest_path.exists():
    previous_bytes = manifest_path.read_bytes()
    previous_manifest = json.loads(previous_bytes)
    previous_manifest_sha = hashlib.sha256(previous_bytes).hexdigest()
    previous_assets = {entry['path']: entry['sha256'] for entry in previous_manifest['native_assets']}
    current_assets = {entry(p)['path']: entry(p)['sha256'] for p in assets}
    assert previous_assets == current_assets, 'Native assets changed since frozen manifest'
manifest = {'generated_utc': datetime.now(timezone.utc).isoformat(),
    'task_started_utc': '2026-11-12T17:27:00+00:00',
    'elapsed_seconds_at_freeze': round(time.time() - 1794504420),
    'branch': 'nick/environment-vfx', 'engine': '5.9.0-60351404+++UE5+Release-5.9',
    'frozen_source_hashes_unchanged': True,
    'previous_manifest_sha256': previous_manifest_sha,
    'native_assets_unchanged_from_previous_manifest': bool(previous_manifest_sha),
    'png_valid_prefixes_and_pixel_payloads_unchanged': True,
    'native_assets': [entry(p) for p in assets], 'native_captures': [entry(p) for p in captures],
    'native_asset_bytes': sum(p.stat().st_size for p in assets),
    'native_capture_bytes': sum(p.stat().st_size for p in captures)}
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps({k: v for k, v in manifest.items() if k not in ('native_assets', 'native_captures')}, indent=2))
