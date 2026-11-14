// ENG-38 stage 2, the glide-lite (the Lead, Nov 14): after the reveal a forced glide carries the flown ship down through
// the cloud deck to ~900 m over the ground, levels off and hands back. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Descent.Glide

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"

namespace TDT1Glide
{
	struct FRun { float EndZ = 0.f; float EndPitch = 0.f; float MinZ = 1e9f; bool bHeldWarning = true; bool bInvulnerable = true; bool bDescent01InRange = true;
		bool bHandedBack = false; bool bOverrideCleared = false; bool bVulnerableAgain = false; float Seconds = 0.f; };

	FRun Fly(ETDShipId Id, float Fps, float StartZ, float GroundZ)
	{
		FRun R;
		FTDTestWorld W(UTDGameInstance::StaticClass());
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, StartZ));
		UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
		if (!Ship || !D) { return R; }
		Ship->SetShipId(Id);
		Ship->SetPlayerControlled(true);
		W.SpawnLocalPlayerController(Ship);
		Ship->TeleportSpacecraft(FVector(0.f, 0.f, StartZ), FRotator::ZeroRotator, false);
		W.Advance(0.2f);
		UTDShipMovementComponent* Move = Ship->GetShipMovement();
		if (!D->BeginGlide(Ship, GroundZ)) { return R; }
		const float Dt = 1.f / Fps;
		for (; R.Seconds < 20.f && D->IsGliding(); R.Seconds += Dt)
		{
			W.Tick(Dt);
			D->TickGlide(Dt); // the game instance's tickables don't run in a test world
			if (!D->IsGliding()) { break; }
			R.MinZ = FMath::Min(R.MinZ, (float)Ship->GetActorLocation().Z);
			R.bHeldWarning &= Move->IsBoundaryWarningHeld();
			R.bInvulnerable &= Ship->GetHealth() && Ship->GetHealth()->IsInvulnerable();
			const float P = UTDDescentSubsystem::GetDescent01(Ship);
			R.bDescent01InRange &= P >= 0.f && P <= 1.f;
		}
		R.bHandedBack = !D->IsGliding() && UTDDescentSubsystem::GetDescent01(Ship) < 0.f;
		R.bOverrideCleared = !Move->HasSpeedOverrideFor(UTDDescentSubsystem::GlideOverride) && !Move->IsPitchHeld();
		R.bVulnerableAgain = Ship->GetHealth() && !Ship->GetHealth()->IsInvulnerable();
		R.EndZ = Ship->GetActorLocation().Z;
		R.EndPitch = Ship->GetActorRotation().Pitch;
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1GlideEndsLevel, "TriadDrift.Travel.Descent.GlideThroughTheDeckEndsLevel", TD_TEST_FLAGS)
bool FTDT1GlideEndsLevel::RunTest(const FString& Parameters)
{
	using namespace TDT1Glide;
	// Arrival 300 m over a deck at 1.15 km above the pad (Verdance), flat ground at the pad: about 550 m of drop.
	const float Ground = 0.f, Start = 115000.f + UTDDescentSubsystem::GlideAboveDeckCm;
	const float Want = Ground + UTDDescentSubsystem::GlideEndAboveGroundCm;
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		for (const float Fps : { 30.f, 60.f, 120.f })
		{
			const FRun R = Fly(Id, Fps, Start, Ground);
			const FString Tag = FString::Printf(TEXT("%s @%.0f fps"), *UEnum::GetValueAsString(Id), Fps);
			AddInfo(FString::Printf(TEXT("%s: %.1f s, end %.0f m (want %.0f), lowest %.0f m, pitch %.1f"), *Tag, R.Seconds, R.EndZ / 100.f, Want / 100.f, R.MinZ / 100.f, R.EndPitch));
			TestTrue(*FString::Printf(TEXT("%s: handed back within the glide + level-off (%.1f s)"), *Tag, R.Seconds), R.bHandedBack && R.Seconds <= UTDDescentSubsystem::GlideSeconds + UTDDescentSubsystem::GlideHandBackSeconds + 1.f);
			TestTrue(*FString::Printf(TEXT("%s: ends near 900 m over the ground (%.0f m)"), *Tag, R.EndZ / 100.f), FMath::Abs(R.EndZ - Want) <= 15000.f);
			TestTrue(*FString::Printf(TEXT("%s: never below 700 m (%.0f m)"), *Tag, R.MinZ / 100.f), R.MinZ >= Ground + 70000.f);
			TestTrue(*FString::Printf(TEXT("%s: ends level (pitch %.1f)"), *Tag, R.EndPitch), FMath::Abs(R.EndPitch) <= 5.f);
			TestTrue(*FString::Printf(TEXT("%s: invulnerable and no edge warning while gliding"), *Tag), R.bInvulnerable && R.bHeldWarning);
			TestTrue(*FString::Printf(TEXT("%s: Descent01 0..1 while gliding, -1 after"), *Tag), R.bDescent01InRange && R.bHandedBack);
			TestTrue(*FString::Printf(TEXT("%s: override and pitch hold cleared, vulnerable again"), *Tag), R.bOverrideCleared && R.bVulnerableAgain);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1GlideRetake, "TriadDrift.Travel.Descent.GlideEndsCleanlyOnARetake", TD_TEST_FLAGS)
bool FTDT1GlideRetake::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 145000.f));
	UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("descent"), D)) { return false; }
	Ship->SetPlayerControlled(true);
	W.SpawnLocalPlayerController(Ship);
	W.Advance(0.2f);
	TestTrue(TEXT("glide begins"), D->BeginGlide(Ship, 0.f));
	for (int32 i = 0; i < 90; ++i) { W.Tick(1.f / 60.f); D->TickGlide(1.f / 60.f); }
	TestTrue(TEXT("mid-glide"), D->IsGliding());
	D->EndGlide(TEXT("retake"));
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	TestTrue(TEXT("a retake ends it: override, pitch hold and invulnerability gone"), !D->IsGliding() && !Move->HasSpeedOverrideFor(UTDDescentSubsystem::GlideOverride)
		&& !Move->IsPitchHeld() && !Ship->GetHealth()->IsInvulnerable());
	TestEqual(TEXT("Descent01 is -1 again"), UTDDescentSubsystem::GetDescent01(Ship), -1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1Departure, "TriadDrift.Travel.Descent.TakeOffComesOutWhereItLeft", TD_TEST_FLAGS)
