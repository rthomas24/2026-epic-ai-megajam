// The Dominion interior lighting states (environment builder, ART-13). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.InteriorLight

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDInteriorLightState.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/PointLightComponent.h"

namespace
{
	UPointLightComponent* TDLightTest_AddLight(AActor* A, bool bTagged)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(A);
		if (!A->GetRootComponent()) { A->SetRootComponent(L); }
		else { L->SetupAttachment(A->GetRootComponent()); }
		L->SetIntensity(1000.f);
		L->SetLightColor(FLinearColor(0.8f, 0.85f, 1.f));
		if (bTagged) { L->ComponentTags.Add(UTDInteriorLightSubsystem::LightTag); }
		L->RegisterComponent();
		return L;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorLightStates, "TriadDrift.World.InteriorLight.NormalAlarmCollapseAndBack", TD_TEST_FLAGS)
bool FTDInteriorLightStates::RunTest(const FString&)
{
	FTDTestWorld W;
	UTDInteriorLightSubsystem* S = W.GetWorld() ? W.GetWorld()->GetSubsystem<UTDInteriorLightSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("subsystem"), S)) { return false; }
	AActor* A = W.Spawn<AActor>();
	UPointLightComponent* Tagged = TDLightTest_AddLight(A, true);
	UPointLightComponent* Other = TDLightTest_AddLight(A, false);
	TArray<UPointLightComponent*> Extra;
	for (int32 i = 0; i < 9; ++i) { Extra.Add(TDLightTest_AddLight(A, true)); }
	TestEqual(TEXT("starts normal"), S->GetMode(), ETDInteriorLightMode::Normal);

	// The moment arrives through T5's static delegate, as in the game.
	ATDPlanetCourse::OnInteriorMoment.Broadcast(TEXT("ReactorFightEnded"), A);
	TestEqual(TEXT("alarm on ReactorFightEnded"), S->GetMode(), ETDInteriorLightMode::Alarm);
	TestEqual(TEXT("the tagged lights are driven"), S->GetLightCount(), 10);
	S->Step(0.2f);
	TestTrue(TEXT("the tagged light turns red"), Tagged->GetLightColor().R > Tagged->GetLightColor().B * 2.f);
	TestTrue(TEXT("an untagged light is left alone"), FMath::IsNearlyEqual(Other->Intensity, 1000.f) && Other->GetLightColor().B > 0.9f);
	float Lo = 1e9f, Hi = 0.f, AlarmSum = 0.f;
	for (int32 i = 0; i < 40; ++i)
	{
		S->Step(0.05f); Lo = FMath::Min(Lo, Tagged->Intensity); Hi = FMath::Max(Hi, Tagged->Intensity);
		AlarmSum += Tagged->Intensity; for (UPointLightComponent* L : Extra) { AlarmSum += L->Intensity; }
	}
	TestTrue(*FString::Printf(TEXT("the alarm pulses (%.0f..%.0f)"), Lo, Hi), Hi > Lo * 2.f);

	ATDPlanetCourse::OnInteriorMoment.Broadcast(TEXT("EscapeStarted"), A);
	S->Step(S->EscapeSeconds - S->CollapseSeconds + 1.f);
	TestEqual(TEXT("collapse in the escape's last seconds"), S->GetMode(), ETDInteriorLightMode::Collapse);
	// The collapse differs from the alarm in a still: sections dead, a few stuttering, darker overall (s11).
	TestEqual(TEXT("all tagged lights driven"), S->GetLightCount(), 1 + Extra.Num());
	auto Total = [&]() { float T = Tagged->Intensity; for (UPointLightComponent* L : Extra) { T += L->Intensity; } return T; };
	float CollapseSum = 0.f; int32 DeadFrames = 0; bool bChanged = false; float Prev = -1.f;
	for (int32 i = 0; i < 100; ++i)
	{
		S->Step(0.05f);
		const float T = Total();
		CollapseSum += T;
		for (UPointLightComponent* L : Extra) { DeadFrames += L->Intensity < 1.f ? 1 : 0; }
		bChanged |= Prev >= 0.f && !FMath::IsNearlyEqual(T, Prev, 1.f);
		Prev = T;
	}
	TestTrue(*FString::Printf(TEXT("darker than the alarm (%.0f vs %.0f)"), CollapseSum / 100.f, AlarmSum / 40.f), CollapseSum / 100.f < 0.6f * AlarmSum / 40.f);
	TestTrue(TEXT("some lights are dead"), DeadFrames > 50);
	TestTrue(TEXT("frames differ (flicker)"), bChanged);

	// A retake from the core chamber: the interior as built.
	ATDPlanetCourse::OnInteriorMoment.Broadcast(TEXT("CoreChamberEntered"), A);
	S->Step(0.1f);
	TestEqual(TEXT("normal again"), S->GetMode(), ETDInteriorLightMode::Normal);
	TestTrue(TEXT("intensity restored"), FMath::IsNearlyEqual(Tagged->Intensity, 1000.f));
	TestTrue(TEXT("colour restored"), Tagged->GetLightColor().Equals(FLinearColor(0.8f, 0.85f, 1.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorLightNothingTagged, "TriadDrift.World.InteriorLight.NothingTaggedNothingHappens", TD_TEST_FLAGS)
bool FTDInteriorLightNothingTagged::RunTest(const FString&)
{
	FTDTestWorld W;
	UTDInteriorLightSubsystem* S = W.GetWorld() ? W.GetWorld()->GetSubsystem<UTDInteriorLightSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("subsystem"), S)) { return false; }
	AActor* A = W.Spawn<AActor>();
	UPointLightComponent* L = TDLightTest_AddLight(A, false);
	ATDPlanetCourse::OnInteriorMoment.Broadcast(TEXT("EscapeStarted"), A);
	for (int32 i = 0; i < 100; ++i) { S->Step(1.f); }
	TestEqual(TEXT("no light driven"), S->GetLightCount(), 0);
	TestTrue(TEXT("the untagged light is untouched"), FMath::IsNearlyEqual(L->Intensity, 1000.f));
	return true;
}

#endif
