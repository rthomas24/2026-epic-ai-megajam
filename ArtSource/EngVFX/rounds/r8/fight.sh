#!/usr/bin/env bash
# fight.sh <tag> [nostart]: a fight with the Bastion flown and firing (enemy bolts and Bastion shells on screen), PIE on
# $TD_PORT, slow motion 0.3, three captures, plus each through a deuteranopia matrix (Machado 2009, severity 1).
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-a}
if [ "$2" != "nostart" ]; then bash $G "('pie_start',)" >/dev/null; sleep 25; $P con:"TD.Stage Waves1to2" sleep:14 >/dev/null; fi
$P con:"god" con:"TD.Squad.Switch Bastion" sleep:1 con:"TD.World.SpawnEnemy ShardFighter 6000 1 -1500" con:"TD.World.SpawnEnemy ShardFighter 7000 1 1500" con:"TD.World.SpawnEnemy HookInterceptor 9000 1 0" sleep:2 >/dev/null
$P con:"TD.Input.Key LeftMouseButton 4" sleep:0.8 con:"slomo 0.3" sleep:0.4 cap:${T}_fight_1 sleep:1.2 cap:${T}_fight_2 sleep:1.2 cap:${T}_fight_3 con:"slomo 1" >/dev/null
cd Saved/EngVFX/r8
ffmpeg -y -loglevel error -i ${T}_fight_1.png -i ${T}_fight_2.png -i ${T}_fight_3.png -filter_complex "hstack=3,scale=2400:-1" ${T}_fight.png
ffmpeg -y -loglevel error -i ${T}_fight.png -vf "colorchannelmixer=rr=0.367:rg=0.861:rb=-0.228:gr=0.280:gg=0.673:gb=0.047:br=-0.012:bg=0.043:bb=0.969" ${T}_fight_deut.png
