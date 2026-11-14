// The magma glow on every ATDMagmaVolume (environment builder). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.MagmaLook

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDMagmaLook.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/PointLightComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMagmaLookGlow, "TriadDrift.World.MagmaLook.EveryVolumeGlowsOverItsFootprint", TD_TEST_FLAGS)
bool FTDMagmaLookGlow::RunTest(const FString&)
{
	FTDTestWorld W;
	UTDMagmaLookSubsystem* S = W.GetWorld() ? W.GetWorld()->GetSubsystem<UTDMagmaLookSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("subsystem"), S)) { return false; }
	ATDMagmaVolume* V = W.Spawn<ATDMagmaVolume>();
	if (!TestNotNull(TEXT("volume"), V)) { return false; }
	V->SetSize(FVector(1600.f, 7000.f, 300.f));	// the conduit route's field
	S->Dress(V);
	S->Dress(V);	// idempotent
	TInlineComponentArray<UPointLightComponent*> Lights(V);
	int32 Glows = 0;
	for (UPointLightComponent* L : Lights)
	{
		if (!L->ComponentHasTag(UTDMagmaLookSubsystem::GlowTag)) { continue; }
		++Glows;
		TestTrue(TEXT("orange"), L->GetLightColor().R > L->GetLightColor().B * 5.f);
		TestTrue(*FString::Printf(TEXT("reaches past the footprint (%.0f cm)"), L->AttenuationRadius), L->AttenuationRadius > 3600.f);
		TestTrue(TEXT("lit, not floodlit"), L->Intensity > 150.f && L->Intensity <= 1200.f);
	}
	TestEqual(TEXT("one glow per volume"), Glows, 1);
	return true;
}

#endif
