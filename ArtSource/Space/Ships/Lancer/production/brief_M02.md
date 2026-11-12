# Lancer M02 — production refinement

Nicholas approved the general M01 design direction and modular layout on
2026-11-12, but explicitly rejected its level of polish. This revision refines
that approved direction in Blender; it does not substitute an unrelated generated
hero hull. Final visual approval and Unreal integration remain separate gates.

Keep the 8 m needle interceptor, forward-swept wings, twin drive nacelles and
four weapon hardpoints. Preserve every M01 attachment name, parent-space position,
forward axis, unit scale and pivot; the canonical attachments.json is read-only.
M01 sources and review evidence are historical references and are not overwritten.

Refinement: a curved, layered fuselage with a proper cockpit rim and nose sensor;
airfoil wings with flowing roots; smaller raked stabilizers; recessed radiator
banks; machined nozzle petals, intakes and service panels; continuous mounting
cradles and shrouds for weapons. Detail belongs around engines, cockpit and weapon
interfaces, while broad armour panels provide calm areas and readable livery.

Palette: coated silver-white alloy, restrained cyan paint, dark composite joints,
bare machined titanium and small cyan emissive inserts. Six shared PBR materials,
per-panel vertex tint variation, opaque canopy, no external assets or shaders
that need an unprovided bake. Unreal reconstruction will need PanelTint vertex
colour multiplied by BaseColor plus the documented scalar material parameters.

Budget: target <=60k assembled triangles, six materials total, no unused slots,
positive unit transforms, UVs, no detached details or coplanar mount surfaces.
Individual reusable mount, minigun body/rotor, pod and rocket meshes remain separate.
Engineering selected +54 cm local-X visual offset for the centre-pivot rocket
(reply 8cd1561). This leaves its tail 4 cm beyond the pod lip; collision and muzzle
positions are unchanged. No firing logic or gameplay interface is implemented here.

Writable: production/** and review/M02/** only on nick/ships-lancer. Shared-main
claim published by Lead at 2d4831f before edits. No Unreal import, other ships,
environment/VFX work or main merge in this task. Use one scheduled Blender process
at a time with two render threads. Save versioned source and measured review
evidence; final polish is presented to Nicholas, not self-approved.

Coordination: engineering questions and claims publication go through Lead. This
worker does not edit shared coordination files. Before resumed edits and commits,
request Lead's latest remote ownership preflight.
