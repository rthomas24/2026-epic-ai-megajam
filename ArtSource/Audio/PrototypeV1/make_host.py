"""Stage the isolated host in the owned ignored root. Does not launch or build."""
from pathlib import Path
import shutil
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
HOST=ROOT/'Saved/AudioMusic/PrototypeV1/Host'
assert HOST.resolve().is_relative_to((ROOT/'Saved/AudioMusic/PrototypeV1').resolve())
shutil.copytree(HERE/'HostTemplate',HOST,dirs_exist_ok=True)
(HOST/'Content').mkdir(exist_ok=True)
print(HOST/'TDAudioV1.uproject')

