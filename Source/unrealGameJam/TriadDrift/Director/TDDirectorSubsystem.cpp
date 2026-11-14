// Triad Drift - Director (Teammate 3). C-03 #2 "Director cam money shots".

#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "TriadDrift/Director/TDDirectorSounds.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Engine/World.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"

FTDDirectorTuning::FTDDirectorTuning()
{
	// Bigger story beats matter more; everything is tunable.
	BaseImportance.Add(ETDHighlightKind::ReactorKill, 1.0f);
	BaseImportance.Add(ETDHighlightKind::GeneratorKill, 0.85f);
	BaseImportance.Add(ETDHighlightKind::CarrierKill, 0.8f);
	BaseImportance.Add(ETDHighlightKind::RamKill, 0.6f);
	BaseImportance.Add(ETDHighlightKind::PerfectDeflect, 0.6f);
	BaseImportance.Add(ETDHighlightKind::MissileReturn, 0.55f);
	BaseImportance.Add(ETDHighlightKind::MultiKill, 0.5f);
	BaseImportance.Add(ETDHighlightKind::SwitchStrikeKill, 0.5f);

	DurationOverrideSeconds.Add(ETDHighlightKind::PerfectDeflect, 0.15f);
}

UTDDirectorSubsystem* UTDDirectorSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDDirectorSubsystem>() : nullptr;
}

bool UTDDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTDDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CameraRig = NewObject<UTDDirectorCameraRig>(this);
	CameraRig->SetWorld(GetWorld());
	CutPresentation = NewObject<UTDCutPresentation>(this);
}

TStatId UTDDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDDirectorSubsystem, STATGROUP_Tickables);
}

void UTDDirectorSubsystem::Tick(float DeltaTime)
{
	// Real (undilated) time: slow-mo must never stretch the camera blends.
	const double Now = RealNow();
	const float RealDelta = LastTickRealTime < 0.0 ? 0.f : static_cast<float>(Now - LastTickRealTime);
	LastTickRealTime = Now;
	EnsureDirectorWidget();
	if (CutPresentation)
	{
		CutPresentation->SetHandledExternally(bCutPresentationHandledExternally);
		CutPresentation->SetExternalRenderer(DirectorWidget != nullptr);
		if (IsShotActive())
		{
			CutPresentation->SetShotKindName(FText::FromString(StaticEnum<ETDHighlightKind>()->GetNameStringByValue((int64)CurrentShot.Kind)));
		}
		const UTDGameInstance* GI = GetWorld() ? Cast<UTDGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
		if (GI && GI->IsTravelPending())
		{
			CutPresentation->ClearAll(); // nothing of this level's presentation survives travel
		}
		else
		{
			CutPresentation->Tick(Now);
		}
	}
	// ENG-35: and out, when the beat ends (or is cancelled).
	if (bSlowMoSounding && !IsShotActive())
	{
		bSlowMoSounding = false;
		FTDDirectorSounds::Play(GetWorld(), ETDDirectorSound::SlowMoOut);
	}
	UpdateAtmosphereShot(Now);
	UpdatePlayerProtection();
	if (!CameraRig)
	{
		return;
	}
	// The death cam lives inside the CUT (highlights are off then); travel still ends it.
	const bool bDeathCamHeld = CameraRig->IsDeathCam() && bSceneCutPending;
	if (CameraRig->IsActive() && !CanTriggerHighlights() && !bDeathCamHeld)
	{
		CameraRig->Cancel(bSceneCutPending ? ETDDirectorCameraCancel::SceneCut : ETDDirectorCameraCancel::Travel);
	}
	CameraRig->Tick(Now, RealDelta);
}

namespace TDDirectorProtect
{
	static TAutoConsoleVariable<int32> CVarProtect(TEXT("td.Director.ProtectPlayer"), 1,
		TEXT("1: the flown ship takes no damage while the director's camera is away, and for ReturnGraceSeconds after (AAA a23). 0: off (to measure)."));
}

