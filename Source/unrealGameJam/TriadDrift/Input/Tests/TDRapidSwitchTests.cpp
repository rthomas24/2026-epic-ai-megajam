// Protect the core (the Lead, Nov 14): 20 rapid switches in 2 s, each with a power pressed and boost held on the ship being
// left, leave exactly one possessed ship, the camera on it, and no power stuck running anywhere.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.RapidSwitch

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1RapidSwitch, "TriadDrift.Input.RapidSwitch.TwentyInTwoSecondsLeaveOneFlownShip", TD_TEST_FLAGS)
bool FTDT1RapidSwitch::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		APlayerController* PC = W.SpawnPlayerController();
		ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
		if (!TestNotNull(TEXT("pc") + Fps, PC) || !TestNotNull(TEXT("squad") + Fps, Squad)) { return false; }
		Squad->SetSquadPlayerController(PC);
		Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
		W.Advance(1.f, Step);

		const ETDShipId Ids[] = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };
		int32 Accepted = 0;
		float T = 0.f, NextPress = 0.f;
		int32 Press = 0;
		int32 Changes = 0;
		ETDShipId Last = Squad->GetActiveShipId();
		for (; T < 2.f; T += Step)
		{
			if (T >= NextPress && Press < 20)
			{
				// The ship being left has its power pressed (never released) and boost held: the worst case for a stuck input.
				if (ATDShip* Left = Squad->GetActiveShip())
				{
					UTDPowerComponent::RoutePowerInput(Left, ETDPowerInput::Signature1, true);
					FTDFlightInput In;
					In.Throttle = 1.f;
					In.bBoost = true;
					Left->SetFlightInput(In);
				}
				const bool bOk = (Press % 2 == 0) ? Squad->RequestSwitchNext() : Squad->RequestSwitchTo(Ids[(Press * 7) % 3]);
				Accepted += bOk ? 1 : 0;
				++Press;
				NextPress += 0.1f;
			}
			W.Tick(Step);
			if (Squad->GetActiveShipId() != Last) { ++Changes; Last = Squad->GetActiveShipId(); }
		}
		W.Advance(0.6f, Step); // a buffered press lands
		AddInfo(FString::Printf(TEXT("%d presses, %d accepted at once, %d ship changes (the rest buffered by the cooldown)%s"), Press, Accepted, Changes, *Fps));
		TestEqual(TEXT("20 presses in 2 s") + Fps, Press, 20);
		TestTrue(TEXT("switches happened (the cooldown spaces them, presses buffer)") + Fps, Changes >= 3);

		ATDShip* Active = Squad->GetActiveShip();
		int32 Possessed = 0, Flown = 0;
		for (const ETDShipId Id : Ids)
		{
			const ATDShip* S = Squad->GetShip(Id);
			if (!S) { continue; }
			Possessed += S->GetController() == PC ? 1 : 0;
			Flown += S->IsPlayerFlown() ? 1 : 0;
		}
		TestEqual(TEXT("exactly one ship possessed by the player") + Fps, Possessed, 1);
		TestEqual(TEXT("exactly one ship flown") + Fps, Flown, 1);
		TestTrue(TEXT("... and it is the active ship") + Fps, Active && PC->GetPawn() == Active && Active->IsPlayerFlown());
		TestTrue(TEXT("the camera is on the flown ship") + Fps, Active && PC->GetViewTarget() == Active);

		// No power stuck on any ship: every power has a finite run; with no input for a while, none may still be running.
		if (Active) { UTDPowerComponent::RoutePowerInput(Active, ETDPowerInput::Signature1, false); }
		W.Advance(15.f, Step);
		int32 Running = 0;
		FString Which;
		for (const ETDShipId Id : Ids)
		{
			const ATDShip* S = Squad->GetShip(Id);
			if (!S) { continue; }
			TArray<UTDPowerComponent*> Powers;
			S->GetComponents<UTDPowerComponent>(Powers);
			for (const UTDPowerComponent* P : Powers)
			{
				if (P && P->IsPowerActive()) { ++Running; Which += FString::Printf(TEXT("%s/%s "), *UEnum::GetValueAsString(Id), *P->GetName()); }
			}
		}
		TestEqual(*FString::Printf(TEXT("no power stuck running after 15 s (%s)%s"), *Which, *Fps), Running, 0);
	}
	return true;
}

#endif
