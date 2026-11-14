#!/usr/bin/env bash
# after_owners.sh <prefix>...: wait (30 s polls, at most 3 h) until the render-slot owner.txt has shown an owner starting with
# each given prefix at least once (case-insensitive), then return. Example: after_owners.sh T4 T2 T3 str
L=/c/Users/ryant/Documents/myRepos/game-jam/Builds/render-slot/LOCK/owner.txt; seen=""
for i in $(seq 1 360); do
  o=$(awk '{print tolower($1); exit}' "$L" 2>/dev/null); [ -n "$o" ] && case " $seen " in *" $o "*) ;; *) seen="$seen $o"; echo "$(date +%H:%M) seen $o";; esac
  ok=1; for t in "$@"; do t=$(echo "$t" | tr 'A-Z' 'a-z'); hit=0; for s in $seen; do case "$s" in "$t"*) hit=1;; esac; done; [ $hit = 0 ] && ok=0; done
  [ $ok = 1 ] && { echo "all seen: $seen"; exit 0; }
  sleep 30
done; echo "3 h passed; seen:$seen"; exit 0
