# HUD Usability M04 — native verification

Candidate: `5a1382d39d2da5c564f6a2d53e65d58d66987025`, branch `nick/hud-usability-20261112`.
Stage: technically verified functional candidate; presentation review and Shipping validation remain open. No main merge.

Open preview.html for all20 byte-identical native screenshots. screenshots.json records actual PNG dimensions, bytes and SHA256. Main captures01/05/06 prove1920x1080,1280x720,2560x1080. These are actual windowed Galaxy gameplay driven through private MCP8026, not browser previews. Original logs remain in Saved/HUDUsabilityM04 (ignored).

Build PASS183.814s; all14 UI tests PASS35.397s. Game01 PASS49.180s; focused game02 PASS31.017s. Every job OS exit0; protected Source/Config/uproject hashes unchanged. Fresh free RAM guards5.007/5.004/5.888/5.327GiB. All heavy processes terminal and resource slot released.

Verified: live ship/health/shield/heat/ammunition/ability/objective data; target metadata displaced clear of reticle; actual death/recovery and auto control transfer (engine logs Lancer->Rift Auto on Death). Actual projectile kill confirmation visibly centered in focused-projectile-06 and07; ordinary hits do not shorten it in lifecycle test. Static health/shield values are not proof of every possible damage case.

Limits/findings: transient missing/clipped glyphs occur in main04/09 and focused captures including before/after; main10 restores fully. Cause unverified: do not label capture-only or all presentation PASS. Main08 explosion occlusion belongs to older gameplay base (Lead reports fixed on newer main), no owned gameplay edit. One nonfatal HTTP socket_send_failure at game01 startup. No fatal/assert/ensure observed. Ship silhouettes/environment are existing gameplay placeholders, outside this task.

Development fixtures: TD.StageWaves, TD.God, TD.Squad.Passive, TD.Power.Targets, real key inputs and TD.Squad.KillActive. Focused kill used real LeftMouseButton projectile fire; no fabricated hit confirmation. Not a natural progression or Shipping acceptance test.

Five original performance samples at60fps cap: frame~16.667ms; game thread2.829–3.269ms; GPU3.221–3.454ms; render thread5.329–24.694ms including one spike. No before baseline, so no regression conclusion.

Reproduction: review verify_usability.ps1 and capture scripts. Do not run without a fresh Lead resource slot,>=4GiB free RAM and zero competing heavy processes. Existing attempt paths refuse overwrite. Scripts pin candidate5a1382d; future evidence commits require a deliberate updated verification plan, not blind reruns.

No character work, shared gameplay/source/config/map or DA_UI_Style changes. No additional capture retries authorized. Next task is separately authorized cinematic menu campaign after evidence checkpoint.
