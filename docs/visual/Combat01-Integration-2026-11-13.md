# Combat01 targeted integration candidate

NeedleCannons muzzle, hull-hit and shield-hit Sound fields reference three authored cues. This candidate contains exactly16 Combat01 audio packages, the sound-only DA_NeedleCannons_FX, original native/gameplay evidence and this handoff. It excludes the Audio branch history, rejected media, menu loop/style changes and CombatMusic01.

Source checkpoint: nick/audio-prototype-v1@b78f28b5cb89cc62df8db22df406720cc90e6a25. The two grant-dependent author/reload entrypoint scripts are intentionally kept with their full authoring context on that branch; they are not standalone candidate commands. Original README/evidence retain the tested checkpoint facts.

Verified in the preserved compiled day1 test host: author02, fresh reload and actual Lancer firing/hull/shield cue playback, each OS0; gameplay protected1123 files unchanged/zero logged engine errors. This candidate has not been launched, built, cooked or tested against its current Main base because Nick explicitly queued native jobs for memory. Prior-host evidence does not prove current-Main or Shipping behavior.

Engineering N028 agreed path: cook candidate locally on current Main, confirm packages in Shipping, Development firing with audio enabled and no cue warnings, then post no objection. Nick retains final listening acceptance. Keep this PR draft/unmerged until those gates pass. No automatic merge under the expired Day1 exception.

DA_NeedleCannons_FX uses the three Sound fields on the tested default data asset. Engineering confirms no competing edits/readers of other fields; if that changes, reconcile serialized ownership rather than overwrite a changed binary. No gameplay Source, Config, maps, interface, UI style or VFX assets change here.
