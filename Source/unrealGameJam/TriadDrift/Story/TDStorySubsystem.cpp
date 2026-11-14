// Triad Drift - story transmissions (see header and storyline.md). Owner: Teammate 3.

#include "TriadDrift/Story/TDStorySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "NiagaraComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Story/TDStoryFX.h"

#define LOCTEXT_NAMESPACE "TDStory"

#if WITH_DEV_AUTOMATION_TESTS
TWeakObjectPtr<const UTDTransmissionSet> UTDStorySubsystem::SetOverrideForTests;
#endif

namespace TDStory
{
	static FTDTransmission E(const TCHAR* Id, ETDSpeaker Speaker, const FText& Text, ETDTransmissionStyle Style = ETDTransmissionStyle::Clean,
		const TCHAR* SharedSound = nullptr)
	{
		FTDTransmission T;
		T.Id = FName(Id);
		T.Speaker = Speaker;
		T.Text = Text;
		T.Style = Style;
		T.SharedSound = SharedSound ? FName(SharedSound) : NAME_None;
		return T;
	}

	/**
	 * Placeholder lines, one per beat (storyline.md §5), in story order. They state the beat plainly; Nicholas's
	 * DA_Story_Transmissions replaces them by id. Rule (§4, §8): nothing before act 4 says "future", "time" or "us".
	 */
	static const TArray<FTDTransmission>& Defaults()
	{
		using S = ETDSpeaker;
		using St = ETDTransmissionStyle;
		static const TArray<FTDTransmission> Table = {
			// Opening: the intro's narration (read by UTDIntroDirector, not fired as transmissions).
			E(TEXT("Intro_N0"), S::Control, LOCTEXT("Intro_N0", "A ship appeared without warning. It answers nothing.")),
			E(TEXT("Intro_N1"), S::Control, LOCTEXT("Intro_N1", "Its machines descend on our worlds, searching for something.")),
			E(TEXT("Intro_N2"), S::Control, LOCTEXT("Intro_N2", "Triad is deployed. Wren. Kato. Moss.")),
			E(TEXT("Intro_N3"), S::Control, LOCTEXT("Intro_N3", "Identify the ship. Learn what it wants. Report back.")),
			E(TEXT("Intro_N4"), S::Control, LOCTEXT("Intro_N4", "And out of the dark... the Null Armada.")),
			// Pilot traits (storyline.md §3): the second line of the pilot name cards (intro and switch). Not fired.
			E(TEXT("Trait_Wren"), S::Wren, LOCTEXT("Trait_Wren", "TRICKSTER  ·  BOLD  ·  INSTINCTIVE")),
			E(TEXT("Trait_Kato"), S::Kato, LOCTEXT("Trait_Kato", "INTERCEPTOR  ·  ANALYTICAL  ·  CAUTIOUS")),
			E(TEXT("Trait_Moss"), S::Moss, LOCTEXT("Trait_Moss", "HEAVY  ·  STEADY  ·  GOOD-HUMOURED")),
			// First Contact (docs/story/First-Contact-Cinematic-Brief.md): the opening, ~2:20, re-shot once the art is in.
			// Draft lines from the brief; Nicholas's words replace them.
			E(TEXT("O_PatrolSeven"), S::PatrolSeven, LOCTEXT("O_PatrolSeven", "Command, there's something out here."), St::Static),
			E(TEXT("O_Command_Orders"), S::Command, LOCTEXT("O_Command_Orders", "Triad, identify the ship and report. Recon only. Do not engage.")),
			E(TEXT("O_Wren_HowBad"), S::Wren, LOCTEXT("O_Wren_HowBad", "Three ships, one mystery. How bad could it be?")),
			E(TEXT("O_Kato_StopSaying"), S::Kato, LOCTEXT("O_Kato_StopSaying", "Please stop saying that.")),
			E(TEXT("O_Moss_Especially"), S::Moss, LOCTEXT("O_Moss_Especially", "Especially before we've seen it.")),
			E(TEXT("O_Wren_BigShip"), S::Wren, LOCTEXT("O_Wren_BigShip", "That's all one ship?")),
			E(TEXT("O_Kato_NoSignature"), S::Kato, LOCTEXT("O_Kato_NoSignature", "No engine readings. And they're not answering.")),
			E(TEXT("O_Moss_Message"), S::Moss, LOCTEXT("O_Moss_Message", "A message?")),
			E(TEXT("O_Kato_Handshake"), S::Kato, LOCTEXT("O_Kato_Handshake", "Wait—a handshake. Encrypted. It's gone.")),
			E(TEXT("O_Wren_WhyNotFiring"), S::Wren, LOCTEXT("O_Wren_WhyNotFiring", "What? Why didn't it shoot us?")),
			E(TEXT("O_Kato_Contacts"), S::Kato, LOCTEXT("O_Kato_Contacts", "Unknown. Multiple contacts are turning toward us.")),
			E(TEXT("O_Command_Disengage"), S::Command, LOCTEXT("O_Command_Disengage", "Triad, pull out. Now.")),
			E(TEXT("O_Unknown_Dont"), S::Unknown, LOCTEXT("O_Unknown_Dont", "Don't-"), St::Static, TEXT("Dont")),
			E(TEXT("O_Wren_Break"), S::Wren, LOCTEXT("O_Wren_Break", "Break formation!")),
			E(TEXT("O_Command_Evade"), S::Command, LOCTEXT("O_Command_Evade", "Pilot, incoming fire! Evade now!")),
			// Gameplay Sequence 01, the tutorial battle (storyline.md §13): the calls per phase. Drafts.
			E(TEXT("TB_Kato_Marking"), S::Kato, LOCTEXT("TB_Kato_Marking", "Keep moving. I've marked one for you.")),
			E(TEXT("TB_Moss_ThreeShips"), S::Moss, LOCTEXT("TB_Moss_ThreeShips", "We've still got three ships. Use them.")),
			E(TEXT("TB_Wren_Bigger"), S::Wren, LOCTEXT("TB_Wren_Bigger", "Gunship. Don't sit in front of it!")),
			E(TEXT("TB_Command_FallBack"), S::Command, LOCTEXT("TB_Command_FallBack", "Triad, withdraw. Take the marked corridor.")),
			E(TEXT("TB_Kato_BreakingOff"), S::Kato, LOCTEXT("TB_Kato_BreakingOff", "They're breaking off.")),
			E(TEXT("TB_Escaped"), S::Command, LOCTEXT("TB_Escaped", "You're clear, Triad. Hold at the rally point.")), // retired (Nick's Lead, #29): T4 still fires it at the escape line; swallowed, never aired
			// Sequence 02, Aftermath (Story/Triad_Drift_Sequence_02_Aftermath.docx, storyline.md §12). Drafts. No "Don't-" here.
			E(TEXT("S2_Moss_Accounted"), S::Moss, LOCTEXT("S2_Moss_Accounted", "Well, we're not dead. I'd say that's a win.")),
			// Approved by Nicholas (issue #29). Delivery: sarcastic, dry, deadpan.
			E(TEXT("S2_Wren_WelcomeParty"), S::Wren, LOCTEXT("S2_Wren_WelcomeParty", "Is it over? I think I blacked out...")),
			E(TEXT("S2_Kato_NotPursuing"), S::Kato, LOCTEXT("S2_Kato_NotPursuing", "Wait. Look behind us.")),
			E(TEXT("S2_Kato_Vectors"), S::Kato, LOCTEXT("S2_Kato_Vectors", "Most of their fleet never even joined the fight.")),
			E(TEXT("S2_Wren_WhatDoing"), S::Wren, LOCTEXT("S2_Wren_WhatDoing", "Would you rather they had?")),
			E(TEXT("S2_Kato_Planetside"), S::Kato, LOCTEXT("S2_Kato_Planetside", "Heading planetside. Every group. Same destination.") /* R020 held: the draft's "it's us" trips the no-early-twist guard (Lead) */),
			E(TEXT("S2_Moss_InTheWay"), S::Moss, LOCTEXT("S2_Moss_InTheWay", "So we got in the way.")),
			E(TEXT("S2_Command_Report"), S::Command, LOCTEXT("S2_Command_Report", "Triad, answer me. We lost you near that ship."), St::Static),
			E(TEXT("S2_Wren_Hostile"), S::Wren, LOCTEXT("S2_Wren_Hostile", "Fighters. A lot of them. All three of us got out.")),
			E(TEXT("S2_Kato_Coordinated"), S::Kato, LOCTEXT("S2_Kato_Coordinated", "They're sending the rest down to Verdance. They didn't launch them for us.")),
			E(TEXT("S2_Command_Outpost"), S::Command, LOCTEXT("S2_Command_Outpost", "An outpost there sent a distress beacon. Machines in the survey zone. Then it went quiet.")),
			E(TEXT("S2_Moss_FindOut"), S::Moss, LOCTEXT("S2_Moss_FindOut", "Anyone answer the beacon?")),
			E(TEXT("S2_Command_Orders"), S::Command, LOCTEXT("S2_Command_Orders", "No response. Find the outpost and recover its survey records. Stay clear of the mothership.")),
			E(TEXT("S2_Wren_FormUp"), S::Wren, LOCTEXT("S2_Wren_FormUp", "All right. Verdance. Stay on me.")),
			E(TEXT("S2_Kato_Recorders"), S::Kato, LOCTEXT("S2_Kato_Recorders", "Leave the recorders on. All channels.")),
			// Act 1: First Contact, the Watcher (waves 1-2).
			E(TEXT("A1_HailNoAnswer"), S::Kato, LOCTEXT("A1_HailNoAnswer", "Hailing on every band. Nothing. It isn't even listening.")),
			E(TEXT("A1_TheySearch"), S::Moss, LOCTEXT("A1_TheySearch", "They're not holding ground. They scan, they sweep, they move on.")),
			E(TEXT("A1_FieldUnreachable"), S::Wren, LOCTEXT("A1_FieldUnreachable", "That shimmer around the ship: some kind of field. We can't get near it.")),
			E(TEXT("A1_TrailToVerdance"), S::Control, LOCTEXT("A1_TrailToVerdance", "Their landers are converging on Verdance. Follow the trail.")),
			// Act 2: Verdance, the Voices.
			E(TEXT("A2_DeviceFound"), S::Kato, LOCTEXT("A2_DeviceFound", "Signal-analysis gear. They were listening to the machines.")),
			E(TEXT("A2_Voices"), S::Unknown, LOCTEXT("A2_Voices", "Maintain the sequence."), St::Static),
			E(TEXT("A2_TrailToCinder"), S::Moss, LOCTEXT("A2_TrailToCinder", "Human voices, under their signal. The data points to Cinder.")),
			// Gameplay 02, Verdance Arrival (storyline.md §14, reduced form §18). Drafts.
			E(TEXT("A2_ConvoyScanned"), S::Kato, LOCTEXT("A2_ConvoyScanned", "Those transports are carrying survey gear. Not troops.")),
			E(TEXT("A2_TransponderSighted"), S::Moss, LOCTEXT("A2_TransponderSighted", "There's a transponder down there. It keeps flickering.")),
			E(TEXT("G2_Wren_Geology"), S::Wren, LOCTEXT("G2_Wren_Geology", "They brought all this just to dig?")),
			E(TEXT("G2_Command_Archive"), S::Command, LOCTEXT("G2_Command_Archive", "Twelve hours since their last transmission. Recover the archive. Keep the station intact.")),
			E(TEXT("G2_Kato_OneLine"), S::Kato, LOCTEXT("G2_Kato_OneLine", "Look where they're scanning. One line, under the ground.")),
			E(TEXT("G2_Moss_LightsOn"), S::Moss, LOCTEXT("G2_Moss_LightsOn", "Power's still on. You'd think someone would answer.")),
			// Cinematic 03, Beneath the Surface (§15; 40-60 s, voice over the room). The three pulses are sound and light, not a line.
			E(TEXT("C3_Moss_NoBodies"), S::Moss, LOCTEXT("C3_Moss_NoBodies", "No bodies. No sign of a fight, either.")),
			E(TEXT("C3_Wren_Survey"), S::Wren, LOCTEXT("C3_Wren_Survey", "Kato. Tell me what we've got.")),
			E(TEXT("C3_Kato_Artificial"), S::Kato, LOCTEXT("C3_Kato_Artificial", "Those are people. Listen. And the thing under the station—someone built it.")),
			E(TEXT("C3_Wren_NotThePlanet"), S::Wren, LOCTEXT("C3_Wren_NotThePlanet", "They're digging for that?")),
			E(TEXT("C3_Kato_Beneath"), S::Kato, LOCTEXT("C3_Kato_Beneath", "Every survey line runs over it.")),
			E(TEXT("C3_Wren_Answered"), S::Wren, LOCTEXT("C3_Wren_Answered", "That was our radio. What did you send?")),
			E(TEXT("C3_Kato_DidntSend"), S::Kato, LOCTEXT("C3_Kato_DidntSend", "Nothing. I didn't touch it.")),
			E(TEXT("C3_Kato_CinderMatch"), S::Kato, LOCTEXT("C3_Kato_CinderMatch", "Cinder. Same readings at a mining site. Hang on, I'm taking a copy—")),
			E(TEXT("C3_Moss_Company"), S::Moss, LOCTEXT("C3_Moss_Company", "We've got company. Coming in fast.")),
			E(TEXT("C3_Wren_HoldThem"), S::Wren, LOCTEXT("C3_Wren_HoldThem", "Keep copying. We'll cover the station.")),
			// Gameplay 03, Facility Defense (§16; reduced: a 60-90 s transfer, three waves, the carry-back).
			// T5 fires these three from the transfer (start, the Heavy's wave, done).
			E(TEXT("G3_TransferStart"), S::Kato, LOCTEXT("G3_TransferStart", "Transfer's moving. If they destroy this station, we lose everything.")),
			E(TEXT("G3_Disruption"), S::Kato, LOCTEXT("G3_Disruption", "The copy's stalled. Something's interfering.")),
			E(TEXT("G3_DataSecured"), S::Kato, LOCTEXT("G3_DataSecured", "Got it. Recordings, coordinates—the lot. Let's get out.")),
			E(TEXT("G3_Moss_NobodyNear"), S::Moss, LOCTEXT("G3_Moss_NobodyNear", "Heard you. Keep it moving.")),
			E(TEXT("G3_Wren_LeftApproach"), S::Wren, LOCTEXT("G3_Wren_LeftApproach", "Left side! They're going for the console.")),
			E(TEXT("G3_Kato_AlmostThere"), S::Kato, LOCTEXT("G3_Kato_AlmostThere", "Archive almost complete. Keep them back.")),
			E(TEXT("G3_Command_Extraction"), S::Command, LOCTEXT("G3_Command_Extraction", "Your way out is marked. Bring that device back.")),
			// Cinematic 04, Verdance Departure (§17, §25: transmissions over the take-off; no shielding).
			E(TEXT("C4_Moss_Trouble"), S::Moss, LOCTEXT("C4_Moss_Trouble", "All that for a recorder full of voices. None of them answering.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C4_Wren_Badly"), S::Wren, LOCTEXT("C4_Wren_Badly", "They wanted that device and its records. We need to know why.")),
			E(TEXT("C4_Kato_SameSignature"), S::Kato, LOCTEXT("C4_Kato_SameSignature", "They weren't searching at random. These two sites have the same geometric signature.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C4_Wren_WhatConnects"), S::Wren, LOCTEXT("C4_Wren_WhatConnects", "Verdance and Cinder. What connects them?")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C4_Kato_SurveyStation"), S::Kato, LOCTEXT("C4_Kato_SurveyStation", "I don't know. Cinder's mining complex has matching readings. They're extracting stabilising crystals there.")),
			E(TEXT("C4_Command_Proceed"), S::Command, LOCTEXT("C4_Command_Proceed", "Proceed to Cinder. Investigate the mining complex and recover the stabilising crystals. Compare them with the signal-analysis records. Maintain reconnaissance priority.")),
			E(TEXT("C4_Moss_Volcanoes"), S::Moss, LOCTEXT("C4_Moss_Volcanoes", "Volcanoes. Great. I was getting tired of the jungle.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C4_Wren_SetCourse"), S::Wren, LOCTEXT("C4_Wren_SetCourse", "Triad, set course for Cinder.")), // retired (N-087: kept for the record, never fired)
			// Act 3: Cinder, the Pattern.
			E(TEXT("A3_CrystalsFound"), S::Kato, LOCTEXT("A3_CrystalsFound", "Samples secured. EMP prototype recovered. Lancer's pulse is online.")),
			E(TEXT("A3_PassageOpen"), S::Kato, LOCTEXT("A3_PassageOpen", "Anchor's down. Route's prepared, but the field's still closed.")),
			// Gameplay 04, Cinder Approach (§19, §25). Waves 3-4 are the interception; the transports ignore the squad.
			E(TEXT("G4_Command_FollowCoordinates"), S::Command, LOCTEXT("G4_Command_FollowCoordinates", "Find the excavation before you engage.")),
			E(TEXT("G4_Kato_Transports"), S::Kato, LOCTEXT("G4_Kato_Transports", "Watch the transports. Same landing point.")),
			E(TEXT("G4_Moss_NotShooting"), S::Moss, LOCTEXT("G4_Moss_NotShooting", "They've got us in sight. Still not interested.")),
			E(TEXT("G4_Kato_Cargo"), S::Kato, LOCTEXT("G4_Kato_Cargo", "Digging equipment. Heavy loads.")),
			E(TEXT("G4_Wren_StartThere"), S::Wren, LOCTEXT("G4_Wren_StartThere", "Then we follow them down.")),
			E(TEXT("G4_Moss_SolidGround"), S::Moss, LOCTEXT("G4_Moss_SolidGround", "I'm putting down well clear of the lava.")),
			E(TEXT("G4_Kato_TerminalAhead"), S::Kato, LOCTEXT("G4_Kato_TerminalAhead", "There. On the terminal. I've seen that shape.")),
			// Cinematic 05, The Pattern (§20, §25: 40-50 s at the terminal). The ONE echo; lines approved by Nicholas.
			E(TEXT("C5_Kato_SameConstruction"), S::Kato, LOCTEXT("C5_Kato_SameConstruction", "Different worlds. Same construction. They knew where to dig.")),
			E(TEXT("C5_Unknown_SameConstruction"), S::Unknown, LOCTEXT("C5_Unknown_SameConstruction", "Same construction."), St::Echo),
			E(TEXT("C5_Kato_Feedback"), S::Kato, LOCTEXT("C5_Kato_Feedback", "Could be feedback. Keep the recording.")),
			E(TEXT("C5_Kato_OriginCore"), S::Kato, LOCTEXT("C5_Kato_OriginCore", "The records call it the Origin Core. These look like components—or keys—to a temporal device.")),
			E(TEXT("C5_Moss_Sample"), S::Moss, LOCTEXT("C5_Moss_Sample", "How much of this stuff do we need?")),
			E(TEXT("C5_Wren_Guarding"), S::Wren, LOCTEXT("C5_Wren_Guarding", "Let's get past the thing guarding it first.")),
			E(TEXT("C5_Kato_Anchor"), S::Kato, LOCTEXT("C5_Kato_Anchor", "That's the field anchor. We'll need it shut down as well.")),
			E(TEXT("C5_Command_Recover"), S::Command, LOCTEXT("C5_Command_Recover", "Recover what you need and get clear. Report before you approach the Dominion.")),
			E(TEXT("C5_Wren_Route"), S::Wren, LOCTEXT("C5_Wren_Route", "Fine. We start with the cache.")),
			// Gameplay 05, Cinder Excavation (§21, §25). The cache line is A3_CrystalsFound, the anchor line A3_PassageOpen.
			E(TEXT("G5_Wren_ClearRoute"), S::Wren, LOCTEXT("G5_Wren_ClearRoute", "Cache is behind it. Stay out of its reach.")),
			E(TEXT("G5_Moss_Guardian"), S::Moss, LOCTEXT("G5_Moss_Guardian", "It's not leaving the crystals. We'll have to get past it.")),
			E(TEXT("G5_Command_AnchorMarked"), S::Command, LOCTEXT("G5_Command_AnchorMarked", "Anchor's marked. Disable it, then head for extraction.")),
			E(TEXT("G5_Wren_BackToShips"), S::Wren, LOCTEXT("G5_Wren_BackToShips", "We've got what we came for. Back to the ships.")),
			// Cinematic 06, The Path to Dominion (§23): transmissions only, six over the Cinder take-off, two as wave 5 forms.
			E(TEXT("C6_Moss_SamplesAboard"), S::Moss, LOCTEXT("C6_Moss_SamplesAboard", "Samples aboard. I'm ready for somewhere with fewer volcanoes.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C6_Kato_GuardLine"), S::Kato, LOCTEXT("C6_Kato_GuardLine", "The anchor is down. The approach is prepared. We still have to clear the guard line.")),
			E(TEXT("C6_Kato_FourGenerators"), S::Kato, LOCTEXT("C6_Kato_FourGenerators", "The crystals can stabilise the approach. Four generators are still sustaining its defensive shield.")),
			E(TEXT("C6_Wren_FindAWayInside"), S::Wren, LOCTEXT("C6_Wren_FindAWayInside", "Then we take down the generators and find a way inside.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C6_Command_FindOutWhy"), S::Command, LOCTEXT("C6_Command_FindOutWhy", "They're collecting parts of the Origin Core. We need to know why. Disable the outer defences and secure access. Keep your channels recording.")),
			E(TEXT("C6_Wren_Understood"), S::Wren, LOCTEXT("C6_Wren_Understood", "Understood. We find out what they intend to do with it.")), // retired (N-087: kept for the record, never fired)
			E(TEXT("C6_Moss_WholeFleet"), S::Moss, LOCTEXT("C6_Moss_WholeFleet", "They've put a whole fleet across our route.")),
			E(TEXT("C6_Wren_StayTogether"), S::Wren, LOCTEXT("C6_Wren_StayTogether", "Stay together. Clear the guard line, then the generators.")),
			// Gameplay 06, The Dominion Assault (§24). An access mission: nothing here orders the ship or reactor destroyed.
			E(TEXT("G6_Wren_ClearCarriers"), S::Wren, LOCTEXT("G6_Wren_ClearCarriers", "Take the carriers. Don't let them keep launching.")),
			E(TEXT("G6_Kato_MarkingTargets"), S::Kato, LOCTEXT("G6_Kato_MarkingTargets", "Four shield generators. Targets marked.")),
			E(TEXT("G6_Moss_TurretsCharging"), S::Moss, LOCTEXT("G6_Moss_TurretsCharging", "Hull turrets. Keep moving across them.")),
			E(TEXT("G6_Kato_ShieldWeakening"), S::Kato, LOCTEXT("G6_Kato_ShieldWeakening", "Generator down. Shield's weakening.")),
			E(TEXT("G6_Command_ShieldCollapsed"), S::Command, LOCTEXT("G6_Command_ShieldCollapsed", "Shield's down. Docking route marked.")),
			E(TEXT("G6_Wren_CoverTheBay"), S::Boarder, LOCTEXT("G6_Wren_CoverTheBay", "Keep that bay covered. We need a way back out.")),
			// Clue fragments as generators fall: less distorted each time, no identified speaker (placeholders for Nicholas).
			E(TEXT("G6_Clue1"), S::Unknown, LOCTEXT("G6_Clue1", "…hold formation… vector two…"), St::Static),
			E(TEXT("G6_Clue2"), S::Unknown, LOCTEXT("G6_Clue2", "…Triad… wait for authentication…"), St::Echo),
			// Act 4: the Dominion. The assault is still before the reveal (only the interior reveals: §25).
			E(TEXT("A4_GeneratorDown1"), S::Control, LOCTEXT("A4_GeneratorDown1", "Generator one is down. Their shield is flickering.")),
			E(TEXT("A4_GeneratorDown2"), S::Kato, LOCTEXT("A4_GeneratorDown2", "Two down. The shield is losing shape.")),
			E(TEXT("A4_GeneratorDown3"), S::Wren, LOCTEXT("A4_GeneratorDown3", "Three! One more and it's open.")),
			E(TEXT("A4_GeneratorDown4"), S::Moss, LOCTEXT("A4_GeneratorDown4", "That's the last one. The shield is gone.")),
			// Cinematic 07, Boarding the Dominion (storyline.md 26): voices over the docking and the hangar. Before the reveal.
			E(TEXT("C7_Moss_WayOut"), S::Moss, LOCTEXT("C7_Moss_WayOut", "We'll keep a way out clear.")),
			E(TEXT("C7_Kato_Layout"), S::Kato, LOCTEXT("C7_Kato_Layout", "That layout. We use the same arrangement.")),
			E(TEXT("C7_Kato_Archive"), S::Kato, LOCTEXT("C7_Kato_Archive", "There's a local command archive ahead.")),
			E(TEXT("C7_Wren_Record"), S::Boarder, LOCTEXT("C7_Wren_Record", "Record everything. Find who's giving the orders.")),
			E(TEXT("A4_DoorsKnowUs"), S::Boarder, LOCTEXT("A4_DoorsKnowUs", "They shouldn't have our access codes.")),
			E(TEXT("A4_Recording_Hangar"), S::Unknown, LOCTEXT("A4_Recording_Hangar", "Keep the recovery teams on the Core search."), St::Echo),
			E(TEXT("A4_Recording_Corridor"), S::FutureMoss, LOCTEXT("A4_Recording_Corridor", "Bring every recovery team back."), St::Echo),
			E(TEXT("A4_Recording_Barracks"), S::FutureWren, LOCTEXT("A4_Recording_Barracks", "Recover the Core. Protect the younger Triad."), St::Echo),
			E(TEXT("A4_Recording_Core"), S::FutureKato, LOCTEXT("A4_Recording_Core", "We are you, from the future. We came back for the Origin Core."), St::Echo),
			// Gameplay 07, Inside the Dominion (storyline.md 27): the squad hears its own voices.
			E(TEXT("G7_Moss_MyVoice"), S::Moss, LOCTEXT("G7_Moss_MyVoice", "That's my voice in those records. Older.")),
			E(TEXT("G7_Kato_Authorisations"), S::Kato, LOCTEXT("G7_Kato_Authorisations", "Those authorisations match all three of us.")),
			// R102 (second draft, StoryFidelity01): the clear older Kato in the archive, after the identity evidence; distinct
			// from the Cinder echo (C5_Unknown_SameConstruction). Opens Cinematic08, so it reads KATO (RECORDED).
			E(TEXT("G7_RecKato_SameConstruction"), S::FutureKato, LOCTEXT("G7_RecKato_SameConstruction", "Same construction.")),
			E(TEXT("G7_Wren_FullRecord"), S::Boarder, LOCTEXT("G7_Wren_FullRecord", "We hear the full record before we touch that reactor.")),
			// Finale payoff: the future command deck, the same moment from the other side ("Don't-" is the SAME sound).
			E(TEXT("F_FutureKato_Signatures"), S::FutureKato, LOCTEXT("F_FutureKato_Signatures", "All three signatures confirmed. It's us.")),
			E(TEXT("F_FutureMoss_Where"), S::FutureMoss, LOCTEXT("F_FutureMoss_Where", "They're exactly where we were.")),
			E(TEXT("F_FutureWren_Channel"), S::FutureWren, LOCTEXT("F_FutureWren_Channel", "Open a channel. Now.")),
			E(TEXT("F_FutureKato_Interference"), S::FutureKato, LOCTEXT("F_FutureKato_Interference", "Temporal interference. Less than a second.")),
			E(TEXT("F_FutureWren_Dont"), S::FutureWren, LOCTEXT("F_FutureWren_Dont", "Don't-"), St::Static, TEXT("Dont")),
			// The intended warning, recovered locally: NOT the shared "Dont" sound (the fragment is never re-recorded or extended).
			E(TEXT("F_FutureWren_FullWarning"), S::FutureWren, LOCTEXT("F_FutureWren_FullWarning", "Don't engage the Dominion. You don't understand what happens if you do.")),
			// Cinematic 08, The Recovered Transmission (storyline.md 28): voices over the sealed threshold, skippable.
			E(TEXT("C8_RecKato_OneEvent"), S::FutureKato, LOCTEXT("C8_RecKato_OneEvent", "The Origin Core might let us change one moment in the past—before we lost everything."), St::Echo),
			E(TEXT("C8_RecWren_LastChance"), S::FutureWren, LOCTEXT("C8_RecWren_LastChance", "We believe recovering it can prevent the catastrophe. It's the last chance we have."), St::Echo),
			E(TEXT("C8_RecMoss_SaveYou"), S::FutureMoss, LOCTEXT("C8_RecMoss_SaveYou", "We came back to save you."), St::Echo),
			E(TEXT("C8_Kato_NoSafeShutdown"), S::Kato, LOCTEXT("C8_Kato_NoSafeShutdown", "I can't verify a safe shutdown. Continuing could put the whole system at risk.")),
			E(TEXT("C8_Moss_CantLetThem"), S::Moss, LOCTEXT("C8_Moss_CantLetThem", "Then we can't let them finish.")),
			E(TEXT("C8_Wren_Order"), S::Wren, LOCTEXT("C8_Wren_Order", "Take out the reactor. Keep the route to the hangar clear.")), // N-046: Wren, approved
			// N-052 Face to Face (reactor room), per boarding pilot. PLACEHOLDERS until Nicholas writes them.
			E(TEXT("G8_Self_Wren_Defender"), S::FutureKato, LOCTEXT("G8_Self_Wren_Defender", "[PLACEHOLDER] Future Kato warns the boarder off.")),
			E(TEXT("G8_Self_Wren_Future"), S::FutureWren, LOCTEXT("G8_Self_Wren_Future", "[PLACEHOLDER] Future Wren speaks to their younger self.")),
			E(TEXT("G8_Self_Wren_Present"), S::Boarder, LOCTEXT("G8_Self_Wren_Present", "[PLACEHOLDER] Wren answers their future self.")),
			E(TEXT("G8_Self_Wren_Overcome1"), S::FutureKato, LOCTEXT("G8_Self_Wren_Overcome1", "[PLACEHOLDER] Future Kato, overcome.")),
			E(TEXT("G8_Self_Wren_Overcome2"), S::FutureMoss, LOCTEXT("G8_Self_Wren_Overcome2", "[PLACEHOLDER] Future Moss, overcome.")),
			E(TEXT("G8_Self_Kato_Defender"), S::FutureWren, LOCTEXT("G8_Self_Kato_Defender", "[PLACEHOLDER] Future Wren warns the boarder off.")),
			E(TEXT("G8_Self_Kato_Future"), S::FutureKato, LOCTEXT("G8_Self_Kato_Future", "[PLACEHOLDER] Future Kato speaks to their younger self.")),
			E(TEXT("G8_Self_Kato_Present"), S::Boarder, LOCTEXT("G8_Self_Kato_Present", "[PLACEHOLDER] Kato answers their future self.")),
			E(TEXT("G8_Self_Kato_Overcome1"), S::FutureWren, LOCTEXT("G8_Self_Kato_Overcome1", "[PLACEHOLDER] Future Wren, overcome.")),
			E(TEXT("G8_Self_Kato_Overcome2"), S::FutureMoss, LOCTEXT("G8_Self_Kato_Overcome2", "[PLACEHOLDER] Future Moss, overcome.")),
			E(TEXT("G8_Self_Moss_Defender"), S::FutureWren, LOCTEXT("G8_Self_Moss_Defender", "[PLACEHOLDER] Future Wren warns the boarder off.")),
			E(TEXT("G8_Self_Moss_Future"), S::FutureMoss, LOCTEXT("G8_Self_Moss_Future", "[PLACEHOLDER] Future Moss speaks to their younger self.")),
			E(TEXT("G8_Self_Moss_Present"), S::Boarder, LOCTEXT("G8_Self_Moss_Present", "[PLACEHOLDER] Moss answers their future self.")),
			E(TEXT("G8_Self_Moss_Overcome1"), S::FutureWren, LOCTEXT("G8_Self_Moss_Overcome1", "[PLACEHOLDER] Future Wren, overcome.")),
			E(TEXT("G8_Self_Moss_Overcome2"), S::FutureKato, LOCTEXT("G8_Self_Moss_Overcome2", "[PLACEHOLDER] Future Kato, overcome.")),
			// Gameplay 08, The Reactor Fight (storyline.md 29).
			E(TEXT("G8_Wren_ConduitsFirst"), S::Boarder, LOCTEXT("G8_Wren_ConduitsFirst", "Conduits first. Then the heart.")),
			E(TEXT("G8_Kato_ConduitDown"), S::Kato, LOCTEXT("G8_Kato_ConduitDown", "Conduit destroyed.")),
			E(TEXT("G8_Moss_KeepMoving"), S::Moss, LOCTEXT("G8_Moss_KeepMoving", "Keep moving. Watch the way back.")),
			E(TEXT("G8_Kato_HeartExposed"), S::Kato, LOCTEXT("G8_Kato_HeartExposed", "The heart is exposed.")),
			E(TEXT("G8_Command_ReactorLost"), S::Command, LOCTEXT("G8_Command_ReactorLost", "Reactor lost. Get back to the hangar.")),
			// Gameplay 09, Escape from the Dominion (storyline.md 30).
			E(TEXT("G9_Moss_HoldingWayOut"), S::Moss, LOCTEXT("G9_Moss_HoldingWayOut", "Keep moving. We're holding the way out.")),
			// N-087: when Moss is the one on foot, Wren holds the way out (FireTransmission routes it).
			E(TEXT("G9_Wren_HoldingWayOut"), S::Wren, LOCTEXT("G9_Wren_HoldingWayOut", "Keep moving. We're holding the way out.")),
			E(TEXT("G9_Kato_HangarConfirmed"), S::Kato, LOCTEXT("G9_Kato_HangarConfirmed", "Hangar signal confirmed. Get the ship clear.")),
			E(TEXT("G9_Wren_PullAway"), S::Wren, LOCTEXT("G9_Wren_PullAway", "Triad, pull away. Follow the clear vector.")),
			// Cinematic 09, The Paradox (storyline.md 31): clear space, then the signal (the caption stays on screen under them).
			E(TEXT("C9_Kato_Clear"), S::Kato, LOCTEXT("C9_Kato_Clear", "Lancer clear.")),
			E(TEXT("C9_Moss_Clear"), S::Moss, LOCTEXT("C9_Moss_Clear", "Bastion clear. Still here.")),
			E(TEXT("C9_Wren_Clear"), S::Wren, LOCTEXT("C9_Wren_Clear", "Rift clear. All three accounted for.")),
			E(TEXT("C9_Moss_TakeThat"), S::Moss, LOCTEXT("C9_Moss_TakeThat", "All three. I'll take that.")),
			E(TEXT("C9_Kato_Identifiers"), S::Kato, LOCTEXT("C9_Kato_Identifiers", "A distress signal. Those are our identifiers.")),
			E(TEXT("C9_Wren_Source"), S::Wren, LOCTEXT("C9_Wren_Source", "The source?")),
			E(TEXT("C9_Kato_Decades"), S::Kato, LOCTEXT("C9_Kato_Decades", "It's dated decades ahead.")),
			// Act 5: Reactor and Escape, the Choice.
			E(TEXT("A5_Choice"), S::Moss, LOCTEXT("A5_Choice", "We can't trust them, and we can't let them finish. We end it here.")),
			E(TEXT("A5_Escape"), S::Control, LOCTEXT("A5_Escape", "The ship is coming apart. Ninety seconds. Move!")),
			E(TEXT("A5_DistressSignal"), S::Unknown, LOCTEXT("A5_DistressSignal", "DISTRESS SIGNAL  ·  TRIAD: WREN, KATO, MOSS  ·  DATED DECADES AHEAD"), St::Static),
		};
		return Table;
	}

	static const UTDTransmissionSet* Override()
	{
#if WITH_DEV_AUTOMATION_TESTS
		if (const UTDTransmissionSet* T = UTDStorySubsystem::SetOverrideForTests.Get())
		{
			return T;
		}
#endif
		return UTDStatics::LoadPresentation<UTDTransmissionSet>(TEXT("DA_Story_Transmissions"));
	}

	/** Nicholas's entry over the default, field by field (an empty field keeps the placeholder's). */
	static void Overlay(FTDTransmission& Base, const FTDTransmission& Over)
	{
		Base.Speaker = Over.Speaker;
		Base.Style = Over.Style;
		Base.bRepeatable = Over.bRepeatable;
		if (!Over.Text.IsEmpty()) { Base.Text = Over.Text; }
		if (!Over.Sound.IsNull()) { Base.Sound = Over.Sound; }
		if (Over.Duration > 0.f) { Base.Duration = Over.Duration; }
		if (!Over.SharedSound.IsNone()) { Base.SharedSound = Over.SharedSound; }
	}

	/** On-screen fallback: the line on air as a subtitle in the lower lane, above T2's tutorial card. */
	class STDTransmissionOverlay : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDTransmissionOverlay) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, UTDStorySubsystem* InStory)
		{
			Story = InStory;
			SetVisibility(EVisibility::HitTestInvisible);
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geo, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
			const FWidgetStyle&, bool) const override
		{
			const UTDStorySubsystem* S = Story.Get();
			if (!S || S->bDrawnExternally)
			{
				return Layer;
			}
			const FTDTransmissionView V = S->GetCurrentTransmission();
			if (!V.bActive || V.Alpha <= 0.f)
			{
				return Layer;
			}
			const FVector2D Size = Geo.GetLocalSize();
			const float Scale = Size.Y / 1080.f;
			const FSlateFontInfo TagFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(17.f * Scale));
			const FSlateFontInfo LineFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(22.f * Scale));
			const FText TagText = FText::Format(LOCTEXT("TagFmt", "{0}  "), V.SpeakerTag);
			auto Measure = [](const FText& T, const FSlateFontInfo& F) -> FVector2D
			{
				if (FSlateApplication::IsInitialized())
				{
					return FVector2D(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T, F));
				}
				return FVector2D(F.Size * 0.6f * T.ToString().Len(), F.Size * 1.3f);
			};
			const FVector2D TagSize = Measure(TagText, TagFont);
			const FVector2D LineSize = Measure(V.Text, LineFont);
			const float W = TagSize.X + LineSize.X;
			// T2's subtitle spot, under its lower lane (83%, y 900 at 1080p); Nick's HUD fills the bottom with panels, so
			// under his the line goes just below his objective block at the top.
			const bool bNickHUD = UTDUIHostSettings::GetSpaceHUDChoice() == ETDSpaceHUDChoice::Nick;
			FVector2D Pos(Size.X * 0.5f - W * 0.5f, Size.Y * (bNickHUD ? 0.17f : 0.83f) - LineSize.Y * 0.5f);
			float A = V.Alpha;
			if (V.Style == ETDTransmissionStyle::Static)
			{
				// A broken signal: the line drops out for a frame now and then.
				const float Noise = FMath::Frac(FMath::Sin(V.Progress01 * 913.7f) * 43758.5453f);
				A *= Noise < 0.12f ? 0.25f : 1.f;
				Pos.X += (Noise - 0.5f) * 3.f * Scale;
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FVector2D Pad(14.f * Scale, 6.f * Scale);
			FSlateDrawElement::MakeBox(Out, Layer, Geo.ToPaintGeometry(FVector2D(W, LineSize.Y) + Pad * 2.f, FSlateLayoutTransform(Pos - Pad)),
				White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f * A));
			FLinearColor TagColor = V.Color;
			TagColor.A = A;
			const float TagY = Pos.Y + (LineSize.Y - TagSize.Y) * 0.5f;
			FSlateDrawElement::MakeText(Out, Layer + 1, Geo.ToPaintGeometry(TagSize, FSlateLayoutTransform(FVector2D(Pos.X, TagY))), TagText, TagFont,
				ESlateDrawEffect::None, TagColor);
			const FVector2D LinePos(Pos.X + TagSize.X, Pos.Y);
			if (V.Style == ETDTransmissionStyle::Echo)
			{
				// A voice that should not be there: a faint double a few pixels behind.
				FSlateDrawElement::MakeText(Out, Layer + 1, Geo.ToPaintGeometry(LineSize, FSlateLayoutTransform(LinePos + FVector2D(4.f, 2.f) * Scale)),
					V.Text, LineFont, ESlateDrawEffect::None, FLinearColor(0.7f, 0.85f, 1.f, 0.35f * A));
			}
			FSlateDrawElement::MakeText(Out, Layer + 2, Geo.ToPaintGeometry(LineSize, FSlateLayoutTransform(LinePos)), V.Text, LineFont,
				ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, A));
			return Layer + 3;
		}

	private:
		TWeakObjectPtr<UTDStorySubsystem> Story;
	};
}

