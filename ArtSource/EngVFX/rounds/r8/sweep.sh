#!/usr/bin/env bash
# sweep.sh <tag>: a quiet Waves1to2 (waves skipped, enemies killed), then: ice breaks S and M via TestShot (captures right after
# the shot), and the first-ten-minutes effects at gameplay distance: each ship firing at a held fighter 120 m ahead (tracers,
# impacts, shield hits, the kill), a ship switch (the cut), and a dropped pickup.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-q}; L=Saved/Logs/td-play-mcp_${TD_PORT:-21031}.log
bash $G "('pie_start',)" >/dev/null; sleep 25
$P con:"TD.Stage Waves1to2" sleep:14 con:"TD.God" con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:3 con:"TD.SkipWave" sleep:3 con:"TD.KillAll" sleep:4 con:"TD.KillAll" >/dev/null
for c in 0 1; do
  $P con:"TD.KillAll" con:"TD.SpaceField.TestShot $c 14" sleep:0.12 cap:${T}_ice${c}_a sleep:0.12 cap:${T}_ice${c}_b sleep:0.25 cap:${T}_ice${c}_c sleep:0.4 cap:${T}_ice${c}_d sleep:2 >/dev/null
  grep -a "TDSpaceField: break" $L | tail -1 | cut -c1-200
done
$P con:"TD.KillAll" con:"TD.World.Hold 900 0" >/dev/null
for ship in Rift Lancer Bastion; do
  $P con:"TD.Squad.Switch $ship" sleep:0.15 cap:${T}_cut_${ship} sleep:1.5 con:"TD.KillAll" con:"TD.World.SpawnEnemy WallGunship 12000 0 0" sleep:1 \
     con:"TD.Input.Key LeftMouseButton 3" sleep:0.5 cap:${T}_fire_${ship}_1 sleep:0.6 cap:${T}_fire_${ship}_2 sleep:2.5 >/dev/null
done
$P con:"TD.KillAll" con:"TD.World.SpawnEnemy ShardFighter 9000 0 0" sleep:1 con:"TD.Dev.KillCentral" sleep:0.2 cap:${T}_expl_fighter sleep:0.5 cap:${T}_expl_fighter_b \
   con:"TD.World.SpawnEnemy WallGunship 14000 0 0" sleep:1 con:"TD.Dev.KillCentral" sleep:0.3 cap:${T}_expl_gunship sleep:0.8 cap:${T}_expl_gunship_b \
   con:"TD.Pickups.Spawn Power 80" sleep:1.5 cap:${T}_pickup >/dev/null
bash $G "('pie_stop',)" >/dev/null