void UTDDirectorSubsystem::UpdatePlayerProtection()
{
	// The camera is away for a shot (cut, hold, blend back), never for the death cam (the player is already dead).
	const bool bAway = (CameraRig && CameraRig->IsActive() && !CameraRig->IsDeathCam()) || Atmosphere.bActive;
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	UTDHealthComponent* Health = Squad ? UTDStatics::GetHealth(Squad->GetActiveShip()) : nullptr;
	if (!bAway || !Health)
	{
		bProtectingPlayer = false;
		LastProtectedHealth = -1.f;
		return;
	}
	const float Now = Health->GetShield() + Health->GetHull();
	if (LastProtectedHealth >= 0.f && Now < LastProtectedHealth - 0.01f)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Director: the player's ship took %.1f damage while the camera was away (shot %d, %s)"),
			LastProtectedHealth - Now, ShotCount, *UEnum::GetValueAsString(CurrentShot.Kind));
	}
	LastProtectedHealth = Now;
	if (TDDirectorProtect::CVarProtect.GetValueOnGameThread() != 0)
	{
		// Refreshed every frame while away; once the camera is back it runs out over ReturnGraceSeconds of game time.
		Health->SetInvulnerableFor(Tuning.ReturnGraceSeconds);
		bProtectingPlayer = true;
	}
}

namespace TDAtmosphereShot
{
	static const FName HUDHold(TEXT("DirectorAtmosphere"));
}

bool UTDDirectorSubsystem::RequestAtmosphereShot(const UObject* WorldContext, ATDShip* Ship, bool bExit, float Seconds,
	TFunction<FTransform(float Alpha01)> CameraFromShipLocal)
{
	UTDDirectorSubsystem* D = Get(WorldContext);
	UWorld* World = D ? D->GetWorld() : nullptr;
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!D || !World || !PC || !Ship || !CameraFromShipLocal || UTDDirectorSettings::GetEffectiveMode() == ETDDirectorCameraMode::Off)
	{
		return false;
	}
	if (D->CameraRig && D->CameraRig->IsActive() && !D->CameraRig->IsDeathCam())
	{
		D->CameraRig->Cancel(ETDDirectorCameraCancel::ViewTakenOver); // a money shot gives way to the entry
	}
	if (!D->AtmosphereCamera)
	{
		FActorSpawnParameters P;
		P.ObjectFlags |= RF_Transient;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		D->AtmosphereCamera = World->SpawnActor<ATDDirectorCameraActor>(ATDDirectorCameraActor::StaticClass(), FTransform::Identity, P); // tick and collision off, like the rig's
		if (D->AtmosphereCamera)
		{
			D->AtmosphereCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			D->AtmosphereCamera->GetCameraComponent()->SetFieldOfView(70.f);
		}
	}
	if (!D->AtmosphereCamera)
	{
		return false;
	}
	D->Atmosphere.bActive = true;
	D->Atmosphere.bExit = bExit;
	D->Atmosphere.Ship = Ship;
	D->Atmosphere.Place = MoveTemp(CameraFromShipLocal);
	D->Atmosphere.Start = World->GetRealTimeSeconds();
	D->Atmosphere.Seconds = FMath::Max(0.1f, Seconds);
	// No engine view blend (an engine blend into this camera and a later SetViewTarget back overflowed the stack): cut to
	// the camera and ease it in from the chase camera's pose over BlendIn, in real time, as the money-shot rig does.
	D->Atmosphere.From = PC->PlayerCameraManager ? FTransform(PC->PlayerCameraManager->GetCameraRotation(), PC->PlayerCameraManager->GetCameraLocation())
		: D->Atmosphere.Place(0.f) * Ship->GetActorTransform();
	D->AtmosphereCamera->SetActorTransform(D->Atmosphere.From);
	PC->SetViewTarget(D->AtmosphereCamera);
	UTDUIHostSubsystem::PushHUDFocus(World, TDAtmosphereShot::HUDHold, 0.f, /*bHideCombat*/ true);
	UE_LOG(LogTriadDrift, Log, TEXT("Director: atmosphere %s shot, %.1f s"), bExit ? TEXT("exit") : TEXT("entry"), Seconds);
	return true;
}

