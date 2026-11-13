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
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSkyPlanetSunDirection, "TriadDrift.World.Sky.PlanetNightSideFacesAwayFromTheSun", TD_TEST_FLAGS)
bool FTDSkyPlanetSunDirection::RunTest(const FString&)
{
	const FTDGalaxyLayoutData Layout = ATDGalaxyLayout::MakeDefaultLayout();
	if (Layout.Planets.Num() == 0 || Layout.Planets[0].Look.Surface.IsNull()) { AddInfo(TEXT("no planet look")); return true; }
	FTDTestWorld W;
	ATDPlaceholderSky* Sky = W.Spawn<ATDPlaceholderSky>(FVector::ZeroVector, FRotator::ZeroRotator);
	Sky->ApplyStyle(ETDPlaceholderSkyStyle::Space);
	for (const FTDPlanetSpec& Spec : Layout.Planets)
	{
		ATDPlanetAtmosphere* P = W.GetWorld()->SpawnActorDeferred<ATDPlanetAtmosphere>(ATDPlanetAtmosphere::StaticClass(), FTransform(Spec.Location));
		P->Look = Spec.Look;
		P->Configure(Spec.Destination, Spec.ReturnPoint, Spec.PlanetRadius, Spec.AtmosphereRadius, Spec.Color);
		P->FinishSpawning(FTransform(Spec.Location));
		W.Advance(0.1f);
		UStaticMeshComponent* Surface = nullptr;
		TInlineComponentArray<UStaticMeshComponent*> Meshes(P);
		for (UStaticMeshComponent* M : Meshes) { if (M->GetName() == TEXT("SurfaceLook")) { Surface = M; } }
		if (!Surface || !Surface->GetStaticMesh()) { AddInfo(TEXT("planet art not in this checkout")); continue; }
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Surface->GetMaterial(0));
		if (!TestNotNull(*FString::Printf(TEXT("%s surface has a dynamic instance"), *P->GetName()), MID)) { continue; }
		FLinearColor Dir;
		TestTrue(*FString::Printf(TEXT("%s material has a SunDirection parameter"), *P->GetName()), MID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("SunDirection")), Dir));
		const FVector ToSun = -Sky->Sun->GetForwardVector();
		TestTrue(*FString::Printf(TEXT("%s SunDirection %s points at the sun %s"), *P->GetName(), *FVector(Dir).ToCompactString(), *ToSun.ToCompactString()),
			FVector(Dir).GetSafeNormal().Equals(ToSun, 0.01f));
		UStaticMeshComponent* Atmo = nullptr;
		for (UStaticMeshComponent* M : Meshes) { if (M->GetName() == TEXT("AtmosphereLook")) { Atmo = M; } }
		if (Atmo && Atmo->GetStaticMesh() && Atmo->IsVisible())
		{
			UMaterialInstanceDynamic* AMID = Cast<UMaterialInstanceDynamic>(Atmo->GetMaterial(0));
			FLinearColor ADir;
			TestTrue(*FString::Printf(TEXT("%s atmosphere has SunDirection"), *P->GetName()),
				AMID && AMID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("SunDirection")), ADir));
			TestTrue(*FString::Printf(TEXT("%s atmosphere limb faces the sun"), *P->GetName()), AMID && FVector(ADir).GetSafeNormal().Equals(ToSun, 0.01f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSkyActiveSun, "TriadDrift.World.Sky.PlanetsFollowWhicheverSunIsActive", TD_TEST_FLAGS)
bool FTDSkyActiveSun::RunTest(const FString&)
{
	FTDTestWorld W;
	TestNull(TEXT("no light, no sun"), ATDPlanetAtmosphere::FindActiveSun(W.GetWorld()));
	// A skybox-style rig: a plain directional light, not flagged as the atmosphere sun.
	AActor* Rig = W.Spawn<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	UDirectionalLightComponent* Plain = NewObject<UDirectionalLightComponent>(Rig, TEXT("PlainSun"));
	Rig->SetRootComponent(Plain);
	Plain->SetAtmosphereSunLight(false);
	Plain->SetIntensity(5.f);
	Plain->RegisterComponent();
	TestTrue(TEXT("a lone plain directional light is the sun"), ATDPlanetAtmosphere::FindActiveSun(W.GetWorld()) == Plain);
	// The atmosphere sun wins over a brighter plain light.
	ATDPlaceholderSky* Sky = W.Spawn<ATDPlaceholderSky>(FVector::ZeroVector, FRotator::ZeroRotator);
	Sky->ApplyStyle(ETDPlaceholderSkyStyle::Space);
	Plain->SetIntensity(50.f);
	TestTrue(TEXT("the atmosphere sun light wins"), ATDPlanetAtmosphere::FindActiveSun(W.GetWorld()) == Sky->Sun);
	return true;
}

#endif
