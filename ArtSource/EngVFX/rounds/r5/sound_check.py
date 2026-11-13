#!/usr/bin/env python3
"""Independent Sound-field check: the audio asset path strings (/Game/.../Audio/..., SC_/SW_/MS_ names) stored in each data
asset, at a git revision vs the working tree. usage: sound_check.py <git rev> [--append]   (run from the repo root)"""
import re, subprocess, sys, glob
rev = sys.argv[1]
PAT = re.compile(rb'/Game/[A-Za-z0-9_/]*(?:Audio|Sound|Music)[A-Za-z0-9_/]*')
files = sorted(sum((glob.glob('Content/TriadDrift/Gameplay/Characters/%s/DA_*.uasset' % d) for d in ('FX', 'Squad', 'NullFleet', 'Dominion')), []))
out, differ = [], 0
for f in files:
    f = f.replace(chr(92), '/')
    new = open(f, 'rb').read()
    try:
        old = subprocess.run(['git', 'cat-file', '--filters', '%s:%s' % (rev, f)], capture_output=True, check=True).stdout
    except subprocess.CalledProcessError:
        out.append('%s | new file' % f); continue
    if old == new:
        continue
    a, b = sorted(set(PAT.findall(old))), sorted(set(PAT.findall(new)))
    differ += a != b
    out.append('%s | audio asset paths before=%d after=%d | %s' % (f, len(a), len(b), 'SAME' if a == b else 'DIFFERENT %s' % (set(a) ^ set(b))))
head = '==== independent check (sound_check.py): audio asset paths in each data asset changed since %s: %d files changed, %d differ' % (rev, len(out), differ)
txt = head + '\n' + '\n'.join(out) + '\n'
print(txt)
if '--append' in sys.argv:
    open('ArtSource/EngVFX/review/wire_sound_check.txt', 'a', encoding='utf-8').write(txt)
