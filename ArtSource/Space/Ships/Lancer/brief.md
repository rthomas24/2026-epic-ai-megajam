# Lancer / M01 source milestone

Owner: Nicholas / Dedicated Ship & Weapons chat, `nick/ships-lancer`.
Writable: this folder only. Lead owns claims, root credits/devlog and integration.

Status: source prototype and silhouette review, not final hero art or an imported Unreal asset.
No binary production, Blender run, Unreal launch or gameplay changes are authorized for this milestone.

Purpose: a readable interceptor from a chase camera at 6–30 m. Shape language:
needle, forward swept, twin drive. Target length 800 cm, span about 680 cm.
Ice-white ceramic armour (#E8F4FF), cyan wing panels (#12C8FF), charcoal joints,
small cyan engine/overdrive inserts (#3FE9FF). Opaque canopy; no translucency.

Primary silhouette: long chamfered prow, compact raised canopy ahead of the centre,
two aft drive nacelles, wings reaching forward at their outer tips. Large quiet
armour panels contrast with clustered machinery at wing roots and engine collars.
Keep both guns visible below the chines; rocket pods hang from exposed pylons.
Modules have rear mounting flanges and common +X firing axes. Engine inserts and
dorsal overdrive spine are meshes only; trails, flashes and ability effects belong
to the existing VFX Agent.

Deliverable: deterministic JSON triangle-mesh sources, separate reusable mount,
six-barrel minigun body/rotor, four-tube rocket pod, spike rocket, attachment
manifest, SVG multi-view review and an offline interactive preview. The hull is
explicitly a code-built design proxy. Final hero hull follows the repository's
concept/image-to-3D or licensed-kitbash route after silhouette approval.

Target assembled source budget: below 20k triangles, <=6 materials total, no
external textures or network dependencies. This is a prototype budget, not a
claim of final art quality. Collision, LODs, authored UVs, final shading and FBX
round trip remain production gates. Weapon components remain separately reusable.

Coordinates: every JSON position is centimetres, +X nose, +Y ship right, +Z up.
Hull pivot is the assembly origin (0,0,0). Weapon pivot is the mount interface;
the rotor pivot lies on its spin axis. All instances use unit scale.

Reference: project art-direction.md and aaa-quality-bar.md, features.md ART ship
requirements and presentation contracts. Original design; no external models,
textures, copied franchise silhouettes or third-party runtime libraries.

Later silhouettes, not in this milestone: Rift uses a crescent and portal ring,
magenta energy-cannon mounts and missile rails; Bastion uses a broad armoured hull,
heavy rotary mounts, rocket pods, dome emitter and a charged-cannon cradle. Their
meshes and ability-effect components are deliberately deferred until Lancer review.
