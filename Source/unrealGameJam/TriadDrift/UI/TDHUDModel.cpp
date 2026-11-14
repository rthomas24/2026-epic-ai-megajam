// Triad Drift - the data behind the richer flight HUD (issue #160; Teammate 2). See the header.

#include "TriadDrift/UI/TDHUDModel.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Waves/TDWarpInMarker.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "UObject/UnrealType.h"
#include "TriadDrift/World/TDPortalGate.h"

int32 FTDHUDModel::CountOf(ETDRadarType Type) const
{
	int32 N = 0;
	for (const FTDRadarContact& C : Contacts)
	{
		N += C.Type == Type ? 1 : 0;
	}
	return N;
}

void FTDHUDModel::Update(const UWorld* World, const TArray<FTDShipHUD>& Cards, const TArray<FTDTargetMarker>& Markers,
	const TArray<AActor*>& Objectives, float Dt)
{
	UpdateSquad(World, Cards, Dt);
	UpdateContacts(World, Markers, Objectives, Dt);
}

void FTDHUDModel::UpdateSquad(const UWorld* World, const TArray<FTDShipHUD>& Cards, float Dt)
{
	const ATDSquadManager* SquadManager = World ? ATDSquadManager::Get(World) : nullptr;
	const UTDPickupSubsystem* Pickups = World ? World->GetSubsystem<UTDPickupSubsystem>() : nullptr;
	const float OverLeft = Pickups ? Pickups->GetOverchargeRemaining() : 0.f;
	const float OverFull = Pickups ? FMath::Max(.01f, Pickups->GetOverchargeSeconds()) : 1.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FTDSquadStatus& S = Squad[Index];
		S.Ship = static_cast<ETDShipId>(Index);
		const FTDShipHUD* Card = Cards.FindByPredicate([Index](const FTDShipHUD& C) { return static_cast<int32>(C.Ship) == Index; });
		S.bValid = Card != nullptr;
		if (!Card)
		{
			LastHealth[Index] = -1.f;
			S.UnderFire01 = 0.f;
			continue;
		}
		S.bFlown = Card->bPlayerControlled;
		S.bAlive = Card->bAlive;
		S.Hull = Card->bAlive ? FMath::Clamp(Card->Health01, 0.f, 1.f) : 0.f;
		S.Shield = Card->bAlive ? FMath::Clamp(Card->Shield01, 0.f, 1.f) : 0.f;
		S.RespawnSeconds = Card->bAlive ? 0.f : FMath::Max(0.f, Card->RespawnSeconds);
		S.Power01 = FMath::Clamp(Card->PowerCooldown01, 0.f, 1.f);
		S.Power201 = FMath::Clamp(Card->Power2Cooldown01, 0.f, 1.f);
		S.Ability = Card->Ability;
		S.Ability01 = FMath::Clamp(Card->AbilityCooldown01, 0.f, 1.f);
		S.Boost01 = FMath::Clamp(Card->Boost01, 0.f, 1.f);
		S.Overcharge01 = FMath::Clamp(OverLeft / OverFull, 0.f, 1.f);
		S.OverchargeSeconds = OverLeft;
		S.bTrouble = S.bAlive && S.Hull < TroubleHull;

		// Power states from the ship's own components (every ship, not only the flown one).
		const ATDShip* ShipActor = SquadManager ? SquadManager->GetShip(S.Ship) : nullptr;
		const UTDPowerComponent* P1 = ShipActor ? UTDPowerComponent::FindPower(ShipActor, ETDPowerInput::Signature1) : nullptr;
		const UTDPowerComponent* P2 = ShipActor ? UTDPowerComponent::FindPower(ShipActor, ETDPowerInput::Signature2) : nullptr;
		const UTDPowerComponent* PA = ShipActor ? UTDPowerComponent::FindPower(ShipActor, ETDPowerInput::Ability) : nullptr;
		S.bPowerActive = P1 && P1->IsPowerActive();
		S.bHasPower2 = P2 != nullptr;
		S.bPower2Active = P2 && P2->IsPowerActive();
		S.bAbilityActive = PA && PA->IsPowerActive();
		S.PowerSeconds = P1 ? P1->GetCooldownRemaining() : 0.f;
		S.AbilitySeconds = PA ? PA->GetCooldownRemaining() : 0.f;
		if (PA && S.Ability == ETDAbilityId::None)
		{
			S.Ability = PA->GetAbilityId();
			S.Ability01 = PA->GetCooldown01();
		}

		// Under fire: hull + shield went down since the last frame (a hit, whatever its source); fades over 1.5 s.
		const float Health = S.bAlive ? S.Hull + S.Shield : -1.f;
		if (S.bAlive && LastHealth[Index] >= 0.f && Health < LastHealth[Index] - 1e-4f)
		{
			S.UnderFire01 = 1.f;
		}
		else
		{
			S.UnderFire01 = FMath::Max(0.f, S.UnderFire01 - Dt / UnderFireSeconds);
		}
		if (!S.bAlive)
		{
			S.UnderFire01 = 0.f;
		}
		LastHealth[Index] = Health;
	}
}