// ------------------------------------------------------------------------------------------------ data

bool UTDStorySubsystem::FindEntry(FName Id, FTDTransmission& Out)
{
	bool bFound = false;
	for (const FTDTransmission& T : TDStory::Defaults())
	{
		if (T.Id == Id)
		{
			Out = T;
			bFound = true;
			break;
		}
	}
	if (const UTDTransmissionSet* Set = TDStory::Override())
	{
		for (const FTDTransmission& T : Set->Entries)
		{
			if (T.Id == Id)
			{
				if (!bFound)
				{
					Out = T; // a beat only Nicholas defines
					bFound = true;
				}
				else
				{
					TDStory::Overlay(Out, T);
				}
				break;
			}
		}
	}
	// One shared slot for lines that must be the identical audio ("Don't-" in the opening and the finale).
	if (bFound && Out.Sound.IsNull() && !Out.SharedSound.IsNone())
	{
		if (const UTDTransmissionSet* Set = TDStory::Override())
		{
			if (const TSoftObjectPtr<USoundBase>* Shared = Set->SharedSounds.Find(Out.SharedSound))
			{
				Out.Sound = *Shared;
			}
		}
	}
	return bFound;
}

TArray<FTDTransmission> UTDStorySubsystem::GetAllEntries()
{
	TArray<FTDTransmission> All;
	for (const FTDTransmission& T : TDStory::Defaults())
	{
		FTDTransmission E;
		FindEntry(T.Id, E);
		All.Add(E);
	}
	if (const UTDTransmissionSet* Set = TDStory::Override())
	{
		for (const FTDTransmission& T : Set->Entries)
		{
			if (!All.ContainsByPredicate([&T](const FTDTransmission& A) { return A.Id == T.Id; }))
			{
				All.Add(T);
			}
		}
	}
	return All;
}

