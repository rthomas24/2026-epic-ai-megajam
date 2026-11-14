# Script-to-Game Fidelity Matrix

This is the traceable second-draft audit, not a claim that the screenplay is integrated or that voice performances are approved. Public game title: **Triad Drift**. Nicholas explicitly rejected **The Paradox** as a subtitle because it reveals the twist; canonical source filenames remain preserved.

## Authority and baseline

- Screenplay: `Story/Triad_Drift_Complete_Script_Second_Draft.docx`, SHA-256 `7b2ab60d23db4b0d868a554d7b5e76ad1bc4c68d1b59901f7c13022db4a8fffa`.
- Nicholas's latest standing goal designates that second draft as the narrative audit authority. Lead confirmed the hierarchy in the serialized StoryFidelity01 dispatch. This supersedes the older review-only audit baseline, while preserving specific later exceptions and separate performance approval.
- Immutable implementation snapshot: main `f975c416a6d4e1afe7d3532ae15effc1ec9ce0a2`. Later main commits require a refreshed comparison; findings here must not be represented as live defects after a fix lands.
- Original human narrative remains preserved; reactor destruction does not mean destroying the Origin Core, uncertain shutdown does not establish a proven malfunction, and the future crew's fate remains unresolved.

## How to use the matrix

[`matrix.json`](matrix.json) tracks every one of the 150 screenplay entries, 259 bark rows, 20 scene introductions and 54 explicit staging actions. A further 32 default runtime entries lack a screenplay mapping and require owner triage. Each item has a stable reference, expected text or beat, speaker where applicable, sequence or trigger context, implementation evidence, status and next verification action. Branch reports are explicitly separate from main and playback proof.

At the **historical f975 baseline**, 22 screenplay rows match normalized source text and attribution, 82 differ in text, and 46 lack a confirmed mapping. Of 259 bark source rows, 258 differ from the second draft and one matches. These counts describe that inspected fallback/source catalogue, not the current main or a played-build result. Every staged action remains pending an implementation/playback trace.

Source evidence includes file and line, the default entry's actual words and enum, and literal references outside tests/definitions. A literal reference can be a lookup or dispatcher and does not prove the line fires; a missing literal reference does not rule out indirect dispatch. Bark CSV evidence does not establish the contents of the imported binary table or a Presentation override. Typographic comparison normalizes apostrophes, dashes and ellipsis only; the matrix also records exact character equality.

B001–B003 retain implementation placeholders `{dir}`/`{Dir}`; the reading copy displays `[direction]`/`[Direction]`. B204 retains the `{n}` count template while the reading copy expands Three/Two/One. These are traceable parameterized representations, not claims of literal DOCX equality. The four console entries include the shared dynamic ID-construction and firing references in `TDDominionInteriorCourse.inl`; those references still do not prove encounter order or completed listening.

## Current-main delta: adc6f23b

The separate `current_main_delta` object in the matrix compares immutable GitHub main `adc6f23b7cb0edf29b0a94350bdbfe369f586188`, which includes batch L `6fe3e90f`. This delta preserves the original baseline evidence and metrics. Read-only API source bytes and hashes were captured privately; no worker Git synchronization or runtime edits occurred.

Of 150 screenplay entries, **48 now match normalized source text**, including two with dynamic Boarder attribution requiring route-aware review; 85 differ and 17 lack a confirmed current mapping. These are text-only counts, with punctuation and whitespace normalized. They are not directly comparable to the baseline's combined text-and-enum matches. Of the 85 differences, 74 already have an audited existing ID and exact requested wording available for T3 review; the remaining scene-matched aliases require owner confirmation. No production or performance batch is authorized by these counts. The bark CSV is byte-identical to baseline; runtime table contents remain unverified.

| Earlier finding | Current source evidence | Remaining gate |
|---|---|---|
| Four console logs had old words and attribution | Correct expected words and UNKNOWN / future Moss / future Wren / future Kato enums at `TDStorySubsystem.cpp:215–218`. | Encounter order, actual performer assets, safe listening and resume/skip playback. |
| R051 had an outdated last phrase | `G3_Kato_AlmostThere` now says “Archive almost complete. Keep them back.” at line 148. | Matched-build trigger and playback. |
| Reactor decision appeared assigned to Moss | R117 maps to **`C8_Wren_Order`**, correct approved Wren words at line 237. Legacy `A5_Choice` is not the current decision mapping. | Verify the actual C8 queue and no legacy duplicate in a played build. |
| Ending packet specified forty-one years | `A5_DistressSignal` now says **DATED DECADES AHEAD**, line 259. | Exact caption layout, timing and screen-only behavior in playback. |
| Future-voice tags looked like question-mark names in a static helper | `SpeakerTagNow` at line 682 uses UNKNOWN and evidence-dependent RECORDED labels; `ResolveSpeaker` at line 830 resolves Boarder to the saved character. | Inspect actual output and identification timing; do not change the static helper on that evidence alone. |

