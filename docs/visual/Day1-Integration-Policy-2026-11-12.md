# Day 1 asset integration exception — November 12, 2026 only

Authority: Nicholas's explicit Day 1 instruction in the Lead Coordinator chat,
reporting Ryan's confirmation. Valid from 2026-11-12 00:00:00 through 23:59:59
America/New_York (UTC-05:00). It expires automatically at 2026-11-13 00:00 EST.
This dated clarification preserves AGENTS.md, CLAIMS.md and features.md ownership
and safety rules; it is not a permanent replacement for Ryan's workflow.

- Nick's workers remain in their own isolated nick/* branches/worktrees and commit locally only after Lead preflight. Under AGENTS.md section 6.9, Lead alone publishes branches and integrates eligible Nick-owned work into main.
- On this date only, Lead may integrate completed, verified model/asset work and
  push main without requesting a separate approval for each eligible integration.
- Fetch, inspect incoming changes and active claims, verify ownership, and review
  one change set at a time. Use a clean integration worktree; preserve all other
  uncommitted work. If main advances, fetch/reassess; never force-push, discard
  changes, auto-resolve binary conflicts or blindly rebase active worker trees.
- No authority to change Ryan-owned gameplay, shared maps/interfaces, project
  configuration or engine association. Any such need still requires approval.
- Do not integrate unfinished native assets, experimental plugins, unverified
  binaries or unapproved ship silhouettes. A pushed branch is not acceptance.
- The exception permits integration, not unrelated development. Existing binary
  production permissions remain agent/path-specific. Publication of a claim is
  not verification of its assets and does not release another active claim.
- Preserve all existing teammate claims. Completed source snapshots may be shared
  read-only while their successor's distinct native-output lane remains active.
- From November 13, return to explicit approval-based, end-of-day integration;
  no extension or scheduled automatic merge is authorized by this document.

Initial pre-native checkpoint (historical): the consolidated visual-assets source package was eligible as
source/prototype work only. Lancer/PR #3 is withheld (unapproved silhouette and
assembly QA failures; user also explicitly prohibited its merge). Native HUD and
Environment/VFX work remain active/unverified and are withheld. The legacy
Environment/VFX/HUD/Audio source branches are already represented in the consolidated
package and must not be merged a second time. Existing shared AGENTS.md is unchanged.

Checkpoint after the initial review above: isolated native SpaceFoundation was
completed, captured and audited (ec3e47f/c9beebc), making those37owned additions
eligible; integrated as9c629e8. This does not activate shared-map placement or claim
full-game/PIE/package/performance validation. HUD activation, rejected M02audio and
unapproved/QA-failing Lancer remain withheld. Original dated authority unchanged.
