"""Snapshot/check M01 sources, settings and assets without changing them."""
import argparse
import hashlib
import json
from pathlib import Path

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[4]
SNAPSHOT=HERE/'baseline_snapshot.json'


def files():
    native=HERE.parent
    manifest=json.loads((native/'review/delivery_manifest.json').read_text())
    result=[REPO/item['path'] for item in manifest['native_assets']]
    result += [path for path in native.iterdir() if path.is_file()]
    result += [path for path in native.parent.iterdir() if path.is_file()]
    result += [REPO/'unrealGameJam.uproject']+list((REPO/'Config').glob('*.ini'))
    return sorted(set(result))


def main():
    p=argparse.ArgumentParser();p.add_argument('--snapshot',action='store_true');args=p.parse_args()
    current={str(p.relative_to(REPO)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files()}
    if args.snapshot:
        assert not SNAPSHOT.exists(),'Refusing an existing baseline snapshot'
        SNAPSHOT.write_text(json.dumps(current,indent=2)+'\n');print('Baseline snapshot:',len(current),'files')
    else:
        expected=json.loads(SNAPSHOT.read_text())
        assert expected==current,'Baseline source/asset/settings changed'
        print('Baseline unchanged:',len(current),'files PASS')


if __name__=='__main__':main()