FText UTDStorySubsystem::GetText(FName Id, const FText& Fallback)
{
	FTDTransmission E;
	return (FindEntry(Id, E) && !E.Text.IsEmpty()) ? E.Text : Fallback;
}

FText UTDStorySubsystem::PilotTrait(ETDShipId Ship)
{
	switch (Ship)
	{
	case ETDShipId::Rift: return GetText(TEXT("Trait_Wren"), FText::GetEmpty());
	case ETDShipId::Lancer: return GetText(TEXT("Trait_Kato"), FText::GetEmpty());
	default: return GetText(TEXT("Trait_Moss"), FText::GetEmpty());
	}
}

void UTDStorySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDStorySubsystem::HandleWaveStarted);
		Bus->OnWaveCleared.AddUniqueDynamic(this, &UTDStorySubsystem::HandleWaveCleared);
	}
	RelayHandle = ATDArchiveTransfer::OnAnyTerminalActivated.AddUObject(this, &UTDStorySubsystem::HandleRelayConnected);
	if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(&InWorld))
	{
		AftermathHandle = Profile->OnAftermathBegins.AddUObject(this, &UTDStorySubsystem::HandleAftermathBegins);
	}
	SurveyHandle = ATDSurveyTerminal::OnAnySurveyRecordsAccessed.AddUObject(this, &UTDStorySubsystem::HandleSurveyRecords);
}

