// Triad Drift - the future crew in the reactor room (Nicholas, N-052 / N-060, issue #29). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDFutureCrew.h"

#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequenceBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace TDFutureCrew
{
	FName PilotOf(ETDShipId Ship)
	{
		return Ship == ETDShipId::Lancer ? FName(TEXT("Kato")) : Ship == ETDShipId::Bastion ? FName(TEXT("Moss")) : FName(TEXT("Wren"));
	}

	ETDShipId ShipOf(FName Pilot)
	{
		return Pilot == TEXT("Kato") ? ETDShipId::Lancer : Pilot == TEXT("Moss") ? ETDShipId::Bastion : ETDShipId::Rift;
	}

	FLinearColor ColorOf(FName Pilot)
	{
		return Pilot == TEXT("Kato") ? FLinearColor(0.1f, 0.85f, 1.f) : Pilot == TEXT("Moss") ? FLinearColor(1.f, 0.62f, 0.08f) : FLinearColor(1.f, 0.15f, 0.85f);
	}

	TArray<FName> OthersThan(FName Pilot)
	{
		TArray<FName> Out;
		for (const TCHAR* P : { TEXT("Wren"), TEXT("Kato"), TEXT("Moss") })
		{
			if (Pilot != P) { Out.Add(P); }
		}
		return Out;
	}

	UAnimSequenceBase* PilotAnim(const TCHAR* Name)
	{
		if (!Name || !*Name)
		{
			return nullptr;
		}
		const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/%s"), Name);
		if (!FindPackage(nullptr, *Package) && !FPackageName::DoesPackageExist(Package))
		{
			return nullptr;
		}
		return LoadObject<UAnimSequenceBase>(nullptr, *(Package + TEXT(".") + Name), nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	const UTDPilotVisuals* ApplyPilotLook(USkeletalMeshComponent* Mesh, float CapsuleHalfHeight)
	{
		if (!Mesh)
		{
			return nullptr;
		}
		const UTDPilotVisuals* V = Cast<UTDPilotVisuals>(ATDCrewCharacter::FindLookAsset(TEXT("DA_Pilot_Visuals"), UTDPilotVisuals::StaticClass()));
		USkeletalMesh* SK = V && !V->SkeletalMesh.IsNull() ? V->SkeletalMesh.LoadSynchronous() : nullptr;
		if (!SK)
		{
			return nullptr;
		}
		Mesh->SetSkeletalMesh(SK);
		for (int32 i = 0; i < V->MaterialOverrides.Num(); ++i)
		{
			if (UMaterialInterface* Mat = V->MaterialOverrides[i].LoadSynchronous()) { Mesh->SetMaterial(i, Mat); }
		}
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(UTDLoopBlendAnimInstance::StaticClass()); // the loops by name, as on the player's own body
		Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight));
		Mesh->SetRelativeRotation(FRotator(0.f, V->MeshYaw, 0.f));
		Mesh->SetRelativeScale3D(FVector(V->MeshScale));
		Mesh->SetVisibility(true);
		if (UTDLoopBlendAnimInstance* Loops = Cast<UTDLoopBlendAnimInstance>(Mesh->GetAnimInstance()))
		{
			UAnimSequenceBase* Idle = PilotAnim(TEXT("A_Pilot_Blade_Idle"));
			Loops->SetLoop(Idle ? Idle : V->Idle.LoadSynchronous(), 1.f);
		}
		return V;
	}

	void ApplyFuturePalette(USkeletalMeshComponent* Mesh, FName Pilot)
	{
		if (!Mesh) { return; }
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Source = Mesh->GetMaterial(Slot);
			FLinearColor ExistingTint;
			// Reuse the authored suit masks. Unrelated/custom materials are not replaced.
			if (!Source || !Source->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitTint")), ExistingTint)) { continue; }
			// Always create an instance owned by this component; never mutate a shared material or ally MID.
			if (UMaterialInstanceDynamic* Suit = UMaterialInstanceDynamic::Create(Source, Mesh))
			{
				Suit->SetVectorParameterValue(TEXT("SuitTint"), FLinearColor(0.025f, 0.025f, 0.03f));
				Suit->SetVectorParameterValue(TEXT("SuitDark"), FLinearColor(0.018f, 0.018f, 0.022f));
				Suit->SetScalarParameterValue(TEXT("SuitDarkAmount"), 1.f);
				Suit->SetScalarParameterValue(TEXT("SuitDarkGlow"), 0.f);
				Suit->SetVectorParameterValue(TEXT("SuitAccent"), ColorOf(Pilot));
				Suit->SetScalarParameterValue(TEXT("SuitAccentGlow"), 1.f);
				Mesh->SetMaterial(Slot, Suit);
				UE_LOG(LogTemp, Display, TEXT("TD.FutureAppearance pilot=%s slot=%d armor=black visor=role source=%s"),
					*Pilot.ToString(), Slot, *Source->GetPathName());
			}
		}
	}

	UStaticMeshComponent* AttachBlade(AActor* Owner, USkeletalMeshComponent* Mesh, const FLinearColor& Color)
	{
		if (!Owner || !Mesh || !Mesh->GetSkeletalMeshAsset())
		{
			return nullptr;
		}
		static const TCHAR* HiltPath = TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeHilt.SM_TD_BladeHilt");
		static const TCHAR* BeamPath = TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeBeam.SM_TD_BladeBeam");
		UStaticMesh* HiltMesh = LoadObject<UStaticMesh>(nullptr, HiltPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
		UStaticMesh* BeamMesh = LoadObject<UStaticMesh>(nullptr, BeamPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
		if (!BeamMesh)
		{
			BeamMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		auto Make = [Owner](UStaticMesh* M, const TCHAR* Name)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner, MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), Name));
			C->SetStaticMesh(M);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCanEverAffectNavigation(false);
			C->SetCastShadow(false);
			return C;
		};
		// The grip the player's own body uses (the grip builder's offset), so the future selves hold it the same way.
		const bool bSocket = Mesh->DoesSocketExist(TEXT("BladeSocket"));
		UStaticMeshComponent* Hilt = Make(HiltMesh, TEXT("FutureHilt"));
		Hilt->SetupAttachment(Mesh, bSocket ? FName(TEXT("BladeSocket")) : FName(TEXT("hand_r")));
		if (!bSocket)
		{
			Hilt->SetRelativeTransform(GetDefault<ATDFirstPersonCharacter>()->HandBladeOffset);
		}
		Hilt->SetRelativeScale3D(FVector(1.f / FMath::Max(0.01f, (float)Mesh->GetComponentScale().X)));
		Hilt->RegisterComponent();
		Owner->AddInstanceComponent(Hilt);
		UStaticMeshComponent* Beam = Make(BeamMesh, TEXT("FutureBeam"));
		const bool bEmitter = HiltMesh && HiltMesh->FindSocket(TEXT("BladeEmitter"));
		Beam->SetupAttachment(Hilt, bEmitter ? FName(TEXT("BladeEmitter")) : NAME_None);
		const float Length = GetDefault<ATDFirstPersonCharacter>()->HandBladeLength;
		const float Authored = FMath::Max(1.f, (float)BeamMesh->GetBoundingBox().GetSize().X);
		if (BeamMesh->GetName() == TEXT("SM_TD_BladeBeam"))
		{
			Beam->SetRelativeScale3D(FVector(Length / Authored, 1.f, 1.f));
			if (UMaterialInstanceDynamic* MID = Beam->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("BladeColor"), Color); }
		}
		else
		{
			Beam->SetRelativeScale3D(FVector(Length / Authored, 0.03f, 0.015f));
			Beam->SetRelativeLocation(FVector(6.f + Length * 0.5f, 0.f, 0.f));
			UTDStatics::TintPlaceholder(Beam, Color, 1.4f);
		}
		Beam->RegisterComponent();
		Owner->AddInstanceComponent(Beam);
		return Beam;
	}
}

