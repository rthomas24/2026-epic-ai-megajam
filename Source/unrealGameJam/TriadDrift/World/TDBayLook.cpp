// Triad Drift - World (environment builder). See TDBayLook.h.

#include "TriadDrift/World/TDBayLook.h"

#include "Components/LightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"

const FName UTDBayLookSubsystem::DoneTag(TEXT("TD.BayLook"));

static TAutoConsoleVariable<int32> CVarTDBayLook(TEXT("td.BayLook"), 1,
	TEXT("Docking bay pocket look (environment builder): 1 = lights scaled down and a fixed exposure in the pocket, 0 = as built (before / after captures)."));

bool UTDBayLookSubsystem::IsBayPocket(const AActor* A)
{
	return A && A->GetClass()->GetName().Contains(TEXT("DockingBay_Interior"));
}

void UTDBayLookSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (TActorIterator<AActor> It(&InWorld); It; ++It) { if (IsBayPocket(*It)) { Dress(*It); } }
	SpawnHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UTDBayLookSubsystem::HandleSpawned));
}

void UTDBayLookSubsystem::Deinitialize()
{
	if (UWorld* W = GetWorld()) { W->RemoveOnActorSpawnedHandler(SpawnHandle); }
	Super::Deinitialize();
}

void UTDBayLookSubsystem::HandleSpawned(AActor* Actor)
{
	if (!IsBayPocket(Actor)) { return; }
	// Attached to the hull's bay socket after spawning: dress it on the next tick, in place.
	TWeakObjectPtr<AActor> Weak(Actor);
	TWeakObjectPtr<UTDBayLookSubsystem> Self(this);
	Actor->GetWorldTimerManager().SetTimerForNextTick([Weak, Self]() { if (Weak.IsValid() && Self.IsValid()) { Self->Dress(Weak.Get()); } });
}

int32 UTDBayLookSubsystem::Dress(AActor* Pocket)
{
	if (!Pocket || Pocket->ActorHasTag(DoneTag) || CVarTDBayLook.GetValueOnGameThread() == 0)
	{
		return 0;
	}
	Pocket->Tags.Add(DoneTag);
	int32 Scaled = 0;
	TInlineComponentArray<ULightComponent*> Lights(Pocket);
	for (ULightComponent* L : Lights)
	{
		L->SetIntensity(L->Intensity * LightScale);
		++Scaled;
	}
	FVector Origin, Extent;
	Pocket->GetActorBounds(false, Origin, Extent);
	if (Extent.IsNearlyZero())
	{
		return Scaled;
	}
	// A box over the pocket (no collision) with a post-process component: the component takes the box as its volume.
	UBoxComponent* Box = NewObject<UBoxComponent>(Pocket, TEXT("BayLookVolume"));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetupAttachment(Pocket->GetRootComponent());
	Box->RegisterComponent();
	Box->SetWorldLocation(Origin);
	Box->SetBoxExtent(Extent / Pocket->GetActorScale3D().GetMax());
	Pocket->AddInstanceComponent(Box);
	UPostProcessComponent* PP = NewObject<UPostProcessComponent>(Pocket, TEXT("BayLookPost"));
	PP->SetupAttachment(Box);
	PP->bUnbound = false;
	PP->BlendRadius = 2500.f;	// eases in over the last 25 m of the glide
	PP->Priority = 15.f;
	FPostProcessSettings& S = PP->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = false;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = ExposureBias;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = BloomIntensity;
	PP->RegisterComponent();
	Pocket->AddInstanceComponent(PP);
	UE_LOG(LogTemp, Log, TEXT("TDBayLook: %s dressed (%d lights x%.2f, manual exposure %.1f over %s)"), *Pocket->GetName(), Scaled, LightScale, ExposureBias, *Extent.ToCompactString());
	return Scaled;
}
