// Triad Drift - ENG-43 on-foot barks: one test per trigger family. Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDGroundBarks.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"

namespace TDBarkTest
{
	static int32 Count(const UTDGroundBarks* B, const TCHAR* Id)
	{
		return B ? B->GetSaid().FilterByPredicate([Id](const FName& N) { return N == Id; }).Num() : 0;
	}
}

// Encounters: contact on the radio, each crew type's first sighting once a run (Kato), "that's the lot" on the clear.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksEncounter, "TriadDrift.Ground.Barks.ContactTypesOnceAndTheLastOne", TD_TEST_FLAGS)
bool FTDBarksEncounter::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-1500.f, 0.f, 95.f));
	UTDGroundBarks* B = UTDGroundBarks::Get(W.GetWorld());
	if (!TestNotNull(TEXT("barks"), B) || !P) { return false; }
	P->GetHealth()->SetInvulnerable(true); // the pilot sees the fight through to its end
	B->ResetForTests();
	B->TickBarks(0.1f); // binds the pilot
	ATDEncounterVolume* E = W.Spawn<ATDEncounterVolume>(FVector(0.f, 0.f, 150.f));
	E->bTriggerOnOverlap = false;
	E->CrewMix = { ETDCrewType::Trooper, ETDCrewType::Heavy };
	E->SpawnPoints = { FVector(0.f, 200.f, 0.f), FVector(0.f, -200.f, 0.f) };
	E->Activate();
	TestEqual(TEXT("contact"), TDBarkTest::Count(B, TEXT("Bark_Foot_Contact")), 1);
	TestEqual(TEXT("first trooper"), TDBarkTest::Count(B, TEXT("Bark_Foot_Trooper")), 1);
	TestEqual(TEXT("first heavy"), TDBarkTest::Count(B, TEXT("Bark_Foot_Heavy")), 1);
	W.Advance(11.f); // past the window cap
	ATDEncounterVolume* E2 = W.Spawn<ATDEncounterVolume>(FVector(0.f, 3000.f, 150.f));
	E2->bTriggerOnOverlap = false;
	E2->CrewMix = { ETDCrewType::Trooper };
	E2->SpawnPoints = { FVector::ZeroVector };
	E2->Activate();
	TestEqual(TEXT("a trooper is announced once a run"), TDBarkTest::Count(B, TEXT("Bark_Foot_Trooper")), 1);
	W.Advance(11.f);
	for (const TWeakObjectPtr<AActor>& M : E->GetSpawned()) { if (UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr) { H->Kill(P, ETDDamageKind::Melee); } }
	W.Advance(0.5f);
	AddInfo(FString::Printf(TEXT("said: %s; first fight cleared %d"), *FString::JoinBy(B->GetSaid(), TEXT(", "), [](const FName& N) { return N.ToString(); }), E->IsCleared()));
	TestEqual(TEXT("the last one"), TDBarkTest::Count(B, TEXT("Bark_Foot_LastOne")), 1);
	return true;
}

// The pilot's health: hit hard, under 30 %, health back.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksHealth, "TriadDrift.Ground.Barks.HurtLowHealthAndHeal", TD_TEST_FLAGS)
bool FTDBarksHealth::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	UTDGroundBarks* B = UTDGroundBarks::Get(W.GetWorld());
	if (!TestNotNull(TEXT("barks"), B) || !P) { return false; }
	B->ResetForTests();
	B->TickBarks(0.1f);
	UTDHealthComponent* H = P->GetHealth();
	H->HullRegenPerSecond = 0.f; // only the refill below gives health back
	UTDStatics::ApplyTDDamage(P, H->MaxHull * 0.2f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("hit hard"), TDBarkTest::Count(B, TEXT("Bark_Foot_Hurt")), 1);
	W.Advance(3.f);
	UTDStatics::ApplyTDDamage(P, H->GetHull() - H->MaxHull * 0.2f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("low health"), TDBarkTest::Count(B, TEXT("Bark_Foot_LowHealth")), 1);
	W.Advance(12.f); // a Flavour line wants the radio quiet
	B->TickBarks(0.1f);
	H->InitMaxValues(H->MaxHull, H->MaxShield, true); // a checkpoint's refill
	B->TickBarks(0.1f);
	TestEqual(TEXT("health back"), TDBarkTest::Count(B, TEXT("Bark_Foot_Heal")), 1);
	return true;
}

// The must-hear lines win: no on-foot bark while a story sequence plays.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBarksStoryFirst, "TriadDrift.Ground.Barks.NeverOverAStoryLine", TD_TEST_FLAGS)
bool FTDBarksStoryFirst::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGroundBarks* B = UTDGroundBarks::Get(W.GetWorld());
	if (!TestNotNull(TEXT("barks"), B)) { return false; }
	B->ResetForTests();
	if (!UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic03")))
	{
		AddInfo(TEXT("no Cinematic 03 here: only the free case"));
	}
	else
	{
		W.Advance(0.2f);
		TestFalse(TEXT("held while the scene plays"), B->Say(TEXT("Bark_Foot_Contact"), ETDFootSpeaker::Wingman, ETDBarkPriority::Critical));
		if (UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld())) { Story->SkipAllowedAfterSeconds = 0.f; Story->SkipSequence(); }
		W.Advance(0.5f);
	}
	TestTrue(TEXT("said once the radio is free"), B->Say(TEXT("Bark_Foot_Contact"), ETDFootSpeaker::Wingman, ETDBarkPriority::Critical));
	TestEqual(TEXT("the radio is a wingman, not the pilot on foot"), B->SpeakerName(ETDFootSpeaker::Wingman) != ATDFirstPersonCharacter::GetOnFootPilotName(W.GetWorld()), true);
	return true;
}

#endif