// ------------------------------------------------------------------------------------------------
// The boarder's own future self
// ------------------------------------------------------------------------------------------------

FTDOnFutureSelfExchange ATDFutureSelf::OnAnyExchange;

ATDFutureSelf::ATDFutureSelf()
{
	PrimaryActorTick.bCanEverTick = true;
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	RootComponent = Capsule;
	Capsule->InitCapsuleSize(40.f, 90.f);
	// Solid to walk into, invisible to every trace (the blade, bolts, interaction, the camera): never a target.
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_WorldStatic);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Capsule->SetCanEverAffectNavigation(false);
	Capsule->CanCharacterStepUpOn = ECB_No;

	Look = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Look"));
	Look->SetupAttachment(Capsule);
	Look->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Look->SetVisibility(false);

	Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
	Placeholder->SetupAttachment(Capsule);
	Placeholder->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Placeholder->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Placeholder->SetStaticMesh(Cube.Object);
	}
}

void ATDFutureSelf::SetPilot(FName InPilot)
{
	Pilot = InPilot.IsNone() ? FName(TEXT("Wren")) : InPilot;
}

FName ATDFutureSelf::GetExchangeId() const
{
	return FName(*FString::Printf(TEXT("FutureSelf_%s"), *Pilot.ToString()));
}

void ATDFutureSelf::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor Color = TDFutureCrew::ColorOf(Pilot);
	if (TDFutureCrew::ApplyPilotLook(Look, Capsule->GetUnscaledCapsuleHalfHeight()))
	{
		TDFutureCrew::ApplyFuturePalette(Look, Pilot);
		Placeholder->SetVisibility(false);
		TDFutureCrew::AttachBlade(this, Look, Color);
	}
	else
	{
		UTDStatics::TintPlaceholder(Placeholder, Color, 0.6f);
	}
}

void ATDFutureSelf::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const FVector To = Player->GetActorLocation() - GetActorLocation();
	// Facing the player, turning at a person's pace.
	const float Want = To.Rotation().Yaw;
	const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, Want, TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
	if (!bExchanged && To.Size2D() <= ExchangeRadius)
	{
		bExchanged = true;
		OnAnyExchange.Broadcast(this, Pilot);
	}
}