void UTDStorySubsystem::HandleRelayConnected(AActor* Terminal)
{
	if (Terminal && Terminal->GetWorld() != GetWorld())
	{
		return; // another world's terminal (the delegate is static)
	}
	// The drone that wakes: T5's dormant drone on the relay deck (ATDPlanetCourse::GetDormantDrone).
	for (TActorIterator<ATDPlanetCourse> It(GetWorld()); It; ++It)
	{
		if (UStaticMeshComponent* Drone = It->GetDormantDrone())
		{
			PulseDrone = Drone;
			break;
		}
	}
	if (HasPlayedAll(TEXT("Cinematic03")))
	{
		return; // a retake after it was heard: nothing to replay (Lead, live walk)
	}
	PlaySequence(this, TEXT("Cinematic03"));
}

void UTDStorySubsystem::HandleAftermathBegins(bool /*bSkippedTutorial*/)
{
	if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this))
	{
		Profile->ConsumeAftermathPending(); // taken
	}
	bAftermathChecked = true;
	MarkPlayed(FName(AftermathStartedId)); // this run's Aftermath has begun: a cut one resumes on the next galaxy load
	if (PlaySequence(this, TEXT("Aftermath")))
	{
		UTDUIHostSubsystem::ShowTacticalDisplay(this, 30.f); // the lanes toward the planets (T2)
	}
}

