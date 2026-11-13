// Story Act 1: the Dominion is seen far off behind its distortion field, unreachable, until the passage opens.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Field

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDistortionField.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFieldBlocks, "TriadDrift.World.Field.DominionBehindTheFieldCannotBeApproached", TD_TEST_FLAGS)
bool FTDFieldBlocks::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.Phase = ETDGamePhase::Waves1to2;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(600000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + 0.2f);
	ATDDistortionField* F = D->GetDistortionField();
	if (!TestNotNull(TEXT("the field is up from wave 1"), F)) { return false; }
	TestTrue(TEXT("the real Dominion stays hidden (nothing about the assault changes)"), D->IsHidden());
	TestEqual(TEXT("still hidden"), (int32)D->GetState(), (int32)ETDDominionState::Hidden);
	TestFalse(TEXT("the mirage is visible"), F->IsHidden());
	TestTrue(TEXT("the field surrounds the hull"), F->GetFieldRadius() > D->GetLocalHullBounds().GetExtent().Size());

	// Far away: no warning.
	ATDShip* Ship = W.Spawn<ATDShip>(D->GetActorLocation() + FVector(-F->GetFieldRadius() * 3.f, 0.f, 0.f));
	W.Advance(0.2f);
	TestFalse(TEXT("no warning far off"), F->IsWarningShown());
	// Flying into the field: pushed back out, the warning, the transmission.
	Ship->SetActorLocation(D->GetActorLocation() + FVector(-F->GetFieldRadius() * 0.8f, 0.f, 0.f));
	W.Advance(0.1f);
	TestTrue(*FString::Printf(TEXT("pushed back to the shell (%.0f vs %.0f)"), FVector::Dist(Ship->GetActorLocation(), D->GetActorLocation()), F->GetFieldRadius()),
		FVector::Dist(Ship->GetActorLocation(), D->GetActorLocation()) >= F->GetFieldRadius() * 0.999f);
	TestTrue(TEXT("repelled"), F->GetRepelCount() >= 1);
	TestTrue(TEXT("warning shown"), F->IsWarningShown());
	FText Reason;
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(D);
	TestTrue(TEXT("the HUD line: DISTORTION FIELD - NO APPROACH"), Travel && Travel->IsHyperdriveLocked(Reason) && Reason.ToString().Contains(TEXT("DISTORTION FIELD")));
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(D);
	TestTrue(TEXT("A1_FieldUnreachable played"), Story && Story->HasPlayed(TEXT("A1_FieldUnreachable")));
	// Flying away clears the warning.
	Ship->SetActorLocation(D->GetActorLocation() + FVector(-F->GetFieldRadius() * 3.f, 0.f, 0.f));
	W.Advance(0.1f);
	TestFalse(TEXT("warning cleared"), F->IsWarningShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFieldCollapses, "TriadDrift.World.Field.PassageOpenCollapsesTheField", TD_TEST_FLAGS)
bool FTDFieldCollapses::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(600000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + 0.2f);
	TWeakObjectPtr<ATDDistortionField> F = D->GetDistortionField();
	if (!TestTrue(TEXT("field"), F.IsValid())) { return false; }
	// Cinder's anchor falls: the passage is open, the Dominion warps in, the field collapses.
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	W.Advance(D->ProgressPollSeconds + 1.f);
	TestTrue(TEXT("the Dominion is here"), D->GetState() != ETDDominionState::Hidden);
	TestFalse(TEXT("the field is gone"), F.IsValid() && !F->IsActorBeingDestroyed());
	TestNull(TEXT("no field reference"), D->GetDistortionField());
	FText Reason;
	const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(D);
	TestFalse(TEXT("no field lock left"), Travel && Travel->IsHyperdriveLocked(Reason) && Reason.ToString().Contains(TEXT("DISTORTION FIELD")));
	return true;
}

#endif
