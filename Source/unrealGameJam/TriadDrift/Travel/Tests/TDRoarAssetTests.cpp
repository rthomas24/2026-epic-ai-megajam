// The descent roar must have a sound to play (the live strips logged "roar silent", but they ran with -nosound).
// Headless content check. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Descent.RoarHasASound

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1RoarHasASound, "TriadDrift.Travel.Descent.RoarHasASound", TD_TEST_FLAGS)
bool FTDT1RoarHasASound::RunTest(const FString& Parameters)
{
	const UTDTravelFX* FX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	if (!TestNotNull(TEXT("DA_Travel_FX"), FX)) { return false; }
	AddInfo(FString::Printf(TEXT("DescentRoar.Sound = '%s'; AtmosphericEntry.Sound = '%s'; AtmosphericExit.Sound = '%s'"),
		*FX->DescentRoar.Sound.ToString(), *FX->AtmosphericEntry.Sound.ToString(), *FX->AtmosphericExit.Sound.ToString()));
	// The entry plays DescentRoar, else AtmosphericEntry's sound (UTDAtmosphereEntryComponent::BeginEntry).
	USoundBase* Roar = !FX->DescentRoar.Sound.IsNull() ? FX->DescentRoar.Sound.LoadSynchronous() : FX->AtmosphericEntry.Sound.LoadSynchronous();
	if (!TestNotNull(TEXT("a roar sound is assigned and loads"), Roar)) { return false; }
	const float Duration = Roar->GetDuration();
	AddInfo(FString::Printf(TEXT("roar '%s': %.2f s%s"), *Roar->GetPathName(), Duration, Roar->IsLooping() ? TEXT(" (looping)") : TEXT("")));
	TestTrue(TEXT("long enough to cover the dive and the load (or loops)"), Roar->IsLooping() || Duration >= 4.f);
	return true;
}

#endif
