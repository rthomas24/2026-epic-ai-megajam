# Wren voice r1: Combat barks and banter (R-042 ask 4)

**For:** Nicholas (listen and pick or confirm the recommended take), Nick's Audio / T3 (integrate the approved file), Ryan / Lead.
**Stage:** technically verified only. The agent that made these cannot hear them.
Nicholas approves by ear. Recommended takes below were picked from timing, median F0,
loudness and a faster-whisper `small.en` transcript, not from a listen.

**Use:** the rest of Wren's current draft lines in this chapter batch, in take 04's voice
(PR #78 master / PR #93 setup). The shared interrupted **Don't—** master and **C8_Wren_Order**
are not in this PR and must not be regenerated.

In-combat radio barks (space and on-foot), the twelve SquadEngage directions, and provisional banter ids. These are radio barks unless noted.

## Files

| File | What it is |
|---|---|
| `VO_<Id>[_Variant]_0N_dry.wav` | Dry take, 50 ms pre-roll, 100 ms tail, 48 kHz, mono, 24-bit |
| `VO_<Id>[_Variant]_0N_radio.wav` | Radio version of the same take (chain below). Used for space / comms / Static lines |
| `raw/VO_*_liora.wav` | Untouched xAI TTS outputs (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/process_voice.py`, `tools/audio_common.py` | Rebuilds the dry / radio / echo files from `raw/` |
| `build_log.json` | Per take: speech bounds, mid-sentence pause, gains, loudness |
| `stt.json` | Local faster-whisper `small.en` transcripts, median F0, SHA256 |
| `barks_r1_sheet.png` | Waveform and spectrogram of every delivered file |

## Recommended takes

The agent cannot hear these. Pick below is from timing, pitch and transcript only.
`small.en` is noisy on short lines and regularly writes 'cash' for 'cache' and 'chip' for 'ship'.
Those known confusions were treated as a match when the rest of the words lined up.

| Line | Recommended | Alternative | Spoken text | Why |
|---|---|---|---|---|
| `Bark_SquadEngageAhead (A)` | **take 02** | take 01 | Bandit, ahead. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 170 Hz sits in the liora range. Take 01 did not transcribe exactly (“banned it, ahead, mine.”). |
| `Bark_SquadEngageAhead (B)` | **take 01** | take 02 | On the one ahead! | Close transcript (0.75): “on the one head.”. Loudness -19.00 LUFS is in the vo window. Median F0 228 Hz sits in the liora range. |
| `Bark_SquadEngageLeft (A)` | **take 02** | take 01 | Bandit, left. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 175 Hz sits in the liora range. Take 01 did not transcribe exactly (“Left. Left.”). |
| `Bark_SquadEngageLeft (B)` | **take 02** | take 01 | On the one left! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. Take 01 did not transcribe exactly (“On the left.”). |
| `Bark_SquadEngageRight (A)` | **take 02** | take 01 | Bandit, right. Mine. | Best available transcript of the pair (0.67): “Bandit mine”. STT is noisy on short barks. Loudness -19.00 LUFS is in the vo window. Median F0 160 Hz sits in the liora range. |
| `Bark_SquadEngageRight (B)` | **take 01** | take 02 | On the one right! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 187 Hz sits in the liora range. |
| `Bark_SquadEngageBehind (A)` | **take 01** | take 02 | Bandit, behind. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.32 LUFS is in the vo window. Median F0 160 Hz sits in the liora range. Take 02 did not transcribe exactly (“Banned it behind mine.”). |
| `Bark_SquadEngageBehind (B)` | **take 01** | take 02 | On the one behind! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 170 Hz sits in the liora range. Take 02 did not transcribe exactly (“behind.”). |
| `Bark_SquadEngageHighAhead (A)` | **take 01** | take 02 | Bandit, high ahead. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.36 LUFS is in the vo window. Median F0 167 Hz sits in the liora range. |
| `Bark_SquadEngageHighAhead (B)` | **take 01** | take 02 | On the one high ahead! | Close transcript (0.85): “on the one high.”. Loudness -19.00 LUFS is in the vo window. Median F0 175 Hz sits in the liora range. |
| `Bark_SquadEngageHighLeft (A)` | **take 01** | take 02 | Bandit, high left. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. Take 02 did not transcribe exactly (“High left. Mine.”). |
| `Bark_SquadEngageHighLeft (B)` | **take 02** | take 01 | On the one high left! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 169 Hz sits in the liora range. Take 01 did not transcribe exactly (“I left.”). |
| `Bark_SquadEngageHighRight (A)` | **take 01** | take 02 | Bandit, high right. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.04 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. Take 02 did not transcribe exactly (“high-right mine.”). |
| `Bark_SquadEngageHighRight (B)` | **take 02** | take 01 | On the one high right! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 173 Hz sits in the liora range. Take 01 did not transcribe exactly (“on the”). |
| `Bark_SquadEngageHighBehind (A)` | **take 02** | take 01 | Bandit, high behind. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 166 Hz sits in the liora range. Take 01 did not transcribe exactly (“Band it, high behind, mine.”). |
| `Bark_SquadEngageHighBehind (B)` | **take 01** | take 02 | On the one high behind! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 183 Hz sits in the liora range. |
| `Bark_SquadEngageLowAhead (A)` | **take 02** | take 01 | Bandit, low ahead. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 170 Hz sits in the liora range. Take 01 did not transcribe exactly (“and at low ahead, mine.”). |
| `Bark_SquadEngageLowAhead (B)` | **take 01** | take 02 | On the one low ahead! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. |
| `Bark_SquadEngageLowLeft (A)` | **take 01** | take 02 | Bandit, low left. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. Take 02 did not transcribe exactly (“Bandit, low K left, mine.”). |
| `Bark_SquadEngageLowLeft (B)` | **take 01** | take 02 | On the one low left! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 191 Hz sits in the liora range. |
| `Bark_SquadEngageLowRight (A)` | **take 02** | take 01 | Bandit, low right. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 169 Hz sits in the liora range. Take 01 did not transcribe exactly (“low right, mine.”). |
| `Bark_SquadEngageLowRight (B)` | **take 01** | take 02 | On the one low right! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 178 Hz sits in the liora range. |
| `Bark_SquadEngageLowBehind (A)` | **take 02** | take 01 | Bandit, low behind. Mine. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. Take 01 did not transcribe exactly (“Bandit”). |
| `Bark_SquadEngageLowBehind (B)` | **take 01** | take 02 | On the one low behind! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 166 Hz sits in the liora range. |
| `Bark_SquadWingmanKill (A)` | **take 01** | take 02 | Splash one! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. Take 02 did not transcribe exactly (“SplashOne.”). |
| `Bark_SquadWingmanKill (B)` | **take 01** | take 02 | And he's gone. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. |
| `Bark_PlayerKill (A)` | **take 01** | take 02 | Nice shot! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 211 Hz sits in the liora range. Take 02 did not transcribe exactly (empty STT). |
| `Bark_PlayerKill (B)` | **take 02** | take 01 | Leave some for me. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 186 Hz sits in the liora range. Take 01 did not transcribe exactly (“Leave some for me. you”). |
| `Bark_MultiKill` | **take 01** | take 02 | Now that's flying! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 164 Hz sits in the liora range. |
| `Bark_SquadLowHull (A)` | **take 01** | take 02 | Taking a beating here! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 197 Hz sits in the liora range. |
| `Bark_SquadLowHull (B)` | **take 01** | take 02 | Hull's going, need a second! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 187 Hz sits in the liora range. |
| `Bark_ShieldDown` | **take 01** | take 02 | Shield's gone! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 230 Hz sits in the liora range. |
| `Bark_ShieldBack` | **take 01** | take 02 | Shield's back. Let's go. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 183 Hz sits in the liora range. |
| `Bark_SquadShipDown (A)` | **take 01** | take 02 | Kato's down! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 198 Hz sits in the liora range. |
| `Bark_SquadShipDown (B)` | **take 01** | take 02 | Moss is out, cover him! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 181 Hz sits in the liora range. Take 02 did not transcribe exactly (“Moss is out, cover him, him.”). |
| `Bark_SquadRespawn` | **take 02** | take 01 | Back in it. Miss me? | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 178 Hz sits in the liora range. Take 01 did not transcribe exactly (“back in it.”). |
| `Bark_SquadWipe` | **take 01** | take 02 | No, no, no... | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. |
| `Bark_MissileIncoming` | **take 02** | take 01 | Missile on you, break! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 224 Hz sits in the liora range. Take 01 did not transcribe exactly (“missile on you a break.”). |
| `Bark_MissileEvaded` | **take 02** | take 01 | Too slow. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 172 Hz sits in the liora range. |
| `Bark_OnMySix` | **take 01** | take 02 | One on your tail! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. |
| `Bark_Anticipated` | **take 01** | take 02 | It moved before I did. How? | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 165 Hz sits in the liora range. Take 02 did not transcribe exactly (“it moved before I did how. a”). |
| `Bark_TargetShielded` | **take 01** | take 02 | It's shielded, hit it harder! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. |
| `Bark_MinesAhead` | **take 01** | take 02 | Mines! Pull up! | Close transcript (1.00): “mines. Minds pull up.”. Loudness -19.00 LUFS is in the vo window. Median F0 225 Hz sits in the liora range. The other take also had a usable read; this one won on timing or loudness. |
| `Bark_CarrierLaunch` | **take 01** | take 02 | More fighters coming out! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 198 Hz sits in the liora range. |
| `Bark_AmmoLow (A)` | **take 01** | take 02 | Running light on missiles. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 169 Hz sits in the liora range. |
| `Bark_AmmoOut (B)` | **take 02** | take 01 | I'm dry! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 188 Hz sits in the liora range. |
| `Bark_WeaponHot` | **take 02** | take 01 | Guns are cooking! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 218 Hz sits in the liora range. Take 01 did not transcribe exactly (“like a king.”). |
| `Bark_BoostEmpty` | **take 02** | take 01 | Out of burn! | Exact (or punctuation-normalized) transcript. Loudness -19.06 LUFS is in the vo window. Median F0 162 Hz sits in the liora range. |
| `Bark_Boundary` | **take 01** | take 02 | Wide of the zone. Bring it round. | Exact (or punctuation-normalized) transcript. Loudness -19.26 LUFS is in the vo window. Median F0 166 Hz sits in the liora range. Take 02 did not transcribe exactly (empty STT). |
| `Bark_WaveCleared` | **take 02** | take 01 | That's all of them. For now. | Exact (or punctuation-normalized) transcript. Loudness -19.74 LUFS is in the vo window. Median F0 167 Hz sits in the liora range. |
| `Bark_WaveIncoming` | **take 02** | take 01 | Here they come again. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 188 Hz sits in the liora range. Take 01 did not transcribe exactly (empty STT). |
| `Bark_Breather` | **take 02** | take 01 | Anyone else enjoying this? | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 194 Hz sits in the liora range. |
| `Bark_SquadSwitchIn (A)` | **take 01** | take 02 | Rift's yours. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 166 Hz sits in the liora range. |
| `Bark_SquadSwitchIn (B)` | **take 01** | take 02 | My turn. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 173 Hz sits in the liora range. |
| `Bark_SquadSwitchStrike` | **take 02** | take 01 | Strike's hot! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 176 Hz sits in the liora range. |
| `Bark_SquadFocusAck` | **take 01** | take 02 | On it! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. |
| `Bark_PowerReady` | **take 01** | take 02 | Portals are up. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 190 Hz sits in the liora range. |
| `Bark_PowerNotReady` | **take 01** | take 02 | Not yet! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 198 Hz sits in the liora range. Take 02 did not transcribe exactly (“Not yet Thanks for watching!”). |
| `Bark_Portal_Open (A)` | **take 01** | take 02 | Door's open. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 184 Hz sits in the liora range. Take 02 did not transcribe exactly (“and that that that that that that that that”). |
| `Bark_Portal_Through (B)` | **take 01** | take 02 | And... through! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 173 Hz sits in the liora range. |
| `Bark_Portal_MissileReturn (C)` | **take 02** | take 01 | Return to sender! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 242 Hz sits in the liora range. Take 01 did not transcribe exactly (“sender.”). |
| `Bark_AbilityUnlocked_Cloak` | **take 02** | take 01 | Cloak's online. Now you don't see me. | Exact (or punctuation-normalized) transcript. Loudness -19.58 LUFS is in the vo window. Median F0 176 Hz sits in the liora range. Take 01 did not transcribe exactly (“on now you don't see me”). |
| `Bark_Cloak_On (A)` | **take 01** | take 02 | Going dark. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 187 Hz sits in the liora range. |
| `Bark_Cloak_Ambush (B)` | **take 01** | take 02 | Surprise. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 172 Hz sits in the liora range. |
| `Bark_AbilityLocked` | **take 02** | take 01 | Haven't got that yet. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 184 Hz sits in the liora range. Take 01 did not transcribe exactly (empty STT). |
| `Bark_Hyper_Spool (A)` | **take 01** | take 02 | Spooling up! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 222 Hz sits in the liora range. Take 02 did not transcribe exactly (“Sp,ooling up.”). |
| `Bark_Hyper_Blocked (B)` | **take 01** | take 02 | Can't jump, too hot here! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 214 Hz sits in the liora range. |
| `Bark_Gate_Enter` | **take 01** | take 02 | Shortcut! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 249 Hz sits in the liora range. |
| `Bark_Atmo_Entry` | **take 01** | take 02 | Going in hot! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 201 Hz sits in the liora range. Take 02 did not transcribe exactly (“hot.”). |
| `Bark_Land_Ready (A)` | **take 02** | take 01 | There's the pad. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 173 Hz sits in the liora range. |
| `Bark_Land_Down (B)` | **take 02** | take 01 | Down. Let's look around. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 181 Hz sits in the liora range. |
| `Bark_TakeOff` | **take 01** | take 02 | Lifting off. | Both takes transcribed as YouTube-outro hallucinations ('See you next time.' / 'Thank you for watching.'), a known small.en failure on short lines. Take 01 is in the vo window at −19.00 LUFS with median F0 182 Hz. Nicholas should confirm by ear. |
| `Bark_Planet_Arrive_Verdance (A)` | **take 01** | take 02 | It glows. Everything glows. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 175 Hz sits in the liora range. |
| `Bark_Planet_Arrive_Cinder (B)` | **take 01** | take 02 | Hot enough for you? | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 176 Hz sits in the liora range. |
| `Bark_Dom_GeneratorHit (A)` | **take 02** | take 01 | It's cracking! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 196 Hz sits in the liora range. |
| `Bark_Dom_GeneratorDown (B)` | **take 01** | take 02 | Generator's gone! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 170 Hz sits in the liora range. |
| `Bark_Dom_Turrets` | **take 01** | take 02 | Turrets, watch the turrets! | Exact (or punctuation-normalized) transcript. Loudness -19.19 LUFS is in the vo window. Median F0 163 Hz sits in the liora range. |
| `Bark_Dom_Tractor` | **take 01** | take 02 | Something's pulling us in. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 176 Hz sits in the liora range. |
| `Bark_Dom_EscapeFlight` | **take 01** | take 02 | Punch it! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 242 Hz sits in the liora range. |
| `Bark_Dom_Clear` | **take 01** | take 02 | We're clear! We're clear! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. |
| `Bark_Foot_Contact` | **take 01** | take 02 | You've got company! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 184 Hz sits in the liora range. |
| `Bark_Foot_Deflect (A)` | **take 02** | take 01 | Back at you. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 174 Hz sits in the liora range. |
| `Bark_Foot_Perfect (B)` | **take 02** | take 01 | Right between the eyes! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. |
| `Bark_Foot_GuardBroken` | **take 01** | take 02 | Guard's gone! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 174 Hz sits in the liora range. |
| `Bark_Foot_Hurt (A)` | **take 01** | take 02 | Ow! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 199 Hz sits in the liora range. |
| `Bark_Foot_LowHealth (B)` | **take 01** | take 02 | I'm hurt, I'm hurt. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. |
| `Bark_Foot_Heal` | **take 02** | take 01 | Better. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 163 Hz sits in the liora range. |
| `Bark_Foot_Kill (A)` | **take 02** | take 01 | Next! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 211 Hz sits in the liora range. |
| `Bark_Foot_LastOne (B)` | **take 01** | take 02 | That's the lot. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 181 Hz sits in the liora range. |
| `Bark_Foot_Dash (A)` | **take 02** | take 01 | Whoo! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Take 01 did not transcribe exactly (“O,”). |
| `Bark_Foot_Slam (B)` | **take 01** | take 02 | Boom. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 168 Hz sits in the liora range. |
| `Bark_Foot_ObjectiveNear (A)` | **take 01** | take 02 | That's it, right ahead. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 183 Hz sits in the liora range. |
| `Bark_Foot_ObjectiveDone (B)` | **take 02** | take 01 | Got it. Heading back. | Exact (or punctuation-normalized) transcript. Loudness -19.84 LUFS is in the vo window. Median F0 169 Hz sits in the liora range. Take 01 did not transcribe exactly (“Heading back.”). |
| `Bark_Foot_DoorLocked (A)` | **take 01** | take 02 | Locked. Figures. | Exact (or punctuation-normalized) transcript. Loudness -19.23 LUFS is in the vo window. Median F0 165 Hz sits in the liora range. |
| `Bark_Foot_DoorOpen (B)` | **take 01** | take 02 | We're in. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 171 Hz sits in the liora range. |
| `Bark_Foot_Hazard` | **take 01** | take 02 | Get out of there! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 189 Hz sits in the liora range. |
| `Bark_WardenPhase2` | **take 01** | take 02 | It's angry now! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 183 Hz sits in the liora range. |
| `Bark_Foot_Idle` | **take 02** | take 01 | You sightseeing down there? | Close transcript (1.00): “You sightseeing down, down there?”. Loudness -19.00 LUFS is in the vo window. Median F0 192 Hz sits in the liora range. The other take also had a usable read; this one won on timing or loudness. |
| `Bark_Dom_Boarded` | **take 01** | take 02 | Bring something back. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 193 Hz sits in the liora range. |
| `Bark_Dom_CoreShield (A)` | **take 02** | take 01 | Shield's dropping! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 187 Hz sits in the liora range. |
| `Bark_Dom_CoreOpen (B)` | **take 01** | take 02 | There's the heart! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 207 Hz sits in the liora range. |
| `Bark_Dom_Escape_Start (A)` | **take 01** | take 02 | Run! Run now! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 245 Hz sits in the liora range. |
| `Bark_Dom_Escape_30 (B)` | **take 01** | take 02 | Thirty seconds! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 222 Hz sits in the liora range. |
| `Bark_Dom_Escape_10 (C)` | **take 01** | take 02 | Move it! | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 204 Hz sits in the liora range. |
| `Bark_Banter_BetYouTen` | **take 01** | take 02 | Bet you ten I get more than you this round. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 163 Hz sits in the liora range. |
| `Bark_Banter_WereInSpace` | **take 01** | take 02 | We're in space, Moss. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 183 Hz sits in the liora range. |
| `Bark_Banter_TidyRaiders` | **take 01** | take 02 | So they're tidy raiders. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 167 Hz sits in the liora range. |
| `Bark_Banter_SaySomethingNice` | **take 01** | take 02 | Kato, say something nice about my flying. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 181 Hz sits in the liora range. |
| `Bark_Banter_IllTakeIt` | **take 01** | take 02 | I'll take it. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 164 Hz sits in the liora range. Take 02 did not transcribe exactly (empty STT). |
| `Bark_Banter_PleaseNotTheStew` | **take 02** | take 01 | Please not the stew. | Exact (or punctuation-normalized) transcript. Loudness -19.00 LUFS is in the vo window. Median F0 189 Hz sits in the liora range. |

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-14.
- **Voice:** `liora` (xAI: "calm, grounded, luminous"). This is the voice of PR #78 take 04,
  which Nicholas selected as the shared `Dont` radio master, and of the PR #93 present-Wren Order audition.
- **Common settings (same as `audition_r1` / `present_order_r1`):** `language: "en"`,
  `output_format: {codec: "wav", sample_rate: 48000}`, `enable_safety_checker: true`,
  all other inputs at their defaults. No `speed` parameter was sent. No seed parameter exists,
  so a rerun will not produce the same audio. The `raw/` files are the record.
- **Cost (this batch):** 220 generations, 4054 characters, at $0.015 per 1,000 characters,
  so **about $0.061**. Speech-to-text checks ran locally (faster-whisper `small.en`) at no cost.
- **Credit:** Voice: xAI Text to Speech (Grok), via fal.ai. Commercial-use / Partner model.
  Do not present as human-generated. Do not use to train models.

Story lines have three takes. Takes 01 and 02 send the draft line with no speech tags.
Take 03 of a multi-sentence line inserts a documented `[pause]` tag between sentences;
the spoken words stay exact. Barks have two exact takes and no pause tags.

### Generations

| Stem | Exact `text` sent | fal `request_id` |
|---|---|---|
| `VO_Bark_SquadEngageAhead_A_01` | `Bandit, ahead. Mine.` | `01a1dade-5f59-7861-b1cb-f9e1b69747e9` |
| `VO_Bark_SquadEngageAhead_A_02` | `Bandit, ahead. Mine.` | `01a1dade-5f91-7f93-954e-7f8847cc69d5` |
| `VO_Bark_SquadEngageAhead_B_01` | `On the one ahead!` | `01a1dade-5f93-71d2-af58-a69f85c6eee6` |
| `VO_Bark_SquadEngageAhead_B_02` | `On the one ahead!` | `01a1dade-5f8c-7261-b3b7-87646f3a0e08` |
| `VO_Bark_SquadEngageLeft_A_01` | `Bandit, left. Mine.` | `01a1dade-6c86-7be2-a29c-4cf4654deff7` |
| `VO_Bark_SquadEngageLeft_A_02` | `Bandit, left. Mine.` | `01a1dade-6cca-7650-8e00-7f75afc187e9` |
| `VO_Bark_SquadEngageLeft_B_01` | `On the one left!` | `01a1dade-6cbc-7770-a70f-22c8e8229e52` |
| `VO_Bark_SquadEngageLeft_B_02` | `On the one left!` | `01a1dade-6d06-7cd3-afd9-6acdbc0bc21e` |
| `VO_Bark_SquadEngageRight_A_01` | `Bandit, right. Mine.` | `01a1dade-6d06-74e1-8f2f-b70ecb488be5` |
| `VO_Bark_SquadEngageRight_A_02` | `Bandit, right. Mine.` | `01a1dade-6cf5-7703-9993-8d54d3e44fc1` |
| `VO_Bark_SquadEngageRight_B_01` | `On the one right!` | `01a1dade-6d30-7a92-8ea7-6385f59129a2` |
| `VO_Bark_SquadEngageRight_B_02` | `On the one right!` | `01a1dade-6ce4-74c1-8055-3f27ceaa4514` |
| `VO_Bark_SquadEngageBehind_A_01` | `Bandit, behind. Mine.` | `01a1dade-79dc-7a00-ae41-3e4bb0b7323b` |
| `VO_Bark_SquadEngageBehind_A_02` | `Bandit, behind. Mine.` | `01a1dade-79f7-7263-82b2-3f0a551529c5` |
| `VO_Bark_SquadEngageBehind_B_01` | `On the one behind!` | `01a1dade-7a44-78c1-8ca3-a227542b3cda` |
| `VO_Bark_SquadEngageBehind_B_02` | `On the one behind!` | `01a1dade-7a51-74a1-8a65-867fba3ea0e4` |
| `VO_Bark_SquadEngageHighAhead_A_01` | `Bandit, high ahead. Mine.` | `01a1dade-7a34-7fb3-a9c7-7d4cf5b419bf` |
| `VO_Bark_SquadEngageHighAhead_A_02` | `Bandit, high ahead. Mine.` | `01a1dade-7a59-7df1-aed0-31f45cd9d9d4` |
| `VO_Bark_SquadEngageHighAhead_B_01` | `On the one high ahead!` | `01a1dade-7a51-74a1-8a65-866dfd6e6311` |
| `VO_Bark_SquadEngageHighAhead_B_02` | `On the one high ahead!` | `01a1dade-7a56-7840-b69d-89a155941419` |
| `VO_Bark_SquadEngageHighLeft_A_01` | `Bandit, high left. Mine.` | `01a1dadf-c6d5-79f3-a66c-f1d305b63f3a` |
| `VO_Bark_SquadEngageHighLeft_A_02` | `Bandit, high left. Mine.` | `01a1dadf-c6e4-72a0-ba1f-8722547301c1` |
| `VO_Bark_SquadEngageHighLeft_B_01` | `On the one high left!` | `01a1dadf-c75f-7142-8ad3-085a8cd1b9dd` |
| `VO_Bark_SquadEngageHighLeft_B_02` | `On the one high left!` | `01a1dadf-c701-7d02-9ea8-3ac33aedf553` |
| `VO_Bark_SquadEngageHighRight_A_01` | `Bandit, high right. Mine.` | `01a1dadf-c75d-71b0-b1f2-893cb9d35b78` |
| `VO_Bark_SquadEngageHighRight_A_02` | `Bandit, high right. Mine.` | `01a1dadf-c761-7870-aec2-d561a97807c3` |
| `VO_Bark_SquadEngageHighRight_B_01` | `On the one high right!` | `01a1dadf-c75e-7142-82f9-9af459ace39e` |
| `VO_Bark_SquadEngageHighRight_B_02` | `On the one high right!` | `01a1dadf-c763-7e71-b2d0-e30afbb7033c` |
| `VO_Bark_SquadEngageHighBehind_A_01` | `Bandit, high behind. Mine.` | `01a1dadf-d201-74a3-8c3f-5adc18dcfac9` |
| `VO_Bark_SquadEngageHighBehind_A_02` | `Bandit, high behind. Mine.` | `01a1dadf-d21a-7081-8aac-00e50cce06f4` |
| `VO_Bark_SquadEngageHighBehind_B_01` | `On the one high behind!` | `01a1dadf-d29c-7ae1-a767-4f49024a41f6` |
| `VO_Bark_SquadEngageHighBehind_B_02` | `On the one high behind!` | `01a1dadf-d26e-7d02-8797-d7e9d728d112` |
| `VO_Bark_SquadEngageLowAhead_A_01` | `Bandit, low ahead. Mine.` | `01a1dadf-d295-7210-a766-7b1ac27eb76b` |
| `VO_Bark_SquadEngageLowAhead_A_02` | `Bandit, low ahead. Mine.` | `01a1dadf-d29d-71f2-ad3f-1b898384c94c` |
| `VO_Bark_SquadEngageLowAhead_B_01` | `On the one low ahead!` | `01a1dadf-d25d-7180-be18-7193d892705d` |
| `VO_Bark_SquadEngageLowAhead_B_02` | `On the one low ahead!` | `01a1dadf-d258-7863-8b5b-9016aaaadc7a` |
| `VO_Bark_SquadEngageLowLeft_A_01` | `Bandit, low left. Mine.` | `01a1dadf-e22a-71c3-9d8c-6859c16573d2` |
| `VO_Bark_SquadEngageLowLeft_A_02` | `Bandit, low left. Mine.` | `01a1dadf-e1d6-7061-a083-1341e182f9c7` |
| `VO_Bark_SquadEngageLowLeft_B_01` | `On the one low left!` | `01a1dadf-e1cf-7970-b4e9-36512f7dadd8` |
| `VO_Bark_SquadEngageLowLeft_B_02` | `On the one low left!` | `01a1dadf-e1d4-7ef0-9f96-d72db5336aa4` |
| `VO_Bark_SquadEngageLowRight_A_01` | `Bandit, low right. Mine.` | `01a1dadf-e1d4-7441-801f-11713cb2f9ce` |
| `VO_Bark_SquadEngageLowRight_A_02` | `Bandit, low right. Mine.` | `01a1dadf-e1e5-73e2-84da-28ff49c2b075` |
| `VO_Bark_SquadEngageLowRight_B_01` | `On the one low right!` | `01a1dadf-e1ce-72f1-a0a5-75c8dfa15f6e` |
| `VO_Bark_SquadEngageLowRight_B_02` | `On the one low right!` | `01a1dadf-e1e5-7091-8ca8-deff85a0e1d9` |
| `VO_Bark_SquadEngageLowBehind_A_01` | `Bandit, low behind. Mine.` | `01a1dadf-f1be-7d60-b3c6-fde97cd22a59` |
| `VO_Bark_SquadEngageLowBehind_A_02` | `Bandit, low behind. Mine.` | `01a1dadf-f1c6-7ac1-b962-5b392b85d1e9` |
| `VO_Bark_SquadEngageLowBehind_B_01` | `On the one low behind!` | `01a1dadf-f2c5-7530-8b30-33c8e53ef9f2` |
| `VO_Bark_SquadEngageLowBehind_B_02` | `On the one low behind!` | `01a1dadf-f286-79b2-92f3-af9433bc63fe` |
| `VO_Bark_SquadWingmanKill_A_01` | `Splash one!` | `01a1dadf-f282-7061-a4c0-fa70fd4eeb99` |
| `VO_Bark_SquadWingmanKill_A_02` | `Splash one!` | `01a1dadf-f2c0-71e3-ad48-420fdc7f78f4` |
| `VO_Bark_SquadWingmanKill_B_01` | `And he's gone.` | `01a1dadf-f277-7a10-9202-cc1247d887a4` |
| `VO_Bark_SquadWingmanKill_B_02` | `And he's gone.` | `01a1dadf-f2c2-7ba0-85c1-4110f6b521e0` |
| `VO_Bark_PlayerKill_A_01` | `Nice shot!` | `01a1dae2-4e60-7b73-b7f7-40d7890a6ddd` |
| `VO_Bark_PlayerKill_A_02` | `Nice shot!` | `01a1dae2-4ea1-7d70-ad40-8fb4f6ac1325` |
| `VO_Bark_PlayerKill_B_01` | `Leave some for me.` | `01a1dae2-4e6d-7902-99b9-24304b66ff05` |
| `VO_Bark_PlayerKill_B_02` | `Leave some for me.` | `01a1dae2-4e7c-78d1-87b5-6d8b02bcfd39` |
| `VO_Bark_MultiKill_01` | `Now that's flying!` | `01a1dae2-4eb1-7973-9894-54effb59ac23` |
| `VO_Bark_MultiKill_02` | `Now that's flying!` | `01a1dae2-4e82-77d1-a1ec-b64ffcdc8477` |
| `VO_Bark_SquadLowHull_A_01` | `Taking a beating here!` | `01a1dae2-4ec2-72d1-a611-e47ebecfcd43` |
| `VO_Bark_SquadLowHull_A_02` | `Taking a beating here!` | `01a1dae2-4e70-7991-9da4-dd036cda0153` |
| `VO_Bark_SquadLowHull_B_01` | `Hull's going, need a second!` | `01a1dae2-5d0b-7672-a804-09d474d4c399` |
| `VO_Bark_SquadLowHull_B_02` | `Hull's going, need a second!` | `01a1dae2-5c76-7a13-aa71-1cd974b9aba3` |
| `VO_Bark_ShieldDown_01` | `Shield's gone!` | `01a1dae2-5c77-76e2-b26d-00d1ab85c1f3` |
| `VO_Bark_ShieldDown_02` | `Shield's gone!` | `01a1dae2-5cbe-7272-8a9e-57a17095e81f` |
| `VO_Bark_ShieldBack_01` | `Shield's back. Let's go.` | `01a1dae2-5cd9-7942-9d0c-1c23508acee1` |
| `VO_Bark_ShieldBack_02` | `Shield's back. Let's go.` | `01a1dae2-5cbd-7703-908f-c2eec03a3e7c` |
| `VO_Bark_SquadShipDown_A_01` | `Kato's down!` | `01a1dae2-5cdb-7920-b70d-6ad70d5c12c3` |
| `VO_Bark_SquadShipDown_A_02` | `Kato's down!` | `01a1dae2-5cf8-7053-aedf-d6bd290ac203` |
| `VO_Bark_SquadShipDown_B_01` | `Moss is out, cover him!` | `01a1dae2-6c64-7e70-8a88-abf3d963d373` |
| `VO_Bark_SquadShipDown_B_02` | `Moss is out, cover him!` | `01a1dae2-6c63-76e3-a0f2-2b61eb9113c3` |
| `VO_Bark_SquadRespawn_01` | `Back in it. Miss me?` | `01a1dae2-6cb4-78d2-b331-8752f046f207` |
| `VO_Bark_SquadRespawn_02` | `Back in it. Miss me?` | `01a1dae2-6c8b-79f3-85a4-f97ae0bc69d5` |
| `VO_Bark_SquadWipe_01` | `No, no, no...` | `01a1dae2-6ce2-70d3-96e8-9cd1ff37b9a4` |
| `VO_Bark_SquadWipe_02` | `No, no, no...` | `01a1dae2-6cc4-7993-b9f5-4bd87728638c` |
| `VO_Bark_MissileIncoming_01` | `Missile on you, break!` | `01a1dae2-6c94-7983-b4e7-cc60c4f43436` |
| `VO_Bark_MissileIncoming_02` | `Missile on you, break!` | `01a1dae2-6cc0-7ec2-95db-6262aece3d5f` |
| `VO_Bark_MissileEvaded_01` | `Too slow.` | `01a1dae2-7d19-7f63-a05b-c952a8b32602` |
| `VO_Bark_MissileEvaded_02` | `Too slow.` | `01a1dae2-7cd0-7ee1-b00b-37fce2c7ac88` |
| `VO_Bark_OnMySix_01` | `One on your tail!` | `01a1dae2-7d36-78c1-bf17-0ebb68932b39` |
| `VO_Bark_OnMySix_02` | `One on your tail!` | `01a1dae2-7cfe-7de0-85be-35843cc891b1` |
| `VO_Bark_Anticipated_01` | `It moved before I did. How?` | `01a1dae2-7d1a-7e62-af1b-dff7766e4bb4` |
| `VO_Bark_Anticipated_02` | `It moved before I did. How?` | `01a1dae2-7d17-7f30-9802-8736f0817ff7` |
| `VO_Bark_TargetShielded_01` | `It's shielded, hit it harder!` | `01a1dae2-7d1d-7031-b6b5-9a302df0a01c` |
| `VO_Bark_TargetShielded_02` | `It's shielded, hit it harder!` | `01a1dae2-7d1c-7943-a6e2-796cb7abbeca` |
| `VO_Bark_MinesAhead_01` | `Mines! Pull up!` | `01a1dae4-2b8e-7b21-acc6-8b99c1766de0` |
| `VO_Bark_MinesAhead_02` | `Mines! Pull up!` | `01a1dae4-2bce-7b12-b394-ff4b6123e3e9` |
| `VO_Bark_CarrierLaunch_01` | `More fighters coming out!` | `01a1dae4-2bdd-72b0-a1d8-4c7443664530` |
| `VO_Bark_CarrierLaunch_02` | `More fighters coming out!` | `01a1dae4-2c23-7e50-895d-e8e89792b276` |
| `VO_Bark_AmmoLow_A_01` | `Running light on missiles.` | `01a1dae4-2c24-75c1-affd-ca11a5ec1937` |
| `VO_Bark_AmmoLow_A_02` | `Running light on missiles.` | `01a1dae4-2be1-7a81-a756-616317bb0217` |
| `VO_Bark_AmmoOut_B_01` | `I'm dry!` | `01a1dae4-2c20-7850-9d3b-15d8617d800e` |
| `VO_Bark_AmmoOut_B_02` | `I'm dry!` | `01a1dae4-2bd2-7113-8096-097bef1bfd4a` |
| `VO_Bark_WeaponHot_01` | `Guns are cooking!` | `01a1dae4-36da-7c91-aa2e-293c7ff530ff` |
| `VO_Bark_WeaponHot_02` | `Guns are cooking!` | `01a1dae4-3692-7ec3-bb5c-9d75184fea69` |
| `VO_Bark_BoostEmpty_01` | `Out of burn!` | `01a1dae4-3635-7800-bc27-8e1e6e1e7ece` |
| `VO_Bark_BoostEmpty_02` | `Out of burn!` | `01a1dae4-363d-7f42-925c-126dda82e08f` |
| `VO_Bark_Boundary_01` | `Wide of the zone. Bring it round.` | `01a1dae4-36c6-7041-bc62-d83440ff7dad` |
| `VO_Bark_Boundary_02` | `Wide of the zone. Bring it round.` | `01a1dae4-3631-7fe1-9a42-e82a9c2a6299` |
| `VO_Bark_WaveCleared_01` | `That's all of them. For now.` | `01a1dae4-3633-7421-bb8b-f3e92f79304f` |
| `VO_Bark_WaveCleared_02` | `That's all of them. For now.` | `01a1dae4-36bb-7431-8e01-10dbd4199332` |
| `VO_Bark_WaveIncoming_01` | `Here they come again.` | `01a1dae4-4452-70a2-b33a-d0eec181a6cc` |
| `VO_Bark_WaveIncoming_02` | `Here they come again.` | `01a1dae4-4473-7a00-8336-58748b3a53e9` |
| `VO_Bark_Breather_01` | `Anyone else enjoying this?` | `01a1dae4-44b8-7882-a78a-0cefa6637da6` |
| `VO_Bark_Breather_02` | `Anyone else enjoying this?` | `01a1dae4-44bb-7921-96db-78ee7ce3c067` |
| `VO_Bark_SquadSwitchIn_A_01` | `Rift's yours.` | `01a1dae4-4497-7fd2-9ac4-5f45b59523dc` |
| `VO_Bark_SquadSwitchIn_A_02` | `Rift's yours.` | `01a1dae4-4540-78b0-bc6c-2a12a4cacb22` |
| `VO_Bark_SquadSwitchIn_B_01` | `My turn.` | `01a1dae4-44bb-7921-96db-78dc936f376c` |
| `VO_Bark_SquadSwitchIn_B_02` | `My turn.` | `01a1dae4-44bd-7283-bf99-fd6e6e106c7b` |
| `VO_Bark_SquadSwitchStrike_01` | `Strike's hot!` | `01a1dae4-529d-7a81-9275-927b4075e0d0` |
| `VO_Bark_SquadSwitchStrike_02` | `Strike's hot!` | `01a1dae4-529d-7a81-9275-926b5129b2f3` |
| `VO_Bark_SquadFocusAck_01` | `On it!` | `01a1dae4-527f-7872-8b9f-ab7477a273fa` |
| `VO_Bark_SquadFocusAck_02` | `On it!` | `01a1dae4-5297-74b1-916b-0216e0d8ded9` |
| `VO_Bark_PowerReady_01` | `Portals are up.` | `01a1dae4-5297-7bd0-b12e-53bc83067c85` |
| `VO_Bark_PowerReady_02` | `Portals are up.` | `01a1dae4-52dc-73f0-b5a0-a1440c9ae8a6` |
| `VO_Bark_PowerNotReady_01` | `Not yet!` | `01a1dae4-52df-7ea3-9c0b-1e2435821825` |
| `VO_Bark_PowerNotReady_02` | `Not yet!` | `01a1dae4-5297-7f80-9dab-c1ee48b2d845` |
| `VO_Bark_Portal_Open_A_01` | `Door's open.` | `01a1dae5-2d1c-78b1-aa9b-eda293238a5e` |
| `VO_Bark_Portal_Open_A_02` | `Door's open.` | `01a1dae5-2d4b-75a3-8cb7-5aff9aa615fa` |
| `VO_Bark_Portal_Through_B_01` | `And... through!` | `01a1dae5-2d66-7b12-bab4-a13f15674935` |
| `VO_Bark_Portal_Through_B_02` | `And... through!` | `01a1dae5-2d79-7c80-aea5-7e514911d137` |
| `VO_Bark_Portal_MissileReturn_C_01` | `Return to sender!` | `01a1dae5-2d47-7441-8cf4-fed8c79b7123` |
| `VO_Bark_Portal_MissileReturn_C_02` | `Return to sender!` | `01a1dae5-2d5e-7c10-9631-a32fadefe543` |
| `VO_Bark_AbilityUnlocked_Cloak_01` | `Cloak's online. Now you don't see me.` | `01a1dae5-2d4b-7110-b4de-4883a1677563` |
| `VO_Bark_AbilityUnlocked_Cloak_02` | `Cloak's online. Now you don't see me.` | `01a1dae5-2d46-7280-a7db-23c973b012c7` |
| `VO_Bark_Cloak_On_A_01` | `Going dark.` | `01a1dae5-351e-70c1-9daa-bfd8c51135a4` |
| `VO_Bark_Cloak_On_A_02` | `Going dark.` | `01a1dae5-359a-7f90-8481-49f24cb71297` |
| `VO_Bark_Cloak_Ambush_B_01` | `Surprise.` | `01a1dae5-35bd-7522-b01c-51816c0851fe` |
| `VO_Bark_Cloak_Ambush_B_02` | `Surprise.` | `01a1dae5-357f-71c1-a2ae-132be087d196` |
| `VO_Bark_AbilityLocked_01` | `Haven't got that yet.` | `01a1dae5-35d6-73c0-91de-6a999564a496` |
| `VO_Bark_AbilityLocked_02` | `Haven't got that yet.` | `01a1dae5-35e1-71f0-98b3-bc883b821a06` |
| `VO_Bark_Hyper_Spool_A_01` | `Spooling up!` | `01a1dae5-3592-7aa0-8952-9e9c8cd6a0b8` |
| `VO_Bark_Hyper_Spool_A_02` | `Spooling up!` | `01a1dae5-3553-7013-b793-af0e97e3e3e7` |
| `VO_Bark_Hyper_Blocked_B_01` | `Can't jump, too hot here!` | `01a1dae5-4633-7771-8399-304ef3ead96d` |
| `VO_Bark_Hyper_Blocked_B_02` | `Can't jump, too hot here!` | `01a1dae5-46cf-7121-b2f1-8790865b002a` |
| `VO_Bark_Gate_Enter_01` | `Shortcut!` | `01a1dae5-46c6-76d2-b363-71b4a3b051c8` |
| `VO_Bark_Gate_Enter_02` | `Shortcut!` | `01a1dae5-46c7-7022-ac73-b78a097225e8` |
| `VO_Bark_Atmo_Entry_01` | `Going in hot!` | `01a1dae5-4686-73d0-ac8b-2a7a57b9b029` |
| `VO_Bark_Atmo_Entry_02` | `Going in hot!` | `01a1dae5-46ca-7ef0-9b76-5a5704a4a273` |
| `VO_Bark_Land_Ready_A_01` | `There's the pad.` | `01a1dae5-46c1-79f1-8b69-0642f6789aa2` |
| `VO_Bark_Land_Ready_A_02` | `There's the pad.` | `01a1dae5-4686-7f60-a600-62dfaf8b306d` |
| `VO_Bark_Land_Down_B_01` | `Down. Let's look around.` | `01a1dae5-5525-7340-bd16-215a52cd64c2` |
| `VO_Bark_Land_Down_B_02` | `Down. Let's look around.` | `01a1dae5-54e8-7d63-84ef-babb9b361ec6` |
| `VO_Bark_TakeOff_01` | `Lifting off.` | `01a1dae5-5527-7750-a473-9e4f2801bac2` |
| `VO_Bark_TakeOff_02` | `Lifting off.` | `01a1dae5-555c-7142-8a2e-33a7b150b55f` |
| `VO_Bark_Planet_Arrive_Verdance_A_01` | `It glows. Everything glows.` | `01a1dae5-5527-7460-8d59-1c27b3659e93` |
| `VO_Bark_Planet_Arrive_Verdance_A_02` | `It glows. Everything glows.` | `01a1dae5-5528-7032-908f-0d866dd6929f` |
| `VO_Bark_Planet_Arrive_Cinder_B_01` | `Hot enough for you?` | `01a1dae5-5531-7c13-9421-dce4919f793c` |
| `VO_Bark_Planet_Arrive_Cinder_B_02` | `Hot enough for you?` | `01a1dae5-5522-78e1-859f-1971d6c75620` |
| `VO_Bark_Dom_GeneratorHit_A_01` | `It's cracking!` | `01a1dae6-1b98-7e22-866d-87560209bd9d` |
| `VO_Bark_Dom_GeneratorHit_A_02` | `It's cracking!` | `01a1dae6-1ba1-7193-95ca-e2c0c4b663b4` |
| `VO_Bark_Dom_GeneratorDown_B_01` | `Generator's gone!` | `01a1dae6-1bad-71c2-83c0-2501475d145c` |
| `VO_Bark_Dom_GeneratorDown_B_02` | `Generator's gone!` | `01a1dae6-1bb2-73a1-b596-b958b7169d87` |
| `VO_Bark_Dom_Turrets_01` | `Turrets, watch the turrets!` | `01a1dae6-1bd0-7893-bfc2-e89eb8268f90` |
| `VO_Bark_Dom_Turrets_02` | `Turrets, watch the turrets!` | `01a1dae6-1bb3-79f3-8891-c8b6906e6c9e` |
| `VO_Bark_Dom_Tractor_01` | `Something's pulling us in.` | `01a1dae6-1bcf-7582-91a0-fb176ee68d04` |
| `VO_Bark_Dom_Tractor_02` | `Something's pulling us in.` | `01a1dae6-1bc5-7461-bb79-890a358e6b85` |
| `VO_Bark_Dom_EscapeFlight_01` | `Punch it!` | `01a1dae6-27c4-70a3-8a8e-836fb6882a3e` |
| `VO_Bark_Dom_EscapeFlight_02` | `Punch it!` | `01a1dae6-2778-78b0-806e-122c90ce61ea` |
| `VO_Bark_Dom_Clear_01` | `We're clear! We're clear!` | `01a1dae6-27e5-70c0-891a-227fd3badd2f` |
| `VO_Bark_Dom_Clear_02` | `We're clear! We're clear!` | `01a1dae6-2828-7a50-b8ce-494de84002f6` |
| `VO_Bark_Foot_Contact_01` | `You've got company!` | `01a1dae6-2820-7622-a949-b09a644ccd85` |
| `VO_Bark_Foot_Contact_02` | `You've got company!` | `01a1dae6-27ed-7be3-8ef7-3b61a2dbc250` |
| `VO_Bark_Foot_Deflect_A_01` | `Back at you.` | `01a1dae6-282a-7943-b44d-41bbca43167a` |
| `VO_Bark_Foot_Deflect_A_02` | `Back at you.` | `01a1dae6-27d1-7bc0-b920-bc4bb15d1979` |
| `VO_Bark_Foot_Perfect_B_01` | `Right between the eyes!` | `01a1dae6-3b87-7f50-8eea-1c6af4c34123` |
| `VO_Bark_Foot_Perfect_B_02` | `Right between the eyes!` | `01a1dae6-3b4a-7eb1-acac-6540552fe94a` |
| `VO_Bark_Foot_GuardBroken_01` | `Guard's gone!` | `01a1dae6-3b85-7680-ac46-ceffc1474458` |
| `VO_Bark_Foot_GuardBroken_02` | `Guard's gone!` | `01a1dae6-3b84-7743-a0ad-b4694834e458` |
| `VO_Bark_Foot_Hurt_A_01` | `Ow!` | `01a1dae6-3b4b-7643-b82b-04ae3970d67a` |
| `VO_Bark_Foot_Hurt_A_02` | `Ow!` | `01a1dae6-3b80-7742-a307-a428eec26898` |
| `VO_Bark_Foot_LowHealth_B_01` | `I'm hurt, I'm hurt.` | `01a1dae6-3b86-7590-93f7-6efb8fc7bc23` |
| `VO_Bark_Foot_LowHealth_B_02` | `I'm hurt, I'm hurt.` | `01a1dae6-3b91-76a2-b59d-43db0652d391` |
| `VO_Bark_Foot_Heal_01` | `Better.` | `01a1dae6-456f-7bd0-8528-160731a80803` |
| `VO_Bark_Foot_Heal_02` | `Better.` | `01a1dae6-45dd-7b62-bbcb-f4f5d468642c` |
| `VO_Bark_Foot_Kill_A_01` | `Next!` | `01a1dae6-4608-7ba3-ad84-29e3a31b4814` |
| `VO_Bark_Foot_Kill_A_02` | `Next!` | `01a1dae6-45cc-7432-bcae-cdafe7388dc7` |
| `VO_Bark_Foot_LastOne_B_01` | `That's the lot.` | `01a1dae6-45df-7210-8cb8-8f8a6d874880` |
| `VO_Bark_Foot_LastOne_B_02` | `That's the lot.` | `01a1dae6-460b-7531-9cfa-c2636c1e6679` |
| `VO_Bark_Foot_Dash_A_01` | `Whoo!` | `01a1dae6-460e-7191-a9b5-8a7dfca8f379` |
| `VO_Bark_Foot_Dash_A_02` | `Whoo!` | `01a1dae6-461d-7c62-bb91-c512bb6c5d4a` |
| `VO_Bark_Foot_Slam_B_01` | `Boom.` | `01a1dae7-520c-7ec0-a914-8e1e7da09f95` |
| `VO_Bark_Foot_Slam_B_02` | `Boom.` | `01a1dae7-52ce-7b83-87d4-e5acb10108cc` |
| `VO_Bark_Foot_ObjectiveNear_A_01` | `That's it, right ahead.` | `01a1dae7-52da-7783-b02e-9873eeadb8da` |
| `VO_Bark_Foot_ObjectiveNear_A_02` | `That's it, right ahead.` | `01a1dae7-52c7-74c2-9cb3-fbe09697a656` |
| `VO_Bark_Foot_ObjectiveDone_B_01` | `Got it. Heading back.` | `01a1dae7-532e-7de1-9468-96fd0c177acf` |
| `VO_Bark_Foot_ObjectiveDone_B_02` | `Got it. Heading back.` | `01a1dae7-5248-7712-9a05-e1029dc83fe0` |
| `VO_Bark_Foot_DoorLocked_A_01` | `Locked. Figures.` | `01a1dae7-52da-77e2-a29e-cd5dc66340a7` |
| `VO_Bark_Foot_DoorLocked_A_02` | `Locked. Figures.` | `01a1dae7-527e-7992-800f-c987c11eac39` |
| `VO_Bark_Foot_DoorOpen_B_01` | `We're in.` | `01a1dae7-6299-77c0-b4db-058a37b06a8a` |
| `VO_Bark_Foot_DoorOpen_B_02` | `We're in.` | `01a1dae7-6288-7160-b600-ac81a1bc9597` |
| `VO_Bark_Foot_Hazard_01` | `Get out of there!` | `01a1dae7-62d9-7143-9b38-9c9340b82f6e` |
| `VO_Bark_Foot_Hazard_02` | `Get out of there!` | `01a1dae7-6294-7373-816d-9bd80b1a74b2` |
| `VO_Bark_WardenPhase2_01` | `It's angry now!` | `01a1dae7-6299-75a3-9012-bc63a0ec7c22` |
| `VO_Bark_WardenPhase2_02` | `It's angry now!` | `01a1dae7-628c-7893-b6bc-cf039bca9d70` |
| `VO_Bark_Foot_Idle_01` | `You sightseeing down there?` | `01a1dae7-6289-7d21-9985-8cb5c1c682b7` |
| `VO_Bark_Foot_Idle_02` | `You sightseeing down there?` | `01a1dae7-6294-75b1-8518-27a12d10c5d6` |
| `VO_Bark_Dom_Boarded_01` | `Bring something back.` | `01a1dae7-a5cd-7513-b0e1-da417b6346e3` |
| `VO_Bark_Dom_Boarded_02` | `Bring something back.` | `01a1dae7-a63b-7b60-98b8-14aa17fa06ce` |
| `VO_Bark_Dom_CoreShield_A_01` | `Shield's dropping!` | `01a1dae7-a642-7421-9f64-d5f05f88a747` |
| `VO_Bark_Dom_CoreShield_A_02` | `Shield's dropping!` | `01a1dae7-a5f9-7d12-bc87-804a44bf02fc` |
| `VO_Bark_Dom_CoreOpen_B_01` | `There's the heart!` | `01a1dae7-a640-78d3-8b41-e30180cc3617` |
| `VO_Bark_Dom_CoreOpen_B_02` | `There's the heart!` | `01a1dae7-a649-76c1-974e-97c509fcc792` |
| `VO_Bark_Dom_Escape_Start_A_01` | `Run! Run now!` | `01a1dae7-a63c-7e50-890c-ad079e4ba3b6` |
| `VO_Bark_Dom_Escape_Start_A_02` | `Run! Run now!` | `01a1dae7-a64e-78d1-8138-0c197725b6a3` |
| `VO_Bark_Dom_Escape_30_B_01` | `Thirty seconds!` | `01a1dae7-b242-7911-8d75-ac600e3bf8bd` |
| `VO_Bark_Dom_Escape_30_B_02` | `Thirty seconds!` | `01a1dae7-b208-76f2-94cc-ee2e94c8dd08` |
| `VO_Bark_Dom_Escape_10_C_01` | `Move it!` | `01a1dae7-b2be-78f0-90a8-19b85109fc8f` |
| `VO_Bark_Dom_Escape_10_C_02` | `Move it!` | `01a1dae7-b2d0-7542-9c0c-2cccda091bea` |
| `VO_Bark_Banter_BetYouTen_01` | `Bet you ten I get more than you this round.` | `01a1dae7-b2d4-7f82-b96a-9fe7519debd1` |
| `VO_Bark_Banter_BetYouTen_02` | `Bet you ten I get more than you this round.` | `01a1dae7-b281-74d3-b6d2-96cf0810d8da` |
| `VO_Bark_Banter_WereInSpace_01` | `We're in space, Moss.` | `01a1dae7-b231-7202-93bd-119f31967e9a` |
| `VO_Bark_Banter_WereInSpace_02` | `We're in space, Moss.` | `01a1dae7-b283-73d3-b443-8b611d6463f3` |
| `VO_Bark_Banter_TidyRaiders_01` | `So they're tidy raiders.` | `01a1dae7-c3bc-7b52-9c2d-4502d56d53e5` |
| `VO_Bark_Banter_TidyRaiders_02` | `So they're tidy raiders.` | `01a1dae7-c399-7d72-9e62-4b14586b674f` |
| `VO_Bark_Banter_SaySomethingNice_01` | `Kato, say something nice about my flying.` | `01a1dae7-c39c-7112-9047-4840bad6f66e` |
| `VO_Bark_Banter_SaySomethingNice_02` | `Kato, say something nice about my flying.` | `01a1dae7-c436-7043-b338-51643aaad163` |
| `VO_Bark_Banter_IllTakeIt_01` | `I'll take it.` | `01a1dae7-c3d1-70f1-bb08-8721a959439c` |
| `VO_Bark_Banter_IllTakeIt_02` | `I'll take it.` | `01a1dae7-c439-71b0-af7f-fe0870fef120` |
| `VO_Bark_Banter_PleaseNotTheStew_01` | `Please not the stew.` | `01a1dae7-c3cc-7070-a300-b6f943af9280` |
| `VO_Bark_Banter_PleaseNotTheStew_02` | `Please not the stew.` | `01a1dae7-c3e6-7651-8544-f942a2f2a79d` |

## Edit (dry takes)

Same dry edit as PR #93 (`present_order_r1`):

1. Find the first and last speech on a 10 ms RMS envelope at −45 dBFS. Everything between those
   bounds is kept, including mid-sentence pauses. DC is removed.
2. Pad **50 ms** of silence before the onset and **100 ms** after the offset.
3. 3 ms linear fade-in, 20 ms cos² fade-out, so the pads are click-free.
4. One linear gain to **−19 LUFS** (the project's `vo` target). Gain is also capped so true peak
   stays at or under −1.5 dBTP (`vo` allows −1.0). 24-bit WAV, 48 kHz, mono.

## Radio chain

Same chain as PR #78 take 04, **without** the mid-word static tail (that tail was only for
the interrupted "Don't—" cut). Built from each normalised dry take, numpy and scipy,
seeded by `sha1(stem)`:

1. Band-limit: 4th-order Butterworth high-pass at 300 Hz, then 6th-order Butterworth low-pass at 3.4 kHz.
2. Light saturation: `tanh(2.2·x)/tanh(2.2)` on the peak-normalised signal, then the 3.4 kHz low-pass again.
3. Carrier hiss under the voice: band-limited noise at −38 dBFS.
4. 250 Hz high-pass and mean removal (DC), a 3 ms fade-in and 12 ms fade-out, one gain to −19 LUFS.

Radio files ship for: `Bark_SquadEngageAhead:A`, `Bark_SquadEngageAhead:B`, `Bark_SquadEngageLeft:A`, `Bark_SquadEngageLeft:B`, `Bark_SquadEngageRight:A`, `Bark_SquadEngageRight:B`, `Bark_SquadEngageBehind:A`, `Bark_SquadEngageBehind:B`, `Bark_SquadEngageHighAhead:A`, `Bark_SquadEngageHighAhead:B`, `Bark_SquadEngageHighLeft:A`, `Bark_SquadEngageHighLeft:B`, `Bark_SquadEngageHighRight:A`, `Bark_SquadEngageHighRight:B`, `Bark_SquadEngageHighBehind:A`, `Bark_SquadEngageHighBehind:B`, `Bark_SquadEngageLowAhead:A`, `Bark_SquadEngageLowAhead:B`, `Bark_SquadEngageLowLeft:A`, `Bark_SquadEngageLowLeft:B`, `Bark_SquadEngageLowRight:A`, `Bark_SquadEngageLowRight:B`, `Bark_SquadEngageLowBehind:A`, `Bark_SquadEngageLowBehind:B`, `Bark_SquadWingmanKill:A`, `Bark_SquadWingmanKill:B`, `Bark_PlayerKill:A`, `Bark_PlayerKill:B`, `Bark_MultiKill`, `Bark_SquadLowHull:A`, `Bark_SquadLowHull:B`, `Bark_ShieldDown`, `Bark_ShieldBack`, `Bark_SquadShipDown:A`, `Bark_SquadShipDown:B`, `Bark_SquadRespawn`, `Bark_SquadWipe`, `Bark_MissileIncoming`, `Bark_MissileEvaded`, `Bark_OnMySix`, `Bark_Anticipated`, `Bark_TargetShielded`, `Bark_MinesAhead`, `Bark_CarrierLaunch`, `Bark_AmmoLow:A`, `Bark_AmmoOut:B`, `Bark_WeaponHot`, `Bark_BoostEmpty`, `Bark_Boundary`, `Bark_WaveCleared`, `Bark_WaveIncoming`, `Bark_Breather`, `Bark_SquadSwitchIn:A`, `Bark_SquadSwitchIn:B`, `Bark_SquadSwitchStrike`, `Bark_SquadFocusAck`, `Bark_PowerReady`, `Bark_PowerNotReady`, `Bark_Portal_Open:A`, `Bark_Portal_Through:B`, `Bark_Portal_MissileReturn:C`, `Bark_AbilityUnlocked_Cloak`, `Bark_Cloak_On:A`, `Bark_Cloak_Ambush:B`, `Bark_AbilityLocked`, `Bark_Hyper_Spool:A`, `Bark_Hyper_Blocked:B`, `Bark_Gate_Enter`, `Bark_Atmo_Entry`, `Bark_Land_Ready:A`, `Bark_Land_Down:B`, `Bark_TakeOff`, `Bark_Planet_Arrive_Verdance:A`, `Bark_Planet_Arrive_Cinder:B`, `Bark_Dom_GeneratorHit:A`, `Bark_Dom_GeneratorDown:B`, `Bark_Dom_Turrets`, `Bark_Dom_Tractor`, `Bark_Dom_EscapeFlight`, `Bark_Dom_Clear`, `Bark_Foot_Contact`, `Bark_Foot_Deflect:A`, `Bark_Foot_Perfect:B`, `Bark_Foot_GuardBroken`, `Bark_Foot_Hurt:A`, `Bark_Foot_LowHealth:B`, `Bark_Foot_Heal`, `Bark_Foot_Kill:A`, `Bark_Foot_LastOne:B`, `Bark_Foot_Dash:A`, `Bark_Foot_Slam:B`, `Bark_Foot_ObjectiveNear:A`, `Bark_Foot_ObjectiveDone:B`, `Bark_Foot_DoorLocked:A`, `Bark_Foot_DoorOpen:B`, `Bark_Foot_Hazard`, `Bark_WardenPhase2`, `Bark_Foot_Idle`, `Bark_Dom_Boarded`, `Bark_Dom_CoreShield:A`, `Bark_Dom_CoreOpen:B`, `Bark_Dom_Escape_Start:A`, `Bark_Dom_Escape_30:B`, `Bark_Dom_Escape_10:C`, `Bark_Banter_BetYouTen`, `Bark_Banter_WereInSpace`, `Bark_Banter_TidyRaiders`, `Bark_Banter_SaySomethingNice`, `Bark_Banter_IllTakeIt`, `Bark_Banter_PleaseNotTheStew`.

## Levels

Loudness is K-weighted (ungated under 3 s, gated integrated at or over 3 s). TP is true peak.
The `vo` window is −21..−17 LUFS. One known miss is noted in the recs if present.

| File | Dur s | LUFS | TP dBTP | Lead ms | Tail ms | Mid pause ms | Median F0 Hz | STT (short) |
|---|---|---|---|---|---|---|---|---|
| `VO_Bark_SquadEngageAhead_A_01_dry` | 2.020 | -19.00 | -1.6 | 50.0 | 100.0 | 222.0 | 171 | banned it, ahead, mine. |
| `VO_Bark_SquadEngageAhead_A_02_dry` | 2.008 | -19.00 | -1.8 | 50.0 | 100.0 | 204.0 | 170 | Bandit ahead, mine. |
| `VO_Bark_SquadEngageAhead_B_01_dry` | 1.250 | -19.00 | -6.8 | 50.2 | 100.2 | 0.0 | 228 | on the one head. |
| `VO_Bark_SquadEngageAhead_B_02_dry` | 0.972 | -19.00 | -5.8 | 51.2 | 100.1 | 0.0 | 196 | head. |
| `VO_Bark_SquadEngageLeft_A_01_dry` | 1.826 | -19.00 | -2.1 | 52.4 | 100.0 | 158.0 | 164 | Left. Left. |
| `VO_Bark_SquadEngageLeft_A_02_dry` | 1.948 | -19.00 | -1.6 | 50.0 | 100.0 | 230.0 | 175 | Bandit left mine. |
| `VO_Bark_SquadEngageLeft_B_01_dry` | 1.110 | -19.00 | -4.3 | 50.0 | 100.0 | 88.0 | 181 | On the left. |
| `VO_Bark_SquadEngageLeft_B_02_dry` | 1.000 | -19.00 | -3.7 | 50.0 | 100.0 | 100.0 | 171 | on the one left. |
| `VO_Bark_SquadEngageRight_A_01_dry` | 1.648 | -19.00 | -1.8 | 50.0 | 100.0 | 292.0 | 150 | right, mine. |
| `VO_Bark_SquadEngageRight_A_02_dry` | 2.036 | -19.00 | -1.7 | 50.0 | 100.0 | 362.0 | 160 | Bandit mine |
| `VO_Bark_SquadEngageRight_B_01_dry` | 0.884 | -19.00 | -5.3 | 50.0 | 101.6 | 0.0 | 187 | on the one right. |
| `VO_Bark_SquadEngageRight_B_02_dry` | 1.034 | -19.00 | -6.8 | 52.0 | 100.8 | 0.0 | 211 | on the one right. |
| `VO_Bark_SquadEngageBehind_A_01_dry` | 2.168 | -19.32 | -1.5 | 53.3 | 100.0 | 202.0 | 160 | Bandit Behind Mine |
| `VO_Bark_SquadEngageBehind_A_02_dry` | 2.180 | -19.11 | -1.5 | 50.1 | 100.0 | 220.0 | 163 | Banned it behind mine. |
| `VO_Bark_SquadEngageBehind_B_01_dry` | 1.224 | -19.00 | -3.3 | 50.0 | 100.1 | 0.0 | 170 | on the one behind. |
| `VO_Bark_SquadEngageBehind_B_02_dry` | 1.112 | -19.00 | -4.0 | 50.0 | 100.0 | 0.0 | 189 | behind. |
| `VO_Bark_SquadEngageHighAhead_A_01_dry` | 2.082 | -19.36 | -1.5 | 50.8 | 100.4 | 194.0 | 167 | Bandit High Ahead Mine |
| `VO_Bark_SquadEngageHighAhead_A_02_dry` | 2.202 | -20.85 | -1.5 | 50.4 | 100.0 | 230.0 | 166 | Bandit, High Ahead, Mine |
| `VO_Bark_SquadEngageHighAhead_B_01_dry` | 1.292 | -19.00 | -4.7 | 50.0 | 100.0 | 0.0 | 175 | on the one high. |
| `VO_Bark_SquadEngageHighAhead_B_02_dry` | 1.338 | -19.00 | -5.0 | 55.8 | 100.0 | 0.0 | 195 | on the high ahead. |
| `VO_Bark_SquadEngageHighLeft_A_01_dry` | 2.010 | -19.00 | -2.0 | 52.7 | 100.0 | 116.0 | 168 | Bandit, high left, mine. |
| `VO_Bark_SquadEngageHighLeft_A_02_dry` | 2.096 | -19.00 | -2.5 | 50.0 | 100.0 | 254.0 | 170 | High left. Mine. |
| `VO_Bark_SquadEngageHighLeft_B_01_dry` | 1.274 | -19.00 | -2.9 | 50.3 | 100.0 | 98.0 | 170 | I left. |
| `VO_Bark_SquadEngageHighLeft_B_02_dry` | 1.468 | -19.00 | -3.6 | 50.9 | 100.0 | 96.0 | 169 | on the one high left. |
| `VO_Bark_SquadEngageHighRight_A_01_dry` | 2.116 | -19.04 | -1.5 | 50.0 | 100.0 | 218.0 | 168 | Bandit High Right Mine |
| `VO_Bark_SquadEngageHighRight_A_02_dry` | 1.918 | -19.00 | -2.3 | 51.7 | 100.0 | 134.0 | 158 | high-right mine. |
| `VO_Bark_SquadEngageHighRight_B_01_dry` | 1.116 | -19.00 | -3.4 | 50.1 | 100.0 | 0.0 | 171 | on the |
| `VO_Bark_SquadEngageHighRight_B_02_dry` | 1.194 | -19.00 | -3.8 | 50.0 | 102.3 | 0.0 | 173 | on the one high right. |
| `VO_Bark_SquadEngageHighBehind_A_01_dry` | 2.364 | -19.83 | -1.5 | 50.0 | 100.0 | 254.0 | 164 | Band it, high behind, mine. |
| `VO_Bark_SquadEngageHighBehind_A_02_dry` | 2.444 | -19.00 | -1.9 | 50.0 | 100.0 | 236.0 | 166 | Bandit high behind, mine. |
| `VO_Bark_SquadEngageHighBehind_B_01_dry` | 1.506 | -19.00 | -4.7 | 50.0 | 100.0 | 0.0 | 183 | on the one high behind. |
| `VO_Bark_SquadEngageHighBehind_B_02_dry` | 1.462 | -19.00 | -3.1 | 50.3 | 100.0 | 0.0 | 170 | on the one high behind. |
| `VO_Bark_SquadEngageLowAhead_A_01_dry` | 1.862 | -19.00 | -2.7 | 50.0 | 100.0 | 94.0 | 162 | and at low ahead, mine. |
| `VO_Bark_SquadEngageLowAhead_A_02_dry` | 1.902 | -19.00 | -2.8 | 50.4 | 100.0 | 188.0 | 170 | Bandit Low Ahead, Mine |
| `VO_Bark_SquadEngageLowAhead_B_01_dry` | 1.306 | -19.00 | -4.9 | 50.1 | 100.0 | 0.0 | 168 | on the one low ahead. |
| `VO_Bark_SquadEngageLowAhead_B_02_dry` | 1.412 | -19.00 | -6.2 | 55.5 | 100.0 | 0.0 | 208 | on the one low ahead. |
| `VO_Bark_SquadEngageLowLeft_A_01_dry` | 1.984 | -19.00 | -3.5 | 50.1 | 100.0 | 90.0 | 171 | Bandit low left, mine. |
| `VO_Bark_SquadEngageLowLeft_A_02_dry` | 1.926 | -19.00 | -2.7 | 50.0 | 100.0 | 146.0 | 171 | Bandit, low K left, mine. |
| `VO_Bark_SquadEngageLowLeft_B_01_dry` | 1.436 | -19.00 | -6.2 | 51.9 | 100.0 | 120.0 | 191 | on the one low left. |
| `VO_Bark_SquadEngageLowLeft_B_02_dry` | 1.266 | -19.00 | -4.8 | 50.1 | 100.0 | 164.0 | 174 | on the one low left. |
| `VO_Bark_SquadEngageLowRight_A_01_dry` | 2.130 | -19.00 | -2.9 | 52.6 | 100.1 | 186.0 | 168 | low right, mine. |
| `VO_Bark_SquadEngageLowRight_A_02_dry` | 2.064 | -19.00 | -1.7 | 50.0 | 100.0 | 188.0 | 169 | Bandit, low right, mine. |
| `VO_Bark_SquadEngageLowRight_B_01_dry` | 1.194 | -19.00 | -5.7 | 50.5 | 100.5 | 0.0 | 178 | on the one low right. |
| `VO_Bark_SquadEngageLowRight_B_02_dry` | 1.202 | -19.00 | -6.8 | 51.9 | 100.0 | 0.0 | 245 | on the one low right. |
| `VO_Bark_SquadEngageLowBehind_A_01_dry` | 2.196 | -19.00 | -1.8 | 50.1 | 100.0 | 266.0 | 168 | Bandit |
| `VO_Bark_SquadEngageLowBehind_A_02_dry` | 2.272 | -19.00 | -2.2 | 50.0 | 100.0 | 300.0 | 171 | Bandit low behind, mine. |
| `VO_Bark_SquadEngageLowBehind_B_01_dry` | 1.418 | -19.00 | -4.3 | 50.3 | 100.0 | 0.0 | 166 | on the one low behind. |
| `VO_Bark_SquadEngageLowBehind_B_02_dry` | 1.530 | -19.00 | -5.2 | 50.0 | 100.4 | 0.0 | 183 | on the one low behind. |
| `VO_Bark_SquadWingmanKill_A_01_dry` | 1.006 | -19.00 | -2.1 | 50.1 | 100.0 | 0.0 | 168 | Splash One. |
| `VO_Bark_SquadWingmanKill_A_02_dry` | 1.066 | -19.01 | -1.5 | 50.1 | 100.0 | 0.0 | 176 | SplashOne. |
| `VO_Bark_SquadWingmanKill_B_01_dry` | 0.904 | -19.00 | -2.3 | 50.6 | 100.0 | 0.0 | 168 | and he's gone. |
| `VO_Bark_SquadWingmanKill_B_02_dry` | 0.936 | -19.00 | -4.2 | 50.0 | 100.3 | 0.0 | 188 | and he's gone. |
| `VO_Bark_PlayerKill_A_01_dry` | 0.802 | -19.00 | -5.2 | 51.5 | 100.0 | 0.0 | 211 | Nice shot. |
| `VO_Bark_PlayerKill_A_02_dry` | 0.824 | -19.00 | -4.1 | 51.1 | 100.0 | 0.0 | 186 |  |
| `VO_Bark_PlayerKill_B_01_dry` | 1.082 | -19.00 | -4.5 | 50.0 | 100.0 | 0.0 | 211 | Leave some for me. you |
| `VO_Bark_PlayerKill_B_02_dry` | 1.072 | -19.00 | -4.0 | 50.1 | 100.0 | 0.0 | 186 | Leave some for me. |
| `VO_Bark_MultiKill_01_dry` | 1.374 | -19.00 | -2.4 | 50.0 | 100.0 | 0.0 | 164 | Now that's flying! |
| `VO_Bark_MultiKill_02_dry` | 1.296 | -19.00 | -2.3 | 50.8 | 100.1 | 0.0 | 162 | Now that's flying. |
| `VO_Bark_SquadLowHull_A_01_dry` | 1.406 | -19.00 | -5.1 | 50.8 | 100.0 | 0.0 | 197 | Taking a beating here. |
| `VO_Bark_SquadLowHull_A_02_dry` | 1.338 | -19.00 | -4.2 | 50.5 | 100.0 | 0.0 | 185 | Taking a beating here. |
| `VO_Bark_SquadLowHull_B_01_dry` | 1.846 | -19.00 | -4.0 | 50.0 | 102.4 | 148.0 | 187 | Hull's going need a second. |
| `VO_Bark_SquadLowHull_B_02_dry` | 1.880 | -19.00 | -2.7 | 50.0 | 100.1 | 236.0 | 174 | Hole's going. Need a second. |
| `VO_Bark_ShieldDown_01_dry` | 1.110 | -19.00 | -5.0 | 50.0 | 100.0 | 0.0 | 230 | Shield's gone. |
| `VO_Bark_ShieldDown_02_dry` | 1.024 | -19.00 | -4.7 | 50.0 | 100.0 | 0.0 | 218 | Shield's gone. |
| `VO_Bark_ShieldBack_01_dry` | 1.616 | -19.00 | -2.0 | 50.0 | 100.0 | 182.0 | 183 | Shield's back, let's go. |
| `VO_Bark_ShieldBack_02_dry` | 1.540 | -19.00 | -1.8 | 50.0 | 100.0 | 118.0 | 181 | Shields back. Let's go. |
| `VO_Bark_SquadShipDown_A_01_dry` | 1.162 | -19.00 | -3.4 | 50.0 | 100.0 | 0.0 | 198 | Kato's down. |
| `VO_Bark_SquadShipDown_A_02_dry` | 1.126 | -19.00 | -3.6 | 50.3 | 100.0 | 0.0 | 180 | Kato's down. |
| `VO_Bark_SquadShipDown_B_01_dry` | 1.584 | -19.00 | -3.0 | 50.0 | 100.0 | 142.0 | 181 | Moss is out, cover him. |
| `VO_Bark_SquadShipDown_B_02_dry` | 1.634 | -19.00 | -3.3 | 50.1 | 100.0 | 100.0 | 196 | Moss is out, cover him, him. |
| `VO_Bark_SquadRespawn_01_dry` | 1.626 | -19.00 | -2.0 | 50.1 | 100.0 | 418.0 | 172 | back in it. |
| `VO_Bark_SquadRespawn_02_dry` | 1.426 | -19.00 | -2.6 | 50.0 | 100.2 | 240.0 | 178 | back in it. Miss me? |
| `VO_Bark_SquadWipe_01_dry` | 1.208 | -19.00 | -6.6 | 50.0 | 100.0 | 0.0 | 168 | No, no, no. |
| `VO_Bark_SquadWipe_02_dry` | 1.310 | -19.00 | -6.3 | 50.0 | 100.0 | 0.0 | 171 | No, no, no. |
| `VO_Bark_MissileIncoming_01_dry` | 1.442 | -19.00 | -4.2 | 52.6 | 100.7 | 80.0 | 174 | missile on you a break. |
| `VO_Bark_MissileIncoming_02_dry` | 1.574 | -19.00 | -6.4 | 50.0 | 100.0 | 112.0 | 224 | Missile on you, break. |
| `VO_Bark_MissileEvaded_01_dry` | 0.884 | -19.00 | -2.2 | 50.1 | 100.0 | 0.0 | 226 | too slow. |
| `VO_Bark_MissileEvaded_02_dry` | 0.922 | -19.00 | -4.7 | 50.4 | 100.0 | 0.0 | 172 | too slow. |
| `VO_Bark_OnMySix_01_dry` | 1.238 | -19.00 | -4.2 | 50.0 | 100.6 | 0.0 | 256 | One on your tail. |
| `VO_Bark_OnMySix_02_dry` | 1.094 | -19.00 | -5.5 | 50.0 | 100.0 | 0.0 | 257 | one on your tail. |
| `VO_Bark_Anticipated_01_dry` | 2.058 | -19.00 | -3.7 | 50.0 | 100.0 | 164.0 | 165 | It moved before I did. How? |
| `VO_Bark_Anticipated_02_dry` | 2.078 | -19.00 | -2.1 | 50.0 | 100.0 | 0.0 | 165 | it moved before I did how. a |
| `VO_Bark_TargetShielded_01_dry` | 1.708 | -19.00 | -3.3 | 50.1 | 100.0 | 226.0 | 171 | It's shielded. Hit it harder. |
| `VO_Bark_TargetShielded_02_dry` | 1.788 | -19.00 | -1.9 | 50.0 | 100.1 | 280.0 | 169 | It's shielded. Hit it harder. |
| `VO_Bark_MinesAhead_01_dry` | 1.210 | -19.00 | -6.6 | 50.0 | 102.8 | 98.0 | 225 | mines. Minds pull up. |
| `VO_Bark_MinesAhead_02_dry` | 1.432 | -19.00 | -4.3 | 51.1 | 100.9 | 164.0 | 211 | Minds pull up. |
| `VO_Bark_CarrierLaunch_01_dry` | 1.398 | -19.00 | -5.3 | 50.1 | 100.0 | 0.0 | 198 | More fighters coming out. |
| `VO_Bark_CarrierLaunch_02_dry` | 1.346 | -19.00 | -4.8 | 51.5 | 100.0 | 0.0 | 215 | More fighters coming out. |
| `VO_Bark_AmmoLow_A_01_dry` | 1.486 | -19.00 | -4.0 | 51.1 | 100.1 | 0.0 | 169 | Running Light on Missiles |
| `VO_Bark_AmmoLow_A_02_dry` | 1.602 | -19.00 | -2.5 | 50.4 | 100.0 | 0.0 | 168 | running light on missiles. |
| `VO_Bark_AmmoOut_B_01_dry` | 0.984 | -19.00 | -5.0 | 50.4 | 100.0 | 0.0 | 206 | I'm dry. |
| `VO_Bark_AmmoOut_B_02_dry` | 0.896 | -19.00 | -4.9 | 55.1 | 100.0 | 0.0 | 188 | I'm dry. |
| `VO_Bark_WeaponHot_01_dry` | 1.034 | -19.00 | -3.5 | 50.5 | 100.0 | 0.0 | 214 | like a king. |
| `VO_Bark_WeaponHot_02_dry` | 1.136 | -19.00 | -3.7 | 50.2 | 100.0 | 0.0 | 218 | Guns are cooking. |
| `VO_Bark_BoostEmpty_01_dry` | 1.036 | -19.00 | -5.1 | 51.9 | 100.0 | 0.0 | 219 | Out of burn. |
| `VO_Bark_BoostEmpty_02_dry` | 0.950 | -19.06 | -1.5 | 50.0 | 100.0 | 82.0 | 162 | out of burn. |
| `VO_Bark_Boundary_01_dry` | 2.296 | -19.26 | -1.5 | 50.0 | 100.0 | 432.0 | 166 | wide of the zone, bring it round. |
| `VO_Bark_Boundary_02_dry` | 2.216 | -19.00 | -1.8 | 50.0 | 100.0 | 370.0 | 183 |  |
| `VO_Bark_WaveCleared_01_dry` | 1.568 | -20.13 | -1.5 | 51.2 | 100.0 | 138.0 | 160 | That's all of them for now. |
| `VO_Bark_WaveCleared_02_dry` | 1.530 | -19.74 | -1.5 | 50.0 | 100.0 | 134.0 | 167 | That's all of them, for now. |
| `VO_Bark_WaveIncoming_01_dry` | 1.212 | -19.00 | -5.6 | 50.0 | 100.0 | 0.0 | 195 |  |
| `VO_Bark_WaveIncoming_02_dry` | 1.242 | -19.00 | -6.6 | 50.1 | 100.0 | 0.0 | 188 | Here they come again. |
| `VO_Bark_Breather_01_dry` | 1.488 | -19.00 | -4.5 | 50.2 | 100.0 | 0.0 | 236 | Anyone else enjoying this? |
| `VO_Bark_Breather_02_dry` | 1.430 | -19.00 | -4.6 | 55.8 | 100.0 | 0.0 | 194 | Anyone else enjoying this? |
| `VO_Bark_SquadSwitchIn_A_01_dry` | 1.132 | -19.00 | -1.8 | 50.0 | 100.0 | 0.0 | 166 | Rift's yours. |
| `VO_Bark_SquadSwitchIn_A_02_dry` | 1.082 | -19.00 | -2.3 | 50.4 | 100.0 | 0.0 | 85 | Riff's Yours. |
| `VO_Bark_SquadSwitchIn_B_01_dry` | 0.826 | -19.00 | -2.0 | 50.0 | 100.0 | 0.0 | 173 | My turn. |
| `VO_Bark_SquadSwitchIn_B_02_dry` | 0.784 | -19.00 | -2.0 | 52.3 | 100.0 | 0.0 | 168 | My turn. |
| `VO_Bark_SquadSwitchStrike_01_dry` | 0.982 | -19.07 | -1.5 | 50.0 | 100.4 | 0.0 | 217 | Strikes Hot |
| `VO_Bark_SquadSwitchStrike_02_dry` | 0.982 | -19.00 | -1.7 | 50.0 | 100.1 | 0.0 | 176 | strikes hot. |
| `VO_Bark_SquadFocusAck_01_dry` | 0.490 | -19.00 | -4.4 | 50.0 | 100.6 | 0.0 | 171 | on it. |
| `VO_Bark_SquadFocusAck_02_dry` | 0.532 | -19.00 | -5.8 | 50.0 | 100.0 | 0.0 | 232 | on it. |
| `VO_Bark_PowerReady_01_dry` | 0.930 | -19.00 | -2.4 | 50.0 | 100.0 | 110.0 | 190 | Portals are up. . |
| `VO_Bark_PowerReady_02_dry` | 0.914 | -19.00 | -3.3 | 51.2 | 100.0 | 130.0 | 200 | Portals are up. |
| `VO_Bark_PowerNotReady_01_dry` | 0.662 | -19.00 | -5.1 | 50.0 | 100.0 | 0.0 | 198 | Not yet. |
| `VO_Bark_PowerNotReady_02_dry` | 0.660 | -19.00 | -5.6 | 50.0 | 101.1 | 0.0 | 221 | Not yet Thanks for watching! |
| `VO_Bark_Portal_Open_A_01_dry` | 0.956 | -19.00 | -4.5 | 50.0 | 100.0 | 0.0 | 184 | doors open. |
| `VO_Bark_Portal_Open_A_02_dry` | 0.936 | -19.00 | -3.1 | 50.0 | 100.0 | 0.0 | 171 | and that that that that that that that that |
| `VO_Bark_Portal_Through_B_01_dry` | 2.516 | -19.00 | -4.0 | 50.0 | 100.0 | 934.0 | 173 | and through. |
| `VO_Bark_Portal_Through_B_02_dry` | 2.088 | -19.00 | -3.6 | 50.9 | 100.0 | 730.0 | 173 | and through. |
| `VO_Bark_Portal_MissileReturn_C_01_dry` | 1.204 | -19.00 | -4.6 | 51.6 | 100.0 | 0.0 | 188 | sender. |
| `VO_Bark_Portal_MissileReturn_C_02_dry` | 1.176 | -19.00 | -5.0 | 52.0 | 100.0 | 0.0 | 242 | Return to sender. |
| `VO_Bark_AbilityUnlocked_Cloak_01_dry` | 2.488 | -19.37 | -1.5 | 51.2 | 100.1 | 202.0 | 168 | on now you don't see me |
| `VO_Bark_AbilityUnlocked_Cloak_02_dry` | 2.458 | -19.58 | -1.5 | 50.1 | 100.0 | 216.0 | 176 | Cloaks online, now you don't see me. |
| `VO_Bark_Cloak_On_A_01_dry` | 0.878 | -19.00 | -3.4 | 50.5 | 100.0 | 116.0 | 187 | going dark. |
| `VO_Bark_Cloak_On_A_02_dry` | 0.890 | -19.00 | -1.5 | 50.1 | 100.0 | 120.0 | 166 | going dark.- |
| `VO_Bark_Cloak_Ambush_B_01_dry` | 0.892 | -19.00 | -1.6 | 50.0 | 100.0 | 0.0 | 172 | Surprise! |
| `VO_Bark_Cloak_Ambush_B_02_dry` | 0.866 | -21.03 | -1.5 | 50.0 | 100.0 | 0.0 | 82 | surprise. |
| `VO_Bark_AbilityLocked_01_dry` | 1.156 | -19.00 | -1.6 | 50.0 | 100.0 | 0.0 | 172 |  |
| `VO_Bark_AbilityLocked_02_dry` | 1.150 | -19.00 | -2.3 | 50.0 | 100.0 | 80.0 | 184 | haven't got that yet. |
| `VO_Bark_Hyper_Spool_A_01_dry` | 0.960 | -19.00 | -5.2 | 50.0 | 100.0 | 92.0 | 222 | Spooling up. |
| `VO_Bark_Hyper_Spool_A_02_dry` | 0.972 | -19.00 | -4.4 | 50.7 | 100.0 | 120.0 | 186 | Sp,ooling up. |
| `VO_Bark_Hyper_Blocked_B_01_dry` | 1.630 | -19.00 | -4.2 | 50.0 | 100.0 | 164.0 | 214 | Can't jump. Too hot here. |
| `VO_Bark_Hyper_Blocked_B_02_dry` | 1.722 | -19.00 | -4.0 | 50.3 | 100.0 | 94.0 | 203 | Can't jump, too hot here. |
| `VO_Bark_Gate_Enter_01_dry` | 0.718 | -19.00 | -2.7 | 50.0 | 100.8 | 0.0 | 249 | Shortcut |
| `VO_Bark_Gate_Enter_02_dry` | 0.742 | -19.00 | -2.5 | 50.0 | 100.0 | 80.0 | 253 | Shortcut |
| `VO_Bark_Atmo_Entry_01_dry` | 0.968 | -19.00 | -5.7 | 50.0 | 100.1 | 0.0 | 201 | , going in hot. |
| `VO_Bark_Atmo_Entry_02_dry` | 0.956 | -19.00 | -4.3 | 53.2 | 100.0 | 0.0 | 201 | hot. |
| `VO_Bark_Land_Ready_A_01_dry` | 0.912 | -19.87 | -1.5 | 50.3 | 100.0 | 0.0 | 170 | There's the pad. |
| `VO_Bark_Land_Ready_A_02_dry` | 0.928 | -19.00 | -2.0 | 55.2 | 100.0 | 0.0 | 173 | There's the pad. |
| `VO_Bark_Land_Down_B_01_dry` | 1.798 | -19.54 | -1.5 | 50.0 | 100.0 | 220.0 | 177 | Down. Let's look around. |
| `VO_Bark_Land_Down_B_02_dry` | 1.618 | -19.00 | -3.5 | 50.2 | 100.0 | 252.0 | 181 | Down, let's look around. |
| `VO_Bark_TakeOff_01_dry` | 0.824 | -19.00 | -3.6 | 50.4 | 100.0 | 0.0 | 182 | See you next time. |
| `VO_Bark_TakeOff_02_dry` | 0.842 | -19.00 | -1.8 | 50.0 | 100.0 | 0.0 | 168 | Thank you for watching. |
| `VO_Bark_Planet_Arrive_Verdance_A_01_dry` | 2.504 | -19.00 | -1.8 | 50.3 | 100.0 | 524.0 | 175 | It glows. Everything glows. |
| `VO_Bark_Planet_Arrive_Verdance_A_02_dry` | 2.154 | -20.37 | -1.5 | 50.0 | 100.0 | 328.0 | 175 | It glows. Everything glows. |
| `VO_Bark_Planet_Arrive_Cinder_B_01_dry` | 0.998 | -19.00 | -3.4 | 50.0 | 100.1 | 0.0 | 176 | hot enough for you? |
| `VO_Bark_Planet_Arrive_Cinder_B_02_dry` | 0.992 | -19.00 | -4.1 | 50.0 | 100.2 | 0.0 | 203 | hot enough for you? |
| `VO_Bark_Dom_GeneratorHit_A_01_dry` | 1.112 | -19.00 | -3.5 | 50.3 | 100.0 | 0.0 | 227 | It's cracking. |
| `VO_Bark_Dom_GeneratorHit_A_02_dry` | 1.002 | -19.00 | -2.8 | 50.4 | 100.0 | 0.0 | 196 | It's cracking. |
| `VO_Bark_Dom_GeneratorDown_B_01_dry` | 1.302 | -19.00 | -4.1 | 50.0 | 100.0 | 0.0 | 170 | Generator's gone. |
| `VO_Bark_Dom_GeneratorDown_B_02_dry` | 1.316 | -19.00 | -5.4 | 53.6 | 100.4 | 0.0 | 247 | Generators gone. |
| `VO_Bark_Dom_Turrets_01_dry` | 1.806 | -19.19 | -1.5 | 50.0 | 100.0 | 88.0 | 163 | Turrets watch the turrets |
| `VO_Bark_Dom_Turrets_02_dry` | 1.814 | -19.52 | -1.5 | 50.9 | 100.0 | 108.0 | 180 | Turrets. Watch the turrets. |
| `VO_Bark_Dom_Tractor_01_dry` | 1.464 | -19.00 | -3.1 | 50.0 | 100.0 | 0.0 | 176 | Something's pulling us in. |
| `VO_Bark_Dom_Tractor_02_dry` | 1.526 | -19.00 | -3.5 | 50.0 | 100.0 | 0.0 | 185 | Something's pulling us in. |
| `VO_Bark_Dom_EscapeFlight_01_dry` | 0.578 | -19.00 | -4.0 | 50.0 | 100.0 | 0.0 | 242 | Punch it! |
| `VO_Bark_Dom_EscapeFlight_02_dry` | 0.588 | -19.00 | -6.2 | 51.2 | 100.8 | 0.0 | 225 | Punch it |
| `VO_Bark_Dom_Clear_01_dry` | 1.398 | -19.00 | -5.1 | 50.0 | 100.0 | 110.0 | 251 | We're clear, we're clear. |
| `VO_Bark_Dom_Clear_02_dry` | 1.396 | -19.00 | -3.8 | 51.0 | 100.0 | 110.0 | 265 | We're clear, we're clear. |
| `VO_Bark_Foot_Contact_01_dry` | 1.106 | -19.00 | -3.4 | 50.0 | 100.0 | 0.0 | 184 | you've got company. |
| `VO_Bark_Foot_Contact_02_dry` | 1.118 | -19.00 | -4.3 | 50.0 | 100.0 | 0.0 | 220 | You've got company. |
| `VO_Bark_Foot_Deflect_A_01_dry` | 0.952 | -19.21 | -1.5 | 50.1 | 100.0 | 0.0 | 176 | Back at you. |
| `VO_Bark_Foot_Deflect_A_02_dry` | 0.838 | -19.00 | -3.0 | 50.1 | 100.0 | 0.0 | 174 | back at you. |
| `VO_Bark_Foot_Perfect_B_01_dry` | 1.432 | -19.00 | -2.2 | 50.4 | 100.0 | 0.0 | 214 | right between the eyes. |
| `VO_Bark_Foot_Perfect_B_02_dry` | 1.514 | -19.00 | -2.6 | 50.0 | 100.0 | 98.0 | 171 | right between the eyes. |
| `VO_Bark_Foot_GuardBroken_01_dry` | 0.972 | -19.00 | -3.0 | 50.3 | 100.0 | 0.0 | 174 | Guard's gone. |
| `VO_Bark_Foot_GuardBroken_02_dry` | 0.938 | -19.56 | -1.5 | 50.1 | 100.0 | 0.0 | 166 | Guards Gone. |
| `VO_Bark_Foot_Hurt_A_01_dry` | 0.578 | -19.00 | -4.7 | 52.2 | 100.0 | 0.0 | 199 | Ow! |
| `VO_Bark_Foot_Hurt_A_02_dry` | 0.664 | -19.00 | -5.5 | 50.0 | 100.0 | 0.0 | 236 | Ow. |
| `VO_Bark_Foot_LowHealth_B_01_dry` | 1.246 | -19.00 | -3.7 | 51.0 | 100.0 | 210.0 | 168 | I'm hurt, I'm hurt. |
| `VO_Bark_Foot_LowHealth_B_02_dry` | 1.298 | -19.00 | -2.7 | 50.0 | 100.2 | 246.0 | 170 | I'm hurt, I'm hurt. |
| `VO_Bark_Foot_Heal_01_dry` | 0.550 | -19.54 | -1.5 | 50.0 | 100.0 | 0.0 | 158 | better. |
| `VO_Bark_Foot_Heal_02_dry` | 0.530 | -19.00 | -1.6 | 50.0 | 100.0 | 0.0 | 163 | better |
| `VO_Bark_Foot_Kill_A_01_dry` | 0.788 | -19.00 | -5.4 | 50.0 | 100.0 | 0.0 | 265 | next |
| `VO_Bark_Foot_Kill_A_02_dry` | 0.676 | -19.00 | -4.1 | 50.2 | 100.0 | 88.0 | 211 | next. |
| `VO_Bark_Foot_LastOne_B_01_dry` | 0.816 | -19.00 | -2.0 | 50.0 | 100.0 | 0.0 | 181 | That's the lot. |
| `VO_Bark_Foot_LastOne_B_02_dry` | 0.812 | -19.14 | -1.5 | 51.0 | 100.0 | 0.0 | 164 | That's the lot. |
| `VO_Bark_Foot_Dash_A_01_dry` | 0.728 | -19.00 | -8.9 | 50.6 | 100.0 | 0.0 | 292 | O, |
| `VO_Bark_Foot_Dash_A_02_dry` | 0.644 | -19.00 | -8.6 | 50.3 | 100.0 | 0.0 | 304 | Whoo |
| `VO_Bark_Foot_Slam_B_01_dry` | 0.634 | -19.00 | -4.9 | 50.0 | 101.0 | 0.0 | 168 | BOOM |
| `VO_Bark_Foot_Slam_B_02_dry` | 0.634 | -19.00 | -4.1 | 51.9 | 100.0 | 0.0 | 160 | BOOM |
| `VO_Bark_Foot_ObjectiveNear_A_01_dry` | 1.338 | -19.00 | -2.9 | 50.0 | 100.2 | 196.0 | 183 | That's it, right ahead. |
| `VO_Bark_Foot_ObjectiveNear_A_02_dry` | 1.456 | -19.00 | -3.2 | 50.0 | 100.0 | 250.0 | 216 | That's it, right ahead. |
| `VO_Bark_Foot_ObjectiveDone_B_01_dry` | 1.510 | -19.01 | -1.5 | 50.0 | 100.0 | 356.0 | 168 | Heading back. |
| `VO_Bark_Foot_ObjectiveDone_B_02_dry` | 1.430 | -19.84 | -1.5 | 50.0 | 100.0 | 288.0 | 169 | Got it. Heading back. |
| `VO_Bark_Foot_DoorLocked_A_01_dry` | 1.510 | -19.23 | -1.5 | 50.0 | 100.0 | 346.0 | 165 | Locked figures. |
| `VO_Bark_Foot_DoorLocked_A_02_dry` | 1.526 | -20.04 | -1.5 | 50.2 | 100.0 | 330.0 | 157 | Locked figures. |
| `VO_Bark_Foot_DoorOpen_B_01_dry` | 0.762 | -19.00 | -5.8 | 50.5 | 100.0 | 0.0 | 171 | We're in! |
| `VO_Bark_Foot_DoorOpen_B_02_dry` | 0.694 | -19.00 | -2.8 | 50.3 | 100.4 | 0.0 | 167 | We're in. |
| `VO_Bark_Foot_Hazard_01_dry` | 0.886 | -19.00 | -4.0 | 50.0 | 100.0 | 0.0 | 189 | Get out of there. |
| `VO_Bark_Foot_Hazard_02_dry` | 1.166 | -19.00 | -5.8 | 50.7 | 100.0 | 0.0 | 241 | Get out of there! |
| `VO_Bark_WardenPhase2_01_dry` | 1.024 | -19.00 | -3.0 | 50.0 | 100.0 | 0.0 | 183 | It's angry now. |
| `VO_Bark_WardenPhase2_02_dry` | 1.140 | -19.00 | -5.0 | 50.0 | 100.0 | 0.0 | 232 | It's angry now. |
| `VO_Bark_Foot_Idle_01_dry` | 1.360 | -19.00 | -4.5 | 53.3 | 100.0 | 0.0 | 203 | you sightseeing down there? |
| `VO_Bark_Foot_Idle_02_dry` | 1.444 | -19.00 | -5.0 | 50.0 | 100.1 | 0.0 | 192 | You sightseeing down, down there? |
| `VO_Bark_Dom_Boarded_01_dry` | 1.158 | -19.00 | -3.3 | 50.2 | 100.0 | 106.0 | 193 | Bring something back. |
| `VO_Bark_Dom_Boarded_02_dry` | 1.206 | -19.00 | -1.9 | 50.3 | 100.0 | 118.0 | 169 | bring something back. |
| `VO_Bark_Dom_CoreShield_A_01_dry` | 1.126 | -19.00 | -3.8 | 50.0 | 100.0 | 0.0 | 208 | Shields Dropping |
| `VO_Bark_Dom_CoreShield_A_02_dry` | 1.130 | -19.00 | -4.5 | 50.1 | 100.0 | 0.0 | 187 | shields dropping. |
| `VO_Bark_Dom_CoreOpen_B_01_dry` | 0.872 | -19.00 | -4.7 | 50.8 | 100.0 | 0.0 | 207 | There's the heart. |
| `VO_Bark_Dom_CoreOpen_B_02_dry` | 0.764 | -19.00 | -4.1 | 50.0 | 100.0 | 0.0 | 226 | There's the heart. |
| `VO_Bark_Dom_Escape_Start_A_01_dry` | 1.554 | -19.00 | -4.4 | 52.7 | 100.1 | 172.0 | 245 | Run. Run now. |
| `VO_Bark_Dom_Escape_Start_A_02_dry` | 1.722 | -19.00 | -5.7 | 50.0 | 100.0 | 286.0 | 271 | Run! Run now! |
| `VO_Bark_Dom_Escape_30_B_01_dry` | 1.212 | -19.00 | -3.3 | 50.0 | 100.0 | 0.0 | 222 | 30 seconds. |
| `VO_Bark_Dom_Escape_30_B_02_dry` | 1.106 | -19.00 | -3.5 | 50.0 | 100.0 | 0.0 | 235 | 30 seconds. |
| `VO_Bark_Dom_Escape_10_C_01_dry` | 0.650 | -19.00 | -6.8 | 50.8 | 104.4 | 0.0 | 204 | Move it. |
| `VO_Bark_Dom_Escape_10_C_02_dry` | 0.614 | -19.00 | -6.7 | 50.1 | 100.4 | 0.0 | 226 | move it |
| `VO_Bark_Banter_BetYouTen_01_dry` | 2.272 | -19.00 | -1.8 | 50.0 | 100.0 | 0.0 | 163 | Bet you ten I get more than you this round. |
| `VO_Bark_Banter_BetYouTen_02_dry` | 2.228 | -19.00 | -3.2 | 50.0 | 100.0 | 0.0 | 165 | Bet you ten I get more than you this round. |
| `VO_Bark_Banter_WereInSpace_01_dry` | 1.404 | -19.00 | -5.0 | 50.5 | 100.0 | 0.0 | 183 | We're in space, Moss. |
| `VO_Bark_Banter_WereInSpace_02_dry` | 1.436 | -19.00 | -4.1 | 50.3 | 100.0 | 0.0 | 181 | We're in space, Moss. |
| `VO_Bark_Banter_TidyRaiders_01_dry` | 1.664 | -19.00 | -3.0 | 50.2 | 100.0 | 0.0 | 167 | so they're tidy raiders. |
| `VO_Bark_Banter_TidyRaiders_02_dry` | 1.724 | -19.00 | -2.3 | 50.0 | 100.0 | 0.0 | 166 | so they're tidy raiders. |
| `VO_Bark_Banter_SaySomethingNice_01_dry` | 2.394 | -19.00 | -2.5 | 50.0 | 100.3 | 0.0 | 181 | Kato, say something nice about my flying. |
| `VO_Bark_Banter_SaySomethingNice_02_dry` | 2.504 | -19.00 | -2.2 | 50.4 | 100.1 | 0.0 | 182 | Kato, say something nice about my flying. |
| `VO_Bark_Banter_IllTakeIt_01_dry` | 0.706 | -19.00 | -2.8 | 50.9 | 100.0 | 0.0 | 164 | I'll take it. |
| `VO_Bark_Banter_IllTakeIt_02_dry` | 0.710 | -20.59 | -1.5 | 50.3 | 100.0 | 0.0 | 167 |  |
| `VO_Bark_Banter_PleaseNotTheStew_01_dry` | 1.276 | -19.00 | -3.9 | 50.8 | 100.3 | 0.0 | 204 | Please not the stew. |
| `VO_Bark_Banter_PleaseNotTheStew_02_dry` | 1.298 | -19.00 | -4.0 | 50.9 | 100.0 | 0.0 | 189 | Please not the stew. |

## SHA256 of delivered takes

| File | SHA256 |
|---|---|
| `VO_Bark_AbilityLocked_01_dry.wav` | `e8233ba957cb230c9c1b942b46f2c7d44104a0b5018fb0f46c78ab75b7652165` |
| `VO_Bark_AbilityLocked_01_radio.wav` | `e7a3b7ad6665888c8f737cb2f2f5ab5e03136d76d395ef22aab8ec168530ddc6` |
| `VO_Bark_AbilityLocked_02_dry.wav` | `a83ad2e26daac6e1049d0236ad43a5995d3d2771ae1e5e1bbe294920618c704f` |
| `VO_Bark_AbilityLocked_02_radio.wav` | `31bd09ecec3c33a1a7ea7a9560626a30a5de81bb16a7b51b01439e78148b09bd` |
| `VO_Bark_AbilityUnlocked_Cloak_01_dry.wav` | `bf69e160809fc372dc3292ca73ad6d4bfd39b7ec9184f1a82f468d0a54a1a01d` |
| `VO_Bark_AbilityUnlocked_Cloak_01_radio.wav` | `67fee5b5b44e3dd051448b66024550d5eaefa313bdb292aefd4c11e125e99d46` |
| `VO_Bark_AbilityUnlocked_Cloak_02_dry.wav` | `4b93a01d0b831283703a952de6231f65719d847a402660ce70aceaf16b8dedf4` |
| `VO_Bark_AbilityUnlocked_Cloak_02_radio.wav` | `ae65c76a716527f4c8acaabd737d64748f18d7f4a770ad6b6bff68d365746613` |
| `VO_Bark_AmmoLow_A_01_dry.wav` | `813ba82cfa33dca59d1d30e1df9ac874b054137f5831bcbe638fde3bf47cd511` |
| `VO_Bark_AmmoLow_A_01_radio.wav` | `86cf07b733f8f33eca1d7bfe7eb4c618d79412307b394bdc08f031ae9513ce22` |
| `VO_Bark_AmmoLow_A_02_dry.wav` | `dbba8f545a6e9b359cd86c1037cd3b6d188a4fe522dc99c94074f46889b1b51d` |
| `VO_Bark_AmmoLow_A_02_radio.wav` | `0573fb1f7e19f066bf20e1df1bfef5a789d6857f1281b56f5e89fd528c449c6e` |
| `VO_Bark_AmmoOut_B_01_dry.wav` | `5b83ce7057caa72ca99bda5f3d53cb1fc51c31e8495ef584fdd06517ebaeea18` |
| `VO_Bark_AmmoOut_B_01_radio.wav` | `278344e572242fc46ede16b1e57a59f931e61b7d214a37290d00158cf36b15fa` |
| `VO_Bark_AmmoOut_B_02_dry.wav` | `e9a54e44731ec5a21bc1a0acc76b49c7441dcbb549e388d625a608a0f350f57b` |
| `VO_Bark_AmmoOut_B_02_radio.wav` | `dac22944b4bade1ada15245a96c2f28d8de7eb01769d8b4a72525581ef2cd77a` |
| `VO_Bark_Anticipated_01_dry.wav` | `49baf3758ef493b8784e7e07aca5f9c2e2e4d93d8935cdc7a24e460b197a7222` |
| `VO_Bark_Anticipated_01_radio.wav` | `04fb14c61d64a2f8297f562800ec7c8463621cb33c6ec70cdc4ee3f473a7b688` |
| `VO_Bark_Anticipated_02_dry.wav` | `b5a2d7acc589468cc1219092bf4736448653d1a3dbaaf28dd78998abe3e4ef82` |
| `VO_Bark_Anticipated_02_radio.wav` | `75ebd2b189cf65f299fa3be2544da1353e9c34143d6623466a0c1542cb72c2dd` |
| `VO_Bark_Atmo_Entry_01_dry.wav` | `3b765897643ed9c7f6969c14f18274355a007049048f35e3e25f75ea8a5a89ab` |
| `VO_Bark_Atmo_Entry_01_radio.wav` | `343f3a04566101404e94f1440903fb830219d83a7a02fbb13e35aabd64c7542a` |
| `VO_Bark_Atmo_Entry_02_dry.wav` | `d9ad0a0e9897e8a8b821b274d4361b70711d1a58f03df141847d044f5c3dba2e` |
| `VO_Bark_Atmo_Entry_02_radio.wav` | `c42827d5f117cd4612dc0332ee3dfe5774dfc5a6468e8b90b2d52b40c62ae9bf` |
| `VO_Bark_Banter_BetYouTen_01_dry.wav` | `7fcb230a3b684e9df965044323c7640d90b51f8f11b335b1b615b1a0c831fd79` |
| `VO_Bark_Banter_BetYouTen_01_radio.wav` | `47d2132ed58fed623330457b876126d772249a2820330a1672be77fa52364cae` |
| `VO_Bark_Banter_BetYouTen_02_dry.wav` | `b80064aed2d4a0581764bf11bc4c87adba2ac3868526b6936a41b236d793a5de` |
| `VO_Bark_Banter_BetYouTen_02_radio.wav` | `9e2618a36db5da12fe4d78c315120c7f79687b308b29f281834e2b0afecad976` |
| `VO_Bark_Banter_IllTakeIt_01_dry.wav` | `2faba00e88aff334b78f068294c85976dff619d9d20f3216f3bf8c8de5976c87` |
| `VO_Bark_Banter_IllTakeIt_01_radio.wav` | `00587b3bddde0d84151e4d69dc6b6bad958d6ae095a977e7d49070e801881b25` |
| `VO_Bark_Banter_IllTakeIt_02_dry.wav` | `8d44c4c029698cab4dc5e67477014e1a587fb9571ca02f1ddfd4106ad83f7910` |
| `VO_Bark_Banter_IllTakeIt_02_radio.wav` | `fed0ce5cd10d92507f72ca9c05725046fe5c175eac97f40129efaad85f7c8fb1` |
| `VO_Bark_Banter_PleaseNotTheStew_01_dry.wav` | `ec3778f98823aad520a6f0dbb91b21ad6d4d5c171994bddf4c861c181a8bd51d` |
| `VO_Bark_Banter_PleaseNotTheStew_01_radio.wav` | `5eb73c0afb6e971eaab8c730ca23d7ec144e4eea2269c04412fca0710bad4d51` |
| `VO_Bark_Banter_PleaseNotTheStew_02_dry.wav` | `76888c56b8b692449cf935801d778cba227f28e3cc3215dee34b831bb908ef96` |
| `VO_Bark_Banter_PleaseNotTheStew_02_radio.wav` | `ef1253567a37fae2e983e5b4187a7a403f123bfe2f1e013f4f47d71c2ad88a32` |
| `VO_Bark_Banter_SaySomethingNice_01_dry.wav` | `d407c79033365c3744436b0f04d340e76e4efe7f5b5d40f20ee3592953028a9b` |
| `VO_Bark_Banter_SaySomethingNice_01_radio.wav` | `47fa40281bd74d17a032577f5394db8323e2fc91578237ee4c5ffb1c49ca3d1b` |
| `VO_Bark_Banter_SaySomethingNice_02_dry.wav` | `28a03e5dcdf5e78f296ee1d102b84d64259f0da08e038b178060aa8d69f92519` |
| `VO_Bark_Banter_SaySomethingNice_02_radio.wav` | `c4f79167318722e49188141ca9f162c16169a529da5de2c8d5591212779ff077` |
| `VO_Bark_Banter_TidyRaiders_01_dry.wav` | `e32bf5a425dd58a88b27b85640b8cd59c05b6b715ade84ee2fb9ce9411260382` |
| `VO_Bark_Banter_TidyRaiders_01_radio.wav` | `67bb569fa6b83cf23e74b469fe1496b714056fe5d04cbd27019b1051869e4cf2` |
| `VO_Bark_Banter_TidyRaiders_02_dry.wav` | `3845d2776c6bf25da5fb0782872da0b2c74b6421983ad9ab1196d1622507cc4a` |
| `VO_Bark_Banter_TidyRaiders_02_radio.wav` | `4d166eba41334e1162f17690221662ee3d31e88a5580264c5f74b43609153cee` |
| `VO_Bark_Banter_WereInSpace_01_dry.wav` | `b4d709fec84b230d8d08b4eabacae1df483f5d67bb1ab3d3c76d7ac3a3f0f188` |
| `VO_Bark_Banter_WereInSpace_01_radio.wav` | `acc5b36de50d5e0c495057bc03919de76ea7dc2db95c036f7c144af97f14cf58` |
| `VO_Bark_Banter_WereInSpace_02_dry.wav` | `a5e3bf85b275898784fb3b5650ac1357a16a004a47a23a2409b3b695d48ddd6f` |
| `VO_Bark_Banter_WereInSpace_02_radio.wav` | `ff3ec4fad940a8b03ece043dc8f7518583a4e890d2ba017a27bf89f4c631cd23` |
| `VO_Bark_BoostEmpty_01_dry.wav` | `3aba4bb27afc3a1be3860f23e6a66dd143df7686c055fcd3ee3409ee476f5442` |
| `VO_Bark_BoostEmpty_01_radio.wav` | `63b9887271d02e924278af043a31c14f58443e302c2f21f7b026a5a387c60c86` |
| `VO_Bark_BoostEmpty_02_dry.wav` | `19850d5fab2862d84f89286872311d374c4f2015bf69f6f6a98e609b63c32531` |
| `VO_Bark_BoostEmpty_02_radio.wav` | `6f68fabb081e88f017c1c14ac31923141fc684a493a4e1d86a8d8715ac428c21` |
| `VO_Bark_Boundary_01_dry.wav` | `f2e80469e69dd3401661118487feb6ac8115d65b09f015156ceb9f4e4e0cf514` |
| `VO_Bark_Boundary_01_radio.wav` | `8e8a05a681307c85c294d7df62c53f0f44ed37519c8b3fddfada780cc16ce227` |
| `VO_Bark_Boundary_02_dry.wav` | `cead5aceab32e1332bc5a31f5ad8b90d9f8e2178596e076e57e5578f90bc41e1` |
| `VO_Bark_Boundary_02_radio.wav` | `c77793b71bfee487726552e1d29f45abd6d26e0461a9ca9819bf19717b12a0a8` |
| `VO_Bark_Breather_01_dry.wav` | `4cabaa9424ded9a846a41288461eb4ad3ccbdbe354de1269a5e68460b205a1f2` |
| `VO_Bark_Breather_01_radio.wav` | `afba81423fb5c24f16c6494e19ddbbddbe6fd836565199a1e7e4ccaf4cac14b2` |
| `VO_Bark_Breather_02_dry.wav` | `9b8fec4007fc5b8bd2a8da48dc77eb12e91b7aba4eef2405f25170d5b7cebd4e` |
| `VO_Bark_Breather_02_radio.wav` | `d0ebe94b9a4fd3f34834f00e535e5a79045c38fddf4adae7c4aa11b288c41601` |
| `VO_Bark_CarrierLaunch_01_dry.wav` | `fd4c0ed5504809ce3da4588a22656f975f7e15d98458ec8e7b3fe9a76edcba18` |
| `VO_Bark_CarrierLaunch_01_radio.wav` | `f55b48a5420edf7344545e33816643b114e0c1fec072a2d3a9b119f95cea85b9` |
| `VO_Bark_CarrierLaunch_02_dry.wav` | `316570e6dd728e6cceafc27ad2c698441fe6d05a5ee4c19c09e4ead073e18393` |
| `VO_Bark_CarrierLaunch_02_radio.wav` | `73706fbe4dbe613c34d1acb2859a52c82939b14fb2cb560861ae70df3accf38d` |
| `VO_Bark_Cloak_Ambush_B_01_dry.wav` | `b9a7c05e70143c62113f2e5d7f24dcdc24ee4904caf1fbbfc096cb8a4498b514` |
| `VO_Bark_Cloak_Ambush_B_01_radio.wav` | `57857cb046a96a18c864a39d61cd4f4a82fcfb88206ec2752fa59c8b1e6861be` |
| `VO_Bark_Cloak_Ambush_B_02_dry.wav` | `cb7397ccdcce8ea971fa12725d6813a5d4eff7df64b67d7f038aae0692d39e67` |
| `VO_Bark_Cloak_Ambush_B_02_radio.wav` | `9f331fd09497686d5cd7b6167f151efdd67e60d714dc8c7212f1eea46b55f37b` |
| `VO_Bark_Cloak_On_A_01_dry.wav` | `907f5c11c9223aed2178ebed9b46995273373b79b99f533b7caee5a58affe172` |
| `VO_Bark_Cloak_On_A_01_radio.wav` | `fc484244c61e92956cf3330db1b923f78a6b1f8f5ff7af7ff1861de351b788e2` |
| `VO_Bark_Cloak_On_A_02_dry.wav` | `b38723a234bf914e90152ef30779ba01ec9a97bfab8abcf5e6cb73e288b10750` |
| `VO_Bark_Cloak_On_A_02_radio.wav` | `50a886364d1156d8b26a80b0fbd30ebeef79ee9d165e3c093cbd93400aab5d15` |
| `VO_Bark_Dom_Boarded_01_dry.wav` | `e3af8e50b295de350a8463f50bd1c5d0f02425b5fb2d705cbe369290ce06fa58` |
| `VO_Bark_Dom_Boarded_01_radio.wav` | `313f5f449d01975d6931e4129f12c46b37ca861c227e9bbc80cc0f5ed3c1109d` |
| `VO_Bark_Dom_Boarded_02_dry.wav` | `cde58f87933f1ecd064fd5f51753ca79fe566715aa2216ed1b879bf91cde3e87` |
| `VO_Bark_Dom_Boarded_02_radio.wav` | `160704da2a67a2730cdea2530ca549090daa04fd0f2d5264c6665e0adf00bc92` |
| `VO_Bark_Dom_Clear_01_dry.wav` | `47e89292831eb4078e2f774b2c608a2c545527dc3994f342d6248b5533eca3be` |
| `VO_Bark_Dom_Clear_01_radio.wav` | `f9aa3fde6879380658615e1a3599320a373eb6f466992c0c1fce92847c783a56` |
| `VO_Bark_Dom_Clear_02_dry.wav` | `34003f4a1e126f742a8f0d6fd4414702dd6da221634091d07b149a0e9cb95978` |
| `VO_Bark_Dom_Clear_02_radio.wav` | `8ab2eeb79dce1348c6edb847a4fa5eb7a6397a25e7ecccb2dd818c011d69eebc` |
| `VO_Bark_Dom_CoreOpen_B_01_dry.wav` | `7e0e9fe6ea4f01beafa3d7f3078be86a5677585506472f8cf4d0f472c7762718` |
| `VO_Bark_Dom_CoreOpen_B_01_radio.wav` | `8a32878e7c7a332c5b1d7d0aa3cf6f5582cae1a23460dcea47925aa8ab3804d3` |
| `VO_Bark_Dom_CoreOpen_B_02_dry.wav` | `6b4158ea75dc0fbbbe8b12343711a5c0b338c9713ea4437a0947b7effbe911b7` |
| `VO_Bark_Dom_CoreOpen_B_02_radio.wav` | `fd1b6bcb7c6821bc2822613e36c92dbcc11cd7fd974b74acca06c4deb86f39d8` |
| `VO_Bark_Dom_CoreShield_A_01_dry.wav` | `ae5ed5b43a41b87489e86598a2e7948e85fb2d66e37a51610832163ccbca3f2b` |
| `VO_Bark_Dom_CoreShield_A_01_radio.wav` | `6ed5c6cd567e3c1bfea4d33b68c9bb0fb2869fb22fe50c9edc59d387f74be56f` |
| `VO_Bark_Dom_CoreShield_A_02_dry.wav` | `b36e42a6241267f1a0b1f3cc562348ff845f514eb15afd6cc4131da3fceeb88f` |
| `VO_Bark_Dom_CoreShield_A_02_radio.wav` | `6c5f5bbb5af4422b4766868f80f7ff55d80dee0422c30ded635030ee576ba1c4` |
| `VO_Bark_Dom_EscapeFlight_01_dry.wav` | `d197bb489f984b5b8b7969a0ff9826977d7661a6ce425909391f94c0fdd90a1e` |
| `VO_Bark_Dom_EscapeFlight_01_radio.wav` | `2fae6a8539fe132a3cd918368215dd43f74faf16e9e11a4328f47a68e98dc852` |
| `VO_Bark_Dom_EscapeFlight_02_dry.wav` | `34fbc8848ba89bc6d51aaf3fa1e381afe86ad00241395d9922972b881b0efb7a` |
| `VO_Bark_Dom_EscapeFlight_02_radio.wav` | `bccd2cb857f5368f05ccae861db9ec5f961030c76498a3e917e6e3c6d2cb0930` |
| `VO_Bark_Dom_Escape_10_C_01_dry.wav` | `cf97219047be5f0a959018ab49fbb31148a10f520dd4ace646722ac859ef0b7f` |
| `VO_Bark_Dom_Escape_10_C_01_radio.wav` | `9b1328dae5e45aab4ab4d066401b6f7ee659e13421097fa8b19c0f7aa1e87e73` |
| `VO_Bark_Dom_Escape_10_C_02_dry.wav` | `d83d503d6a712cc5b3dd9c72b0f72aed777241448b14fbcbe2e3e68e726ce3a2` |
| `VO_Bark_Dom_Escape_10_C_02_radio.wav` | `dfdf4f9fe486bfbf3aee512440242a11514759b229ec091a9070a986df8d5e39` |
| `VO_Bark_Dom_Escape_30_B_01_dry.wav` | `984b992aa6e80d8ad3df9ff700c5587fc5d7a4213abdf2e222360ce3e792b141` |
| `VO_Bark_Dom_Escape_30_B_01_radio.wav` | `84f99bb22bdf62bc728eaccdad6bf95cef2be13215371709c20838afa4690603` |
| `VO_Bark_Dom_Escape_30_B_02_dry.wav` | `c2dcbb2baa6177d1d182db359d8b7cc33ca0cd2ec70f96b896127443cd4c9772` |
| `VO_Bark_Dom_Escape_30_B_02_radio.wav` | `7d1a2b7e4a66162be7f2b9dc4c9258dab2f8da297d0272a3e5f3d352a2bbff82` |
| `VO_Bark_Dom_Escape_Start_A_01_dry.wav` | `435538708f28b08696550f99ebbc82b371d9b995ce5929fb9a2bb1f152a2370f` |
| `VO_Bark_Dom_Escape_Start_A_01_radio.wav` | `43c2300a956bd18a0cb6d023da0732c6c4188bde6b43473ea7490ef81513b9ec` |
| `VO_Bark_Dom_Escape_Start_A_02_dry.wav` | `74f48c2e9e0236d9c6741b8ef6034bbd91711b67dcb6eccaddeecfae7eabf1ba` |
| `VO_Bark_Dom_Escape_Start_A_02_radio.wav` | `0df2acd3a8a1633db5bbe3d57da2af5d8045161605d43188996832747f99e4cd` |
| `VO_Bark_Dom_GeneratorDown_B_01_dry.wav` | `563c21ba4c45d89e752b9784adca914ef945e53b197f53e4de0931a8706c65fb` |
| `VO_Bark_Dom_GeneratorDown_B_01_radio.wav` | `e89bcf6c49da832e25c69c5e5ea7d3af72cd487b1f856741e6f1f247b889c3bd` |
| `VO_Bark_Dom_GeneratorDown_B_02_dry.wav` | `906f44f1cda33e54eaaa4efc4d31d1814c815659dbcb0483c6c72294b79acd6c` |
| `VO_Bark_Dom_GeneratorDown_B_02_radio.wav` | `b5f1f6964638ac6717c175dd18a14506bfd84b61ca20b6845b697abaf07c6ddc` |
| `VO_Bark_Dom_GeneratorHit_A_01_dry.wav` | `f065e1fd4eb54f843ddb09a244a7ad17bf9a2c21d573943c33079508dc1a0977` |
| `VO_Bark_Dom_GeneratorHit_A_01_radio.wav` | `cafa4eb76b45e9962b2a47e2cf754ecdc3d1a4d724e23855e048989f8a161dda` |
| `VO_Bark_Dom_GeneratorHit_A_02_dry.wav` | `05afe60bba33a87b92fa89f504278f060920bd1f8e023ac4dc03e3f71d1dc4eb` |
| `VO_Bark_Dom_GeneratorHit_A_02_radio.wav` | `d21b23c4d553d79515292360ff9c06dd621b4c7565ac539df14630a80b507a11` |
| `VO_Bark_Dom_Tractor_01_dry.wav` | `f72e7ef33738c5258c81608b4ac788705490edefaf893c6c2cfd42ee45006240` |
| `VO_Bark_Dom_Tractor_01_radio.wav` | `52b791f4c92baf7a7f60eef193d789c80af3254d383ba98ecc8bc21b15aa6860` |
| `VO_Bark_Dom_Tractor_02_dry.wav` | `b61d12e3e956ea777c3640ebdc759f0754d08bf05302deeb01666e8afb31db75` |
| `VO_Bark_Dom_Tractor_02_radio.wav` | `439a5d6b70c7a3b87442e7d2b25cfe359f297081f4527a2f8030a5195f09fc23` |
| `VO_Bark_Dom_Turrets_01_dry.wav` | `e29c3f1ca6b51fabb8da203757e0f9d10afee42c0675388945599dd5fdcf2ca3` |
| `VO_Bark_Dom_Turrets_01_radio.wav` | `ac7932160b53bc2ba9287fdf721bcc4579ebd01203ac521e58d283d65fa0a2da` |
| `VO_Bark_Dom_Turrets_02_dry.wav` | `fab683d860bb0b665523bbfbb4305c5f556ffa7007a31440990452ce1b120291` |
| `VO_Bark_Dom_Turrets_02_radio.wav` | `1dd6666bb630aa146c775df49cf6e5c7aa6e11d24551d04e9c51baf1758dc456` |
| `VO_Bark_Foot_Contact_01_dry.wav` | `7746eab29b0b84c20a85a4c198bdbb1802e6c52153cc90329a1479a60616c4eb` |
| `VO_Bark_Foot_Contact_01_radio.wav` | `de20379a89789c66326b2cc33dd7abba50f7666c950f1326112985a80b109f38` |
| `VO_Bark_Foot_Contact_02_dry.wav` | `5b6b23b502180698dc875dcb0044ca03fe1628f4dccefe03f6a215c3d2e6eaf9` |
| `VO_Bark_Foot_Contact_02_radio.wav` | `bbdbf449a617985dc9c6c859b7f335a74d077b1e1e2c48e0db73960cffffb349` |
| `VO_Bark_Foot_Dash_A_01_dry.wav` | `e5209c3860b2ea82c41a1581062612a784820973fc5815c89f70769f00e18add` |
| `VO_Bark_Foot_Dash_A_01_radio.wav` | `1b65a813053efe708010ebcf39f6bf57b42797cace1eb391a6b38b609cea61a2` |
| `VO_Bark_Foot_Dash_A_02_dry.wav` | `3c9891220eb2f3b95b34dbe16908ca844256e773d2df7bad2b3dd5286c51fd3d` |
| `VO_Bark_Foot_Dash_A_02_radio.wav` | `8658b78e71c5cbc5aac7bf74e75594b96122974717d93474060f921f340d2401` |
| `VO_Bark_Foot_Deflect_A_01_dry.wav` | `4ff8cfd054d9c0b1aa85893646cddfaf83bfc6fc2c61913a9f27f2ba8e7a469b` |
| `VO_Bark_Foot_Deflect_A_01_radio.wav` | `cf9445a5b34553793be51603da68ef1049522ac74ab2969e025b3632b1a576d1` |
| `VO_Bark_Foot_Deflect_A_02_dry.wav` | `ff038da5d9df323c2ef1b6da45786f9ff31de61ac8a633524fc81dc04eb2a497` |
| `VO_Bark_Foot_Deflect_A_02_radio.wav` | `b5ef96a30ec15f7741f8d4304554e8ba5d692cda99396aec3c4bd367b21440d9` |
| `VO_Bark_Foot_DoorLocked_A_01_dry.wav` | `36966b706edc8f469580a45dbb3cf1394e70ca68824a32aad4396509850dc5a8` |
| `VO_Bark_Foot_DoorLocked_A_01_radio.wav` | `219c853eb439971595ae8fe1ee9c93f26ddb8b3e816d091b0ed9091e09feac99` |
| `VO_Bark_Foot_DoorLocked_A_02_dry.wav` | `c587326208b4a57aa80fd4cc625fc0e8491372d5e55d38aa46e4fb88d16afd21` |
| `VO_Bark_Foot_DoorLocked_A_02_radio.wav` | `4c75a8bc79147fcb25bbd7af883ddb31a516464c6726b389fdcf776c2086f41a` |
| `VO_Bark_Foot_DoorOpen_B_01_dry.wav` | `bf21cd73cc609e260d75b69bbcd40f3c9fc10dbd3c8ff7408a929c6a4f47375f` |
| `VO_Bark_Foot_DoorOpen_B_01_radio.wav` | `515195dcb0eb69a9f53974496f00804142225d20fdda1aa9f03d7b4b769ec49d` |
| `VO_Bark_Foot_DoorOpen_B_02_dry.wav` | `047eae082e396f00b754fe9a91dd5561fcf1444e6fe2f7faec95775b042ee662` |
| `VO_Bark_Foot_DoorOpen_B_02_radio.wav` | `729c8a2f6be916c54841c08f31ca33ac96e17b1bf873fe6593b55242bc96f541` |
| `VO_Bark_Foot_GuardBroken_01_dry.wav` | `7f5b4c2fe96082d6c4909e2ea52213a11366dce9a0d1a8e6dfc348a2b827e5ed` |
| `VO_Bark_Foot_GuardBroken_01_radio.wav` | `77498a8692852c9db7c0a65e38e89cda6d8e0037b98a60b8d31f38e262cc5635` |
| `VO_Bark_Foot_GuardBroken_02_dry.wav` | `94bba3c86acbaa39638ca716fb413f9e4c206604181192ea0812a57be4d98bf9` |
| `VO_Bark_Foot_GuardBroken_02_radio.wav` | `2462f5e0e5ccf9dd9f64918630b2638186a6dc022c65f95b2f7de9c17b7c43ec` |
| `VO_Bark_Foot_Hazard_01_dry.wav` | `17bd019026c5c979e01dbf6dcf07c1c0cd3898adaac208514345c818292d9317` |
| `VO_Bark_Foot_Hazard_01_radio.wav` | `cfb4df1d5d61e052f0c7f8804d820dd3e7ec6a5fd24af3f32d5e3d4f3731ed98` |
| `VO_Bark_Foot_Hazard_02_dry.wav` | `718945a887f8a75246b8826dd844319579ea6c2222edcb9e9560c8599382aecf` |
| `VO_Bark_Foot_Hazard_02_radio.wav` | `b1c26b02bd3e98a9c136762f8f5f9bf4efbdb0033b87510db453556abb2f8afa` |
| `VO_Bark_Foot_Heal_01_dry.wav` | `931ce0d34485172ad05a0b7fd5d8353e8e47871775fbe48e2c4a8995be9d93aa` |
| `VO_Bark_Foot_Heal_01_radio.wav` | `a8c1b6b2807e1aa6abe47625e6d0c6dfeffd470707e63d9def70d3801a7ab950` |
| `VO_Bark_Foot_Heal_02_dry.wav` | `4cd37f304239a3485dee7db77fb0450ffc642c649998afc9c8d3978aed801238` |
| `VO_Bark_Foot_Heal_02_radio.wav` | `362f5929083953804a870ce17a3c827db7246acdc996f8c1631d50d04767f0ac` |
| `VO_Bark_Foot_Hurt_A_01_dry.wav` | `1c011eec5bbed6dae43887b744f62a4c5ac42d178a0487541b7e6c60beec833d` |
| `VO_Bark_Foot_Hurt_A_01_radio.wav` | `0e07269293d6c9b320188d20a12ff7783c8f6a97b3a4481eb13fe69b2bab5503` |
| `VO_Bark_Foot_Hurt_A_02_dry.wav` | `7a7c62859b35f54c30b5abf0159d869b5bde3bf9391f310ce540187203699931` |
| `VO_Bark_Foot_Hurt_A_02_radio.wav` | `6048199152627a4771e006af2837d22a54c883770b14a59162579be3e22fe335` |
| `VO_Bark_Foot_Idle_01_dry.wav` | `c2adbc3619ac0b9b5eb59256c1ea4e469e030003fb730bc8a7af8763e4e86f4c` |
| `VO_Bark_Foot_Idle_01_radio.wav` | `eb393c7322293bdc5bfe87d0bb2ea43b39ff449d0dda6022827c8de28e7d0f40` |
| `VO_Bark_Foot_Idle_02_dry.wav` | `df07179c980cddbcb78cf6935946781b6ea6c9e0ab39b87d2f6447887188c660` |
| `VO_Bark_Foot_Idle_02_radio.wav` | `f976821781082957423904a93c97ec65dbe1347cc01d8bd625e42e16b25e0bd7` |
| `VO_Bark_Foot_Kill_A_01_dry.wav` | `2343fa7ad4c27a13c8c60756d9fbc389569c1168daf75b7c5ffab42ca206c4d0` |
| `VO_Bark_Foot_Kill_A_01_radio.wav` | `01f4cb1d394b8fb50463407308db80420aeeaf722583c0cd288fd721bc14abba` |
| `VO_Bark_Foot_Kill_A_02_dry.wav` | `7dc6f8a85d433425f8f37a5429fd0fecefa2bea186fc85e9adb36e210fdf70b9` |
| `VO_Bark_Foot_Kill_A_02_radio.wav` | `6bed6d910848359091ffe30016809bdc53d6311d4149eb734b4fb1581cd76fae` |
| `VO_Bark_Foot_LastOne_B_01_dry.wav` | `c1e277dd42ae15d2a6f5c9e0b4565055b8fd50f53bf86dd2639c1383b1bd66a9` |
| `VO_Bark_Foot_LastOne_B_01_radio.wav` | `db9b664b167ee77720f61802f9edcb9c119527fbad9924d2fe154ddc6aaab882` |
| `VO_Bark_Foot_LastOne_B_02_dry.wav` | `a005ae95d99b384aa098bda6e37dc52d3a35525d6e715ba4da039766b0c590c5` |
| `VO_Bark_Foot_LastOne_B_02_radio.wav` | `caf9a93915203d3ccc84e458d4337266fdce9bf74021bc2ec795cc0624af3c51` |
| `VO_Bark_Foot_LowHealth_B_01_dry.wav` | `ccd0ee856c0d31f60d70b289039b3b6d79ebddafb286ac9cfd6c0bb450eb3fc0` |
| `VO_Bark_Foot_LowHealth_B_01_radio.wav` | `4c77ef0e98d356af3453735e650019be4a84a5fceaf0ccbbc1b212ddef291b5d` |
| `VO_Bark_Foot_LowHealth_B_02_dry.wav` | `4639d0285df29a867982a65f18d66b14f0d875bb819194fdb71ded48e207b378` |
| `VO_Bark_Foot_LowHealth_B_02_radio.wav` | `a603c4c3c35d26660e78a0a3fa8658a98bc2442b000a5c9b9043fbf22ca13e35` |
| `VO_Bark_Foot_ObjectiveDone_B_01_dry.wav` | `27c0e4bea6be2ac51434cc27c3ea6210ceff6c33d9fee774c2b18f0ab7eeac88` |
| `VO_Bark_Foot_ObjectiveDone_B_01_radio.wav` | `230a146d39be3c832a124e919623d2fcc46cfeb1f74ecefd1732003101a9eea0` |
| `VO_Bark_Foot_ObjectiveDone_B_02_dry.wav` | `8953f525106323fed9131e8fb76df358de9ffdc6ee7ed3535057b2cddab5a802` |
| `VO_Bark_Foot_ObjectiveDone_B_02_radio.wav` | `5c46fc7e4193337a530141ff83f1c92b9890d534d8437714d8b3119052fac986` |
| `VO_Bark_Foot_ObjectiveNear_A_01_dry.wav` | `f30cd318e2b5ba15f3ce379088b54c92696c3c58e6327b3c14742e9b5ba0dde8` |
| `VO_Bark_Foot_ObjectiveNear_A_01_radio.wav` | `994bd03d0f145b107fbac2c0b35447a918dec36b2c685405da737c54c18131ad` |
| `VO_Bark_Foot_ObjectiveNear_A_02_dry.wav` | `1fc69f5dad807931d172f2bdf02ae1670ae43cb27fab211b34e1b5dbe79f8774` |
| `VO_Bark_Foot_ObjectiveNear_A_02_radio.wav` | `33d05881ca7b0f9960eff3509ebc314095579be5b727c321e8ceb22d4ec0c8a2` |
| `VO_Bark_Foot_Perfect_B_01_dry.wav` | `32673144b2bd41a300539e25639504f1a20298d7e9f685575b23636c2eec7793` |
| `VO_Bark_Foot_Perfect_B_01_radio.wav` | `8910865565ca4e3726c4727aa5d4549767272aec3d88e9fde6deaf7f3b8c65aa` |
| `VO_Bark_Foot_Perfect_B_02_dry.wav` | `2ba20010469ef85b97b859366b78d51e465b484744efe94e8c230d62564c17f0` |
| `VO_Bark_Foot_Perfect_B_02_radio.wav` | `0f066d1acded982911123c24535d51edaf7474ea0cfe45f060c6f8451f880df3` |
| `VO_Bark_Foot_Slam_B_01_dry.wav` | `c4822857b3fcfa68a33d41e627c75234643cef8d48b9b68262a5de34dfca0c33` |
| `VO_Bark_Foot_Slam_B_01_radio.wav` | `5af765d3da2345f44e60b5960f74547c8a7be16336f309bac2bd48c2169ed578` |
| `VO_Bark_Foot_Slam_B_02_dry.wav` | `c247376a1c6c1a44aecd9ecdc0a523132eeda3217e16e5a88c34da5d6591e1db` |
| `VO_Bark_Foot_Slam_B_02_radio.wav` | `33beddf05dfa49a952320ced0fa90aaaa3bd7ac7e6fe8bd68cb651e33f3ab38c` |
| `VO_Bark_Gate_Enter_01_dry.wav` | `ad6907acba542570e4711e73013861d2d6b4634d7d185986c07679b094f47360` |
| `VO_Bark_Gate_Enter_01_radio.wav` | `eb853edb1d52433f797e0eb28a5f82c376f82d1bfd002fe914f0422340538222` |
| `VO_Bark_Gate_Enter_02_dry.wav` | `555d9af17c6a2394dddd51e3b6bc51d4912d85d3db65b53dd92bf8d34005ec1b` |
| `VO_Bark_Gate_Enter_02_radio.wav` | `c35fd0067fe05773e67dd86790eae268e3f460c5ade0bf522634b641ad50d9d7` |
| `VO_Bark_Hyper_Blocked_B_01_dry.wav` | `1cf283ecb455a00c9d0a81d4d43de4b7fcf105316f7b1d166ca8b2580979ffce` |
| `VO_Bark_Hyper_Blocked_B_01_radio.wav` | `85562615a0b611932923efcae0f52a46c4de629d8b7f2a16b1cd8273b3743b22` |
| `VO_Bark_Hyper_Blocked_B_02_dry.wav` | `e64dd90e05d255ff683cbe5df5c6f42c595e61beb5c4407cb4e1745966ea4190` |
| `VO_Bark_Hyper_Blocked_B_02_radio.wav` | `2733f16a45925dd8a990705535a359a560bb8041e76ea1f2a14290a7835e0857` |
| `VO_Bark_Hyper_Spool_A_01_dry.wav` | `4b38e84aa4e11c6811329110fd59f8dd666c521418bb84de15320d731037fb06` |
| `VO_Bark_Hyper_Spool_A_01_radio.wav` | `311c4a2c6e6b939f3c5ea847fe0888823450f6caccd8521407258c9637fe0525` |
| `VO_Bark_Hyper_Spool_A_02_dry.wav` | `906a6ec609cb7db9d6bd03c3902f729767bf9cc6abd971aa1a85e6df8f7fb5fa` |
| `VO_Bark_Hyper_Spool_A_02_radio.wav` | `756cd09fb3dca45e395f1f8558d34aa4b3ddbcf57272ee8a682a5431badab69a` |
| `VO_Bark_Land_Down_B_01_dry.wav` | `95038cc3337f75cf288dd9d57fb48eb7ff46a73133452cbe5f2f83f24fc085a7` |
| `VO_Bark_Land_Down_B_01_radio.wav` | `af2d55b8067f8cb7fc86bced2f05c393226e136788197deb9df456f4109fcb80` |
| `VO_Bark_Land_Down_B_02_dry.wav` | `267ffbecb2d5ecc6e2bba0e2c45f87ccc31fdaae259f631ea4d0f52329e81137` |
| `VO_Bark_Land_Down_B_02_radio.wav` | `5a6678bc6a5fa390f76d8e7870ab368796c7f5528e9f21b0d764df23f12db5f6` |
| `VO_Bark_Land_Ready_A_01_dry.wav` | `a70762efac678c82669019b1e100c1032352740850e3dc91784bc202d553e957` |
| `VO_Bark_Land_Ready_A_01_radio.wav` | `37695e7e105d1ce13dde145373ce0accb327a163cc38344866463accd36fff9e` |
| `VO_Bark_Land_Ready_A_02_dry.wav` | `2b24cf46db11d6a2eda2fc75fd96c0be9922050d0ce907d3f1f1c0b2ac6e10f9` |
| `VO_Bark_Land_Ready_A_02_radio.wav` | `d73d594b43860aa02e42abf64108d2ed47af912d4da0dc7713a2151a63d29265` |
| `VO_Bark_MinesAhead_01_dry.wav` | `ced44407d1ecc6ff342c92250a5aef8d48ed6c819e58b63a76dc0d0dbb2905fd` |
| `VO_Bark_MinesAhead_01_radio.wav` | `d59ef19cc90b8ef668050dd54b9cfbbd979897ef4b9ddb634125ff41c37f430c` |
| `VO_Bark_MinesAhead_02_dry.wav` | `decee3fd609ab866f4e7021eae05c382e94c907f1310e4f1b4db88c44b15d0d9` |
| `VO_Bark_MinesAhead_02_radio.wav` | `35803a3b25d5981ff62616202491d1228f2ba307640bb42e7797f9692cc42a08` |
| `VO_Bark_MissileEvaded_01_dry.wav` | `103094297e2422c5d9de8af0b2eb95e3339d2dc9f9d382ac22bed1252d6bf4fa` |
| `VO_Bark_MissileEvaded_01_radio.wav` | `62d19a1bba4378481ac963688f288309bfd30c27ba9b8c119f16e61126ad095b` |
| `VO_Bark_MissileEvaded_02_dry.wav` | `ec4e1667ba160243479f6ecbfcbaa6a35eca7b97cf04b59a2b5d65b4c9e50d48` |
| `VO_Bark_MissileEvaded_02_radio.wav` | `a2927fbf3c7f619b76e99ace156422e2886a9d4617744d2de101425da56298de` |
| `VO_Bark_MissileIncoming_01_dry.wav` | `6add8546befd469cd1fbc86bc8bbf4e08a8954d0993d28e76e462e2b9028a0cd` |
| `VO_Bark_MissileIncoming_01_radio.wav` | `fcdcdf1977bdef4b6f7cb987b6aad39045cf9a90a56877f333c52606de487ddc` |
| `VO_Bark_MissileIncoming_02_dry.wav` | `75ff0c829511d0c9cfd88b5513da5d45b1560c27e06820b5a2aa711101c0809a` |
| `VO_Bark_MissileIncoming_02_radio.wav` | `9d1228801d564f1fb6f5a10f18d3327c77ddbfcf207005e39172efb5e1950bcf` |
| `VO_Bark_MultiKill_01_dry.wav` | `641499056c10c0712e58fa4010c94be103c83b77cd2ee1c14db32bd6afa35602` |
| `VO_Bark_MultiKill_01_radio.wav` | `a7ab72932d6a72a0e0526c5e6e9538d2a44abeb756f91105a6f1ed6b6fbf9d60` |
| `VO_Bark_MultiKill_02_dry.wav` | `26955ef121a30e3290ee8225867a64dca82c5bbce04fba9f109b3ff69a03da24` |
| `VO_Bark_MultiKill_02_radio.wav` | `40655af575492e811b1b73795004e0da6a700669aa144f72833b7b2a3cfa5e41` |
| `VO_Bark_OnMySix_01_dry.wav` | `07c7602d7a7281382406a273c0d03490e95bf784578c5f10fe6148658eb69735` |
| `VO_Bark_OnMySix_01_radio.wav` | `31030b22f06749f71174abac9af9ae6e7a024aa02362875d37ecb96e45f4db9d` |
| `VO_Bark_OnMySix_02_dry.wav` | `75d379ae366a09f590e7ed66eaf311124c05d2407ad20283a14b1a20fac378cf` |
| `VO_Bark_OnMySix_02_radio.wav` | `0816c7e1787201ca9fad0d40420a7901766ff3e11f85daa44daf6512afa63ebd` |
| `VO_Bark_Planet_Arrive_Cinder_B_01_dry.wav` | `ddbff83d05023bb3e089776af3de3a687c3d8dd145941eb851751e3a9466785f` |
| `VO_Bark_Planet_Arrive_Cinder_B_01_radio.wav` | `d4873459fe5577176ba75b5d6c268a1446edccc3f1b39f1d075ddb143868930b` |
| `VO_Bark_Planet_Arrive_Cinder_B_02_dry.wav` | `0c35dab5b1811effaa4614a24844f05ae05ce323079564b63a6c851229d77644` |
| `VO_Bark_Planet_Arrive_Cinder_B_02_radio.wav` | `15e7395d43cea4b0c48c8ac75aa51a2e1b591cb4cb9a0a170fab2c907a6d08ca` |
| `VO_Bark_Planet_Arrive_Verdance_A_01_dry.wav` | `c9a2086b23c30a16d98a90e3727067515556cdd5a9ee1f6f0a481b671bf1ad5c` |
| `VO_Bark_Planet_Arrive_Verdance_A_01_radio.wav` | `70355860e3acecece194d4b17be8e77830aa5e0ceba6a483cf36416c3307b73e` |
| `VO_Bark_Planet_Arrive_Verdance_A_02_dry.wav` | `959db16d6c5d4feab70363a4ee95d03f7f86f6a1b420ad20f6ecb7e40bd6c1ba` |
| `VO_Bark_Planet_Arrive_Verdance_A_02_radio.wav` | `3bc95ec9aae89c333c5fa882bd6ccc7bd1ec22db19b4d60db4d32fa9faa35c24` |
| `VO_Bark_PlayerKill_A_01_dry.wav` | `44dcdf37bdb55b4796e8d45b3aef991277d55c71eb056da6983b5f1c0b650f71` |
| `VO_Bark_PlayerKill_A_01_radio.wav` | `a0963140670dfbaa20a2eca149f1cf89698c797836a6d26abfa7f9067fe4d07e` |
| `VO_Bark_PlayerKill_A_02_dry.wav` | `bed0fff7604322d36dfcff60564eec856d9c20db433e4b45a840e4cead7b14dc` |
| `VO_Bark_PlayerKill_A_02_radio.wav` | `ab901e64be88d1d566b3a5ca773f323eedcb486f8056e6dcd1334238a0604b70` |
| `VO_Bark_PlayerKill_B_01_dry.wav` | `d60f0b1249748c95b621fe6b5ffbc4916901d5e7146f29798d5ce38aa4a374c3` |
| `VO_Bark_PlayerKill_B_01_radio.wav` | `6f1b63c7b6123e2dd8383bae11387e235d23077598520f360a4631e83626d567` |
| `VO_Bark_PlayerKill_B_02_dry.wav` | `65bdec0a0743ae34645865f121cba6e8eb6402421f22cfbb492109aec46ea167` |
| `VO_Bark_PlayerKill_B_02_radio.wav` | `05933bf5f56c3fd5a9a92a9f3df8b9767bd39d13902fd8b82e165a6107bc7f31` |
| `VO_Bark_Portal_MissileReturn_C_01_dry.wav` | `e316188c8bba98fdc81654df0a24fdd99819223a716faa3bde71bfe2732c90fc` |
| `VO_Bark_Portal_MissileReturn_C_01_radio.wav` | `612a0348467fb5eb929110427862146ce9ebb8b74c1e73b57ab687b826b70194` |
| `VO_Bark_Portal_MissileReturn_C_02_dry.wav` | `6a8f935b2f91398a18446d0d4ba339339c04f219259367d25d992fc4a7638836` |
| `VO_Bark_Portal_MissileReturn_C_02_radio.wav` | `067182b5b08febe21a66524773e79df722ca709e8548f554f214023b33ea36e2` |
| `VO_Bark_Portal_Open_A_01_dry.wav` | `ad3f31657741ca2f8079603ae7a5bcbc6fe52c237d58df875d7f8ef91d9e79d8` |
| `VO_Bark_Portal_Open_A_01_radio.wav` | `3d746cfd1bbd85dbe964cd7fe4355a4aa8d98a6db50c281bf1b16db6660140ab` |
| `VO_Bark_Portal_Open_A_02_dry.wav` | `75ba31c234f4693212132292528410ab1d4b7a536c3a237b4989f539e41d8d42` |
| `VO_Bark_Portal_Open_A_02_radio.wav` | `3ab7cc45f76d373086b169f1599bcfc53b748aa3863d30e9de3606d69c44e8de` |
| `VO_Bark_Portal_Through_B_01_dry.wav` | `248b7946f4effd8556884037def8b271714ff020a88feebb0135dfbe9a2d7614` |
| `VO_Bark_Portal_Through_B_01_radio.wav` | `2714c9c066c710a283aa3d29b5330b79c4c9702146e4a0c19500e6cb0e840676` |
| `VO_Bark_Portal_Through_B_02_dry.wav` | `4497976e675c686d58c5cbd7a6e7ab9afb501c6acba4fbf4214679f59b32f910` |
| `VO_Bark_Portal_Through_B_02_radio.wav` | `891fc3a0f8ab0176a5ee91db56ccf414b9ce30b53b3e46ed8550a4e68f035e96` |
| `VO_Bark_PowerNotReady_01_dry.wav` | `2e5679a52ef38583d3ed498574e286047be716c7d00f7ba5f40c15de87912f89` |
| `VO_Bark_PowerNotReady_01_radio.wav` | `71c3ee021460c508ea30acdf9efa13ca2a84c5f2508d14cb96d74bb72c217c9d` |
| `VO_Bark_PowerNotReady_02_dry.wav` | `937b2ec4012c0838de75d9f62441501be30b59c5677fbbf5f3c5c21d79d268ab` |
| `VO_Bark_PowerNotReady_02_radio.wav` | `ee65643b813a3495c19e0d09d456d531ee8c97dbcda5724e226111278945630a` |
| `VO_Bark_PowerReady_01_dry.wav` | `2e980ccc4a25537cc4c42deef28dac3a300f05ad9d63a2d36d47d5bfcd9825cc` |
| `VO_Bark_PowerReady_01_radio.wav` | `1626fd0bdc9bde35266a343a8255c8f4d28ed670792d8ef02327bb50e599870b` |
| `VO_Bark_PowerReady_02_dry.wav` | `e17c9618eb37578fc63e4ec1303c8e22ad3021b3b5cb993f10e1f99edc592be6` |
| `VO_Bark_PowerReady_02_radio.wav` | `0e9381d0d02bab58d4036126feb2a7711d4397db778b829b938d587e25f9fdb4` |
| `VO_Bark_ShieldBack_01_dry.wav` | `808cd0f514d679221a3811e01ed87a69c3b6982d5d53c44a738fcfdff40a189e` |
| `VO_Bark_ShieldBack_01_radio.wav` | `739f1e08ab1df04e3cee740320426f729132189c18e0416315b81639798a35a0` |
| `VO_Bark_ShieldBack_02_dry.wav` | `2f16b7a499f0584b586646eee85e43fd3293a0d82e1f88b85e682a73cc87f85b` |
| `VO_Bark_ShieldBack_02_radio.wav` | `9bdd605d891d3eb74f9318fe97248102909bc081af5bb7dda5fcb630055fc247` |
| `VO_Bark_ShieldDown_01_dry.wav` | `834a5e76f82fa4d9ae874c9aac1bc9468cab69715988bdabcb326b67ae472800` |
| `VO_Bark_ShieldDown_01_radio.wav` | `83dd7802cdf4115aa0e98adb62f992ffe553faa74c00980aa0672adff3b6e627` |
| `VO_Bark_ShieldDown_02_dry.wav` | `2c0908dd5d138eb4c64dc2b8bf3af7a79c16ff14a727d907b62bccbdd0509625` |
| `VO_Bark_ShieldDown_02_radio.wav` | `1e04a5c3bf4b4aeecb62bb0013390886b34fa2b1ca06197d1c365eccb76ab996` |
| `VO_Bark_SquadEngageAhead_A_01_dry.wav` | `00128dad8993311282e58d9162f14c95c7713abeb68d9c59e95ddb40ea877cc8` |
| `VO_Bark_SquadEngageAhead_A_01_radio.wav` | `0b90fdc3c34ceda2e59e466b78d9c0766b0349ca63f89f4cc4854b45d7b160d3` |
| `VO_Bark_SquadEngageAhead_A_02_dry.wav` | `d48e9ef3147112ba9b02c2848cadac97aa6da11f703e5e76e1cdc4ffff1e5b65` |
| `VO_Bark_SquadEngageAhead_A_02_radio.wav` | `b82d6e6042363c14195e508fc5349bfb6cdfed7f4e79b61e4c027f113ccf7705` |
| `VO_Bark_SquadEngageAhead_B_01_dry.wav` | `b9f237c889e312aeaaf7b6e746220a1f96707c97415a4ce142b192416e5d739a` |
| `VO_Bark_SquadEngageAhead_B_01_radio.wav` | `6c5211e74eb0448aff4ae8f26b0078e60a19b70936e03156408ccc09e370cd8a` |
| `VO_Bark_SquadEngageAhead_B_02_dry.wav` | `231170f08a6b48f1693440181baa46d8e76a65ad45e9020797a4690863cf8c9f` |
| `VO_Bark_SquadEngageAhead_B_02_radio.wav` | `f7ebb1a52af1993ad91927e4d61ef9eb6c349470cb6a512de7ebbb3513c58ffb` |
| `VO_Bark_SquadEngageBehind_A_01_dry.wav` | `1df7639fc5fb0601fe12769a4bda7ebb05c0f86e5fdf95505daf2d62341a5bbd` |
| `VO_Bark_SquadEngageBehind_A_01_radio.wav` | `d80a7cb0bff4f8a4b112891fc68d6d97fad8071592affb5b44732d24c113a1e6` |
| `VO_Bark_SquadEngageBehind_A_02_dry.wav` | `8c2897255728a8b610b89af591f964ca31ec05cffe4ef062e9ea035ea3897cd2` |
| `VO_Bark_SquadEngageBehind_A_02_radio.wav` | `dce76d0e51eb87a09b0e5f0f9956dbe0f9471f9bbfa72ba53e83d2597f127bb0` |
| `VO_Bark_SquadEngageBehind_B_01_dry.wav` | `72f7f515ceb0fd5d1de859ef6ed35aaae5b6f57550e6d85908213dd9b586435a` |
| `VO_Bark_SquadEngageBehind_B_01_radio.wav` | `a003e509b49c4a146be9fdddcece8b1265e66e58b1871e37143709724663ff90` |
| `VO_Bark_SquadEngageBehind_B_02_dry.wav` | `517afefd0b48a1013a765023230ea1dad383947de1169075dc31277cfcf94782` |
| `VO_Bark_SquadEngageBehind_B_02_radio.wav` | `5440ca34be658a579688b1d27bb2cabcf955ab37b315dd76d3272389fb31484c` |
| `VO_Bark_SquadEngageHighAhead_A_01_dry.wav` | `3ae670404ba17a508a1a972903f515567c8ae0aa8184494fedc5718ad9df4850` |
| `VO_Bark_SquadEngageHighAhead_A_01_radio.wav` | `86fefc729ffc5ed59c0ba392a4587745ae19509425fc4e3258e88b9572557533` |
| `VO_Bark_SquadEngageHighAhead_A_02_dry.wav` | `97a1bf22831d4e45c8f4152d160665b91db68d95e5da0d215e017cdc76fe4be9` |
| `VO_Bark_SquadEngageHighAhead_A_02_radio.wav` | `df84adc1d4e68e166eeeff001ab32927ba939fa34da88c24ab9fb6362e23c8c8` |
| `VO_Bark_SquadEngageHighAhead_B_01_dry.wav` | `84a8a66e2d3b812c3e77763a26585bfc22346d65a570849c9abdb3e0c730e05e` |
| `VO_Bark_SquadEngageHighAhead_B_01_radio.wav` | `487defc6058cbbe38a6c3c8032731fff43cd8b5cc84efc3262a9f010c2166ee4` |
| `VO_Bark_SquadEngageHighAhead_B_02_dry.wav` | `8c404efe7343413a54b1127225defb9daae9d68f5ac61eb0f011e2a41158fc3f` |
| `VO_Bark_SquadEngageHighAhead_B_02_radio.wav` | `d633c0c525f3fcce337c9ac508eebff0b2103f583e6ee4a9c1467b8e23c67651` |
| `VO_Bark_SquadEngageHighBehind_A_01_dry.wav` | `24d7c4a7cafa39aca53913ded2fb9c597e474acf41cb9a72a0e4d8f36dedcf7e` |
| `VO_Bark_SquadEngageHighBehind_A_01_radio.wav` | `a7d9db6c4b7cbb6c53a2907857690ef9a9f9c06f1b3dcf18de29972bd518f263` |
| `VO_Bark_SquadEngageHighBehind_A_02_dry.wav` | `e5e5b7175eb96fa9032af595fa38e00d2253d7a44f36b3a11e7f7999428c5b3e` |
| `VO_Bark_SquadEngageHighBehind_A_02_radio.wav` | `7bc3e8f0f2571b08937d9f91959254c27a3cda94d2888cfe1dd54ad9faa0d4f5` |
| `VO_Bark_SquadEngageHighBehind_B_01_dry.wav` | `65c1474133fa921626b31d6d649d092de52e95608f281af1288f36929975c911` |
| `VO_Bark_SquadEngageHighBehind_B_01_radio.wav` | `3233f9dd53b8618b849c0dd047d8856c7f57cd83608e3695018bbd20b1debcc9` |
| `VO_Bark_SquadEngageHighBehind_B_02_dry.wav` | `9068c4a4b4922ab1ab6f5b6fbf09f6de5dbd0e6fcec8a8be4166d33111fdc2ad` |
| `VO_Bark_SquadEngageHighBehind_B_02_radio.wav` | `583a5c850fff18125f8ece5ec3b362999b2c6f3ef2efaf6885a72f7ba36f0207` |
| `VO_Bark_SquadEngageHighLeft_A_01_dry.wav` | `30cabf4fa4a50358c2ba9e4e9e52c1a6757971464f34cb49209ed899bad6f89f` |
| `VO_Bark_SquadEngageHighLeft_A_01_radio.wav` | `9f22b1f1edb1733242237959fd2daf7b29b6b926f3ae6961b44abae9d7db16f8` |
| `VO_Bark_SquadEngageHighLeft_A_02_dry.wav` | `f7f05f5926f229bb6857182c8bab5caa1102e4953f1e99a6e9929d3bab5bc304` |
| `VO_Bark_SquadEngageHighLeft_A_02_radio.wav` | `1d485f72f432d03caaf87e84f462668084bc167b96343ffc89dec850bb15889a` |
| `VO_Bark_SquadEngageHighLeft_B_01_dry.wav` | `3374008e7e53cacc2006fd9b85b9e630809aeb097dbd0e9db8ec544bf36fa180` |
| `VO_Bark_SquadEngageHighLeft_B_01_radio.wav` | `db2b2218625df343748286d81c32b5e81aa45a6018b260755ea59a47948c89b5` |
| `VO_Bark_SquadEngageHighLeft_B_02_dry.wav` | `1f6fedd696ca3935d133cdbae1f9f23e80b9d1917689f36c010154243cfb7d76` |
| `VO_Bark_SquadEngageHighLeft_B_02_radio.wav` | `d0bb9a8d65b46852fb6b9a8e82d38b4242bfa867e5c6705dc7e4bd96f8bb9a04` |
| `VO_Bark_SquadEngageHighRight_A_01_dry.wav` | `a9d54ced06831601dd084efb7f40bbc30f348afb237a43b52fd47def4f740077` |
| `VO_Bark_SquadEngageHighRight_A_01_radio.wav` | `5dbd48e22b834db16adb8ee2f813620e9ce99d42ca7c129234637ad05526646a` |
| `VO_Bark_SquadEngageHighRight_A_02_dry.wav` | `57d4c2e2fdf9e8621e4468562349bcee6e1b269036d7276339bdb1550d57aa45` |
| `VO_Bark_SquadEngageHighRight_A_02_radio.wav` | `1007a1ea46d1aba8ad62940ffb38c1a668d5952101945968604ef4f787c457e4` |
| `VO_Bark_SquadEngageHighRight_B_01_dry.wav` | `e312cf6516d2455600b58fcb97ec24337770887237e5c2ffdd9487c9814f4dc5` |
| `VO_Bark_SquadEngageHighRight_B_01_radio.wav` | `5792337466d312cf911345b92bc34b2f9ec1e05e78a2bb91a1cd7ad5cb1ae531` |
| `VO_Bark_SquadEngageHighRight_B_02_dry.wav` | `bc1028b47c5db0ad1faabee9e12483fa7a886378cbdd7ce0fad14c3cc4393709` |
| `VO_Bark_SquadEngageHighRight_B_02_radio.wav` | `ec8ad33404cbf3b036ad9191e4404894697198b31fdf2822e4461373e9a835df` |
| `VO_Bark_SquadEngageLeft_A_01_dry.wav` | `134da5d231f8ce4e6bf8b8537a5e31e8235facc2e3d1ffb90877f7cf7ec07e78` |
| `VO_Bark_SquadEngageLeft_A_01_radio.wav` | `6cbce5cedcd3e1fb333aa625cae05394247c0c7e05c3c6b5a3d9aeb2067dc645` |
| `VO_Bark_SquadEngageLeft_A_02_dry.wav` | `ed9780fe60df12019704b26e197aec5a60d2ec06a4e486843e96db30324d99c5` |
| `VO_Bark_SquadEngageLeft_A_02_radio.wav` | `0c45785a0f852e2a793bff6f33533c0a8819b5b7f900bc101b6fdfc16c1821b0` |
| `VO_Bark_SquadEngageLeft_B_01_dry.wav` | `7217f3f6b6575da38cc03f03d8168aa468e08b6047cc5412b0dbb102682d2d02` |
| `VO_Bark_SquadEngageLeft_B_01_radio.wav` | `c1cfe06d2f54d9836a8748b85745fa244b4f74b85a73e2ce927a088fc6ed22d7` |
| `VO_Bark_SquadEngageLeft_B_02_dry.wav` | `fa35a4b674459ffa967d9aae387cd2b929ef2793b178b60266a3fe4b7f4523a3` |
| `VO_Bark_SquadEngageLeft_B_02_radio.wav` | `3870b68f1f1891279fed94166aacdf5f6b0ced7a8ade4c679453e7ee9009dc89` |
| `VO_Bark_SquadEngageLowAhead_A_01_dry.wav` | `52f354a89278c9d46929ee3b7d666ea632a7dcfba072fe527e418c950855be03` |
| `VO_Bark_SquadEngageLowAhead_A_01_radio.wav` | `c66992ecafd79ea810a4c170a341e2cc3eeb44d07bf35ca262b742f977f7b03e` |
| `VO_Bark_SquadEngageLowAhead_A_02_dry.wav` | `85bf8a82b03e813a0140dc59cf0c72f37cee80da14f12d744cb3868d2ec00db2` |
| `VO_Bark_SquadEngageLowAhead_A_02_radio.wav` | `c626dfc29d30f2852758643017f2b6d7a8a1625e7f8fd72d1364a2aace9dd817` |
| `VO_Bark_SquadEngageLowAhead_B_01_dry.wav` | `b04ee6386cf8e8bc0837b9bbe1939b92ba9edb1beeba312283a6dd284cd614b4` |
| `VO_Bark_SquadEngageLowAhead_B_01_radio.wav` | `316b6233d7e75fe42e9b2b113eadbedf0a4f2c61da2c5c21dd5422bb62f06e67` |
| `VO_Bark_SquadEngageLowAhead_B_02_dry.wav` | `16e998708b0c1459fe9e5a801b6678e634bf15bf4ff832755d516994407ce508` |
| `VO_Bark_SquadEngageLowAhead_B_02_radio.wav` | `4caf6ada0df1606695b2fe641cb20e5ccb1eb483c59758cdf1e497522bf9c705` |
| `VO_Bark_SquadEngageLowBehind_A_01_dry.wav` | `d38788f958f864cabc25c9e4722886a51c36cf8dfae3c6658d6393c17637ecc7` |
| `VO_Bark_SquadEngageLowBehind_A_01_radio.wav` | `6cb05ae9e05c5c97ee89f213e718255948d4c5e462b992f2c3b94fe5692b8fc5` |
| `VO_Bark_SquadEngageLowBehind_A_02_dry.wav` | `77d50f47966e6d547a7eb7640d99cf3910a946d63842e4d00ee13d3f41fe8fca` |
| `VO_Bark_SquadEngageLowBehind_A_02_radio.wav` | `edb3f07364c5bc6dfa316a837b7f0a8e30300623f9ebe3ef50663b37a4cfcce4` |
| `VO_Bark_SquadEngageLowBehind_B_01_dry.wav` | `92baa96b08f2b2061b218e317ac58bc0f82c3ca8559da3cdd002d57c9de2cb56` |
| `VO_Bark_SquadEngageLowBehind_B_01_radio.wav` | `9c467871256054a3482667195be82a1f104d3e45b4b3ff5c96c9c15c7abf074c` |
| `VO_Bark_SquadEngageLowBehind_B_02_dry.wav` | `6af5333430ed6c1249d82831755629e840819fee1ec8292d43ed88d6b2c58094` |
| `VO_Bark_SquadEngageLowBehind_B_02_radio.wav` | `3d543e0493b20fc8cae541781108120ba86eb3744229196b0c6e58f4eb36e699` |
| `VO_Bark_SquadEngageLowLeft_A_01_dry.wav` | `5ea6b0d3b51fc564cd306eb6cba594c86581d368d4e088822bc8171bc2398544` |
| `VO_Bark_SquadEngageLowLeft_A_01_radio.wav` | `1a7882d849a7ccbcdd53ea5bec706e48ae9e6e95122829683bc57ed801139464` |
| `VO_Bark_SquadEngageLowLeft_A_02_dry.wav` | `767c6550abc5d58df59b59517a18c591a5c898940b8469ded96d8f145a7bc600` |
| `VO_Bark_SquadEngageLowLeft_A_02_radio.wav` | `9d3fa9de414ac9250e8a1fb212a5497f24f92bf47d0df4e2897e7917b6daf6ef` |
| `VO_Bark_SquadEngageLowLeft_B_01_dry.wav` | `b6df4d8b174f6e2b5d634d56548c8551f1d5008f2f1b8dc14152cab974ed2adb` |
| `VO_Bark_SquadEngageLowLeft_B_01_radio.wav` | `3438b8c7766e4d53b12242514fafdc58a53b35d92dbd4c4ff4eb0eb0b81dcdc3` |
| `VO_Bark_SquadEngageLowLeft_B_02_dry.wav` | `5a7a2ea016e4e22e5e84b7c68938658f236de02a07edcd0fc4cbeff506979171` |
| `VO_Bark_SquadEngageLowLeft_B_02_radio.wav` | `3361d755c0cd8799df309d3b8a0278600bb74ce84ecd6223f08dc2076d944b94` |
| `VO_Bark_SquadEngageLowRight_A_01_dry.wav` | `36e2af23421654b6dab49abd17dcbabecd610efa2bcfffcb5511ba143a6c5aab` |
| `VO_Bark_SquadEngageLowRight_A_01_radio.wav` | `29b8c183d73fac7d9a387fa567afd1bc4ebcc5f8d6e9745c8c642e219d185271` |
| `VO_Bark_SquadEngageLowRight_A_02_dry.wav` | `96d959b289930bf819170b62144593815bf506629b643cd50d604ad050012dd9` |
| `VO_Bark_SquadEngageLowRight_A_02_radio.wav` | `56d7760e0d6aee13fced99859f1628cad63aef238fc1a2631f105d3a56853c8d` |
| `VO_Bark_SquadEngageLowRight_B_01_dry.wav` | `a59858d8352e209dec568e8e8e8320945fc445077499d819b529bde70bfc6819` |
| `VO_Bark_SquadEngageLowRight_B_01_radio.wav` | `e80d2ce55edaed5192306d8f0c26afe8b8fb93f2e4c12a31ea632875f6addef2` |
| `VO_Bark_SquadEngageLowRight_B_02_dry.wav` | `746938a98cb634b8315928969e3484f52542b52d5d77abe73526882158c9cc50` |
| `VO_Bark_SquadEngageLowRight_B_02_radio.wav` | `ae0f8d9398fa49e2336404ca5d9e70845c1783654486416e440692e8380757c6` |
| `VO_Bark_SquadEngageRight_A_01_dry.wav` | `67d6efda8ea993d8171414ccfec753bd35f81d3eed248003cc38f0d642182902` |
| `VO_Bark_SquadEngageRight_A_01_radio.wav` | `641f6c5e7c0c3e7771b3fd54523f9ea3b73cbb4fdc9999e6022627f627549ebe` |
| `VO_Bark_SquadEngageRight_A_02_dry.wav` | `016edf9372a5990aa17d9fb5d88a7f14d2585ff332f49e09433dde353bc8eefa` |
| `VO_Bark_SquadEngageRight_A_02_radio.wav` | `9134fe6edbd17462ba7df9baa46170ea210418208108b63fb81db4ca94992b8e` |
| `VO_Bark_SquadEngageRight_B_01_dry.wav` | `aaadf5ece54ea876abb62860866e6c9a78762a661bea1b3efa7c1036c552a1df` |
| `VO_Bark_SquadEngageRight_B_01_radio.wav` | `eca5eb29503def4b19f94c8299d90c9db799cc33e32d06bde2703f79edd82721` |
| `VO_Bark_SquadEngageRight_B_02_dry.wav` | `4f8f99427dbc98c01d0660e491e7a3bde129656a7709a12773f6b021280825be` |
| `VO_Bark_SquadEngageRight_B_02_radio.wav` | `ea455b4a9e86cce1fdb1b1afe5b8926d8bb08885ea2514e60319777115f33f72` |
| `VO_Bark_SquadFocusAck_01_dry.wav` | `f8df5f19ee37c40d03dd9842e2cc5096aff9b806b4b59715172c6341efb85a10` |
| `VO_Bark_SquadFocusAck_01_radio.wav` | `aa18450a49ec17a2f34fcfe45b89b221da913b3c5b69c455c8603a894ee5f5b9` |
| `VO_Bark_SquadFocusAck_02_dry.wav` | `f7026ccf33525a2ab32331829a349224ec88dbc06e97b68487705c14433d363f` |
| `VO_Bark_SquadFocusAck_02_radio.wav` | `8dce4084f2acfd641e5a9a47980af6532a738369ca65d83183e6646020fc8775` |
| `VO_Bark_SquadLowHull_A_01_dry.wav` | `5b62795574e6d7291602b48d72ea6061e3fb279ff63ffccb7a4b7cad760c45bc` |
| `VO_Bark_SquadLowHull_A_01_radio.wav` | `e9fc202592b34aa1aa355b6476922a2593aaa9621622d63e08cc5d12fff848f8` |
| `VO_Bark_SquadLowHull_A_02_dry.wav` | `3b8af87dd1c5b01265b71640ace218ffed30d047fbd660cb273086803dfa3cdb` |
| `VO_Bark_SquadLowHull_A_02_radio.wav` | `e32e80f3dd8c352e304df2e9664ba051bda185069a71e3b30340ae2aacead725` |
| `VO_Bark_SquadLowHull_B_01_dry.wav` | `e56d3dd68ada7c3f8b05c5f28e96649a1379188b7af45da2239e2dfb747b0012` |
| `VO_Bark_SquadLowHull_B_01_radio.wav` | `c0a292e2556e361e4ca55fe7dba1e3dcfe558952a3ee076b7031e69a3e30ad15` |
| `VO_Bark_SquadLowHull_B_02_dry.wav` | `f1dffe058d165ecad85c7aa05b185c0a7d453f327f64abf4e33d86e056b024d8` |
| `VO_Bark_SquadLowHull_B_02_radio.wav` | `bb5292cbb5e6bac0e5561e49bb198e0bb161f52445fd25dfd68f64831a29971b` |
| `VO_Bark_SquadRespawn_01_dry.wav` | `ebe5b62c74878887face5762074d7327324bcca576e01f028945477ebf03617f` |
| `VO_Bark_SquadRespawn_01_radio.wav` | `a4a05d5862d6bfe19ae5c157518d11a7a3969fc226a9f7595b4bde0eeff549f4` |
| `VO_Bark_SquadRespawn_02_dry.wav` | `83055caa6a1683d5b8f018df3e66307910c8b2344472f25bab970c30939a64e3` |
| `VO_Bark_SquadRespawn_02_radio.wav` | `8e578d7a583997c9b23b6a86584b8eecc9778867aa870f136c4c30e154b4159c` |
| `VO_Bark_SquadShipDown_A_01_dry.wav` | `763a9c41fab3d7f7fb4e3e1da2be2869d27a4df64800168ac482fcee4893c537` |
| `VO_Bark_SquadShipDown_A_01_radio.wav` | `20be0a6ed04afa4296b82b09b4b875ef5772d29ca82e162e82df1ac96af3d428` |
| `VO_Bark_SquadShipDown_A_02_dry.wav` | `e066fe8858a77fdfb743b35470e68412d6c292c30664dc76b923ac87e3e62274` |
| `VO_Bark_SquadShipDown_A_02_radio.wav` | `066ecc58772aff3010b0f55dfe63103b84409d3100ee558c1df776b757e8c953` |
| `VO_Bark_SquadShipDown_B_01_dry.wav` | `7afef9a809dfc30f6dc5b912d1e454824641c4f03f4cf0d71cb0f9f33d4ec6e5` |
| `VO_Bark_SquadShipDown_B_01_radio.wav` | `d5e26ab4b89c1c61f5478b9a507a6182416309a9271392303d6ad3ae74f1afaf` |
| `VO_Bark_SquadShipDown_B_02_dry.wav` | `3256bcc11b00d3bc909b0a501e4ccd4a3194d4d3c3c2bb2bad76c0c5d92c1b89` |
| `VO_Bark_SquadShipDown_B_02_radio.wav` | `1b2f5e1f18ecc92b17c0c3a49fb80e0ce1dd1bf910eb428c95969b95a409facd` |
| `VO_Bark_SquadSwitchIn_A_01_dry.wav` | `50527b331b33784330129fbd16a9e77b15fb3067d681e69fddf187965ee60dab` |
| `VO_Bark_SquadSwitchIn_A_01_radio.wav` | `232abdfc0cac416ebd498df67fcb817e03edd2b7348ba96f78a823b44653ae27` |
| `VO_Bark_SquadSwitchIn_A_02_dry.wav` | `9b420cc97ac321a1afc200ce5c26ab85086fbefb7211dc17d65c8e2ab8f34054` |
| `VO_Bark_SquadSwitchIn_A_02_radio.wav` | `eb7d6ae16a99c05d3fbd475705a28c233ee2b033607467fac8c3a87daf93e8f8` |
| `VO_Bark_SquadSwitchIn_B_01_dry.wav` | `7ace86b133730b0a2ffeb063e71446767677bd5c9e0dad5f21afe1ee143e0232` |
| `VO_Bark_SquadSwitchIn_B_01_radio.wav` | `c484bb637a87334c4526a131e17f22ac5cb05d22c93e9a647c095375b693e84b` |
| `VO_Bark_SquadSwitchIn_B_02_dry.wav` | `d5f1b1486feea81e2034741d878a4f7941de04d6b66d1a56a710d93efe32094c` |
| `VO_Bark_SquadSwitchIn_B_02_radio.wav` | `da0b772969dd49f1f7039f6bbf909d8f34801ecb0715525db747160c69cd13eb` |
| `VO_Bark_SquadSwitchStrike_01_dry.wav` | `5190ee20bb9aedd166952ea375a20cd74b12fd6498cd060ab6d2e8ac45d896b8` |
| `VO_Bark_SquadSwitchStrike_01_radio.wav` | `54be6751ff4925b9b57f1f91270e069d4f76da89177dde3665de9da28ae5a23d` |
| `VO_Bark_SquadSwitchStrike_02_dry.wav` | `da4c2ae6fb3eae9e32c92b696595be1e9d2f605d76b852add8e8343a0e85b893` |
| `VO_Bark_SquadSwitchStrike_02_radio.wav` | `2579dcebdf7cb069c4284d2a6431f0359b7bbf128855cfa6a9df3bdb036f31c1` |
| `VO_Bark_SquadWingmanKill_A_01_dry.wav` | `a4aa1bbcbd472fcb796a85dfbd6ac114e32048b2fece1249cda24b10381cb2a9` |
| `VO_Bark_SquadWingmanKill_A_01_radio.wav` | `275a7aa15efd4b772737a428d116e05a49edbc56ecad5d3304c645ba7ab3d75d` |
| `VO_Bark_SquadWingmanKill_A_02_dry.wav` | `eb71bae1f06117f3e0762103718cd0cb348a42d469d15576fb3439887f90e761` |
| `VO_Bark_SquadWingmanKill_A_02_radio.wav` | `5ac00381b3018614cf96c0c10e8d4fc2014a7e31ef9bc0f90d8fed698ea435c4` |
| `VO_Bark_SquadWingmanKill_B_01_dry.wav` | `4c447378da5c10f4e56170378d834544d9d141c553fc19a782479ed362a294d0` |
| `VO_Bark_SquadWingmanKill_B_01_radio.wav` | `271f686c007abe123e4ea675250d543e6e443f4816fa1689fc702e3b6a19eb42` |
| `VO_Bark_SquadWingmanKill_B_02_dry.wav` | `3381d86f955d6c8bb1884aaf690d5c8b665300449087e1cbd6aadcd4f3439ce7` |
| `VO_Bark_SquadWingmanKill_B_02_radio.wav` | `88e617b18b765302a20d2a4ca1da9818824068ca855bca076c504e287bb52766` |
| `VO_Bark_SquadWipe_01_dry.wav` | `2746b9ffa8cb08386cb8a4559245a73b75ee87a0247bacb9ac2caa0829f616a5` |
| `VO_Bark_SquadWipe_01_radio.wav` | `446a70a7d7cd9e593548ce73d426b1beca96f4c4ca9becbc5e6373f75739de0f` |
| `VO_Bark_SquadWipe_02_dry.wav` | `e0d6ba1cd23654866bed401ff123443314c8c37678b1f8679635b47f4f9aa95a` |
| `VO_Bark_SquadWipe_02_radio.wav` | `0e04e1aa39cb0a2f6579d8b708513f5a2f6b8f05a4b9da131fc72eba3d59cc9c` |
| `VO_Bark_TakeOff_01_dry.wav` | `87cc58f602caa79211919c1ceb61a3906d3447a0aae4e389dc44ebb8cbbc0cbb` |
| `VO_Bark_TakeOff_01_radio.wav` | `003579c657ffd3379a2c28c9036925f88a704ef3766543b7f5047c3c75fea760` |
| `VO_Bark_TakeOff_02_dry.wav` | `8c3169fed7b03083a77d227dd2e5870e141394019035be357a78117230d3df86` |
| `VO_Bark_TakeOff_02_radio.wav` | `7af3104b14d1dcc2518321f9a114366fd648240e3ff034ec6600b508e314dad2` |
| `VO_Bark_TargetShielded_01_dry.wav` | `756054824197880118aa970339cee546d2d9414847741a611a028178a8173862` |
| `VO_Bark_TargetShielded_01_radio.wav` | `24b1b57ceac98079e8e411a501e7bbe486421d8baff862a35b16d59bb4aa0743` |
| `VO_Bark_TargetShielded_02_dry.wav` | `94045d1d52014c192c6093ace51e405f5f43346034256490fd30363f9bda0652` |
| `VO_Bark_TargetShielded_02_radio.wav` | `94a7081e3b985a57bb4dbd52def6ae35f9f4db9db0b8b287fdf5a0db5c3b8fe9` |
| `VO_Bark_WardenPhase2_01_dry.wav` | `8a4877d219beccbeb6d6ba91af3f4fde7c71287cdc6f76361a3365c0519f5798` |
| `VO_Bark_WardenPhase2_01_radio.wav` | `96559f41a390563a02eaffa0b150994fe062c69684b5e657dca0c91d6197de66` |
| `VO_Bark_WardenPhase2_02_dry.wav` | `82b6b2858cdb50e607488787db70204b1ee655010ed650fb96f7f61bf434e24a` |
| `VO_Bark_WardenPhase2_02_radio.wav` | `c486430fec42f9c90eafd273614f2e422f80972dab3c767cb78ad0dd72fe0c9b` |
| `VO_Bark_WaveCleared_01_dry.wav` | `b6e4e84e3fba60abf5cc2b7ee068f7902485d5f50225f38cef1b3d5f54b71e39` |
| `VO_Bark_WaveCleared_01_radio.wav` | `8efeb2b3b43e1440c619e3301e2ad03085e84baa95bdafa5cadaa652c7436d04` |
| `VO_Bark_WaveCleared_02_dry.wav` | `24807e8e03a6c515ba773b82bf9dc940679b677c1a370d5d34baaeea9e06c0c3` |
| `VO_Bark_WaveCleared_02_radio.wav` | `5dde6ec7d43c976050d9b920c742f6faa48d209d56b742d8742c87856282e9eb` |
| `VO_Bark_WaveIncoming_01_dry.wav` | `e5b99d54a4c6f43aff603a98f89d10419d386894fd598fc2cede27ea36c05c8b` |
| `VO_Bark_WaveIncoming_01_radio.wav` | `6955d9561f82f7f74e54d76f8e763c9b5b6098c8f3c1f97b3734ba41fea77006` |
| `VO_Bark_WaveIncoming_02_dry.wav` | `0c29d392e18edd612031afe5422db583f1e56fafd9679b8443f064b8e1c73628` |
| `VO_Bark_WaveIncoming_02_radio.wav` | `24a4d65f772cd32f8b3394a511f2e76914c753c1bc44130931f6434233266f54` |
| `VO_Bark_WeaponHot_01_dry.wav` | `ade271922945042ee5d121219fbf1ec8d758d17a3f7cec7cae7f9aa98d8cbdef` |
| `VO_Bark_WeaponHot_01_radio.wav` | `17b95a88e34c8e274fed4b357aa932ca58df69f8097765d2b70b1503c80a2ad1` |
| `VO_Bark_WeaponHot_02_dry.wav` | `9521e7dabcbeefbdfb49260a400da8935918be56b4422bac3a11e46aa14a331d` |
| `VO_Bark_WeaponHot_02_radio.wav` | `6b099b52b02518432be60dd8e8ebf41376058dba5fd0e3fbcefbe18e569190fa` |

## Skipped / out of scope

- `O_Unknown_Dont` / `F_FutureWren_Dont` — shared master from PR #78 take 04 radio.
- `C8_Wren_Order` — present-Wren audition in PR #93 (Nicholas has not picked a master yet).
- Retired N-087 ids and Control/Unknown lines that only mention Wren are not in this set.

Working-draft wording is recorded as it stands in `dialogs.md` / `docs/story/transmissions.md`
on the day of generation. Nicholas may later replace a line; the take is still useful as a temp.