void UTDDirectorSubsystem::EndAtmosphereShot(const UObject* WorldContext, bool bCut)
{
	UTDDirectorSubsystem* D = Get(WorldContext);
	if (!D || !D->Atmosphere.bActive)
	{
		return;
	}
	D->Atmosphere = FAtmosphereShot();
	UWorld* World = D->GetWorld();
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		if (APawn* Pawn = PC->GetPawn(); Pawn && PC->GetViewTarget() == D->AtmosphereCamera)
		{
			PC->SetViewTarget(Pawn); // a cut either way (bCut false is reserved for a later eased return)
		}
	}
	UTDUIHostSubsystem::PopHUDFocus(World, TDAtmosphereShot::HUDHold);
}

bool UTDDirectorSubsystem::IsAtmosphereShotActive(const UObject* WorldContext)
{
	const UTDDirectorSubsystem* D = Get(WorldContext);
	return D && D->Atmosphere.bActive;
}

void UTDDirectorSubsystem::UpdateAtmosphereShot(double)
{
	if (!Atmosphere.bActive)
	{
		return;
	}
	UWorld* World = GetWorld();
	const ATDShip* Ship = Atmosphere.Ship.Get();
	const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	if (!Ship || !AtmosphereCamera || (GI && GI->IsTravelPending()) || Now - Atmosphere.Start > Atmosphere.Seconds + 1.0)
	{
		EndAtmosphereShot(this, true); // the ship went, travel, or T1 never ended it
		return;
	}
	const float Alpha = FMath::Clamp(static_cast<float>((Now - Atmosphere.Start) / Atmosphere.Seconds), 0.f, 1.f);
	const FTransform Want = Atmosphere.Place(Alpha) * Ship->GetActorTransform();
	const float In = FMath::Clamp(static_cast<float>((Now - Atmosphere.Start) / 0.3), 0.f, 1.f);
	if (In < 1.f)
	{
		const float E = FMath::InterpEaseInOut(0.f, 1.f, In, 2.f);
		FTransform T;
		T.Blend(Atmosphere.From, Want, E);
		AtmosphereCamera->SetActorTransform(T);
	}
	else
	{
		AtmosphereCamera->SetActorTransform(Want);
	}
}

void UTDDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BindBus();
	if (CutPresentation)
	{
		CutPresentation->Init(&InWorld);
		CutPresentation->OnCardsChanged.AddUObject(this, &UTDDirectorSubsystem::RelayCards);
	}
}

FTDDirectorCards UTDDirectorSubsystem::GetDirectorCards() const
{
	return CutPresentation ? CutPresentation->GetCards() : FTDDirectorCards();
}

FTDDirectorCards UTDDirectorSubsystem::GetDirectorCardsFor(const UObject* WorldContextObject)
{
	const UTDDirectorSubsystem* D = Get(WorldContextObject);
	return D ? D->GetDirectorCards() : FTDDirectorCards();
}

void UTDDirectorSubsystem::RelayCards(const FTDDirectorCards& Cards)
{
	OnDirectorCardsChanged.Broadcast(Cards);
}

void UTDDirectorSubsystem::EnsureDirectorWidget()
{
	// N-038: Nick's renderer for the cards, made once per world for the first local player. The world takes it with it
	// (travel), it never takes input, and the fallback overlay yields while it exists.
	if (DirectorWidget || bDirectorWidgetLooked)
	{
		return;
	}
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !World->IsGameWorld() || !PC || !PC->IsLocalController())
	{
		return; // try again next tick: the player isn't there yet
	}
	bDirectorWidgetLooked = true;
	TSubclassOf<UUserWidget> Class = DirectorWidgetClassOverride;
	if (!Class)
	{
		if (const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
		{
			Class = Style->DirectorWidgetClass.LoadSynchronous();
		}
	}
	if (!Class)
	{
		return; // no renderer: the fallback overlay draws the cards
	}
	DirectorWidget = CreateWidget<UUserWidget>(PC, Class);
	if (DirectorWidget)
	{
		DirectorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (World->GetGameViewport())
		{
			DirectorWidget->AddToViewport(UTDCutPresentation::OverlayZOrder(false));
		}
		UE_LOG(LogTriadDrift, Log, TEXT("Director: the cards are drawn by %s"), *Class->GetName());
	}
}