void FTDHUDModel::Add(ETDRadarType Type, const FVector& WorldPos, uint8 Kind)
{
	const FVector Local = Frame.InverseTransformPositionNoScale(WorldPos) / 100.f; // X forward, Y right, Z up (m)
	FTDRadarContact& C = Contacts.AddDefaulted_GetRef();
	C.Type = Type;
	C.Kind = Kind;
	C.Plane = FVector2D(Local.Y, Local.X);
	C.Up = Local.Z;
	C.Range = Local.Size();
	C.bBeyond = C.Plane.Size() > RadarRange;
}

void FTDHUDModel::UpdateContacts(const UWorld* World, const TArray<FTDTargetMarker>& Markers, const TArray<AActor*>& Objectives, float Dt)
{
	Contacts.Reset();
	if (Contacts.Max() < 128)
	{
		Contacts.Reserve(128);
	}
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Me = PC ? PC->GetPawn() : nullptr;
	bHasFrame = Me != nullptr;
	if (!Me)
	{
		return;
	}
	Frame = FTransform(Me->GetActorQuat(), Me->GetActorLocation());
	const FVector Here = Me->GetActorLocation();
	const float ListCm = ListRange * 100.f;

	// Enemies: every live hostile the registry knows (not only the targeting list), with its kind and whether it hunts us.
	Scratch.Reset();
	if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(World))
	{
		Registry->GetHostilesOf(ETDTeam::Player, Scratch);
	}
	for (const AActor* A : Scratch)
	{
		if (!IsValid(A) || FVector::DistSquared(Here, A->GetActorLocation()) > ListCm * ListCm)
		{
			continue;
		}
		Add(ETDRadarType::Enemy, A->GetActorLocation(), UTDFlightHUDWidget::KindOf(A));
		const APawn* EnemyPawn = Cast<APawn>(A);
		const ATDEnemyAIController* AI = EnemyPawn ? Cast<ATDEnemyAIController>(EnemyPawn->GetController()) : nullptr;
		Contacts.Last().bThreat = AI && AI->GetTarget() == Me;
	}
	// Missiles homing on the player: the source's target markers already list them.
	for (const FTDTargetMarker& M : Markers)
	{
		if (M.bIncomingMissile)
		{
			Add(ETDRadarType::Missile, M.Actor.IsValid() ? M.Actor->GetActorLocation() : M.WorldLocation);
			Contacts.Last().bThreat = true;
		}
	}
	// Wingmen.
	if (const ATDSquadManager* SquadManager = ATDSquadManager::Get(World))
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const ATDShip* Ship = SquadManager->GetShip(static_cast<ETDShipId>(Index));
			if (Ship && Ship != Me && !Ship->IsHidden())
			{
				Add(ETDRadarType::Wingman, Ship->GetActorLocation(), static_cast<uint8>(Index));
			}
		}
	}
	// Objective waypoints (generators, the reactor, planets, the landing pad...).
	for (const AActor* Marker : Objectives)
	{
		if (IsValid(Marker) && Marker != Me)
		{
			Add(ETDRadarType::Objective, Marker->GetActorLocation());
		}
	}
	// Pickups.
	if (const UTDPickupSubsystem* Pickups = World->GetSubsystem<UTDPickupSubsystem>())
	{
		for (const ATDPickup* Pk : Pickups->GetPickups())
		{
			if (IsValid(Pk) && FVector::DistSquared(Here, Pk->GetActorLocation()) <= ListCm * ListCm)
			{
				Add(ETDRadarType::Pickup, Pk->GetActorLocation(), static_cast<uint8>(Pk->GetType()));
			}
		}
	}
	// Portal gates: the list is rescanned once a second (they never move); only open ones show.
	GateScanLeft -= Dt;
	if (GateScanLeft <= 0.f)
	{
		GateScanLeft = 1.f;
		Gates.Reset();
		for (TActorIterator<ATDPortalGate> It(const_cast<UWorld*>(World)); It; ++It)
		{
			Gates.Add(*It);
		}
	}
	for (const TWeakObjectPtr<AActor>& G : Gates)
	{
		const ATDPortalGate* Gate = Cast<ATDPortalGate>(G.Get());
		if (Gate && Gate->IsGateActive())
		{
			Add(ETDRadarType::Gate, Gate->GetActorLocation());
		}
	}
	// Warp-in markers: a group arriving in about 1.5 s.
	for (TActorIterator<ATDWarpInMarker> It(const_cast<UWorld*>(World)); It; ++It)
	{
		Add(ETDRadarType::WarpIn, It->GetActorLocation());
		Contacts.Last().bThreat = true;
	}

	// Incoming groups (T4's wave director): a blip at the warp-in point once it is known, otherwise listed with no blip.
	IncomingScanLeft -= Dt;
	if (IncomingScanLeft <= 0.f)
	{
		IncomingScanLeft = .25f;
		ReadIncoming(World);
	}
	for (FIncoming& G : Incoming)
	{
		G.Seconds = FMath::Max(0.f, G.Seconds - Dt);
		if (G.bHasLocation)
		{
			Add(ETDRadarType::Incoming, G.Location);
		}
		else
		{
			FTDRadarContact& C = Contacts.AddDefaulted_GetRef();
			C.Type = ETDRadarType::Incoming;
			C.Range = 1e9f;
			C.bBeyond = true;
		}
		FTDRadarContact& C = Contacts.Last();
		C.Id = G.Id;
		C.Count = G.Count;
		C.Seconds = G.Seconds;
		C.bHasLocation = G.bHasLocation;
		C.bNextWave = G.bNextWave;
		C.bThreat = !G.bNextWave;
	}

	// The must-shows first (missiles, warp-ins, objectives, gates), then the nearest; cap at MaxContacts.
	if (Contacts.Num() > MaxContacts)
	{
		auto Rank = [](ETDRadarType T)
		{
			switch (T)
			{
			case ETDRadarType::Missile: return 0;
			case ETDRadarType::WarpIn: case ETDRadarType::Incoming: return 1;
			case ETDRadarType::Objective: return 2;
			case ETDRadarType::Wingman: return 3;
			case ETDRadarType::Gate: return 4;
			default: return 5;
			}
		};
		Contacts.Sort([&Rank](const FTDRadarContact& A, const FTDRadarContact& B)
		{
			const int32 RA = Rank(A.Type), RB = Rank(B.Type);
			return RA != RB ? RA < RB : A.Range < B.Range;
		});
		Contacts.SetNum(MaxContacts, EAllowShrinking::No);
	}
}

