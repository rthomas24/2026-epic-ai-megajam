# Rift M01 — Phase 1 refined-foundation review

Agent: Ships & Weapons Agent  
Role: ship and modular weapon visual asset development  
Task: Rift refined foundation, technical validation and visual review

**Stage: technically verified source foundation; Nick's visual approval pending.**
No native import, main integration, gameplay validation or final AAA acceptance.
Bastion production has not started. Lancer geometry/materials/attachments/reviews
and its branch were preserved unchanged; the unmodified reference image has its
source SHA recorded in fleet_reference.json.

## Verified geometry and materials — pass02

| Item | Result |
|---|---|
| Assembly | Seven parts, six unique mesh roles, 38,624 triangles |
| Dimensions | 6.82 m length × 8.50 m span × 1.94 m height |
| Materials | Six shared opaque PBR materials, zero image textures |
| Topology | No boundary, non-manifold, wire, loose, duplicate, degenerate, inverted or inconsistent-normal findings |
| UVs / transforms | Valid UVs, zero reported zero-area/overlap/out-of-bounds UV findings; positive unit scales |
| Construction | Zero floating islands, coplanar overlaps, hidden back-to-back or enclosed findings |
| Locators | All 19 proposed local/root positions, axes and right-handed bases pass |
| Clearance | All six 100 m rays clear: three muzzles, two exhausts, portal exit |
| Emissive area | 1.7648% of assembled triangle surface, below 5% gate |
| Material values | Paint metallic0/roughness0.52; titanium metallic1/roughness0.39; canopy roughness0.28; composite0.73; magenta emission1.7 |

Vertex alpha supplies subtle roughness variation via the documented parameter
formula. No shader baking or texture dependencies are hidden in the package.
See validation_summary.json and geometry_qa.json for exact data and UV metrics.

| Assembled mesh | Triangles |
|---|---:|
| Hull | 18,280 |
| Pulse_L | 2,352 |
| Pulse_R, shared Pulse mesh | 2,352 |
| Splitter | 2,832 |
| PortalGuard | 4,272 |
| PortalRotor | 1,152 |
| Engine, twin nozzles | 7,384 |

The sole retained warning is lowest Z = -0.82 m: a flight-centered spacecraft
deliberately extends below its origin. Moving the origin to the floor would
break the intended mounting coordinates. No geometry warning was waived or QA
tolerance relaxed. Dynamic portal rotation, weapon collision, LODs, split-vertex
cost and actual engine performance remain for the later native handoff.

## Visual evidence and independent review

Nine 1800×1200 EEVEE material renders: front three-quarter, front, rear, side,
top, rear three-quarter, underside, isolated weapons and portal close-up. The
overview, six-angle clay, three actual-edge wire views and 128 px black silhouette
sources support construction inspection. The HTML also displays silhouettes at
64 px for Nick's readability decision; no automatic silhouette approval is claimed.
Wire curves are temporary diagnostic overlays and do not modify delivered meshes.

Fresh independent final review: silhouette4, proportions4, detail3, construction4,
shading4, materials3, style4, technical4 out of5 for this **bounded foundation
stage**, not a passing final hero/AAA rubric. Verdict: comparable to approved
Lancer foundation and sufficiently inspectable for Nick's explicit decision.

Known presentation limitations remain visible: the ring reads as a vertical bar
in a small side view; nose-ring spars are simpler than the flowing hull; rear and
underside structure is sparse; saturated violet appears lavender under the studio
lights and paint looks uniformly fresh. Finish variation remains subtle. No
additional decorative modeling is needed to reach this review gate; wait for
Nick's direction. Full assessment in art_review.md, first review in round01/.

## Failure history / resources

Pass01 exited0 after about10s but technical QA failed on coplanar triangle pairs:
Pulse35 each, Splitter12, Hull6, PortalGuard4. All other gates already passed.
Exact diagnostics and images are archived in round01. Pass02 separates coincident
hood/bearing caps, moves the charge shroud, increases coil clearance and lifts the
fore armor away from keel tangency. Paint/metal roughness was adjusted in response
to the independent reviewer. No new parts, details, locators or QA relaxation.

Both runs used Blender **5.1.1**, one headless process, two threads; the installed
version differs from the skills' target5.2. Pass02 launched with **5.63 GiB free
RAM** and no competing heavy process, exited0 at about14.6s, then released the slot.
The final render operation took9.72s. No measured peak RAM is available. The HIP
library warning concerns an unused AMD backend; EEVEE completed all renders.
No Unreal/editor/cook/gameplay test was run for these source-only assets.

Task began 2026-11-12 14:38:59 EST. Technical model/review rendering completed
15:13 EST, approximately34min including coordination/resource queue. Final total
duration including package/publication is reported in the completion message.
Exact Codex model, reasoning effort, task tokens and cost are unavailable; no
estimates are represented as measured usage. Tools/provenance in production/.

## Ownership and integration boundary

Exact source roots were published before edits at e4b5715, on nick/ships-rift in
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/ships-rift`. No ownership conflict
found. Only Rift/production/** and Rift/review/M01/** changed; no Ryan files,
shared maps/config, Blueprints, VFX/audio or native files changed. Lead performs
serialized fetch/precommit/publication and maintains shared claims/credits/log.

See production/README.md for hull+six root-relative part mapping, Engine-part
preference and vertex-color material reconstruction. **Engineering dependency:**
generic first-mesh Muzzle_0...N discovery does not provide weapon-specific Splitter
selection. Hull aliases cover the two primaries; Muzzle_Secondary is proposed for
the separate cannon. Ryan must review binding before native integration. Proposed
portal/camera/ability frames are art handoff, not newly implemented gameplay APIs.

Review/checkpoint publication is ready for Lead audit. Native and shared-main
asset integration remain held. Nick must approve Rift before Bastion production.
