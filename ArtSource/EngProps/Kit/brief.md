# Kit: the planet-outpost modular kit (brief)

Requested by Ryan's Lead, Nov 13 evening, after Ryan watched the walk test on Grok Bot's Rodin relay and wreck:
"we need to use something else to polish these up and make them sharp and walkable on and in." New rule: nothing the
player walks on, into or through is an AI mesh; those become reference or out-of-reach exterior dressing.

- **Purpose and camera:** everything the on-foot pilot walks on, into or through on Verdance and Cinder: the relay
  climb, the wreck interior, the research outpost and console room, Cinder's dig-site cabin. Third-person camera at
  2-6 m, first person at 0.5-3 m, the approach at 15-300 m.
- **Route C** (code-built, `scripts/kit_pieces.py` on the DominionInterior helper library), original work.
- **Shape language:** clean, engineered, modular. Light composite panels in dark gunmetal frames, hazard yellow on
  everything you can bump into, light strips that carry the planet's colour (teal on Verdance, ember on Cinder).
- **Grid and pivots:** 50 cm grid, 400 cm modules; pivot on the walkable plane (Z = 0) at the centre of the
  footprint; +X forward; Unreal centimetres.
- **Walkability (the pilot is 180 cm, capsule radius 34-40 cm):** floor collision top = visual floor top; ramps 14
  degrees (100 cm over 400); stairs with 50 cm level treads and 12.5 cm risers over a ramp collision through the middle
  of each tread (feet within 6.25 cm of the tread); doorways 160 x 260 clear; rooms 300 cm to the roof; railings 110 cm;
  walls block the pawn.
- **Finish:** every edge sharper than 30 degrees bevelled 1.5 cm in 2 segments, hardened and weighted normals.
- **Material:** one 2K trim sheet (`T_Kit_Trim_BC / _N / _ORM`) for everything, plus a Glow slot (teal or ember
  instance) and a Screen slot. No per-piece textures.
- **Budget:** under 2,500 triangles per piece after bevels.
- **Owner:** Ryan's side (Environment moved to Ryan on Nov 13). Sources `ArtSource/EngProps/Kit/**`, Unreal
  `Content/TriadDrift/Gameplay/Props/Kit/**`.
