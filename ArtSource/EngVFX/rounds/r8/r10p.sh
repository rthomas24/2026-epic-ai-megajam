#!/usr/bin/env bash
# r10p.sh <tag>: 10h. One lock turn (about 12 min): the cloak overlay material's mesh-particle flag, then the cloak off/on
# from the chase camera and from the side, then T6's StageBreak for classes 0-3 (L at 200 m) from the ship's view.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "cloak shell + StageBreak captures (about 12 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT $R/cloak_flag.py 2>&1 | tail -1 | cut -c1-200
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-p}
C() { (echo "CMD = '$1'"; cat $R/cloak_prev.py) > Saved/EngVFX/_cl.py; python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cl.py 2>&1 | tail -1 | cut -c1-120; }
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" >/dev/null
cat > Saved/EngVFX/_side.py <<'PY'
import sys, os, json, time
sys.path.insert(0, os.path.join(os.getcwd(), 'ArtSource', 'EngVFX', 'scripts'))
import rig
cam = rig.py('''
o = pawn.get_actor_location()
c = o + pawn.get_actor_right_vector() * 2500 + unreal.Vector(0, 0, 400)
rr = ML.find_look_at_rotation(c, o)
json.dumps({'loc': vec(c), 'rot': {'pitch': rr.pitch, 'yaw': rr.yaw, 'roll': rr.roll}})''')
rig.capture(cam, os.path.join(rig.ROOT, 'Saved', 'EngVFX', 'r8', sys.argv[1] + '.png'), 960, 540, 40)
print('side', sys.argv[1])
PY
C off; $P sleep:1 cap:${T}_cloak_off >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_side_off
C on;  $P sleep:1.5 cap:${T}_cloak_on_a sleep:0.4 cap:${T}_cloak_on_b >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_side_on
C off
for c in "0 150" "1 150" "2 200" "3 150"; do set -- $c
  $P con:"TD.World.Hold 0 0" con:"TD.SpaceField.StageBreak $1 $2 2.5" sleep:2.55 cap:${T}_break$1_a sleep:0.15 cap:${T}_break$1_b sleep:0.3 cap:${T}_break$1_c >/dev/null
  sleep 2
done
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
