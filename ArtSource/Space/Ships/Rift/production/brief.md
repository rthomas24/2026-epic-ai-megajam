# Rift M01 — refined fleet foundation

Agent: Ships & Weapons Agent. Role: ship and modular weapon visual development.
Task start: 2026-11-12T19:38:59Z (14:38:59 EST).

Nick's direct fleet brief authorizes a crescent-wing portal specialist developed
to the approved Lancer M02 refined-foundation standard. This is Phase 1 only:
stop for Nick's manual review before Bastion. Preserve Lancer 4b7ef12 and PR #3.
Final AAA polish, native integration and gameplay validation are separate gates.

## Design

Shape language: crescent, flowing, precise. Approximately 7 m long, 8.4 m wide,
1.9 m high; seen at 6–30 m chase distance. A broad curved monocoque embraces an
open nose portal ring. Its thin outer horns and compact central cockpit distinguish
it from Lancer's needle and Bastion's future armored mass. Continuous airfoil
sections replace flat plank wings; functional detail clusters at cockpit, drives,
energy cannons and ring bearings. Calm violet armor provides the primary read.

Violet #6B2BD9 and deep plum #2A0F4F, magenta #FF2BD6 in controlled emitter bands,
dark composite, bare titanium and restrained light ceramic fleet trim. Opaque
canopy, moderate gloss, dielectric paint, rougher exposed machinery, no bloom-led
presentation. Six shared flat PBR materials with per-panel color/finish variation.
Target <=60k fully assembled triangles, with density on curves rather than bolts.

Two modular Phase Pulse cannon modules sit in wing-root fairings; one central
Splitter charge-cannon cradle fits under the forward keel. Ring guard and inner
rotor remain separate, with a deliberate X-axis spin pivot. Twin engines share an
Engine component with local exhaust locators. No firing, effects or power logic.

## Workflow / routing reconciliation

Read AGENTS.md, AAA.md, CLAIMS.md, MESSAGEBOARD.md and features.md ART-03 before
edits. No existing Rift/Bastion ship source was found in the project. The local
asset-routing guide restricts code-only heroes to proxies; features.md ART-03
expressly permits "route A ... or the kits". Lead dispatch at main e4b5715 resolves
this task to the repository's licensed MIT hard-surface kit, following Nick's
approved kit-based Lancer foundation standard. No shared skill was rewritten.
This choice does not establish final hero/AAA acceptance. No paid generator,
browser generation, downloaded model or external texture is used.

The direct written design and requested refined-model review define this bounded
foundation task. Provide measured QA, full material/clay/wire/silhouette evidence
and fresh independent art review, then stop for Nick. Do not silently declare
the stage final because it renders or passes automated geometry checks.

## Ownership and contract

Published claim e4b5715: write ONLY Rift/production/** and Rift/review/M01/**
on nick/ships-rift, isolated Worktrees/ships-rift. Reserved native Rift/Bastion
and Bastion source paths remain read-only. Lead owns synchronization, shared
boards, commit/push preflight and integration. Schedule every Blender run with
Lead, one two-thread process, fresh RAM/process check on this 16 GB PC.

Current engineering supports UTDShipVisuals.Mesh plus FTDShipVisualPart entries,
ParentSocket=None with root-relative transforms and MaterialOverrides. Prefer a
part named Engine; do not add a runtime interface. Record new Rift locators in
UE centimetres (+X forward,+Y right,+Z up), preserving component-local pivots.
Blender metres use -Y forward,+X right,+Z up. Socket choices are proposed art
handoff for engineering review; no canonical Rift names previously existed.

Persistent reporting: every completion, partial or blocker report begins Agent,
Role and Task, includes branch/worktree, commit/push, files, validation, ownership,
integration readiness and remaining approvals. Exact model/effort/usage metrics
are reported only if available. Completion sound is coordinated once through Lead.