bool FTDT1Departure::RunTest(const FString& Parameters)
{
	// ENG-38 2.9: leave from two different places on the ground, come out above two different parts of the globe, outside
	// the atmosphere (no re-entry), heading away from the planet.
	TArray<FVector> Outs;
	for (const FTransform& Departure : { FTransform(FRotator(0.f, 0.f, 0.f), FVector(150000.f, 0.f, 120000.f)),
		FTransform(FRotator(0.f, 180.f, 0.f), FVector(-150000.f, 100000.f, 120000.f)) })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		const FVector Centre(400000.f, 0.f, 0.f);
		ATDPlanetAtmosphere* Planet = W.GetWorld()->SpawnActorDeferred<ATDPlanetAtmosphere>(ATDPlanetAtmosphere::StaticClass(), FTransform(Centre),
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!TestNotNull(TEXT("planet"), Planet)) { return false; }
		Planet->Configure(ETDDestination::Verdance, NAME_None, 30000.f, 66000.f, FLinearColor::Green);
		Planet->SetAutoTravel(false);
		Planet->FinishSpawning(FTransform(Centre));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
		if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("descent"), D)) { return false; }
		Ship->SetPlayerControlled(true);
		W.SpawnLocalPlayerController(Ship);
		W.Advance(0.1f);
		D->RecordDeparture(ETDDestination::Verdance, Departure);
		TestTrue(TEXT("placed"), D->PlaceDeparture(W.GetWorld()) && !D->HasPendingDeparture());
		const FVector L = Ship->GetActorLocation();
		const float Dist = FVector::Dist(L, Centre);
		TestTrue(*FString::Printf(TEXT("outside the atmosphere (%.0f vs %.0f)"), Dist, Planet->GetAtmosphereRadius()), Dist > Planet->GetAtmosphereRadius() * 1.1f);
		TestTrue(TEXT("heading away from the planet"), FVector::DotProduct(Ship->GetActorForwardVector(), (L - Centre).GetSafeNormal()) > 0.15f);
		Outs.Add((L - Centre).GetSafeNormal());
	}
	if (Outs.Num() == 2)
	{
		const float Deg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Outs[0], Outs[1]), -1.f, 1.f)));
		AddInfo(FString::Printf(TEXT("the two departures come out %.1f deg apart on the globe"), Deg));
		TestTrue(TEXT("two places on the ground: two places on the globe"), Deg > 5.f);
	}
	return true;
}

#endif
