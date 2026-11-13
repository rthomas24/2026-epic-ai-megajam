// Triad Drift - flight feel readouts for agent playtests (dev builds only). Owner: Teammate 1.
//
//   TD.Flight.Threats   log the player ship's hull / shield, damage taken since the last call, the nearest enemy and how
//                       many enemies are within 1 km / 2 km (pilot's-eye view of a fight: am I being chased, am I hit)
//   TD.Flight.Feedback  log the no-art feedback: tick cost, frame time, dust motes, bursts, camera kick, placeholder counts

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "Misc/App.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Containers/Ticker.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/UnrealType.h"

namespace TDFlightDebug
{

	// Every squad ship's art: visuals asset, mesh, orientation and size against the collision, sockets, what still
	// shows of the placeholder, and where the exhaust is.
	void ShipArt(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		// Probe: does Core's loader find the squad visuals, and does the registry see them?
		for (const TCHAR* Name : { TEXT("DA_Rift_Visuals"), TEXT("DA_Lancer_Visuals"), TEXT("DA_Bastion_Visuals") })
		{
			UObject* Loaded = UTDStatics::LoadPresentationAsset(Name, UTDShipVisuals::StaticClass());
			int32 InCharacters = 0;
			FString Where;
			if (IAssetRegistry* Registry = IAssetRegistry::Get())
			{
				TArray<FAssetData> Found;
				Registry->GetAssetsByPath(FName(TEXT("/Game/TriadDrift/Gameplay/Characters")), Found, true);
				for (const FAssetData& D : Found)
				{
					if (D.AssetName == FName(Name))
					{
						++InCharacters;
						UObject* Direct = D.GetSoftObjectPath().TryLoad();
						Where = FString::Printf(TEXT("%s -> direct load %s (%s)"), *D.GetSoftObjectPath().ToString(), *GetNameSafe(Direct), Direct ? *Direct->GetClass()->GetName() : TEXT("-"));
					}
				}
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.ShipArt: probe %s: loader %s | registry: %d assets under Gameplay/Characters, match %d %s | still loading %d"),
					Name, *GetNameSafe(Loaded), Found.Num(), InCharacters, *Where, Registry->IsLoadingAssets() ? 1 : 0);
			}
		}
		for (TActorIterator<ATDShip> It(World); It; ++It)
		{
			ATDShip* Ship = *It;
			const UStaticMeshComponent* Hull = Ship->GetHullMesh();
			const UStaticMesh* Mesh = Hull ? Hull->GetStaticMesh() : nullptr;
			const float Radius = Ship->GetCollisionSphere() ? Ship->GetCollisionSphere()->GetScaledSphereRadius() : 0.f;
			FBox Local(ForceInit);
			if (Mesh) { Local = Mesh->GetBoundingBox(); }
			const FVector Size = Local.IsValid ? Local.GetSize() * (Hull ? Hull->GetComponentScale() : FVector::OneVector) : FVector::ZeroVector;
			FString Sockets;
			if (Mesh)
			{
				for (const UStaticMeshSocket* Sock : Mesh->Sockets)
				{
					if (Sock) { Sockets += FString::Printf(TEXT("%s(%.0f,%.0f,%.0f) "), *Sock->SocketName.ToString(), Sock->RelativeLocation.X, Sock->RelativeLocation.Y, Sock->RelativeLocation.Z); }
				}
			}
			FString Visible;
			TInlineComponentArray<UStaticMeshComponent*> Meshes(Ship);
			for (const UStaticMeshComponent* M : Meshes)
			{
				if (M && M->IsVisible() && M->GetStaticMesh() && !M->bHiddenInGame)
				{
					Visible += FString::Printf(TEXT("%s[%s] "), *M->GetName(), *M->GetStaticMesh()->GetName());
				}
			}
			const FVector Exhaust = Ship->GetActorTransform().InverseTransformPosition(Ship->GetEngineExhaustLocation());
			const UMaterialInterface* Mat = Hull ? Hull->GetMaterial(0) : nullptr;
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.ShipArt: %s %s player %d | visuals %s | real art %d | mesh %s size %.0f x %.0f x %.0f cm (long axis %s) | collision radius %.0f | hull rot %s | material %s"),
				*Ship->GetName(), *UEnum::GetValueAsString(Ship->GetShipId()), Ship->IsPlayerFlown() ? 1 : 0, *GetNameSafe(Ship->GetShipVisuals()),
				Ship->HasRealArt() ? 1 : 0, *GetNameSafe(Mesh), Size.X, Size.Y, Size.Z,
				Size.X >= Size.Y && Size.X >= Size.Z ? TEXT("X") : (Size.Y >= Size.Z ? TEXT("Y") : TEXT("Z")), Radius,
				Hull ? *Hull->GetRelativeRotation().ToCompactString() : TEXT("-"), *GetNameSafe(Mat));
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.ShipArt:   sockets %s"), *Sockets);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.ShipArt:   visible %s| exhaust local (%.0f, %.0f, %.0f)"), *Visible, Exhaust.X, Exhaust.Y, Exhaust.Z);
		}
	}

	void Feedback(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		UTDFlightFeedbackComponent* FB = Ship ? Ship->GetFlightFeedback() : nullptr;
		if (!FB)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Feedback: no player ship"));
			return;
		}
		FString Counts;
		for (const TCHAR* M : { TEXT("Dodge"), TEXT("BoostStart"), TEXT("DriftRelease"), TEXT("Brake"), TEXT("HyperdriveJump"),
			TEXT("HyperdriveDropOut"), TEXT("PortalTransit"), TEXT("AtmosphereEntry"), TEXT("Touchdown"), TEXT("TakeOff") })
		{
			Counts += FString::Printf(TEXT(" %s=%d"), M, FB->GetPlaceholderCount(M));
		}
		const FRotator Off = FB->GetCameraOffset();
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Feedback: tick %.4f ms | frame %.2f ms | dust %d | bursts %d | cam P%.2f Y%.2f R%.2f FOV%+.2f |%s"),
			FB->GetAverageTickMs(), FApp::GetDeltaTime() * 1000.0, FB->GetVisibleDustCount(), FB->GetActiveBurstCount(),
			Off.Pitch, Off.Yaw, Off.Roll, FB->GetFOVOffset(), *Counts);
	}

	// Chase-camera feel readout for agent playtests: one line of what the player sees (ship on screen, FOV, roll,
	// speed, turn rate). "TD.Flight.Cam" logs once; "TD.Flight.Cam rec <seconds>" logs every 0.1 s;
	// "TD.Flight.Cam set <Cam|FB> <Field> <Value>" changes a chase-camera or feedback tuning value on every squad ship
	// (dev only, to tune live; the chosen values go into the code defaults).
	static void CamLine(UWorld* World, const FString& Tag)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		if (!Ship || !PC->PlayerCameraManager)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Cam: [%s] no player ship"), *Tag);
			return;
		}
		const FVector Eye = PC->PlayerCameraManager->GetCameraLocation();
		FVector2D Screen(-1.f, -1.f);
		int32 W = 0, H = 0;
		PC->GetViewportSize(W, H);
		if (W > 0 && H > 0 && PC->ProjectWorldLocationToScreen(Ship->GetActorLocation(), Screen, false))
		{
			Screen.X /= (float)W;
			Screen.Y /= (float)H;
		}
		static TMap<TWeakObjectPtr<ATDShip>, FRotator> LastRot;
		static double LastTime = 0.0;
		const double Now = World->GetRealTimeSeconds();
		const FRotator Rot = Ship->GetActorRotation();
		float YawRate = 0.f, PitchRate = 0.f;
		if (const FRotator* Prev = LastRot.Find(Ship); Prev && Now > LastTime)
		{
			YawRate = FMath::FindDeltaAngleDegrees(Prev->Yaw, Rot.Yaw) / float(Now - LastTime);
			PitchRate = FMath::FindDeltaAngleDegrees(Prev->Pitch, Rot.Pitch) / float(Now - LastTime);
		}
		LastRot.Add(Ship, Rot);
		LastTime = Now;
		const FVector Local = Ship->GetActorTransform().InverseTransformPosition(Eye);
		// Is the view from the camera to the ship blocked (camera inside or behind a hillside)?
		FHitResult Block;
		FCollisionQueryParams BlockParams(SCENE_QUERY_STAT(TDCamBlock), false, Ship);
		const bool bBlocked = World->LineTraceSingleByChannel(Block, Ship->GetActorLocation(), Eye, ECC_Visibility, BlockParams);
		const UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback();
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Cam: [%s] %s screen %.3f %.3f | fov %.1f camroll %.1f shiproll %.1f | cam local %.0f %.0f %.0f dist %.0f | speed %.0f boost %d drift %d | yaw %.0f pitch %.0f deg/s | dust %d | alt %.0f m camblocked %d | shake %.2f fovkick %.1f"),
			*Tag, *UEnum::GetValueAsString(Ship->GetShipId()), Screen.X, Screen.Y, PC->PlayerCameraManager->GetFOVAngle(),
			Ship->GetCameraRoll(), Rot.Roll, Local.X, Local.Y, Local.Z, FVector::Dist(Eye, Ship->GetActorLocation()),
			(float)Ship->GetVelocity().Size(), Ship->IsBoosting() ? 1 : 0, Ship->IsDrifting() ? 1 : 0, YawRate, PitchRate,
			FB ? FB->GetVisibleDustCount() : -1, Ship->GetAltitudeMeters(), bBlocked ? 1 : 0,
			FB ? FMath::Abs(FB->GetCameraOffset().Pitch) + FMath::Abs(FB->GetCameraOffset().Yaw) + FMath::Abs(FB->GetCameraOffset().Roll) : 0.f, FB ? FB->GetFOVOffset() : 0.f);
	}

	static bool SetTuning(UWorld* World, const FString& Which, const FString& Field, float Value)
	{
		int32 Count = 0;
		for (TActorIterator<ATDShip> It(World); It; ++It)
		{
			ATDShip* Ship = *It;
			UScriptStruct* Struct = nullptr;
			void* Data = nullptr;
			if (Which.Equals(TEXT("Cam"), ESearchCase::IgnoreCase))
			{
				const FStructProperty* Prop = CastField<FStructProperty>(ATDShip::StaticClass()->FindPropertyByName(TEXT("CameraTuning")));
				if (!Prop) { return false; }
				Struct = Prop->Struct;
				Data = Prop->ContainerPtrToValuePtr<void>(Ship);
			}
			else if (UTDFlightFeedbackComponent* FB = Ship->GetFlightFeedback())
			{
				Struct = FTDFlightFeedbackTuning::StaticStruct();
				Data = &FB->Tuning;
			}
			const FNumericProperty* Num = Struct ? CastField<FNumericProperty>(Struct->FindPropertyByName(FName(*Field))) : nullptr;
			if (!Num || !Data) { return false; }
			Num->SetFloatingPointPropertyValue(Num->ContainerPtrToValuePtr<void>(Data), Value);
			++Count;
		}
		return Count > 0;
	}

	void Cam(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		if (Args.Num() >= 3 && Args[0] == TEXT("goto"))
		{
			// Dev: put the player's ship at X Y (m) [Z m above that, else 60 m over the ground] [yaw deg]. For terrain checks.
			APlayerController* PC = World->GetFirstPlayerController();
			ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
			if (!Ship) { UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Cam: goto needs a player ship")); return; }
			const float X = FCString::Atof(*Args[1]) * 100.f, Y = FCString::Atof(*Args[2]) * 100.f;
			const float Yaw = Args.Num() >= 5 ? FCString::Atof(*Args[4]) : Ship->GetActorRotation().Yaw;
			float Z = Ship->GetActorLocation().Z;
			FHitResult Hit;
			FCollisionQueryParams P(SCENE_QUERY_STAT(TDGoto), false, Ship);
			const bool bGround = World->LineTraceSingleByChannel(Hit, FVector(X, Y, 1000000.f), FVector(X, Y, -1000000.f), ECC_Visibility, P);
			if (Args.Num() >= 4) { Z = (bGround ? Hit.ImpactPoint.Z : 0.f) + FCString::Atof(*Args[3]) * 100.f; }
			else if (bGround) { Z = Hit.ImpactPoint.Z + 6000.f; }
			Ship->TeleportSpacecraft(FVector(X, Y, Z), FRotator(0.f, Yaw, 0.f), true);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Cam: goto %.0f %.0f m, ground %s at %.0f m, ship at %.0f m"), X / 100.f, Y / 100.f,
				bGround ? TEXT("found") : TEXT("none"), bGround ? Hit.ImpactPoint.Z / 100.f : 0.f, Z / 100.f);
			return;
		}
		if (Args.Num() >= 4 && Args[0] == TEXT("set"))
		{
			const bool bOk = SetTuning(World, Args[1], Args[2], FCString::Atof(*Args[3]));
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Cam: set %s.%s = %s %s"), *Args[1], *Args[2], *Args[3], bOk ? TEXT("ok") : TEXT("FAILED (unknown field)"));
			return;
		}
		if (Args.Num() >= 2 && Args[0] == TEXT("rec"))
		{
			const double Until = FPlatformTime::Seconds() + FCString::Atof(*Args[1]);
			const FString Tag = Args.Num() >= 3 ? Args[2] : TEXT("rec");
			TWeakObjectPtr<UWorld> WeakWorld(World);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld, Until, Tag](float) -> bool
			{
				if (!WeakWorld.IsValid() || FPlatformTime::Seconds() > Until) { return false; }
				CamLine(WeakWorld.Get(), Tag);
				return true;
			}), 0.1f);
			return;
		}
		CamLine(World, Args.Num() ? Args[0] : TEXT("now"));
	}

	void Threats(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		UTDCombatRegistry* Registry = World ? UTDCombatRegistry::Get(World) : nullptr;
		if (!Ship || !Registry || !Ship->GetHealth())
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Threats: no player ship"));
			return;
		}
		static TWeakObjectPtr<ATDShip> LastShip;
		static float LastTotal = -1.f;
		const UTDHealthComponent* H = Ship->GetHealth();
		const float Total = H->GetHull() + H->GetShield();
		const float Taken = (LastShip.Get() == Ship && LastTotal >= 0.f) ? FMath::Max(0.f, LastTotal - Total) : 0.f;
		LastShip = Ship;
		LastTotal = Total;

		TArray<AActor*> Enemies;
		Registry->GetActorsOfTeam(ETDTeam::Enemy, Enemies, false);
		float Nearest = -1.f;
		int32 Within1 = 0, Within2 = 0, Alive = 0;
		for (const AActor* E : Enemies)
		{
			if (!IsValid(E))
			{
				continue;
			}
			++Alive;
			const float D = FVector::Dist(E->GetActorLocation(), Ship->GetActorLocation());
			Nearest = Nearest < 0.f ? D : FMath::Min(Nearest, D);
			Within1 += D <= 100000.f ? 1 : 0;
			Within2 += D <= 200000.f ? 1 : 0;
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Threats: hull %.0f shield %.0f taken %.0f | enemies %d nearest %.0f m within1km %d within2km %d | speed %.0f boosting %d drifting %d"),
			H->GetHull(), H->GetShield(), Taken, Alive, Nearest / 100.f, Within1, Within2, (float)Ship->GetVelocity().Size(),
			Ship->IsBoosting() ? 1 : 0, Ship->IsDrifting() ? 1 : 0);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GTDFlightThreatsCmd(TEXT("TD.Flight.Threats"),
	TEXT("Log the player ship's hull/shield, damage since the last call, and nearby enemies (agent feel playtests)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::Threats));

static FAutoConsoleCommandWithWorldAndArgs GTDFlightFeedbackCmd(TEXT("TD.Flight.Feedback"),
	TEXT("Log the no-art flight feedback: its tick cost, dust motes shown, active bursts, camera kick and placeholder counts."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::Feedback));

static FAutoConsoleCommandWithWorldAndArgs GTDFlightCamCmd(TEXT("TD.Flight.Cam"),
	TEXT("Chase-camera feel readout: [tag] once | rec <seconds> [tag] every 0.1 s | set <Cam|FB> <Field> <Value> on every squad ship (dev tuning)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::Cam));

static FAutoConsoleCommandWithWorldAndArgs GTDFlightShipArtCmd(TEXT("TD.Flight.ShipArt"),
	TEXT("Log every squad ship's art: visuals, mesh size against the collision, sockets, visible parts, exhaust point."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::ShipArt));

#endif // !UE_BUILD_SHIPPING