void UTDStorySubsystem::HandleSurveyRecords(AActor* Terminal)
{
	if (Terminal && Terminal->GetWorld() != GetWorld())
	{
		return;
	}
	if (HasPlayedAll(TEXT("Cinematic05")))
	{
		return;
	}
	PlaySequence(this, TEXT("Cinematic05"));
}

void UTDStorySubsystem::HandleLineOnAir(FName Id)
{
	// T2's scan monitor, keyed to the lines: Verdance's structure, the scan sites, the Cinder correlation (C03); the
	// Cinder overlay, then the cache and anchor markers (C05).
	int32 Stage = 0;
	// T2's navigation pulse, like sensor noise: toward Verdance in Aftermath ("They're sending the rest down to Verdance"), toward Cinder on the
	// Verdance departure ("Proceed to Cinder").
	const ETDDestination PulseTo = Id == TEXT("S2_Kato_Coordinated") ? ETDDestination::Verdance
		: (Id == TEXT("C4_Command_Proceed") ? ETDDestination::Cinder : ETDDestination::Galaxy);
	if (PulseTo != ETDDestination::Galaxy)
	{
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this);
		if (Pawn && Galaxy)
		{
			const FTDGalaxyLayoutData& L = Galaxy->GetLayout();
			const FTDPlanetSpec* V = L.Planets.FindByPredicate([PulseTo](const FTDPlanetSpec& P) { return P.Destination == PulseTo; });
			if (V)
			{
				const FVector From = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 1000.f;
				UTDUIHostSubsystem::ShowNavigationPulse(this, From, Galaxy->GetActorTransform().TransformPosition(V->Location), 1.5f);
			}
		}
	}
	if (Id == TEXT("C3_Kato_Artificial")) { Stage = 1; }
	else if (Id == TEXT("C3_Kato_Beneath")) { Stage = 2; }
	else if (Id == TEXT("C3_Kato_CinderMatch")) { Stage = 3; }
	else if (Id == TEXT("C5_Kato_SameConstruction")) { Stage = 4; }
	else if (Id == TEXT("C5_Kato_Anchor")) { Stage = 5; }
	if (Stage > 0)
	{
		bMonitorUp = UTDUIHostSubsystem::ShowScanMonitor(this, Stage, 0.f) || bMonitorUp;
	}
	// The three pulses: the drone answers ("Did we just answer that thing?" follows it).
	if (Id == TEXT("C3_Wren_Answered") && PulseDrone.IsValid())
	{
		UStaticMeshComponent* Drone = PulseDrone.Get();
		if (!PulseLight.IsValid())
		{
			UPointLightComponent* L = NewObject<UPointLightComponent>(Drone->GetOwner());
			L->SetupAttachment(Drone);
			L->SetLightColor(FLinearColor(0.2f, 0.55f, 1.f));
			L->SetAttenuationRadius(2500.f);
			L->SetIntensity(0.f);
			L->RegisterComponent();
			PulseLight = L;
		}
		PulseStart = RealNow();
		if (const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
		{
			UTDStatics::PlayCue(this, Style->ThreePulse, Drone->GetComponentLocation());
		}
	}
}

void UTDStorySubsystem::UpdateDronePulses(double Now)
{
	UPointLightComponent* L = PulseLight.Get();
	if (!L || PulseStart < 0.0)
	{
		return;
	}
	// Three pulses (0.25 s on, 0.35 s apart), then the optic stays a steady blue.
	const float T = static_cast<float>(Now - PulseStart);
	const float Cycle = 0.6f;
	const bool bOn = T >= 3.f * Cycle || FMath::Fmod(T, Cycle) < 0.25f;
	L->SetIntensity(T >= 3.f * Cycle ? 2500.f : (bOn ? 12000.f : 0.f));
}

void UTDStorySubsystem::HandleWaveStarted(int32 WaveNumber, float /*Intensity01*/)
{
	// Act 1, First Contact: the hail nobody answers; the field around the ship (storyline.md §5).
	// (A1_FieldUnreachable is T4's: fired when the player first approaches the distortion field.)
	if (WaveNumber == 1) { FireTransmission(TEXT("A1_HailNoAnswer")); }
	// Cinematic 06's last two lines, as the guard line (wave 5) forms up (storyline.md §23).
	else if (WaveNumber == 5)
	{
		FireTransmission(TEXT("C6_Moss_WholeFleet"));
		FireTransmission(TEXT("C6_Wren_StayTogether"));
	}
}

const TArray<FName>& UTDStorySubsystem::DepartureSet(ETDDestination Planet)
{
	// Cinematic 04, Verdance Departure (§17, §25) and Cinematic 06, The Path to Dominion (§23): transmissions over the
	// existing take-off, no cutscene (Story/README.md: "transmissions over existing take-off transitions").
	// N-087 (Nick's Lead under Nicholas's delegation, issue #29): three calls each; the five C4 and three C6 lines cut are
	// kept in the table, marked retired.
	static const TArray<FName> Verdance = { TEXT("C4_Wren_Badly"), TEXT("C4_Kato_SurveyStation"), TEXT("C4_Command_Proceed") };
	static const TArray<FName> Cinder = { TEXT("C6_Kato_GuardLine"), TEXT("C6_Kato_FourGenerators"), TEXT("C6_Command_FindOutWhy") };
	static const TArray<FName> None;
	return Planet == ETDDestination::Verdance ? Verdance : (Planet == ETDDestination::Cinder ? Cinder : None);
}

