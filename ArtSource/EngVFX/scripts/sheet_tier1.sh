#!/usr/bin/env bash
# Triad Drift ENG-37: capture the tier-1 review frames from the editor's Play-In-Editor session (port 8017).
# usage: sheet_tier1.sh <round tag, e.g. r1> [section ...]   sections: oneshots engines tracers (default: all)
# Output: ArtSource/EngVFX/review/tier1_<tag>_<section>.png. Every frame is a real in-game capture on L_Galaxy.
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; cd "$ROOT"
G="$HERE/g.sh"; TAG="$1"; shift; SECTIONS="${*:-oneshots engines tracers}"
CAP="Saved/EngVFX/cap"; OUT="ArtSource/EngVFX/review"; mkdir -p "$OUT"
FF="ffmpeg -y -loglevel error"
strip() { # strip <name> <n frames> <crop>: stack frames <name>_1..n vertically after cropping
  local in="" fc="" n=$2; for i in $(seq 1 $n); do in="$in -i $CAP/$1_$i.png"; fc="$fc[$((i-1))]crop=$3[c$i];"; done
  local lab=""; for i in $(seq 1 $n); do lab="$lab[c$i]"; done
  $FF $in -filter_complex "${fc}${lab}vstack=inputs=$n" "$CAP/$1_strip.png"
}
row() { # row <name> <python list of systems> <dist> <gap> <z> <frames> <crop>
  $G con "slomo 0.03" >/dev/null; $G py "CMD = ('clear',)" >/dev/null
  $G py "CMD = ('row', $2, $3, $4, 90, $5)" >/dev/null
  for i in $(seq 1 $6); do $G cap "$1_$i" 1600 >/dev/null; done
  strip "$1" "$6" "$7"
}
$G con "TD.God 1" >/dev/null
for s in $SECTIONS; do case $s in
 oneshots)
  row os_expl "['NS_TD_Explosion_Small','NS_TD_Explosion_Medium','NS_TD_Explosion_Large']" 3600 1900 300 4 "1500:560:50:60"
  row os_imp "['NS_TD_Impact_Hull','NS_TD_Impact_Shield','NS_TD_Muzzle_Rift','NS_TD_Muzzle_Lancer','NS_TD_Muzzle_Bastion','NS_TD_Muzzle_Enemy']" 1300 420 230 3 "900:220:300:380,scale=1500:-1:flags=lanczos"
  $FF -i $CAP/os_expl_strip.png -i $CAP/os_imp_strip.png -filter_complex "[0][1]vstack" "$OUT/tier1_${TAG}_oneshots.png" ;;
 engines)
  row en_a "['NS_TD_Engine_Rift','NS_TD_Engine_Lancer','NS_TD_Engine_Bastion','NS_TD_Engine_Enemy','NS_TD_EngineBoost_Lancer']" 1300 520 230 2 "1400:300:100:210"
  $G py "CMD = ('clear',)" >/dev/null; $G con "slomo 1" >/dev/null
  $G py "CMD = ('engine', 'NS_TD_Engine_Rift', 0.0, (-215, 0, 0))" >/dev/null; sleep 1; $G cap en_b_1 1600 >/dev/null
  $G py "CMD = ('clear',)" >/dev/null; $G py "CMD = ('engine', 'NS_TD_Engine_Rift', 1.0, (-215, 0, 0))" >/dev/null; sleep 1; $G cap en_b_2 1600 >/dev/null
  $FF -i $CAP/en_b_1.png -i $CAP/en_b_2.png -filter_complex "[0]crop=400:300:600:480,scale=700:525:flags=lanczos[a];[1]crop=400:300:600:480,scale=700:525:flags=lanczos[b];[a][b]hstack" $CAP/en_b_strip.png
  $FF -i $CAP/en_a_strip.png -i $CAP/en_b_strip.png -filter_complex "[0][1]vstack" "$OUT/tier1_${TAG}_engines.png" ;;
 tracers)
  n=0; in=""; fc=""; lab=""
  for pair in "Rift:NS_TD_Tracer_Rift" "Lancer:NS_TD_Tracer_Lancer" "Bastion:NS_TD_Tracer_Bastion" "Lancer:NS_TD_Tracer_Enemy"; do
    ship="${pair%%:*}"; sys="${pair##*:}"
    $G py "CMD = ('clear',)" >/dev/null; $G con "slomo 1" >/dev/null; $G con "TD.Squad.Switch $ship" >/dev/null; sleep 2
    $G con "slomo 0.12" >/dev/null; $G con "TD.Input.Keys LeftMouseButton 6" >/dev/null; sleep 3
    $G con "slomo 0.004" >/dev/null; $G py "CMD = ('tracers', '$sys')"; sleep 2; $G cap "tr_${sys}" 1600 >/dev/null
    in="$in -i $CAP/tr_${sys}.png"; fc="$fc[$n]crop=1100:330:250:260[c$n];"; lab="$lab[c$n]"; n=$((n+1))
  done
  $FF $in -filter_complex "${fc}${lab}vstack=inputs=$n" "$OUT/tier1_${TAG}_tracers.png"
  $G con "slomo 1" >/dev/null ;;
esac; done
echo "sheets: $(ls $OUT | tr '\n' ' ')"
