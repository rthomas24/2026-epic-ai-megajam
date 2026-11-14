# Voice pickup and binding register

Baseline: `f975c416a6d4e1afe7d3532ae15effc1ec9ce0a2`. Exact expected text, character, scene and line reference for all screenplay and bark entries are in [`matrix.json`](matrix.json). Use its **effective expected text**, not a superseded first draft or an immutable second-draft line overridden by an explicit later decision.

Current text-only delta: main `adc6f23b7cb0edf29b0a94350bdbfe369f586188` now has the correct four console-log words/enums, exact R051 phrase, Wren's `C8_Wren_Order` and the decades-ahead packet. These source corrections do not establish that recordings were produced or bound. Use the matrix's separate current-main delta for owner-confirmed aliases; the complete tracked-asset inventory below remains explicitly tied to f975. Later #29 exceptions remain approved queued work, not grounds to record superseded wording. The approved Cinder temporal-device line remains unchanged; [retention reconfirmed](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6872047241).

No final production recording or recorded-voice binding is verifiable in the **f975 asset-inventory snapshot**. This does not establish that private/unmerged recordings do not exist or replace a full asset inventory of newer main. This sheet coordinates existing owners; it does not authorize parallel casting, automatic production of every draft line, replacement of approved performances or selection of a master take.

| Priority / group | Line references / IDs | Character and delivery | Required pickup or binding action |
|---|---|---|---|
| Shared interrupted word | R007 `O_Unknown_Dont`; R109 `F_FutureWren_Dont` | Future Wren, processed radio. Audience UNKNOWN in opening. Both use the **same** processed master. | Ryan/Grok owns production. Five PR78 radio candidates exist; processed radio **quality is approved**, master take selection remains separate. Preserve take01/take04 decision; obtain immutable candidate manifest and selected master through Lead. |
| Approved sarcastic Aftermath | R016 `S2_Wren_WelcomeParty` | Present Wren: **“Is it over? I think I blacked out...”** Dry, deadpan sarcasm. | Exact text and direction approved; no new recording produced by the approval update. Queue with Ryan's existing voice team after casting/production clearance. Do not play literal blackout symptoms. |
| Four required console logs | R097 Hangar; R098 Corridor; R100 Barracks; R103 Core | Hangar future Kato filtered, label UNKNOWN; then Moss/Wren/Kato RECORDED. Older recognizable voices as the interior evidence develops. | Bind the approved four lines in encounter order. Preserve safe movement/listening. No verified final recordings at base. Do not read the base-main placeholder lines. |
| Cinder echo and archive source | R064–R066; R102 | One heavily processed anonymous future-Kato echo on Cinder; clear older Kato “Same construction.” in Dominion archive after older-self evidence. | Separate scene bindings and treatments. Do not reuse the anonymous Cinder event ID as the clear archive event; do not use engineering's three A3_Echo placeholders. |
| Recovered warning and case | R105–R113 | Future crew recordings; shared R109 fragment followed by separately contextualized full warning R110. | Full warning is separate from the fragment that reached the present crew. No recording of an extended opening warning. Preserve the possibility-based case, not definitive temporal physics. |
| Reactor decision | R115–R117 | Present Kato, Moss, Wren. Wren: “Take out the reactor. Keep the route to the hangar clear.” | Coordinate exact approved response sequence after the screen-only unverified warning. No claimed reactor malfunction or definitive safe solution. |
| Boarding branches / defenders | R118/R118K/R118M, R119–R126M | Future and present crew according to the boarding pilot. All three future crew present; only two nonmatching counterparts defend. | Branch-specific performances require owner mapping and production clearance. A/B refer to actual defender identities, not defeat order. Matched future counterpart remains nondefending; defeat does not prove death. |
| Escape radio exception | R133 | “Keep moving. We're holding the way out.” Moss if Wren/Kato boards; Wren if Moss boards. | Two character bindings for existing words; do not record the second-draft replacement “We're here. Keep coming.” under this delegated decision. |
| Survival and distress response | R136–R142 | Present Kato, Moss, Wren; relief followed by uncertainty. R139 optional. | Preserve all-three-survive accounting and unresolved distress. Caption R143 is screen-only. |
| Remaining screenplay dialogue | All other R references, excluding R114/R143 | Character and effective exact text in matrix. | Resolve current batch mapping before recording. New audit authority does not approve every voice/performance; follow existing production lane. |
| Gameplay barks and efforts | B001–B259, alternatives in matrix | Preserve character voice, short functional calls and distinct nonverbal efforts. | CSV has 259 rows/274 alternatives; six are nonverbal effort directions. B204 is a count template, not an extra spoken template alongside four expanded counts. Verify runtime import/rotation before production. Existing Crew hit/death/melee files are not proof of approved character vocal efforts. |
| Never spoken | R114 reactor warning; R143 distress packet | Screen-only | No voice pickup. Credits have no added narration. |

## Asset and provenance evidence

- `Source/unrealGameJam/TriadDrift/Story/TDStoryTypes.h:55–62`: optional `Sound`; absent sound allows subtitle-only fallback.
- `.../TDStorySubsystem.cpp:31–41`: default entries set ID/speaker/text/style/shared key without assigning production sound.
- `.../TDStorySubsystem.cpp:224` loads `DA_Story_Transmissions`; `:354–360` resolves shared keys including `Dont`.
- `docs/story/transmissions.md:9–15` documents the production asset/override contract.
- Full tracked path inventory found no `DA_Story_Transmissions`, `Content/TriadDrift/Audio/VO/` or `ArtSource/Audio/FirstContact01/`. Presentation contains only `DA_Credits` and `DA_UI_Style` at this snapshot.
- `Squad/TDBarkLines.cpp:31–68` loads Presentation `DT_Barks` or the binary fallback table. `Tools/td_barks.csv` is source data; it is not proof of runtime table contents or VO. `FTDBarkRow` contains no sound field.
- `docs/visual/STATUS.md:44` is the current distinction between PR78 approved radio quality and an unselected master. Older audition requests must not reopen the approved treatment.

## Production handoff requirements

For each delivered take, Ryan/Audio should return immutable source filename/hash, actor/character, line ID and exact text, treatment, selected/approved status and evidence, Unreal sound path, owning data entry/shared key, cook inclusion and main commit. Lead coordinates selection and updates this register after integration. A source WAV, audition candidate, imported sound, bound ID and audible matched-build performance are distinct stages.

Current stage: **source inventory and pickup tracking only**. No generation, replacement, listening test or gameplay VO validation performed. No Unreal processes launched by this inventory.
