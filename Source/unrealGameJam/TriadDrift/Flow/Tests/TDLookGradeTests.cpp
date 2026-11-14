// ART-05 / ART-13: the colour grade per level (environment builder). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flow.LookGrade

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/PostProcessComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDLookGradePerStyle, "TriadDrift.Flow.LookGrade.EveryStyleIsGradedAboveTheLevelsOwnPost", TD_TEST_FLAGS)
bool FTDLookGradePerStyle::RunTest(const FString&)
{
	for (const ETDPlaceholderSkyStyle Style : { ETDPlaceholderSkyStyle::Space, ETDPlaceholderSkyStyle::Planet, ETDPlaceholderSkyStyle::Verdance, ETDPlaceholderSkyStyle::Cinder })
	{
		FTDTestWorld W;
		ATDPlaceholderSky* Sky = W.Spawn<ATDPlaceholderSky>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("placeholder sky"), Sky)) { return false; }
		Sky->ApplyStyle(Style);
		const FString Name = StaticEnum<ETDPlaceholderSkyStyle>()->GetNameStringByValue((int64)Style);
		if (!TestNotNull(*(Name + TEXT(": grade component")), Sky->Grade.Get())) { continue; }
		// Unbound and above the interior's exposure volume (priority 10), so the grade survives a level that brings exposure.
		TestTrue(*(Name + TEXT(": unbound, above priority 10")), Sky->Grade->bUnbound && Sky->Grade->Priority > 10.f);
		const FPostProcessSettings& G = Sky->Grade->Settings;
		TestTrue(*(Name + TEXT(": saturated (1.0 .. 1.2)")), G.bOverride_ColorSaturation && G.ColorSaturation.W > 1.f && G.ColorSaturation.W <= 1.2f);
		TestTrue(*(Name + TEXT(": a little contrast (1.0 .. 1.15)")), G.bOverride_ColorContrast && G.ColorContrast.W >= 1.f && G.ColorContrast.W <= 1.15f);
		// The grade never touches exposure or bloom (those stay with the level's own exposure settings).
		TestFalse(*(Name + TEXT(": no exposure in the grade")), G.bOverride_AutoExposureBias || G.bOverride_AutoExposureMethod);
		TestFalse(*(Name + TEXT(": no bloom in the grade")), G.bOverride_BloomIntensity || G.bOverride_BloomThreshold);
	}
	return true;
}

#endif