void FTDHUDModel::ReadIncoming(const UWorld* World)
{
	Incoming.Reset();
	const ATDWaveDirector* Director = ATDWaveDirector::Get(World);
	UFunction* F = Director ? Director->FindFunction(TEXT("GetPendingGroups")) : nullptr;
	const FArrayProperty* Out = F ? CastField<FArrayProperty>(F->FindPropertyByName(TEXT("OutGroups"))) : nullptr;
	const FStructProperty* Inner = Out ? CastField<FStructProperty>(Out->Inner) : nullptr;
	if (!Inner)
	{
		return; // not on this build yet
	}
	const UScriptStruct* S = Inner->Struct;
	const FNameProperty* PId = FindFProperty<FNameProperty>(S, TEXT("EnemyId"));
	const FIntProperty* PCount = FindFProperty<FIntProperty>(S, TEXT("Count"));
	const FFloatProperty* PSec = FindFProperty<FFloatProperty>(S, TEXT("SecondsUntilSpawn"));
	const FStructProperty* PLoc = FindFProperty<FStructProperty>(S, TEXT("Location"));
	const FBoolProperty* PHas = FindFProperty<FBoolProperty>(S, TEXT("bHasLocation"));
	const FBoolProperty* PNext = FindFProperty<FBoolProperty>(S, TEXT("bNextWave"));
	uint8* Buf = static_cast<uint8*>(FMemory_Alloca(F->ParmsSize));
	FMemory::Memzero(Buf, F->ParmsSize);
	for (TFieldIterator<FProperty> It(F); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Buf);
	}
	const_cast<ATDWaveDirector*>(Director)->ProcessEvent(F, Buf);
	FScriptArrayHelper Arr(Out, Out->ContainerPtrToValuePtr<void>(Buf));
	for (int32 i = 0; i < Arr.Num(); ++i)
	{
		const uint8* E = Arr.GetRawPtr(i);
		FIncoming& G = Incoming.AddDefaulted_GetRef();
		G.Id = PId ? PId->GetPropertyValue_InContainer(E) : NAME_None;
		G.Count = PCount ? PCount->GetPropertyValue_InContainer(E) : 0;
		G.Seconds = PSec ? PSec->GetPropertyValue_InContainer(E) : 0.f;
		if (PLoc && PLoc->Struct == TBaseStructure<FVector>::Get())
		{
			G.Location = *PLoc->ContainerPtrToValuePtr<FVector>(E);
		}
		G.bHasLocation = PHas && PHas->GetPropertyValue_InContainer(E);
		G.bNextWave = PNext && PNext->GetPropertyValue_InContainer(E);
	}
	for (TFieldIterator<FProperty> It(F); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Buf);
	}
}

