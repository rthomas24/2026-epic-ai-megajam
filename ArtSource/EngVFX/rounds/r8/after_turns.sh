#!/usr/bin/env bash
# after_turns.sh: wait (polling every 30 s, at most 3 h) until the render-slot owner.txt has shown each of T1 T3 T2 T4 T6 T5
# at least once (Lead: the builders go after the teammates' play turns), then return.
L=/c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/LOCK/owner.txt; seen=""
for i in $(seq 1 360); do
  o=$(head -c 3 "$L" 2>/dev/null | tr -d ' '); case " $seen " in *" $o "*) ;; *) [ -n "$o" ] && seen="$seen $o" && echo "$(date +%H:%M) seen $o";; esac
  ok=1; for t in T1 T3 T2 T4 T6 T5; do case " $seen " in *" $t "*) ;; *) ok=0;; esac; done
  [ $ok = 1 ] && { echo "all teammates had a turn: $seen"; exit 0; }
  sleep 30
done; echo "3 h passed; seen:$seen"; exit 0
