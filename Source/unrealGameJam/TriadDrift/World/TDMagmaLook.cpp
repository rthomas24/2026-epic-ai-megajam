// Triad Drift - World (environment builder). See TDMagmaLook.h.

#include "TriadDrift/World/TDMagmaLook.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Misc/PackageName.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"

const FName UTDMagmaLookSubsystem::GlowTag(TEXT("TD.MagmaGlow"));

void UTDMagmaLookSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (TActorIterator<ATDMagmaVolume> It(&InWorld); It; ++It) { Dress(*It); }
	SpawnHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UTDMagmaLookSubsystem::HandleSpawned));
}

void UTDMagmaLookSubsystem::Deinitialize()
{
	if (UWorld* W = GetWorld()) { W->RemoveOnActorSpawnedHandler(SpawnHandle); }
	Super::Deinitialize();
}

void UTDMagmaLookSubsystem::HandleSpawned(AActor* Actor)
{
	// T5 sizes a volume right after spawning it (SetSize): dress it on the next tick, when its box is final.
	if (ATDMagmaVolume* V = Cast<ATDMagmaVolume>(Actor))
	{
		TWeakObjectPtr<ATDMagmaVolume> Weak(V);
		TWeakObjectPtr<const UTDMagmaLookSubsystem> Self(this);
		V->GetWorldTimerManager().SetTimerForNextTick([Weak, Self]() { if (Weak.IsValid() && Self.IsValid()) { Self->Dress(Weak.Get()); } });
	}
}

void UTDMagmaLookSubsystem::Dress(ATDMagmaVolume* Volume) const
{
	if (!Volume || !Volume->Box)
	{
		return;
	}
	TInlineComponentArray<UPointLightComponent*> Existing(Volume);
	for (UPointLightComponent* L : Existing) { if (L->ComponentHasTag(GlowTag)) { return; } }
	const FVector Ext = Volume->Box->GetScaledBoxExtent();	// half size; the surface is at the box top + 10 cm
	const float AreaM2 = (Ext.X * 2.f / 100.f) * (Ext.Y * 2.f / 100.f);
	UPointLightComponent* L = NewObject<UPointLightComponent>(Volume, TEXT("MagmaGlow"));
	L->ComponentTags.Add(GlowTag);
	L->SetupAttachment(Volume->GetRootComponent());
	// low over the surface, so the banks and anyone walking up are lit orange from below
	L->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(FMath::Min(MaxCandelas, 150.f + AreaM2 * CandelasPerSquareMetre));
	L->SetLightColor(GlowColor);
	L->SetAttenuationRadius(FMath::Max(Ext.X, Ext.Y) + 600.f);	// the banks, not the whole plain
	L->SetSourceRadius(FMath::Min(Ext.X, Ext.Y) * 0.5f);
	L->SetCastShadows(false);
	L->RegisterComponent();
	Volume->AddInstanceComponent(L);

	// The effects builder's heat (embers, spatter, shimmer) on the lava surface, sized to the footprint; optional.
	static const TCHAR* HeatPath = TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Magma_Heat.NS_TD_Magma_Heat");
	if (FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Magma_Heat")))
	{
		if (UNiagaraSystem* Heat = LoadObject<UNiagaraSystem>(nullptr, HeatPath))
		{
			if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAttached(Heat, Volume->GetRootComponent(), NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false))
			{
				N->ComponentTags.Add(GlowTag);
				N->SetVariableFloat(TEXT("User.SizeX"), Ext.X * 2.f);
				N->SetVariableFloat(TEXT("User.SizeY"), Ext.Y * 2.f);
				N->SetVariableFloat(TEXT("User.Intensity"), 1.f);
			}
		}
	}
}