#if !UE_BUILD_SHIPPING
namespace TDHUDModelDebug
{
	static const TCHAR* TypeName(ETDRadarType T)
	{
		switch (T)
		{
		case ETDRadarType::Enemy: return TEXT("enemy");
		case ETDRadarType::Missile: return TEXT("missile");
		case ETDRadarType::Wingman: return TEXT("wingman");
		case ETDRadarType::Objective: return TEXT("objective");
		case ETDRadarType::Pickup: return TEXT("pickup");
		case ETDRadarType::Gate: return TEXT("gate");
		case ETDRadarType::WarpIn: return TEXT("warp-in");
		case ETDRadarType::Incoming: return TEXT("incoming");
		default: return TEXT("?");
		}
	}

	// TD.UI.Radar: log the flight HUD's squad block and radar contacts (issue #160, before the drawing lands).
	static FAutoConsoleCommandWithWorld CmdRadar(TEXT("TD.UI.Radar"), TEXT("TD.UI.Radar: log the flight HUD's squad status and radar contacts (#160)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
			const UTDFlightHUDWidget* HUD = Host ? Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD()) : nullptr;
			if (!HUD)
			{
				UE_LOG(LogTemp, Display, TEXT("TD.UI.Radar: no flight HUD"));
				return;
			}
			const FTDHUDModel& M = HUD->GetHUDModel();
			for (const FTDSquadStatus& S : M.Squad)
			{
				UE_LOG(LogTemp, Display, TEXT("TD.UI.Radar squad %d valid=%d flown=%d alive=%d hull=%.2f shield=%.2f power=%.2f%s power2=%s ability=%d %.2f%s boost=%.2f over=%.2f fire=%.2f trouble=%d"),
					(int32)S.Ship, S.bValid, S.bFlown, S.bAlive, S.Hull, S.Shield, S.Power01, S.bPowerActive ? TEXT(" ACTIVE") : TEXT(""),
					S.bHasPower2 ? *FString::Printf(TEXT("%.2f%s"), S.Power201, S.bPower2Active ? TEXT(" ACTIVE") : TEXT("")) : TEXT("-"),
					(int32)S.Ability, S.Ability01, S.bAbilityActive ? TEXT(" ACTIVE") : TEXT(""), S.Boost01, S.Overcharge01, S.UnderFire01, S.bTrouble);
			}
			UE_LOG(LogTemp, Display, TEXT("TD.UI.Radar contacts %d (enemy %d, missile %d, wingman %d, objective %d, pickup %d, gate %d, warp-in %d)"),
				M.Contacts.Num(), M.CountOf(ETDRadarType::Enemy), M.CountOf(ETDRadarType::Missile), M.CountOf(ETDRadarType::Wingman),
				M.CountOf(ETDRadarType::Objective), M.CountOf(ETDRadarType::Pickup), M.CountOf(ETDRadarType::Gate), M.CountOf(ETDRadarType::WarpIn));
			UE_LOG(LogTemp, Display, TEXT("TD.UI.Radar incoming %d"), M.CountOf(ETDRadarType::Incoming));
			for (const FTDRadarContact& C : M.Contacts)
			{
				UE_LOG(LogTemp, Display, TEXT("  %-9s kind=%d right=%.0f fwd=%.0f up=%.0f range=%.0f%s%s%s"), TypeName(C.Type), C.Kind, C.Plane.X, C.Plane.Y,
					C.Up, C.Range, C.bBeyond ? TEXT(" rim") : TEXT(""), C.bThreat ? TEXT(" THREAT") : TEXT(""),
					C.Type == ETDRadarType::Incoming ? *FString::Printf(TEXT(" %s x%d in %.1fs%s%s"), *C.Id.ToString(), C.Count, C.Seconds,
						C.bHasLocation ? TEXT("") : TEXT(" (no point yet)"), C.bNextWave ? TEXT(" next wave") : TEXT("")) : TEXT(""));
			}
		}));
}
#endif