void UTDDirectorSubsystem::SetDirectorWidgetForReview(TSubclassOf<UUserWidget> Class)
{
	if (DirectorWidget)
	{
		DirectorWidget->RemoveFromParent();
		DirectorWidget = nullptr;
	}
	DirectorWidgetClassOverride = Class;
	bDirectorWidgetLooked = false; // looked up again next tick
}

void UTDDirectorSubsystem::Deinitialize()
{
	UnbindBus();
	if (CutPresentation)
	{
		CutPresentation->Shutdown(); // world teardown (travel): the overlay goes with it
	}
	if (CameraRig)
	{
		CameraRig->Cancel(ETDDirectorCameraCancel::Travel);
	}
	Super::Deinitialize();
}

void UTDDirectorSubsystem::BindBus()
{
	if (bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnActorKilled.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleActorKilled);
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleTakeStarted);
		bBusBound = true;
	}
}

void UTDDirectorSubsystem::UnbindBus()
{
	if (!bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnActorKilled.RemoveAll(this);
		Bus->OnSceneCut.RemoveAll(this);
		Bus->OnTakeStarted.RemoveAll(this);
	}
	bBusBound = false;
}

double UTDDirectorSubsystem::RealNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

double UTDDirectorSubsystem::GameNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

float UTDDirectorSubsystem::GetBaseImportance(ETDHighlightKind Kind) const
{
	const float* V = Tuning.BaseImportance.Find(Kind);
	return V ? *V : 0.5f;
}

bool UTDDirectorSubsystem::IsShotActive() const
{
	return RealNow() < ShotEndRealTime;
}

