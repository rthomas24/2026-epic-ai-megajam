// Triad Drift - story transmissions (see header and storyline.md). Owner: Teammate 3.

#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Flow/TDGameInstance.h"
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
			// First Contact (docs/story/First-Contact-Cinematic-Brief.md): the opening, ~2:20, re-shot once the art is in.
			// Draft lines from the brief; Nicholas's words replace them.
			E(TEXT("O_PatrolSeven"), S::PatrolSeven, LOCTEXT("O_PatrolSeven", "Command, we have an unidentified structure on the outer perimeter. It's not on any chart. Wait, something's coming out of-"), St::Static),
			E(TEXT("O_Command_Orders"), S::Command, LOCTEXT("O_Command_Orders", "Triad Squadron, investigate Patrol Seven's last coordinates. Identify the contact and report. Recon only. Do not engage.")),
			E(TEXT("O_Wren_HowBad"), S::Wren, LOCTEXT("O_Wren_HowBad", "Three ships, one mystery. How bad could it be?")),
			E(TEXT("O_Kato_StopSaying"), S::Kato, LOCTEXT("O_Kato_StopSaying", "Please stop saying that.")),
			E(TEXT("O_Moss_Especially"), S::Moss, LOCTEXT("O_Moss_Especially", "Especially before we've seen it.")),
			E(TEXT("O_Wren_BigShip"), S::Wren, LOCTEXT("O_Wren_BigShip", "Okay. That is a big ship.")),
			E(TEXT("O_Kato_NoSignature"), S::Kato, LOCTEXT("O_Kato_NoSignature", "No propulsion signature. No answer to our hails. Wait, we're receiving something.")),
			E(TEXT("O_Moss_Message"), S::Moss, LOCTEXT("O_Moss_Message", "A message?")),
			E(TEXT("O_Kato_Handshake"), S::Kato, LOCTEXT("O_Kato_Handshake", "More like a handshake. And now it's gone.")),
			E(TEXT("O_Wren_WhyNotFiring"), S::Wren, LOCTEXT("O_Wren_WhyNotFiring", "Why isn't that one firing?")),
			E(TEXT("O_Kato_Contacts"), S::Kato, LOCTEXT("O_Kato_Contacts", "Unknown. Multiple contacts are turning toward us.")),
			E(TEXT("O_Command_Disengage"), S::Command, LOCTEXT("O_Command_Disengage", "Triad, disengage. Repeat, disengage immediately.")),
			E(TEXT("O_Unknown_Dont"), S::Unknown, LOCTEXT("O_Unknown_Dont", "Don't-"), St::Static, TEXT("Dont")),
			E(TEXT("O_Wren_Break"), S::Wren, LOCTEXT("O_Wren_Break", "Triad, break formation!")),
			E(TEXT("O_Command_Evade"), S::Command, LOCTEXT("O_Command_Evade", "Pilot, incoming fire! Evade now!")),
			// Act 1: First Contact, the Watcher (waves 1-2).
			E(TEXT("A1_HailNoAnswer"), S::Kato, LOCTEXT("A1_HailNoAnswer", "Hailing on every band. Nothing. It isn't even listening.")),
			E(TEXT("A1_TheySearch"), S::Moss, LOCTEXT("A1_TheySearch", "They're not holding ground. They scan, they sweep, they move on.")),
			E(TEXT("A1_FieldUnreachable"), S::Wren, LOCTEXT("A1_FieldUnreachable", "That shimmer around the ship: some kind of field. We can't get near it.")),
			E(TEXT("A1_TrailToVerdance"), S::Control, LOCTEXT("A1_TrailToVerdance", "Their landers are converging on Verdance. Follow the trail.")),
			// Act 2: Verdance, the Voices.
			E(TEXT("A2_DeviceFound"), S::Kato, LOCTEXT("A2_DeviceFound", "A signal-analysis device. Someone here was listening to them.")),
			E(TEXT("A2_Voices"), S::Unknown, LOCTEXT("A2_Voices", "...hold the line... vector two... [static] ...is anyone receiving..."), St::Static),
			E(TEXT("A2_TrailToCinder"), S::Moss, LOCTEXT("A2_TrailToCinder", "Human voices, under their signal. The data points to Cinder.")),
			// Act 3: Cinder, the Pattern.
			E(TEXT("A3_CrystalsFound"), S::Kato, LOCTEXT("A3_CrystalsFound", "These crystals stabilise the field. They were digging a way through it.")),
			E(TEXT("A3_EchoWren"), S::Unknown, LOCTEXT("A3_EchoWren", "...I see an opening. I'm taking it..."), St::Echo),
			E(TEXT("A3_EchoKato"), S::Unknown, LOCTEXT("A3_EchoKato", "...understand it before we commit..."), St::Echo),
			E(TEXT("A3_EchoMoss"), S::Unknown, LOCTEXT("A3_EchoMoss", "...stay together. We're not done yet..."), St::Echo),
			E(TEXT("A3_PassageOpen"), S::Control, LOCTEXT("A3_PassageOpen", "The anchor is down. There's a passage through the field. Go.")),
			// Act 4: the Dominion, the Truth (the reveal may begin here).
			E(TEXT("A4_GeneratorDown1"), S::Control, LOCTEXT("A4_GeneratorDown1", "Generator one is down. The field is flickering.")),
			E(TEXT("A4_GeneratorDown2"), S::Kato, LOCTEXT("A4_GeneratorDown2", "Two down. The field is losing shape.")),
			E(TEXT("A4_GeneratorDown3"), S::Wren, LOCTEXT("A4_GeneratorDown3", "Three! One more and it's open.")),
			E(TEXT("A4_GeneratorDown4"), S::Moss, LOCTEXT("A4_GeneratorDown4", "That's the last one. The field is gone.")),
			E(TEXT("A4_DoorsKnowUs"), S::Wren, LOCTEXT("A4_DoorsKnowUs", "The door opened for my ID. Why does it know my ID?")),
			E(TEXT("A4_Recording_Hangar"), S::FutureWren, LOCTEXT("A4_Recording_Hangar", "If you're hearing this, you came anyway. Of course you did."), St::Echo),
			E(TEXT("A4_Recording_Corridor"), S::FutureKato, LOCTEXT("A4_Recording_Corridor", "We ran every number a thousand times. This is the only way."), St::Echo),
			E(TEXT("A4_Recording_Barracks"), S::FutureMoss, LOCTEXT("A4_Recording_Barracks", "We were you, once. We know what you'll choose."), St::Echo),
			E(TEXT("A4_Recording_Core"), S::FutureWren, LOCTEXT("A4_Recording_Core", "The Origin Core can stop it. Don't take that from us."), St::Echo),
			// Finale payoff: the future command deck, the same moment from the other side ("Don't-" is the SAME sound).
			E(TEXT("F_FutureKato_Signatures"), S::FutureKato, LOCTEXT("F_FutureKato_Signatures", "All three signatures confirmed. It's us.")),
			E(TEXT("F_FutureMoss_Where"), S::FutureMoss, LOCTEXT("F_FutureMoss_Where", "They're exactly where we were.")),
			E(TEXT("F_FutureWren_Channel"), S::FutureWren, LOCTEXT("F_FutureWren_Channel", "Open a channel. Now.")),
			E(TEXT("F_FutureKato_Interference"), S::FutureKato, LOCTEXT("F_FutureKato_Interference", "Temporal interference. Less than a second.")),
			E(TEXT("F_FutureWren_Dont"), S::FutureWren, LOCTEXT("F_FutureWren_Dont", "Don't-"), St::Static, TEXT("Dont")),
			// Act 5: Reactor and Escape, the Choice.
			E(TEXT("A5_Choice"), S::Moss, LOCTEXT("A5_Choice", "We can't trust them, and we can't let them finish. We end it here.")),
			E(TEXT("A5_Escape"), S::Control, LOCTEXT("A5_Escape", "The ship is coming apart. Ninety seconds. Move!")),
			E(TEXT("A5_DistressSignal"), S::Unknown, LOCTEXT("A5_DistressSignal", "DISTRESS SIGNAL  ·  TRIAD: WREN, KATO, MOSS  ·  DATED FORTY-ONE YEARS FROM TODAY"), St::Static),
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
			// Centred at 62% height: above the pilot name card (70%), the boost bar (73%) and the tutorial card (77-84%).
			FVector2D Pos(Size.X * 0.5f - W * 0.5f, Size.Y * 0.62f - LineSize.Y * 0.5f);
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
	default: return LOCTEXT("TagControl", "CONTROL");
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

