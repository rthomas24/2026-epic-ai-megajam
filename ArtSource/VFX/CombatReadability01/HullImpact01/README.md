# Compact hull strike

Stage: **SOURCE PROTOTYPE / NATIVE NOT RUN**. No native assets, gameplay binding,
visual approval or measured frame cost. The published muzzle checkpoint d915088
and its native proposal remain unchanged.

The shader adds a brief contact flash and four irregular moving molten streaks.
Their lengths, directions, travel and extinction times differ. A faint expanding
ejecta patch fills the contact point without a uniform shock ring, regular radial
star, full-screen flash or lingering smoke. Approved squad tints remain visible.
Normalized particle age controls the whole event; no world-clock setup is needed.
Pixel derivatives soften subpixel streaks and attenuate their brightness as they
widen. A fixed circular cutoff keeps visible support within the proposed 40 cm
sprite. Those are authored bounds, not measured Niagara component bounds.

The intended budget is one CPU sprite per impact, 120 ms, intensity 2.0, no image
textures, lights, collision, ribbons, decals, simulation volumes or tick Blueprint.
The four streaks are projected shader detail, not four simulated debris particles.
Use the existing Niagara authoring recipe only after its capability has been
verified; do not duplicate unproven lifecycle code into another native builder.

`build_materials.py` implements only the four material assets in the owned
HullImpact01 destination: M_HullImpact and MI_HullImpact_{Rift,Lancer,Bastion}.
It uses the existing material-node wiring pattern, refuses all existing outputs,
checks the exact owned content mount, compiles every material/instance before
saving, and retains input hashes, statistics or a failure traceback. It has not
run in Unreal. Materials alone do not implement the 120 ms lifetime or the
outward position offset; those require the later verified Niagara emitter.
Do not bind these material assets into a Niagara-system cue slot.

Read-only contract inspection: TDProjectile::HandleImpact selects ImpactHull when
the victim has health and no active shield, and passes ImpactNormal.Rotation to
PlayCue. The proposed local +X 2 cm spawn offset therefore follows the hit normal.
PlayCue currently spawns at a world location and does not attach the burst or pass
weapon color. Material instances must carry the palette. No shared cue is edited.

Native checks still needed: template one-shot/count/size/position and lifespan;
compile/stack diagnostics and fresh reload; fixed-exposure lifetime captures;
front/grazing/rear camera occlusion and contact clipping; small-pixel sparkle;
normal/boost motion separation; distinguish hull and shield hits; no doubled
placeholder; readable enemies/reticle under rapid firing; actual draw/frame cost.
Preserve Audio's Sound fields and leave shield/world-impact slots unchanged.

Provenance: original team procedural HLSL authored with Codex, informed by the
read-only StarterKit/HullImpact.ush lifecycle intent. No downloaded textures or
third-party media. Epic template paths are proposed scaffolding only. Lead records
the eventual source/tool/template provenance before integration.
