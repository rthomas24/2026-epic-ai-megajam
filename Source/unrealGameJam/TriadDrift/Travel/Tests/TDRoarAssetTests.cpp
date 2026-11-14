// The descent roar must have a sound on both legs (the live strips logged "roar silent", but they ran with -nosound).
// Headless content check. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Descent.Roar

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1RoarHasASound, "TriadDrift.Travel.Descent.RoarHasASound", TD_TEST_FLAGS)
bool FTDT1RoarHasASound::RunTest(const FString& Parameters)
{
	const UTDTravelFX* FX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	if (!TestNotNull(TEXT("DA_Travel_FX"), FX)) { return false; }
	AddInfo(FString::Printf(TEXT("DescentRoar.Sound = '%s'; AtmosphericEntry.Sound = '%s'; AtmosphericExit.Sound = '%s'"),
		*FX->DescentRoar.Sound.ToString(), *FX->AtmosphericEntry.Sound.ToString(), *FX->AtmosphericExit.Sound.ToString()));
	for (const bool bExit : { false, true })
	{
		// Either the dedicated roar (SW_Travel_DescentRoar_01 since batch M) or the entry cue's sound: both are fine.
		USoundBase* Roar = UTDDescentSubsystem::ResolveRoar(FX, bExit);
		if (!TestNotNull(bExit ? TEXT("the climb out has a roar") : TEXT("the entry has a roar"), Roar)) { continue; }
		AddInfo(FString::Printf(TEXT("%s roar '%s': %.2f s%s"), bExit ? TEXT("exit") : TEXT("entry"), *Roar->GetPathName(), Roar->GetDuration(), Roar->IsLooping() ? TEXT(" (looping)") : TEXT("")));
		TestTrue(TEXT("long enough to cover the dive and the load (or loops)"), Roar->IsLooping() || Roar->GetDuration() >= 4.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1RoarOrder, "TriadDrift.Travel.Descent.RoarOrderForBothLegs", TD_TEST_FLAGS)
bool FTDT1RoarOrder::RunTest(const FString& Parameters)
{
	UTDTravelFX* FX = NewObject<UTDTravelFX>(GetTransientPackage());
	USoundWave* Entry = NewObject<USoundWave>(GetTransientPackage(), TEXT("TDT1_Entry"));
	USoundWave* Roar = NewObject<USoundWave>(GetTransientPackage(), TEXT("TDT1_Roar"));
	USoundWave* Exit = NewObject<USoundWave>(GetTransientPackage(), TEXT("TDT1_Exit"));
	TestNull(TEXT("nothing assigned: null"), UTDDescentSubsystem::ResolveRoar(FX, true));
	FX->AtmosphericEntry.Sound = Entry;
	TestTrue(TEXT("entry only: the entry sound on both legs"), UTDDescentSubsystem::ResolveRoar(FX, false) == Entry && UTDDescentSubsystem::ResolveRoar(FX, true) == Entry);
	FX->DescentRoar.Sound = Roar;
	TestTrue(TEXT("a roar: the roar on both legs"), UTDDescentSubsystem::ResolveRoar(FX, false) == Roar && UTDDescentSubsystem::ResolveRoar(FX, true) == Roar);
	FX->AtmosphericExit.Sound = Exit;
	TestTrue(TEXT("an exit sound: the climb out has its own, the entry keeps the roar"), UTDDescentSubsystem::ResolveRoar(FX, true) == Exit && UTDDescentSubsystem::ResolveRoar(FX, false) == Roar);
	return true;
}

#endif
