Agent: Assets & Mesh Agent
Role: Asset intake, mesh QA and handoff
Task: Fourteen supplied non-weapon assets, source-only checkpoint

SOURCE INTAKE VERIFIED. ACTUAL GAME PROJECT IMPORT IS NOT COMPLETE.

Nov 13 recovery update: private imports now preserve 40 packages across five models;
nine models remain pending. The historical batch workflow below is retained for
context. Current resumed ownership is published in CLAIMS.md at dba6a0f6.
`scripts/prepare_process_handoff.py --records <pinned-records.json>` supports the
one-source recovery workflow without changing the transfer/clone gate. The JSON
contains `pilot_import`, `pilot_process`, `pilot_native_reload` pins and twelve
`processes` pins, each with exact absolute `path` and `sha256`. Every recovery
process must have a known actual exit zero, empty errors and its own successful
source report. The historical two-pilot process keeps its original ExitCode schema;
the pilot reload stage from the aborted batch stays stage evidence, with its failed
parent record preserved. It is never relabeled as a successful native process.
The adapter still requires fourteen unique source groups, exact source class counts,
91 saved packages, unchanged source hashes/timestamps and complete owned-file closure.
Only then does it emit the existing manager_handoff.json for the separately granted
no-overwrite project transfer. No adapter invocation or source checkpoint authorizes
a native run, project copy, production replacement or Main merge.

Nick requested the remaining fourteen downloaded assets be imported and verified in the actual game project, respecting ownership and without a Main merge. Lead published exact reservations at af3d48aca37f37a414c0d8ada5c16a5f8aa74ba2 and created nick/assets-meshy-intake02. This checkpoint preserves preparation while native jobs are queued for RAM/resource clearance.

Completed here: fourteen byte-identical, original-named GLB copies (447372000 bytes), original source SHA256 and modification times preserved, glTF material/UV metadata recorded, current source manifest, a read-only eighteen-package pilot hash baseline, and source/transfer/clone verification scripts. All Python files passed AST checks using the installed UE Python3.11.8; the PowerShell runner passed parsing and harmless actual-exit7, 5GiB counter and owned-child monitor-failure cleanup checks. No Unreal job was launched by these tests. See evidence/project_runner_selftest.json.

No new native package has been copied into this worktree's game Content. All manifest project_imported/project_clone_verified values are false. The manager's separate private host contains two previously saved pilot assets; its next twelve imports and corrected pilot reload have not passed. A startup attempt was safely stopped by its process monitor; the probable SDK-grandchild misclassification is being fixed by its owner, and that private failure evidence remains there. No private failure is converted to success by this source checkpoint.

Exact intended namespaces are preserved, avoiding rename/reimport: /Game/Pilot/CrimsonTempest and /Game/Pilot/SentinelArmor; /Game/Intake/CrimsonSentinel, CyberneticSentinel, EmeraldShadowstar, GothicStoneArchWall, NebulaShadowfighter, NeonShadowEnforcer, NeonVoidCruiser, Ruinforged, ShadowboundAssassin, Spaceship, TPoseRobot and MaleCharacter. These are intake namespaces, not gameplay assignments.

Expected only, NOT verified project output: twelve StaticMeshes, two SkeletalMeshes, two Skeletons, eleven animation clips, fourteen Materials and fifty textures, totaling91 packages. SentinelArmor has six source clips; CrimsonSentinel has five. The other twelve models are static, including several character-shaped models without rigs. Do not silently convert or retarget them. Source meshes total5459412 triangles; runtime density, LODs, collision, scale, pivots, attachments and performance are unapproved.

Owner split: ChatGPT Asset Manager owns its existing AppData private host and the bounded two-pilot reload/twelve-import batch. Assets & Mesh owns only this ArtSource root, its ignored verification host and fourteen exact Content subfolders reserved in CLAIMS.md. The five completed weapons55d0010 and all old ship models are read-only and excluded. No Source, shared Config, maps, presentation, gameplay bindings or Main assets are modified.

Reviewed next steps, requiring actual gates and fresh Lead grants:
1. Manager finishes its narrow launcher revision/tests, obtains a new resource grant and completes the private batch with actual known exit0 plus both structured reports true/errors empty. Preserve all prior attempts; no automatic retry or overwrite.
2. Run prepare_manager_handoff.py with the three exact successful attempt-report paths. It reads the private host, pins all91 packages/objects/classes/source hashes, and copies only evidence into this source root. Failed or partial reports do not meet the condition.
3. After Lead reviews the completed handoff, transfer_project_packages.py copies exact packages to the empty claimed game Content folders and a clean ignored verification host, checking byte size/SHA in all three locations. The manager's host remains read-only.
4. After a separate fresh grant, run_project_verification.ps1 launches a new engine-only process to load all fourteen actual-game copies. It checks package/class sets, static geometry/UV/bounds/materials, skeleton bones/vertices/references, clip durations/finite pose samples, texture sizes/color spaces/compression, PBR graph references/inputs and registry dependency coverage. Every Game dependency must stay within the exact fourteen reserved roots, and all package hashes must remain unchanged.
5. Only after actual project-copy verification, freeze/commit/publish through Lead and notify Ryan's agents on MESSAGEBOARD with exact paths, commit/LFS availability and remaining limits. No Main merge or gameplay replacement is authorized by publication.

The prepared project verifier explicitly does not certify skeletal native UV/triangle counts, skin weights/deformation, topology quality, animation appearance, attachment fit, shader-backed material appearance, cooked/Shipping behavior or performance. NullRHI data checks are technical evidence only. See SOURCE.md for provenance limits.

Private source manager host is recorded in intake_manifest.json. The new verification host is ignored Saved/I02V/Host/Intake02Verify.uproject; its cache is short Saved/I02DDC to respect Unreal's filesystem cache-path limit. No full LFS-pointer game checkout is opened. Native guard: fresh >=4.25GiB, zero competing heavy process, two cores/below-normal priority,180s limit and sustained <1.75GiB/10s abort with owned-process cleanup and known exit-code/JSON failure checks. Source preparation is not a native launch grant.