bool UTDStorySubsystem::FireTransmission(FName Id)
{
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
	if (!Entry.bRepeatable)
	{
		MarkPlayed(Id); // claimed now: a Continue before it airs never replays it
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

void UTDStorySubsystem::StartNext()
{
	OnAir = Queue[0];
	Queue.RemoveAt(0);
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
	Current = FTDTransmissionView();
	Current.bActive = true;
	Current.Id = OnAir.Id;
	Current.Speaker = OnAir.Speaker;
	Current.SpeakerTag = SpeakerTag(OnAir.Speaker);
	Current.Text = OnAir.Text;
	Current.Style = OnAir.Style;
	Current.Color = SpeakerColor(OnAir.Speaker);
	Current.TimeLeft = OnAirSeconds;
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Story: on air %s [%s] \"%s\" (%.1f s)"), *OnAir.Id.ToString(), *Current.SpeakerTag.ToString(),
		*OnAir.Text.ToString(), OnAirSeconds);
	OnTransmission.Broadcast(Current);
}

void UTDStorySubsystem::Tick(float DeltaTime)
{
	const double Now = RealNow();
	if (Current.bActive)
	{
		const float T = static_cast<float>(Now - OnAirStart);
		Current.Progress01 = FMath::Clamp(T / FMath::Max(OnAirSeconds, 0.01f), 0.f, 1.f);
		Current.TimeLeft = FMath::Max(0.f, OnAirSeconds - T);
		Current.Alpha = FMath::Min(FMath::Clamp(T / 0.2f, 0.f, 1.f), FMath::Clamp(Current.TimeLeft / 0.3f, 0.f, 1.f));
		if (T >= OnAirSeconds)
		{
			Current = FTDTransmissionView();
			Voice = nullptr;
			NextAllowed = Now + GapSeconds;
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
	RemoveOverlay();
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

	static FAutoConsoleCommandWithWorldAndArgs CmdReset(TEXT("TD.Story.Reset"), TEXT("TD.Story.Reset: forget which transmissions played this run"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTDStorySubsystem* S = UTDStorySubsystem::Get(World)) { S->ResetPlayed(); }
		}));
}

#undef LOCTEXT_NAMESPACE
