// The engineering nebula sky and the space lighting that matches it (art audit, Nov 13).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Sky

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDSkySphere.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSkySunDirection, "TriadDrift.World.Sky.SunLightShinesFromTheBakedSun", TD_TEST_FLAGS)
bool FTDSkySunDirection::RunTest(const FString&)
{
	// README (ArtSource/EngProps/Sky): the sun sits at (0.779, 0.546, 0.309); the light points the other way.
	const FVector ToSun(0.779f, 0.546f, 0.309f);
	const FVector LightForward = ATDSkySphere::SunLightRotation().Vector();
	TestTrue(*FString::Printf(TEXT("light forward %s is minus the sun direction"), *LightForward.ToCompactString()),
		LightForward.Equals(-ToSun.GetSafeNormal(), 0.01f));

	FTDTestWorld W;
	ATDPlaceholderSky* Sky = W.Spawn<ATDPlaceholderSky>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("placeholder sky"), Sky)) { return false; }
	Sky->ApplyStyle(ETDPlaceholderSkyStyle::Space);
	TestTrue(TEXT("space sun follows the baked sun"), Sky->Sun->GetForwardVector().Equals(LightForward, 0.01f));
	TestTrue(TEXT("space sun drives the atmosphere / planet night side"), Sky->Sun->IsUsedAsAtmosphereSunLight());
	TestFalse(TEXT("fill never casts shadows"), Sky->Fill->CastShadows);
	TestTrue(TEXT("fill is soft (well under the sun)"), Sky->Fill->Intensity <= Sky->Sun->Intensity * 0.25f);
	const FPostProcessSettings& S = Sky->Exposure->Settings;
	TestEqual(TEXT("manual exposure"), (int32)S.AutoExposureMethod, (int32)EAutoExposureMethod::AEM_Manual);
	TestTrue(TEXT("bloom capped"), S.bOverride_BloomIntensity && S.BloomIntensity <= 1.f);
	TestTrue(TEXT("bloom only from emitters"), S.bOverride_BloomThreshold && S.BloomThreshold >= 1.f);
	TestTrue(TEXT("motion blur about 0.2"), S.bOverride_MotionBlurAmount && FMath::IsNearlyEqual(S.MotionBlurAmount, 0.2f, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSkySphereNeverInTheWay, "TriadDrift.World.Sky.SphereIsTaggedAndNeverCollides", TD_TEST_FLAGS)
bool FTDSkySphereNeverInTheWay::RunTest(const FString&)
{
	FTDTestWorld W;
	if (!ATDSkySphere::IsArtAvailable())
	{
		AddInfo(TEXT("sky art not in this checkout: no sky is spawned (the star backdrop stays)"));
		TestNull(TEXT("no art -> SpawnIfAvailable spawns nothing"), ATDSkySphere::SpawnIfAvailable(W.GetWorld()));
		return true;
	}
	ATDSkySphere* Sky = W.Spawn<ATDSkySphere>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("sky"), Sky)) { return false; }
	TestTrue(TEXT("tagged TD.Engineering.Sky"), Sky->ActorHasTag(ATDSkySphere::EngineeringSkyTag));
	TInlineComponentArray<UPrimitiveComponent*> Prims(Sky);
	for (UPrimitiveComponent* P : Prims)
	{
		TestFalse(TEXT("no collision"), P->IsCollisionEnabled());
		TestFalse(TEXT("no shadow"), P->CastShadow);
	}
	TestTrue(TEXT("radius far beyond the play space (> 50 km)"), Sky->RadiusCm >= 5000000.f);
	// Nick's sky wins.
	AActor* Presentation = W.Spawn<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	Presentation->Tags.Add(ATDPlaceholderSky::PresentationSkyTag);
	Sky->RefreshYield();
	TestTrue(TEXT("hides while a presentation sky exists"), Sky->IsYielding() && Sky->IsHidden());
	Presentation->Destroy();
	Sky->RefreshYield();
	TestFalse(TEXT("back when it is gone"), Sky->IsYielding());
	return true;
}

#endif