**Known queued work, not newly discovered regressions:** N-088 explicitly excludes later issue #29 decisions from this first batch-L integration. Wren's S2 sarcasm, C4/C6 shortened selections, conditional outside-pilot escape call and `TB_Escaped` retirement retain their approvals and engineering-branch evidence; current-main and playback checks are still open. T5's approved future-crew trio remains its existing N-052 queue (`features.md:145,165`); the inspected current course still has the two-drone fallback at `TDDominionInteriorCourse.inl:427`. Do not start duplicate defender work.

The clear older-Kato archive callback R102 still needs an owner-confirmed distinct binding in the inspected current files. Several scene-matched entries use Boarder where the screenplay names Wren; this needs an explicit route-aware attribution check, not automatic replacement of the enum.

**Cinder temporal suspicion is approved and stays.** Original narrative paragraph 22 already states the crew concludes on Cinder that the Dominion seeks components/access keys to a temporal device. `Story/README.md:134`, D2 R067 and current `C5_Kato_OriginCore` at line 174 agree. [Lead reconfirmed retention](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6872047241). Future identities, maker, exact function and causality remain withheld; no substitute line is requested.

## Exceptions to preserve

| Reference | Effective decision | Evidence |
|---|---|---|
| R016 | Wren: “Is it over? I think I blacked out...” Dry, deadpan sarcasm; no literal blackout. | [Direct approval relay](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871262780) |
| R014 / tutorial boundary | Keep Kato's existing “They're breaking off.” Retire extra `TB_Escaped`; preserve withdrawal, completion boundary, Aftermath and music fade. | [Delegated decision](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871591719) |
| R054–R056 | C4 order: `C4_Wren_Badly`, `C4_Kato_SurveyStation`, `C4_Command_Proceed`; retain existing words. | [Exact selection](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871349665) |
| R079–R083 | C6 takeoff: GuardLine, FourGenerators, FindOutWhy; guard-wave start: WholeFleet, StayTogether. Retain existing words. | [Delegated decision](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871349665) |
| R133 | “Keep moving. We're holding the way out.” Moss speaks if Wren/Kato boards; Wren speaks if Moss boards. | [Delegated speaker correction](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871349665) |

Ryan reports `54e89611` retires `TB_Escaped` and `205d3746` restores the exact C4 selection, with a clean build and 26/26 tests. [Report](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6871822468). Those objects were unavailable locally during the baseline pass: **reported engineering branch only**, not verified main integration or playback. The previous C4 selection discrepancy is corrected at the reported-branch stage; its main/playback gates remain open, consistent with N-088's later-decision queue.

## Prioritized owner follow-through

1. **Reveal and decision continuity, T3/T5:** verify the now-correct four recordings in order and their actual performer/labels; UNKNOWN Hangar must be performed by future Kato. Preserve safe console listening and required encounter state. Consume Lead's separate ART-20 queue/save/Continue/retake findings rather than duplicate that audit.
2. **Reactor choice and ending, T3:** retain the now-correct `C8_Wren_Order` and decades-ahead packet. Confirm scene-matched aliases R110–R117 and R136–R143 and prove the intended sequence in a matched build. Screen-only warning and packet must not become spoken lines.
3. **Second-draft words and speakers, T3:** review the current delta's existing-ID correction payload, then confirm remaining aliases and unmapped entries. Preserve current source fixes and explicit exceptions. Do not reimplement known batch-L hooks or duplicate queued T5 work.
4. **Branch exceptions, Lead/T3:** verify exact C4/C6 sets, S2 sarcasm, conditional escape radio and boundary retirement on integrated main, then in a matched played build.
5. **Voice, Ryan/Audio:** use [`voice-pickups.md`](voice-pickups.md); no final production voice binding is verified at this snapshot. Preserve shared-word identity and anonymous/recorded labels.
6. **Title exposure, T3/Lead:** `TDCredits.cpp:127` displays `"The Paradox" by Nicholas Tosti`. Check whether credits can be reached before the reveal and route a spoiler-safe attribution through its owner. No public subtitle string was found in the bounded Source/Config/Plugins text search; assets and submission copy still require checks.

## Verification and handoff gates

Documentation/source audit only: script bytes verified, all ledger references accounted for, per-line comparisons captured. No Unreal process was launched by this audit. No audio listening, camera/animation review, native build, binary-table inspection or packaged-playback acceptance is claimed.

Lead owns publication, index and claims updates. T3 owns story/runtime changes, T5 interior and defenders, Ryan's voice team production, Audio integration and mix. After integration, refresh against the exact main revision and attach played-build evidence for opening/skip, all four logs and resume/retake, Cinder field versus Dominion shield, boarding branches, reactor decision, 90-second run plus separate 60-second flight, final caption and credits transition. A technically passing queue test is not proof of readable pacing or emotional delivery.