bool UTDDirectorSubsystem::CanTriggerHighlights() const
{
	if (bSceneCutPending)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	if (const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (GI->IsRetakePending() || GI->IsTravelPending())
		{
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// Highlights
// ---------------------------------------------------------------------------------------------

bool UTDDirectorSubsystem::ReportHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01, AActor* Secondary)
{
	UWorld* World = GetWorld();
	if (!World || !Subject || !CanTriggerHighlights())
	{
		return false; // m12: no subject = nothing to frame
	}
	if (Atmosphere.bActive)
	{
		return false; // T1's entry owns the camera
	}
	const float Importance = FMath::Clamp(Importance01 < 0.f ? GetBaseImportance(Kind) : Importance01, 0.f, 1.f);
	if (Importance < Tuning.MinImportance)
	{
		return false;
	}

	const double Now = RealNow();
	const bool bBypass = Importance >= Tuning.CooldownBypassImportance;
	const bool bShotWasActive = IsShotActive();
	if (IsShotActive())
	{
		// A running shot is replaced by something clearly bigger (generator kill during a ram kill),
		// and story beats (bypass level: generator, carrier, reactor) are never dropped: they take
		// over any shot that isn't more important, so all four generator kills get their beat.
		const bool bClearlyBigger = Importance >= CurrentShot.Importance01 + Tuning.PreemptMargin;
		const bool bStoryBeat = bBypass && Importance >= CurrentShot.Importance01;
		if (!bClearlyBigger && !bStoryBeat)
		{
			return false;
		}
	}
	else if (!bBypass)
	{
		if (Now < GlobalCooldownEndRealTime)
		{
			return false;
		}
		if (const double* KindEnd = KindCooldownEnd.Find(Kind); KindEnd && Now < *KindEnd)
		{
			return false;
		}
	}

	// Accept: build the shot.
	FTDDirectorShot Shot;
	Shot.Kind = Kind;
	Shot.Subject = Subject;
	Shot.Secondary = Secondary;
	Shot.Importance01 = Importance;
	const float* Override = Tuning.DurationOverrideSeconds.Find(Kind);
	Shot.DurationSeconds = Override ? *Override : FMath::Lerp(Tuning.MinSlowMoSeconds, Tuning.MaxSlowMoSeconds, Importance);
	Shot.Dilation = FMath::Lerp(Tuning.DilationAtMinImportance, Tuning.DilationAtMaxImportance, Importance);
	Shot.StartRealTime = static_cast<float>(Now);
	BuildCameraHint(Shot);

	CurrentShot = Shot;
	ShotEndRealTime = Now + Shot.DurationSeconds;
	GlobalCooldownEndRealTime = ShotEndRealTime + Tuning.GlobalCooldownSeconds;
	KindCooldownEnd.Add(Kind, Now + Tuning.SameKindCooldownSeconds);
	++ShotCount;
	UE_LOG(LogTriadDrift, Log, TEXT("Director shot %d: %s on %s (importance %.2f, %.1f s at %.2fx)"), ShotCount,
		*UEnum::GetValueAsString(Kind), *GetNameSafe(Subject), Importance, Shot.DurationSeconds, Shot.Dilation);

	if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(World))
	{
		Feel->RequestSlowMo(Shot.DurationSeconds, Shot.Dilation);
	}
	// ENG-35: the slow-motion whoosh in (once per beat: a shot replacing a running one keeps the slow motion going).
	if (!bShotWasActive)
	{
		FTDDirectorSounds::Play(World, ETDDirectorSound::SlowMoIn);
	}
	bSlowMoSounding = true;
	if (CutPresentation)
	{
		CutPresentation->HandleShotStarted(Shot.DurationSeconds + 0.2f); // thin bars for the shot (+ return)
	}
	// C-03 #2: the camera. Full = cut to the director camera; Reduced = FOV punch only; Off = none.
	if (CameraRig)
	{
		switch (UTDDirectorSettings::GetEffectiveMode())
		{
		case ETDDirectorCameraMode::Full:
			CameraRig->BeginShot(Shot, ShotEndRealTime);
			break;
		case ETDDirectorCameraMode::Reduced:
			CameraRig->PunchFOV();
			break;
		default:
			break;
		}
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(World))
	{
		Bus->BroadcastHighlightMoment(Kind, Subject, Importance);
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(World))
	{
		Flow->AddStat(TEXT("HighlightMoments"), 1);
	}
	OnDirectorShot.Broadcast(CurrentShot);
	return true;
}

void UTDDirectorSubsystem::BuildCameraHint(FTDDirectorShot& Shot)
{
	const AActor* Subject = Shot.Subject.Get();
	const AActor* Other = Shot.Secondary.Get();
	FVector Focus = Subject ? Subject->GetActorLocation() : (Other ? Other->GetActorLocation() : FVector::ZeroVector);
	if (Subject && Other && FVector::Dist(Subject->GetActorLocation(), Other->GetActorLocation()) <= Tuning.MaxTwoShotDistance)
	{
		// Two-shot: frame between them, weighted to the subject.
		Focus = FMath::Lerp(Subject->GetActorLocation(), Other->GetActorLocation(), 0.35f);
	}
	Shot.FocusLocation = Focus;

	// Travel direction of the action (subject's velocity, else its facing, else world forward).
	FVector Travel = Subject ? Subject->GetVelocity() : FVector::ZeroVector;
	if (Travel.SizeSquared() < 1.f)
	{
		Travel = Subject ? Subject->GetActorForwardVector() : FVector::ForwardVector;
	}
	Travel = Travel.GetSafeNormal();
	FVector Side = FVector::CrossProduct(Travel, FVector::UpVector).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::RightVector;
	}
	if (bCameraSideFlip)
	{
		Side = -Side;
	}
	bCameraSideFlip = !bCameraSideFlip; // alternate sides shot to shot

	float Dist = FMath::Lerp(Tuning.CameraDistanceMin, Tuning.CameraDistanceMax, Shot.Importance01);
	// Multi-kill (Lead): the victims are gone the instant they die, so a shot on the killer read like the
	// chase view. Frame the cluster of explosions instead: hold on its centre from behind, above and to
	// the side of the player, far enough out for the whole spread.
	if (Shot.Kind == ETDHighlightKind::MultiKill && !PendingClusterCentroid.IsZero())
	{
		const AActor* Killer = Shot.Subject.Get();
		const FVector From = Killer ? Killer->GetActorLocation() : PendingClusterCentroid - FVector::ForwardVector * 3000.f;
		FVector Toward = (PendingClusterCentroid - From).GetSafeNormal();
		if (Toward.IsNearlyZero()) { Toward = FVector::ForwardVector; }
		FVector ClusterSide = FVector::CrossProduct(Toward, FVector::UpVector).GetSafeNormal();
		if (ClusterSide.IsNearlyZero()) { ClusterSide = FVector::RightVector; }
		const float ClusterDist = FMath::Max(Tuning.CameraDistanceMax, PendingClusterSpread * 3.f + 2000.f);
		Shot.FocusLocation = PendingClusterCentroid;
		Shot.bStaticFocus = true;
		Shot.IgnoreForPlacement = RecentKillVictims;
		Shot.SuggestedCameraLocation = PendingClusterCentroid - Toward * ClusterDist * 0.75f + ClusterSide * ClusterDist * 0.45f + FVector::UpVector * ClusterDist * 0.45f;
		Shot.SuggestedCameraRotation = (PendingClusterCentroid - Shot.SuggestedCameraLocation).Rotation();
		return;
	}
	// Big subjects (a carrier, a generator on the Dominion's hull) need the camera further out, or the
	// frame is a wall of hull (live carrier-kill capture): at least SubjectFramingRadii x its radius.
	if (Subject)
	{
		FVector Origin, Extent;
		Subject->GetActorBounds(/*bOnlyColliding*/ true, Origin, Extent);
		Dist = FMath::Max(Dist, Extent.Size() * Tuning.SubjectFramingRadii);
	}
	// Side-on, a little ahead and above: the action crosses the frame toward the lens.
	Shot.SuggestedCameraLocation = Focus + Side * Dist * 0.85f + Travel * Dist * 0.3f + FVector::UpVector * Dist * 0.3f;
	Shot.SuggestedCameraRotation = (Focus - Shot.SuggestedCameraLocation).Rotation();
}

