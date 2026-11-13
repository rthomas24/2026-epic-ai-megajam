# Localized shield hit

Stage: **SOURCE PROTOTYPE / NATIVE NOT RUN**. No native assets or gameplay
binding. HullImpact01 and muzzle d915088 remain preserved.

The hull strike uses short molten streaks; this shield response instead sends
interrupted energy arcs outward with a fainter delayed inner ripple. Uneven arc
lengths, small boundary warps and an elliptical projection break up the complete
neon-ring look. A tiny cool-white contact core disappears early. No hex overlay,
global shield bubble or screen flash is added. Approved squad palettes remain.

All angular harmonics use integer frequencies, so the atan2 wrap has no deliberate
seam. Pixel derivatives filter the thin rings and reduce their brightness as they
become subpixel. A fixed cutoff bounds visible support to the proposed 56 cm
sprite. This is authored support, not a measured Niagara component bound.

Target budget: one CPU sprite per shield impact, 160 ms, intensity 1.6, no image
textures, lights, ribbons, collision, volumetrics or tick Blueprint. Thin arcs
cover less visible area than a filled flash; the full quad still incurs normal
translucent raster/shader cost. No GPU/FPS saving is claimed before measurement.

Future material assets: M_ShieldImpact and three MI_ShieldImpact squad variants.
Use the established material-node recipe after Lead review. Reuse the verified
Niagara lifecycle path when available; no duplicated unproven native builder.
ParticleRelativeTime drives Age; the material alone does not enforce 160 ms or
implement the local+X 2 cm spawn offset. Those belong to the future emitter.

Read-only contract: HandleImpact selects ImpactShield while victim health reports
an active shield and passes the hit normal to PlayCue. Palette values must be
baked into material instances because PlayCue does not forward WeaponFX.Color.
This effect is a shield-hit cue, not a shield-break event. No gameplay changes or
Sound-field edits are made, and no shared presentation data asset is loaded/saved.

Native review needs fresh compile/reload, finite-lifetime captures, grazing and
rear-angle occlusion, thin-arc stability at gameplay resolution, rapid impacts
against enemies/reticles, bloom/exposure control, and actual frame cost. Human
approval of the earlier hull shader study does not approve this new shield look.

Provenance: original team procedural shader authored with Codex; read-only
StarterKit/ShieldImpact.ush supplied lifecycle context. No external textures or
media. Engine template use remains proposed until native authoring; Lead records
the actual source/tool/template provenance before integration.
