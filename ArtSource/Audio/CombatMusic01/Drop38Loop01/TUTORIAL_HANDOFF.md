# Tutorial music — final requested behavior

Nick explicitly resumed implementation on November13: beat drop at player control; loop original source0:38–1:35; fade over20seconds when the tutorial ends; show an actual game-audio video after verification. No unrelated audio production is included.

Use the existing prepared57-second loop (`manifest.json`, sourceSHA94531142). Preserve original139.2s Wave1110, source exports, prior style backups and all earlier evidence. Entry uses a quick150ms fade once, not on each wrap. The final120ms wrap blend uses the original pre-roll immediately preceding38s; the file retains exactly57seconds and returns to the exact38s sample. Source QA passes: -17.3LUFS/-6.6dBTP, no clipping. Nick has not yet approved the join in game.

The opening is now a playable tutorial battle. Do not use the old fixed49s cinematic or its11-second padded music proposal as the tutorial clock. If the tutorial contains a noninteractive build-up, source0..38 may play before control, but source38 must coincide with actual control release. Do not postpone responsive controls to fit a guessed music timeline. Lead coordinates the exact tutorial hooks with Ryan's owner.

## Required runtime integration (engineering-owned)

- Start the beat-drop Wave at actual tutorial control/possession release. A normal wave's three-second spawn delay is not the start event.
- Sustain this one loop throughout the tutorial, including instruction pauses and between enemies/waves. Tutorial completion, not a quiet combat signal, begins its exit.
- Start a20-second fade immediately at the real completion/next-area transition event. Suggested smooth endpoints: gain=0.5*(1+cos(pi*clamp(elapsed/20,0,1))). Do not restart this timer on repeated completion notices or reset it on a loop boundary.
- Continue playback/wrapping under that envelope until it reaches zero, then stop and release the voice. Authored Wave volume0.5 and the existing Music class/Settings still multiply once.
- The current WorldSubsystem explicitly stops/destroys its component on Deinitialize, and travel currently starts a one-second stop. A twenty-second request alone will be cut short by map teardown. Engineering must preserve the outgoing voice/envelope across that transition or arrange an equivalent existing flow-owned handoff. Do not stall player control/loading for20seconds solely for this sound. Do not add another independent music controller.
- No duplicate tutorial voices on skip, retake, resume, or map re-entry. Quit/menu, invalid teardown and application exit still need bounded cleanup; specify those deliberate fast-stop exceptions in the owner handoff.

## Acceptance and video

Record the tested build/asset hashes and actual event timestamps. Exercise real input/normal tutorial progression. Demonstrate control/drop alignment, at least one57-second wrap, a quiet instruction/combat gap without restart, tutorial completion and the entire20-second exit. Check Music volume/mute and teardown with no leaked voice. Source previews and console-forced phase changes do not substitute for these route checks.

Use actual in-game audio captured from the owned game PID/tree only, without microphone/other-app capture. Private process_capture.cpp is prepared under Saved/AudioMusic/PrototypeV1/CombatMusic01/ProcessCapture01; it is not yet compiled or tested. SDK26100/Windows26300 support the process-loopback API. Confirm activation/output/non-silence and correlate its timeline with the scene recorder before producing the demonstration. Never overlay the prepared song onto footage and call it gameplay proof.

Video should show the control handoff, an uncut loop join and uncut20-second fade, with concise labels for any omitted waiting time. Keep an unedited private original. Report remaining subjective listening approval and any untested skip/travel edge honestly. Native import/bind, build, capture and commits require the renewed Lead grants/preflights; no new native result is claimed by this document.