// ---------------------------------------------------------------------------------------------
// Automatic detection from kills
// ---------------------------------------------------------------------------------------------

void UTDDirectorSubsystem::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	if (!Victim || !Killer)
	{
		return;
	}
	// Only the player's side earns money shots, and only for killing hostiles.
	if (UTDStatics::GetTeam(Killer) != ETDTeam::Player || !UTDStatics::AreTeamsHostile(ETDTeam::Player, UTDStatics::GetTeam(Victim)))
	{
		return;
	}
	// M8: kills by the ship the player flies (with a squad present); wingman kills don't count.
	const ATDSquadManager* SquadForKills = ATDSquadManager::Get(this);
	const bool bPlayerKill = !SquadForKills || !SquadForKills->GetActiveShip() || Killer == SquadForKills->GetActiveShip();

	// Multi-kill bookkeeping, on REAL time (M8: slow-mo must not stretch the window).
	const double Now = RealNow();
	if (bPlayerKill)
	{
		RecentKillTimes.Add(Now);
		RecentKillLocations.Add(Victim->GetActorLocation());
		RecentKillVictims.Add(Victim);
	}
	while (RecentKillTimes.Num() > 0 && Now - RecentKillTimes[0] > Tuning.MultiKillWindowSeconds)
	{
		RecentKillTimes.RemoveAt(0, EAllowShrinking::No);
		if (RecentKillLocations.Num() > 0) { RecentKillLocations.RemoveAt(0, EAllowShrinking::No); }
		if (RecentKillVictims.Num() > 0) { RecentKillVictims.RemoveAt(0, EAllowShrinking::No); }
	}
	if (RecentKillTimes.Num() > 32)
	{
		RecentKillTimes.RemoveAt(0, RecentKillTimes.Num() - 32, EAllowShrinking::No);
		RecentKillLocations.RemoveAt(0, FMath::Max(0, RecentKillLocations.Num() - 32), EAllowShrinking::No);
		RecentKillVictims.RemoveAt(0, FMath::Max(0, RecentKillVictims.Num() - 32), EAllowShrinking::No);
	}
	// Where the streak happened (the multi-kill frames the cluster, not the killer).
	PendingClusterCentroid = FVector::ZeroVector;
	PendingClusterSpread = 0.f;
	for (const FVector& L : RecentKillLocations) { PendingClusterCentroid += L; }
	if (RecentKillLocations.Num() > 0)
	{
		PendingClusterCentroid /= RecentKillLocations.Num();
		for (const FVector& L : RecentKillLocations) { PendingClusterSpread = FMath::Max(PendingClusterSpread, FVector::Dist(L, PendingClusterCentroid)); }
	}

	// Pick the single best reading of this kill.
	ETDHighlightKind BestKind = ETDHighlightKind::RamKill;
	float BestImportance = -1.f;
	AActor* BestSubject = nullptr;
	AActor* BestSecondary = nullptr;
	auto Consider = [&](ETDHighlightKind K, float Importance, AActor* Subject, AActor* Secondary)
	{
		if (Importance > BestImportance)
		{
			BestKind = K;
			BestImportance = Importance;
			BestSubject = Subject;
			BestSecondary = Secondary;
		}
	};

	if (Victim->ActorHasTag(TDDirectorTags::Reactor))
	{
		Consider(ETDHighlightKind::ReactorKill, GetBaseImportance(ETDHighlightKind::ReactorKill), Victim, Killer);
	}
	if (Victim->ActorHasTag(TDDirectorTags::Generator))
	{
		Consider(ETDHighlightKind::GeneratorKill, GetBaseImportance(ETDHighlightKind::GeneratorKill), Victim, Killer);
	}
	if (Victim->ActorHasTag(TDDirectorTags::Carrier))
	{
		Consider(ETDHighlightKind::CarrierKill, GetBaseImportance(ETDHighlightKind::CarrierKill), Victim, Killer);
	}
	if (Kind == ETDDamageKind::Ram)
	{
		Consider(ETDHighlightKind::RamKill, GetBaseImportance(ETDHighlightKind::RamKill), Killer, Victim);
	}
	const int32 Streak = RecentKillTimes.Num();
	if (bPlayerKill && Streak >= Tuning.MultiKillCount)
	{
		// M8: never into the story tier (cap below the cooldown-bypass level).
		const float Extra = Tuning.MultiKillImportancePerExtraKill * static_cast<float>(Streak - Tuning.MultiKillCount);
		const float Cap = FMath::Min(Tuning.MultiKillMaxImportance, Tuning.CooldownBypassImportance - 0.05f);
		Consider(ETDHighlightKind::MultiKill, FMath::Min(Cap, GetBaseImportance(ETDHighlightKind::MultiKill) + Extra), Killer, Victim);
	}
	if (const ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		if (Squad->IsSwitchStrikeActive() && Squad->GetDamageMultiplier(Killer) > 1.f)
		{
			Consider(ETDHighlightKind::SwitchStrikeKill, GetBaseImportance(ETDHighlightKind::SwitchStrikeKill), Killer, Victim);
		}
	}

	if (BestImportance >= 0.f && ReportHighlight(BestKind, BestSubject, BestImportance, BestSecondary))
	{
		if (BestKind == ETDHighlightKind::MultiKill)
		{
			RecentKillTimes.Reset(); // M8: the next multi-kill needs a fresh run of kills
			RecentKillLocations.Reset();
			RecentKillVictims.Reset();
		}
	}
}

void UTDDirectorSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	// "CUT!": no money shots until the next take starts, and stop any running one.
	bSceneCutPending = true;
	if (CameraRig)
	{
		CameraRig->Cancel(ETDDirectorCameraCancel::SceneCut); // the retake starts on the player's own view
		// The pilot died on foot: cut to the third-person death cam for the CUT, the one time the player
		// sees their own pilot. In a ship the CUT stays presentation only.
		APlayerController* Player = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (Player)
		{
			CameraRig->BeginDeathCam(Player->GetPawn());
		}
	}
	if (CutPresentation)
	{
		CutPresentation->HandleSceneCut(DelaySeconds);
	}
	if (IsShotActive())
	{
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(GetWorld()))
		{
			Feel->CancelAll(); // the retake runs at normal speed
		}
	}
	ShotEndRealTime = -1000.0;
	RecentKillTimes.Reset();
}

void UTDDirectorSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	if (CutPresentation)
	{
		CutPresentation->HandleTakeStarted(SceneId, TakeNumber);
	}
	if (CameraRig && CameraRig->IsDeathCam())
	{
		CameraRig->Cancel(ETDDirectorCameraCancel::SceneCut); // back to the pilot's own eyes for the new take
	}
	bSceneCutPending = false;
}