void UTDStorySubsystem::UpdateDepartures(double Now)
{
	const UTDGameInstance* GI = Cast<UTDGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI)
	{
		bDeparturesChecked = true; // a bare world: nothing to carry
		return;
	}
	if (GI->GetCurrentDestination() != ETDDestination::Galaxy)
	{
		return; // a planet, the interior, the menu: wait (the game mode sets the destination at StartPlay)
	}
	if (DeparturesAt < 0.0)
	{
		DeparturesAt = Now + DepartureDelaySeconds; // let the ascent settle before the radio starts
		return;
	}
	if (Now < DeparturesAt)
	{
		return;
	}
	bDeparturesChecked = true;
	const UTDFlowSettings* Flow = UTDFlowSettings::Get();
	const TSet<FName>& Done = GI->GetProgressRef().CompletedObjectives;
	auto AllDone = [&Done](const TArray<FName>& Ids)
	{
		if (Ids.Num() == 0) { return false; }
		for (const FName& Id : Ids) { if (!Done.Contains(Id)) { return false; } }
		return true;
	};
	// Only the latest finished planet's set: a run continued after Cinder never hears Verdance's departure late.
	const ETDDestination Planet = AllDone(Flow->CinderObjectives) ? ETDDestination::Cinder
		: (AllDone(Flow->VerdanceObjectives) ? ETDDestination::Verdance : ETDDestination::Galaxy);
	const TArray<FName>& Set = DepartureSet(Planet);
	if (Set.Num() == 0 || HasPlayed(Set[0]))
	{
		return; // played already this run (once per run, kept in the save)
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: %s departure, %d transmissions"), Planet == ETDDestination::Cinder ? TEXT("Cinder") : TEXT("Verdance"), Set.Num());
	for (const FName& Id : Set)
	{
		FireTransmission(Id);
	}
}

void UTDStorySubsystem::HandleWaveCleared(int32 WaveNumber)
{
	// They search, they don't hold ground; then the trail to Verdance.
	if (WaveNumber == 1) { FireTransmission(TEXT("A1_TheySearch")); }
	else if (WaveNumber == 2) { FireTransmission(TEXT("A1_TrailToVerdance")); }
}

FText UTDStorySubsystem::SpeakerTag(ETDSpeaker Speaker)
{
	switch (Speaker)
	{
	case ETDSpeaker::Wren: return LOCTEXT("TagWren", "WREN");
	case ETDSpeaker::Kato: return LOCTEXT("TagKato", "KATO");
	case ETDSpeaker::Moss: return LOCTEXT("TagMoss", "MOSS");
	case ETDSpeaker::Unknown: return LOCTEXT("TagUnknown", "UNKNOWN SIGNAL");
	case ETDSpeaker::Command: return LOCTEXT("TagCommand", "COMMAND");
	case ETDSpeaker::PatrolSeven: return LOCTEXT("TagPatrol", "PATROL SEVEN");
	case ETDSpeaker::FutureWren: return LOCTEXT("TagFutureWren", "WREN?");
	case ETDSpeaker::FutureKato: return LOCTEXT("TagFutureKato", "KATO?");
	case ETDSpeaker::FutureMoss: return LOCTEXT("TagFutureMoss", "MOSS?");
	case ETDSpeaker::Boarder: return LOCTEXT("TagBoarder", "BOARDING PILOT");
	default: return LOCTEXT("TagControl", "CONTROL");
	}
}

bool UTDStorySubsystem::HasPlayedAll(FName SequenceId) const
{
	const TArray<FName>& Ids = SequenceIds(SequenceId);
	for (const FName& Id : Ids)
	{
		if (!IsRetired(Id) && !HasPlayed(Id))
		{
			return false;
		}
	}
	return Ids.Num() > 0;
}

bool UTDStorySubsystem::IsMustHearSequencePlaying(const UObject* WorldContext)
{
	const UTDStorySubsystem* S = Get(WorldContext);
	return S && S->IsSequencePlaying(TEXT("Aftermath"));
}

bool UTDStorySubsystem::HasLineAired(const UObject* WorldContext, FName Id)
{
	const UTDStorySubsystem* S = Get(WorldContext);
	return S && S->HasAired(Id);
}

bool UTDStorySubsystem::HasAired(FName Id) const
{
	if (Aired.Contains(Id))
	{
		return true;
	}
	// A reloaded run: played earlier (saved) and not waiting in this world's queue or on air now.
	return HasPlayed(Id) && !(Current.bActive && Current.Id == Id) && !Queue.ContainsByPredicate([Id](const FTDTransmission& T) { return T.Id == Id; });
}

FText UTDStorySubsystem::SpeakerTagNow(ETDSpeaker Speaker) const
{
	const bool bFinal = HasAired(TEXT("A4_Recording_Core")) || HasAired(TEXT("G7_Kato_Authorisations"));
	switch (Speaker)
	{
	case ETDSpeaker::FutureMoss:
		return (bFinal || HasAired(TEXT("G7_Moss_MyVoice"))) ? LOCTEXT("TagRecMoss", "MOSS (RECORDED)") : LOCTEXT("TagRecUnknownM", "UNKNOWN");
	case ETDSpeaker::FutureWren:
		return bFinal ? LOCTEXT("TagRecWren", "WREN (RECORDED)") : LOCTEXT("TagRecUnknownW", "UNKNOWN");
	case ETDSpeaker::FutureKato:
		return bFinal ? LOCTEXT("TagRecKato", "KATO (RECORDED)") : LOCTEXT("TagRecUnknownK", "UNKNOWN");
	case ETDSpeaker::Unknown:
		return LOCTEXT("TagRecUnknown", "UNKNOWN");
	default:
		return SpeakerTag(Speaker);
	}
}

FLinearColor UTDStorySubsystem::SpeakerColor(ETDSpeaker Speaker)
{
	switch (Speaker)
	{
	case ETDSpeaker::Wren: return FLinearColor(1.f, 0.25f, 0.85f);
	case ETDSpeaker::Kato: return FLinearColor(0.1f, 0.9f, 1.f);
	case ETDSpeaker::Moss: return FLinearColor(1.f, 0.65f, 0.1f);
	case ETDSpeaker::FutureWren: return FLinearColor(0.75f, 0.45f, 0.7f);
	case ETDSpeaker::FutureKato: return FLinearColor(0.45f, 0.7f, 0.75f);
	case ETDSpeaker::FutureMoss: return FLinearColor(0.75f, 0.6f, 0.4f);
	case ETDSpeaker::Unknown: return FLinearColor(1.f, 0.55f, 0.5f);
	case ETDSpeaker::PatrolSeven: return FLinearColor(0.7f, 0.95f, 0.6f);
	default: return FLinearColor(0.85f, 0.9f, 1.f);
	}
}

float UTDStorySubsystem::SecondsFor(const FTDTransmission& Entry)
{
	float Seconds = Entry.Duration > 0.f ? Entry.Duration : FMath::Clamp(1.6f + 0.055f * Entry.Text.ToString().Len(), 2.5f, 7.f);
	if (const USoundBase* Sound = Entry.Sound.Get())
	{
		Seconds = FMath::Max(Seconds, Sound->GetDuration() + 0.3f);
	}
	return Seconds;
}

// ------------------------------------------------------------------------------------------------ firing

UTDStorySubsystem* UTDStorySubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDStorySubsystem>() : nullptr;
}

bool UTDStorySubsystem::Fire(const UObject* WorldContextObject, FName Id)
{
	UTDStorySubsystem* S = Get(WorldContextObject);
	return S && S->FireTransmission(Id);
}

FTDTransmissionView UTDStorySubsystem::GetCurrentTransmissionFor(const UObject* WorldContextObject)
{
	const UTDStorySubsystem* S = Get(WorldContextObject);
	return S ? S->GetCurrentTransmission() : FTDTransmissionView();
}

bool UTDStorySubsystem::HasPlayed(FName Id) const
{
	if (const UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		return GI->HasPlayedTransmission(Id);
	}
	return LocalPlayed.Contains(Id);
}

void UTDStorySubsystem::MarkPlayed(FName Id)
{
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->MarkTransmissionPlayed(Id);
		return;
	}
	LocalPlayed.Add(Id);
}

void UTDStorySubsystem::ResetPlayed()
{
	LocalPlayed.Reset();
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->ForgetPlayedTransmissions();
	}
}

FName UTDStorySubsystem::RouteForBoarder(const UObject* WorldContext, FName Id)
{
	// N-087: the escape radio is Moss's, unless Moss boarded; then Wren holds the way out.
	if (Id == TEXT("G9_Moss_HoldingWayOut") && ResolveSpeaker(WorldContext, ETDSpeaker::Boarder) == ETDSpeaker::Moss)
	{
		return TEXT("G9_Wren_HoldingWayOut");
	}
	return Id;
}

bool UTDStorySubsystem::IsRetired(FName Id)
{
	// Kept in the table for the record (N-087, #29), never aired. A caller that still fires one is swallowed quietly.
	static const TSet<FName> Retired = { TEXT("TB_Escaped"), TEXT("C4_Moss_Volcanoes"), TEXT("C4_Wren_SetCourse"),
		TEXT("C4_Wren_WhatConnects"), TEXT("C4_Kato_SameSignature"), TEXT("C4_Moss_Trouble"), TEXT("C6_Moss_SamplesAboard"),
		TEXT("C6_Wren_FindAWayInside"), TEXT("C6_Wren_Understood") };
	return Retired.Contains(Id);
}

bool UTDStorySubsystem::FireTransmission(FName Id)
{
	Id = RouteForBoarder(this, Id);
	if (IsRetired(Id))
	{
		UE_LOG(LogTriadDrift, Verbose, TEXT("TD.Story: %s is retired, not aired"), *Id.ToString());
		return false;
	}
	FTDTransmission Entry;
	if (Id.IsNone() || !FindEntry(Id, Entry))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Story: no transmission '%s'"), *Id.ToString());
		return false;
	}
	const bool bQueued = Queue.ContainsByPredicate([Id](const FTDTransmission& T) { return T.Id == Id; });
	if (!Entry.bRepeatable && (HasPlayed(Id) || bQueued || (Current.bActive && Current.Id == Id)))
	{
		return false; // once per run
	}
	if (!Entry.bRepeatable && !IsRequiredLine(Id))
	{
		MarkPlayed(Id); // ambient: claimed now, a Continue before it airs never replays it
	}
	Queue.Add(Entry);
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: %s queued (%d waiting)"), *Id.ToString(), Queue.Num());
	return true;
}

// ------------------------------------------------------------------------------------------------ tick

