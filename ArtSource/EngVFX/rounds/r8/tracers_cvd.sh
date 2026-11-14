#!/usr/bin/env bash
# tracers_cvd.sh <tag>: Bastion's shell and the enemy bolt side by side, attached to the flown ship (60 m/s) 20 m and 60 m
# ahead, seen from the chase camera; then the same through a deuteranopia matrix (Machado 2009, severity 1). PIE running.
cd /c/Users/ryant/Documents/myRepos/game-jam/.wt/vfx; export MSYS_NO_PATHCONV=1
G=ArtSource/EngVFX/rounds/r8/gpy.sh; P="python ArtSource/EngVFX/scripts/play.py ${TD_PORT:-21031} Saved/EngVFX/r8"; T=${1:-a}
$P con:"TD.KillAll" con:"TD.World.Hold 900 60" sleep:1 >/dev/null
bash $G "('clear',)" >/dev/null
for x in 2000 6000; do
  bash $G "('attach_set', 'NS_TD_Tracer_Bastion', ($x, -450, 250), {})" >/dev/null
  bash $G "('attach_set', 'NS_TD_Tracer_Enemy', ($x, 450, 250), {})" >/dev/null
done
$P sleep:1.5 cap:${T}_tracers >/dev/null; bash $G "('clear',)" >/dev/null
cd Saved/EngVFX/r8
ffmpeg -y -loglevel error -i ${T}_tracers.png -vf "crop=900:420:350:220" ${T}_tracers_c.png
ffmpeg -y -loglevel error -i ${T}_tracers_c.png -vf "colorchannelmixer=rr=0.367:rg=0.861:rb=-0.228:gr=0.280:gg=0.673:gb=0.047:br=-0.012:bg=0.043:bb=0.969" ${T}_tracers_deut.png
ffmpeg -y -loglevel error -i ${T}_tracers_c.png -i ${T}_tracers_deut.png -filter_complex hstack=2 ${T}_tracers_pair.png
