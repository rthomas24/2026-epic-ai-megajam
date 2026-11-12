"""Reuse the frozen generator; write only this owned UnrealNative directory."""
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent


def load_source(name):
    path = SOURCE / (name + '.py')
    spec = importlib.util.spec_from_file_location('td_frozen_' + name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    generator = load_source('generate_sky')
    print(generator.validate())
    export = HERE / 'export'
    export.mkdir(exist_ok=True)
    outputs = [export / 'T_TD_Nebula_BC.png', export / 'T_TD_Stars_BC.png', HERE / 'presets.json']
    if any(p.exists() for p in outputs):
        raise RuntimeError('Refusing existing outputs; inspect before rebuilding')
    width, height = 1024, 512
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            pixels.extend(round(c * 255) for c in generator.sample_nebula(x / width, y / (height - 1)))
    generator.png(outputs[0], width, height, pixels)
    generator.png(outputs[1], 2048, 1024, generator.make_stars(2048, 1024))
    # An exact data copy feeds the frozen builder's SOURCE override.
    outputs[2].write_bytes((SOURCE / 'presets.json').read_bytes())
    provenance = {
        'frozen_source_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in SOURCE.iterdir() if p.is_file()},
        'source_checkpoint': 'e2d4139',
        'generator_validation': generator.validate(),
        'outputs': {p.name: {'bytes': p.stat().st_size,
                            'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in outputs},
        'third_party_assets': [],
        'tools': ['OpenAI Codex', 'Unreal Engine 5.9 Python', 'Python standard library'],
        'author': 'Nicholas Tosti / Environment & VFX Agent',
    }
    (HERE / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(provenance['outputs'], indent=2))


if __name__ == '__main__':
    main()
