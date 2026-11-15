# Future armor #215

Nicholas requested black armor for future Wren/Kato/Moss with magenta/cyan/amber visors, and approved the actual Session78 reactor screenshot on November15. This approval covers the displayed appearance; final cooked presence remains unverified.

The future-only M_FuturePilot master is cloned from M_Pilot. Its existing non-accent armor tint now covers middle luminances as well as bright panels. Existing visor mask, normal and ORM texture wiring remain. Runtime components receive individual dynamic instances and copy the source texture/uniform parameters before applying the future palette. Player and ally appearance code is unchanged.

Reproduction: Tools/td-make-future-armor.py, run headlessly on Entry with a private UserDir; refuses an existing destination. Original M_Pilot, MI_Pilot, three role instances and DA_Pilot_Visuals hashes were unchanged. Existing Characters always-cook directory covers this asset.

Editor51 and Shipping52 compile passed. Actual-parent isolation test51 passed for all three roles and unchanged shared/player material. Native78 used the original saved reactor approach, normal walking and paused inspection; both defenders approached/attacked normally. Wren and Kato are visible in the approved capture; Moss's palette application is logged but was not separately photographed. Exit0/133.9s. No health/damage/progression override.

The earlier parameter-only prototype3b847141 compiled but failed native appearance review in76; it is superseded by this master. The generated concept image was only a labeled preview and is not a shipping asset. No cook/package was performed on Nicholas's machine.