bool UTDStorySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTDStorySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDStorySubsystem, STATGROUP_Tickables);
}

double UTDStorySubsystem::RealNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

bool UTDStorySubsystem::IsHeld() const
{
	// Never over a money shot or the intro: the line waits for them (it is never dropped).
	if (const UTDDirectorSubsystem* D = UTDDirectorSubsystem::Get(this); D && D->IsShotActive())
	{
		return true;
	}
	if (const UTDIntroDirector* Intro = UTDIntroDirector::Get(this); Intro && Intro->IsPlaying())
	{
		return true;
	}
	return false;
}

TFunction<ETDShipId(const UObject*)> UTDStorySubsystem::BoardingShipResolver;

ETDSpeaker UTDStorySubsystem::ResolveSpeaker(const UObject* WorldContext, ETDSpeaker Speaker)
{
	if (Speaker != ETDSpeaker::Boarder)
	{
		return Speaker;
	}
	// T5's travel record names the docked ship when it is wired (BoardingShipResolver); else the run's last flown ship.
	ETDShipId Ship = ETDShipId::Rift;
	if (BoardingShipResolver)
	{
		Ship = BoardingShipResolver(WorldContext);
	}
	else if (const UTDGameInstance* GI = UTDGameInstance::Get(WorldContext))
	{
		Ship = GI->GetProgressRef().LastShip;
	}
	return Ship == ETDShipId::Lancer ? ETDSpeaker::Kato : (Ship == ETDShipId::Bastion ? ETDSpeaker::Moss : ETDSpeaker::Wren);
}

const TArray<FName>& UTDStorySubsystem::SequenceIds(FName SequenceId)
{
	// Cinematic 08 (Lead, Nov 13: voices over the sealed threshold, mandatory, skippable): the command deck's five lines
	// (the same "Dont" sound as the opening), the recovered full warning, the commanders' case, the decision.
	// Ryan (Nov 13): voices over the sealed threshold, the player in control: the mandatory final record first.
	static const TArray<FName> Cinematic08 = { TEXT("G7_RecKato_SameConstruction"), TEXT("A4_Recording_Core"), TEXT("G7_Wren_FullRecord"), TEXT("F_FutureKato_Signatures"), TEXT("F_FutureMoss_Where"), TEXT("F_FutureWren_Channel"),
		TEXT("F_FutureKato_Interference"), TEXT("F_FutureWren_Dont"), TEXT("F_FutureWren_FullWarning"), TEXT("C8_RecKato_OneEvent"),
		TEXT("C8_RecWren_LastChance"), TEXT("C8_RecMoss_SaveYou"), TEXT("C8_Kato_NoSafeShutdown"), TEXT("C8_Moss_CantLetThem"),
		TEXT("C8_Wren_Order") };
	// Cinematic 07 over the docking (G6_Wren_CoverTheBay, then Moss) and the hangar arrival (Kato's layout line).
	static const TArray<FName> Docking = { TEXT("G6_Wren_CoverTheBay"), TEXT("C7_Moss_WayOut") };
	static const TArray<FName> HangarArrival = { TEXT("C7_Kato_Layout") };
	// After the first door's access response (A4_DoorsKnowUs): the archive ahead, then the order.
	static const TArray<FName> FirstDoor = { TEXT("A4_DoorsKnowUs"), TEXT("C7_Kato_Archive"), TEXT("C7_Wren_Record") };
	// Cinematic 09 at the escape radius.
	static const TArray<FName> ClearSpace = { TEXT("C9_Kato_Clear"), TEXT("C9_Moss_Clear"), TEXT("C9_Wren_Clear") };
	// Cinematic 03, Beneath the Surface (storyline 15, 25): the relay connected; voices, the monitor, the drone's pulses.
	static const TArray<FName> Cinematic03 = { TEXT("C3_Moss_NoBodies"), TEXT("C3_Wren_Survey"), TEXT("C3_Kato_Artificial"), TEXT("A2_Voices"),
		TEXT("C3_Wren_NotThePlanet"), TEXT("C3_Kato_Beneath"), TEXT("C3_Wren_Answered"), TEXT("C3_Kato_DidntSend"), TEXT("C3_Kato_CinderMatch"),
		TEXT("C3_Moss_Company"), TEXT("C3_Wren_HoldThem") };
	// Cinematic 05, The Pattern (storyline 20, 25): at the Cinder survey terminal; the one echo, the Origin Core, the targets.
	static const TArray<FName> Cinematic05 = { TEXT("C5_Kato_SameConstruction"), TEXT("C5_Unknown_SameConstruction"), TEXT("C5_Kato_Feedback"),
		TEXT("C5_Kato_OriginCore"), TEXT("C5_Moss_Sample"), TEXT("C5_Wren_Guarding"), TEXT("C5_Kato_Anchor"), TEXT("C5_Command_Recover"),
		TEXT("C5_Wren_Route") };
	// Cinematic 02, Aftermath (storyline 12): after the tutorial's withdrawal, voices over the regrouped squad with T2's
	// tactical display; nothing is fought between it and Verdance.
	static const TArray<FName> Aftermath = { TEXT("S2_Moss_Accounted"), TEXT("S2_Wren_WelcomeParty"), TEXT("S2_Kato_NotPursuing"),
		TEXT("S2_Kato_Vectors"), TEXT("S2_Wren_WhatDoing"), TEXT("S2_Kato_Planetside"), TEXT("S2_Moss_InTheWay"), TEXT("S2_Command_Report"),
		TEXT("S2_Wren_Hostile"), TEXT("S2_Kato_Coordinated"), TEXT("S2_Command_Outpost"), TEXT("S2_Moss_FindOut"), TEXT("S2_Command_Orders"),
		TEXT("S2_Wren_FormUp"), TEXT("S2_Kato_Recorders") };
	// N-052 Face to Face, per boarding pilot: on the reactor room's entry, and as the two defenders are overcome.
	static const TMap<FName, TArray<FName>> Face = []()
	{
		TMap<FName, TArray<FName>> M;
		for (const TCHAR* B : { TEXT("Wren"), TEXT("Kato"), TEXT("Moss") })
		{
			const FString P = FString::Printf(TEXT("G8_Self_%s_"), B);
			M.Add(FName(*FString::Printf(TEXT("FaceToFace_%s"), B)), { FName(*(P + TEXT("Defender"))), FName(*(P + TEXT("Future"))), FName(*(P + TEXT("Present"))) });
			M.Add(FName(*FString::Printf(TEXT("FaceToFaceOvercome_%s"), B)), { FName(*(P + TEXT("Overcome1"))), FName(*(P + TEXT("Overcome2"))) });
		}
		return M;
	}();
	if (const TArray<FName>* F = Face.Find(SequenceId)) { return *F; }
	static const TArray<FName> None;
	if (SequenceId == TEXT("Aftermath")) { return Aftermath; }
	if (SequenceId == TEXT("Cinematic03")) { return Cinematic03; }
	if (SequenceId == TEXT("Cinematic05")) { return Cinematic05; }
	if (SequenceId == TEXT("Cinematic08")) { return Cinematic08; }
	if (SequenceId == TEXT("Docking")) { return Docking; }
	if (SequenceId == TEXT("HangarArrival")) { return HangarArrival; }
	if (SequenceId == TEXT("FirstDoor")) { return FirstDoor; }
	if (SequenceId == TEXT("ClearSpace")) { return ClearSpace; }
	return None;
}

bool UTDStorySubsystem::PlaySequence(const UObject* WorldContext, FName SequenceId)
{
	UTDStorySubsystem* Story = Get(WorldContext);
	const TArray<FName>& Ids = SequenceIds(SequenceId);
	if (!Story || Ids.Num() == 0)
	{
		return false;
	}
	if (Story->ActiveSequence == SequenceId)
	{
		return true; // already running
	}
	if (!Story->ActiveSequence.IsNone())
	{
		Story->FinishSequence(false); // one at a time: the earlier one is done as far as its gate is concerned
	}
	for (const FName& Id : Ids)
	{
		Story->FireTransmission(Id);
	}
	Story->ActiveSequence = SequenceId;
	Story->SequenceStart = Story->RealNow();
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: sequence %s"), *SequenceId.ToString());
	return true;
}

bool UTDStorySubsystem::IsSequencePlaying(FName SequenceId) const
{
	return !ActiveSequence.IsNone() && (SequenceId.IsNone() || ActiveSequence == SequenceId);
}

bool UTDStorySubsystem::IsRequiredLine(FName Id)
{
	// Lead (live walk): Aftermath too: a descent can cut it, and its unheard lines replay on the next galaxy load.
	return Id.ToString().StartsWith(TEXT("A4_Recording_")) || SequenceIds(TEXT("Cinematic08")).Contains(Id)
		|| SequenceIds(TEXT("Aftermath")).Contains(Id);
}

bool UTDStorySubsystem::CanSkipSequence() const
{
	return !ActiveSequence.IsNone() && RealNow() - SequenceStart >= SkipAllowedAfterSeconds;
}

bool UTDStorySubsystem::SkipSequence()
{
	if (!CanSkipSequence())
	{
		return false; // nothing playing, or not yet (N-060: ten seconds before Skip)
	}
	const TArray<FName>& Ids = SequenceIds(ActiveSequence);
	// Its discoveries are kept: every line counts as heard (and saved), none plays again.
	Queue.RemoveAll([&Ids](const FTDTransmission& T) { return Ids.Contains(T.Id); });
	for (const FName& Id : Ids)
	{
		MarkPlayed(Id);
	}
	if (Current.bActive && Ids.Contains(Current.Id))
	{
		if (Voice) { Voice->Stop(); }
		Voice = nullptr;
		const FName Cut = Current.Id;
		Current = FTDTransmissionView();
		NextAllowed = RealNow() + GapSeconds;
		OnTransmissionEnded.Broadcast(Cut, false);
	}
	FinishSequence(true);
	return true;
}

void UTDStorySubsystem::FinishSequence(bool bSkipped)
{
	const FName Done = ActiveSequence;
	if (bMonitorUp && (Done == TEXT("Cinematic03") || Done == TEXT("Cinematic05")))
	{
		UTDUIHostSubsystem::HideScanMonitor(this);
		bMonitorUp = false;
	}
	ActiveSequence = NAME_None;
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: sequence %s %s"), *Done.ToString(), bSkipped ? TEXT("skipped") : TEXT("finished"));
	OnSequenceFinished.Broadcast(Done, bSkipped);
}

