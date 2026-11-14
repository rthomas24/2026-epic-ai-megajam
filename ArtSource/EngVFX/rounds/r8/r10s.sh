#!/usr/bin/env bash
# r10s.sh <tag>: ONE queued lock turn (<= 15 min), in the Lead's order (12:40): the cloak shell material + emitter first
# (needs the renderer), then the breakup (T4's camera), the cloak through the REAL ability (unlock, press E in a wave) and
# through the preview, the tractor beam in a real dock, the plasma mines in a real wave.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1; export TD_PORT=${TD_PORT:-21031}; R=ArtSource/EngVFX/rounds/r8
unset NULLRHI; trap 'bash $R/ed.sh unlock' EXIT
bash $R/ed.sh lock "breakup, cloak (real E), tractor, mines (15 min)" || exit 1
echo "== locked turn $(date +%H:%M)"; bash $R/ed.sh start >/dev/null || exit 1
(echo "ONLY_MATS = ['M_TD_VFX_CloakShell2']"; cat ArtSource/EngVFX/scripts/build_materials.py) > Saved/EngVFX/_mats.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_mats.py 2>&1 | tail -1 | cut -c1-200
(echo "ONLY = ['__none__']"; cat ArtSource/EngVFX/scripts/build_tier1.py; echo; echo "ONLY2 = ['=NS_TD_Cloak_Shimmer']"; echo "CHUNK = (0, 1)"; cat ArtSource/EngVFX/scripts/build_tier2.py) > Saved/EngVFX/_chunk.py
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_chunk.py 2>&1 | grep "NS_TD_Cloak_Shimmer" | cut -c1-200
cat > Saved/EngVFX/_cloakfx.py <<'PY'
da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/FX/DA_Ability_Cloak_FX')
c = da.get_editor_property('loop')
'Loop.vfx=%s scale=%s' % (c.get_editor_property('vfx'), c.get_editor_property('vfx_scale'))
PY
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakfx.py 2>&1 | tail -1 | cut -c1-200
G=$R/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py $TD_PORT Saved/EngVFX/r8"; T=${1:-s}
bash $G "('pie_start',)" >/dev/null; sleep 25
echo "-- breakup $(date +%H:%M:%S)"
$P con:"TD.Stage ReactorExposed" sleep:14 con:"god" con:"TD.Director.ReactorKill" sleep:1 cap:${T}_breakup_1 sleep:1.5 cap:${T}_breakup_2 sleep:1.5 cap:${T}_breakup_3 sleep:1.5 cap:${T}_breakup_4 sleep:1.5 cap:${T}_breakup_5 sleep:1.5 cap:${T}_breakup_6 sleep:2 cap:${T}_breakup_7 sleep:3 cap:${T}_breakup_8 >/dev/null
echo "-- cloak, real ability $(date +%H:%M:%S)"
$P con:"TD.Stage Waves1to2" sleep:12 con:"god" con:"TD.KillAll" sleep:2 con:"TD.World.Hold 900 0" con:"TD.Unlock Cloak Rift" sleep:1 cap:${T}_cloak_off >/dev/null
python Saved/EngVFX/_side.py ${T}_cloak_side_off
$P con:"TD.Input.Key E 0.2" sleep:1.0 cap:${T}_cloak_E_a >/dev/null; python Saved/EngVFX/_side.py ${T}_cloak_E_side; $P sleep:0.5 cap:${T}_cloak_E_b >/dev/null
cat > Saved/EngVFX/_cloakq.py <<'PY'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
n = [str(k.get_asset().get_name()) + (' active' if k.is_active() else ' inactive') for k in pawn.get_components_by_class(unreal.NiagaraComponent) if k.get_asset()]
'niagara on the flown ship: %s' % n
PY
python ArtSource/EngVFX/scripts/run_editor_py.py $TD_PORT Saved/EngVFX/_cloakq.py 2>&1 | tail -1 | cut -c1-400
echo "-- dock $(date +%H:%M:%S)"
$P con:"TD.Stage DominionDocking" sleep:14 con:"god" sleep:2 cap:${T}_dock_1 sleep:1.5 cap:${T}_dock_2 sleep:1.5 cap:${T}_dock_3 sleep:1.5 cap:${T}_dock_4 >/dev/null
echo "-- mines $(date +%H:%M:%S)"
$P con:"TD.Stage Wave5" sleep:12 con:"god" con:"TD.KillAll" sleep:1 con:"TD.World.Hold 900 0" con:"TD.World.SpawnEnemy MaulBomber 25000 3000 0" con:"TD.World.SpawnEnemy MaulBomber 30000 -3000 1500" sleep:9 cap:${T}_mines_1 sleep:2 cap:${T}_mines_2 sleep:2 cap:${T}_mines_3 >/dev/null
bash $G "('pie_stop',)" >/dev/null
bash $R/ed.sh unlock; echo "== closed $(date +%H:%M)"
