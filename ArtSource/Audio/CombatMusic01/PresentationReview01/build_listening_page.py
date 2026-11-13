"""Index existing private audio without rewriting media or presenting it as game capture."""
from pathlib import Path
import hashlib
import html
import json
import subprocess

ROOT = Path(__file__).resolve().parents[4]
PRIVATE = ROOT/'Saved/AudioMusic/PrototypeV1'
OUT = PRIVATE/'ListeningReview01'


def build():
    assert not OUT.exists(), 'Preserve existing review; no overwrite'
    items = [
        ('PR11 menu theme — 1:47', 'MenuTheme01/export/MUS_Menu_EtherialPulse_01.wav',
         'Nick’s original Suno track. Prepared source preview, not a game recording. Browser Loop repeats the file; it does not prove the Unreal join.'),
        ('Combat theme — 2:19.2', 'CombatMusic01/MUS_Combat_EtherealPulseRemix_01.wav',
         'Nick’s original Ethereal Pulse (Remix). Prepared source level; Unreal Wave volume is 0.5. Combat binding and playback are pending.'),
    ]
    for group, folder, prefix, source in [
        ('PR12 Lancer fire', 'prepared/NeedleCannonShort', 'SFX_NeedleCannon', 'AK-47 with animations / A.D'),
        ('PR12 hull impact', 'export/Hull', 'SFX_Combat_HullImpact', '50 Free Game Sounds Pack / PlaceHolder'),
        ('PR12 shield impact', 'export/Shield', 'SFX_Combat_ShieldImpact', '50 Free Game Sounds Pack / PlaceHolder'),
    ]:
        for take in range(1, 4):
            items.append((f'{group} — take {take}', f'Combat01/{folder}/{prefix}_{take:02}.wav',
                          f'{source}. Exact prepared input used by the native candidate; source level, without runtime pitch/volume variation or distance.'))
    items.append(('PR12 rapid-fire overlap — 5 seconds', 'Combat01/stress_20hz_final.wav',
                  'Existing offline stress mix at proposed cue gains. -19.1 LUFS / -4.2 dBTP. Not captured in game.'))
    rows, cards = [], []
    for title, relative, note in items:
        path = PRIVATE/relative
        assert path.is_file(), path
        probe = json.loads(subprocess.check_output([
            r'C:\Program Files\Shotcut\ffprobe.exe', '-v', 'error', '-show_streams',
            '-show_format', '-of', 'json', str(path)], text=True))
        stream = probe['streams'][0]
        seconds = float(probe['format']['duration'])
        fmt = {'duration_s': seconds, 'rate': int(stream['sample_rate']),
               'channels': stream['channels'], 'bits': int(stream.get('bits_per_sample', 0))}
        rows.append({'title': title, 'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), **fmt})
        cards.append(f'<section><h2>{html.escape(title)}</h2><p>{html.escape(note)}</p><audio controls preload="none" src="{html.escape(path.as_uri(), quote=True)}"></audio><label><input type="checkbox" onchange="this.parentElement.previousElementSibling.loop=this.checked"> Loop this preview</label><p>{seconds:.3f} s · {fmt["rate"]} Hz · {fmt["channels"]} channels</p></section>')
    missing = [
        'PR11: actual menu playback across 107 seconds, including the loop join; New Game fade and menu return, with one menu voice and no combat track.',
        'PR12: actual current-host Lancer bursts plus hull/shield hits in context. Prior logs prove playback, but no PCM recording was saved. Compare near and distant cues and repeated overlap.',
        'Combat theme: real wave and ground-encounter entry, hold/exit and new fight without restart; at least 142 seconds of continuous play to hear the wrap.',
        'Intensity: matched quiet/full-fight recording and component-gain evidence after engineering connects MusicIntensity01 to the actual mix.',
        'Settings and cleanup: Music slider/mute, CUT/retake, victory, pause-menu return and travel; retain actual output-device and build details.',
    ]
    body = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Triad Drift listening review</title><style>body{max-width:920px;margin:36px auto;padding:0 20px;background:#10151d;color:#e6edf7;font:16px/1.5 system-ui}h1{font-size:30px}h2{font-size:20px}section{padding:18px;margin:16px 0;border:1px solid #344254;border-radius:10px}audio{width:100%}p,li{color:#c2cddc}label{display:block;margin-top:10px}button{padding:10px 16px}</style><h1>Triad Drift — listening review</h1><p>Existing menu, combat music and PR12 effects. These are prepared-file previews and an offline stress mix. None is an in-game audio recording. Nick approved the original menu listening; the loop join and PR12 context still need his review. All media is preserved unchanged.</p><button onclick="document.querySelectorAll('audio').forEach(a=>a.pause())">Stop playback</button>'''
    body += ''.join(cards)+'<section><h2>Still needed from the current game</h2><ul>'+''.join('<li>'+html.escape(x)+'</li>' for x in missing)+'</ul><p>Private team review only. Existing owned/free sources; no purchase, download, conversion, synthesis, native launch or integration performed for this page.</p></section>'
    body += "<script>document.querySelectorAll('audio').forEach(a=>a.addEventListener('play',()=>document.querySelectorAll('audio').forEach(b=>{if(b!==a)b.pause()})));</script></html>"
    OUT.mkdir(parents=True)
    (OUT/'index.html').write_text(body, encoding='utf-8')
    (OUT/'media_receipt.json').write_text(json.dumps({'stage': 'Existing source previews only', 'files': rows, 'missing_in_game_recordings': missing}, indent=2)+'\n')
    print(json.dumps({'page': str(OUT/'index.html'), 'existing_media': len(rows), 'media_modified': 0}))


if __name__ == '__main__':
    build()