void UTDStorySubsystem::UpdateSequence()
{
	if (ActiveSequence.IsNone())
	{
		return;
	}
	const TArray<FName>& Ids = SequenceIds(ActiveSequence);
	const bool bOnAir = Current.bActive && Ids.Contains(Current.Id);
	const bool bQueued = Queue.ContainsByPredicate([&Ids](const FTDTransmission& T) { return Ids.Contains(T.Id); });
	if (!bOnAir && !bQueued)
	{
		FinishSequence(false);
	}
}

void UTDStorySubsystem::StartNext()
{
	OnAir = Queue[0];
	Queue.RemoveAt(0);
	OnAir.Speaker = ResolveSpeaker(this, OnAir.Speaker); // the boarding pilot, now
	OnAirStart = RealNow();
	OnAirSeconds = SecondsFor(OnAir);
	const FName SpeakerName = FName(*StaticEnum<ETDSpeaker>()->GetNameStringByValue((int64)OnAir.Speaker));
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastBarkRequested(OnAir.Id, SpeakerName); // presentation audio can answer the id
	}
	if (USoundBase* Sound = OnAir.Sound.LoadSynchronous())
	{
		OnAirSeconds = SecondsFor(OnAir);
		Voice = UGameplayStatics::SpawnSound2D(this, Sound);
	}
	if (SoundOnly.Contains(OnAir.Id))
	{
		// Its words are on screen already: the sound only, holding the queue for the line's time.
		Current = FTDTransmissionView();
		NextAllowed = RealNow() + OnAirSeconds + GapSeconds;
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: %s (sound only, %.1f s)"), *OnAir.Id.ToString(), OnAirSeconds);
		return;
	}
	Current = FTDTransmissionView();
	Current.bActive = true;
	Current.Id = OnAir.Id;
	Current.Speaker = OnAir.Speaker;
	Aired.Add(OnAir.Id); // first: the final record names its own speaker as it plays
	Current.SpeakerTag = SpeakerTagNow(OnAir.Speaker);
	if (OnAir.Id.ToString().StartsWith(TEXT("G8_Self_")) && (OnAir.Speaker == ETDSpeaker::FutureWren || OnAir.Speaker == ETDSpeaker::FutureKato
		|| OnAir.Speaker == ETDSpeaker::FutureMoss))
	{
		// N-052: face to face in the reactor room, not a recording.
		const FString Name = OnAir.Speaker == ETDSpeaker::FutureWren ? TEXT("WREN") : (OnAir.Speaker == ETDSpeaker::FutureKato ? TEXT("KATO") : TEXT("MOSS"));
		Current.SpeakerTag = FText::FromString(Name + TEXT(" (FUTURE)"));
	}
	Current.Text = OnAir.Text;
	Current.Style = OnAir.Style;
	Current.Color = SpeakerColor(OnAir.Speaker);
	Current.TimeLeft = OnAirSeconds;
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: on air %s [%s] \"%s\" (%.1f s)"), *OnAir.Id.ToString(), *Current.SpeakerTag.ToString(),
		*OnAir.Text.ToString(), OnAirSeconds);
	OnTransmission.Broadcast(Current);
	PlayStaticTear(Current.Id);
	HandleLineOnAir(Current.Id);
}

bool UTDStorySubsystem::HasStaticDelivery(FName Id)
{
	// The lines whose "[static]" moved from the subtitle into the delivery note (StoryFidelity01).
	static const TSet<FName> Static = { TEXT("A2_Voices"), TEXT("G6_Clue1"), TEXT("G6_Clue2") };
	return Static.Contains(Id);
}

void UTDStorySubsystem::PlayStaticTear(FName Id)
{
	if (!HasStaticDelivery(Id) || UTDSettingsSubsystem::IsReduceMotion(this))
	{
		return;
	}
	const UTDStoryFX* FX = UTDStoryFX::Get();
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* Cam = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (!FX || !Cam || !Cam->GetRootComponent())
	{
		return; // null-safe: no asset yet, or no camera
	}
	if (UNiagaraComponent* N = UTDStatics::PlayCueAttached(FX->TransmissionStatic, Cam->GetRootComponent()))
	{
		N->SetRelativeLocationAndRotation(FVector(60.f, 0.f, 0.f), FRotator::ZeroRotator); // just in front of the lens, facing it
	}
}

const UTDStoryFX* UTDStoryFX::Get()
{
	static TWeakObjectPtr<const UTDStoryFX> Cached;
	static bool bTried = false;
	if (!Cached.IsValid() && !bTried)
	{
		bTried = true;
		Cached = LoadObject<UTDStoryFX>(nullptr, AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Cached.Get();
}

void UTDStorySubsystem::Tick(float DeltaTime)
{
	const double Now = RealNow();
	if (!bDeparturesChecked)
	{
		UpdateDepartures(Now);
	}
	if (Current.bActive)
	{
		const float T = static_cast<float>(Now - OnAirStart);
		Current.Progress01 = FMath::Clamp(T / FMath::Max(OnAirSeconds, 0.01f), 0.f, 1.f);
		Current.TimeLeft = FMath::Max(0.f, OnAirSeconds - T);
		Current.Alpha = FMath::Min(FMath::Clamp(T / 0.2f, 0.f, 1.f), FMath::Clamp(Current.TimeLeft / 0.3f, 0.f, 1.f));
		if (T >= OnAirSeconds)
		{
			const FName Done = Current.Id;
			Current = FTDTransmissionView();
			Voice = nullptr;
			NextAllowed = Now + GapSeconds;
			if (IsRequiredLine(Done))
			{
				MarkPlayed(Done); // heard in full: now it is saved (N-062)
			}
			OnTransmissionEnded.Broadcast(Done, true);
		}
	}
	UpdateSequence();
	UpdateDronePulses(Now);
	if (!bAftermathChecked)
	{
		// Created after T7 armed it (a level load between the withdrawal and here): take it now.
		bAftermathChecked = true;
		if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this); Profile && Profile->IsAftermathPending())
		{
			HandleAftermathBegins(false);
		}
	}
	if (!bAftermathResumeChecked && ATDGalaxyLayout::Get(this))
	{
		// Lead (live walk): Aftermath was begun this run but a descent cut it: its unheard lines play again here.
		bAftermathResumeChecked = true;
		if (bAftermathChecked && ActiveSequence.IsNone() && HasPlayed(FName(AftermathStartedId)) && !HasPlayedAll(TEXT("Aftermath")))
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: Aftermath was cut short; resuming its unheard lines"));
			if (PlaySequence(this, TEXT("Aftermath")))
			{
				UTDUIHostSubsystem::ShowTacticalDisplay(this, 30.f);
			}
		}
	}
	if (!Current.bActive && Queue.Num() > 0 && Now >= NextAllowed && !IsHeld())
	{
		StartNext();
	}
	if (Current.bActive || Queue.Num() > 0)
	{
		EnsureOverlay();
	}
}

void UTDStorySubsystem::EnsureOverlay()
{
	UWorld* World = GetWorld();
	if (Overlay.IsValid() || !World || !GEngine || !GEngine->GameViewport || GEngine->GameViewport->GetWorld() != World)
	{
		return;
	}
	Overlay = SNew(TDStory::STDTransmissionOverlay, this);
	GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 905);
}

void UTDStorySubsystem::RemoveOverlay()
{
	if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	}
	Overlay.Reset();
}

void UTDStorySubsystem::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnWaveStarted.RemoveDynamic(this, &UTDStorySubsystem::HandleWaveStarted);
		Bus->OnWaveCleared.RemoveDynamic(this, &UTDStorySubsystem::HandleWaveCleared);
	}
	RemoveOverlay();
	ATDArchiveTransfer::OnAnyTerminalActivated.Remove(RelayHandle);
	ATDSurveyTerminal::OnAnySurveyRecordsAccessed.Remove(SurveyHandle);
	if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this))
	{
		Profile->OnAftermathBegins.Remove(AftermathHandle);
	}
	Super::Deinitialize();
}

// ------------------------------------------------------------------------------------------------ console

namespace TDStory
{
	static FAutoConsoleCommandWithWorldAndArgs CmdFire(TEXT("TD.Story.Fire"), TEXT("TD.Story.Fire <Id>: fire a story transmission"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 1) { UE_LOG(LogTriadDrift, Display, TEXT("TD.Story.Fire <Id>")); return; }
			const bool bOk = UTDStorySubsystem::Fire(World, FName(*Args[0]));
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Story.Fire %s: %s"), *Args[0], bOk ? TEXT("queued") : TEXT("refused (unknown, or already played this run)"));
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdList(TEXT("TD.Story.List"), TEXT("TD.Story.List: every transmission id, speaker, line and whether it played"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			const UTDStorySubsystem* S = UTDStorySubsystem::Get(World);
			for (const FTDTransmission& T : UTDStorySubsystem::GetAllEntries())
			{
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Story %s%s [%s] \"%s\""), *T.Id.ToString(), (S && S->HasPlayed(T.Id)) ? TEXT(" (played)") : TEXT(""),
					*UTDStorySubsystem::SpeakerTag(T.Speaker).ToString(), *T.Text.ToString());
			}
		}));

	static FAutoConsoleCommandWithWorld CmdStorySkip(TEXT("TD.Story.Skip"), TEXT("TD.Story.Skip: skip the playing story sequence (Cinematic 08 at the threshold)"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { if (UTDStorySubsystem* S = UTDStorySubsystem::Get(World)) { S->SkipSequence(); } }));
	static FAutoConsoleCommandWithWorldAndArgs CmdStorySeq(TEXT("TD.Story.Sequence"), TEXT("TD.Story.Sequence <Cinematic08|Docking|HangarArrival|FirstDoor|ClearSpace>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* World) { if (A.Num() > 0) { UTDStorySubsystem::PlaySequence(World, FName(*A[0])); } }));
	static FAutoConsoleCommandWithWorldAndArgs CmdReset(TEXT("TD.Story.Reset"), TEXT("TD.Story.Reset: forget which transmissions played this run"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTDStorySubsystem* S = UTDStorySubsystem::Get(World)) { S->ResetPlayed(); }
		}));
}

#undef LOCTEXT_NAMESPACE
