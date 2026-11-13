// Triad Drift - Combat (Teammate 2). Dev tool: freeze and screenshot the moment an enemy dies (non-shipping command).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/Ticker.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDDevDeathCapture.generated.h"

class UWorld;

/**
 * TD.Dev.CaptureOnDeath <class|any|off> [count=1] [hold seconds=3]: the next `count` deaths of an actor whose class name
 * contains `class` (case-insensitive; "any" matches every kill) drop the world to near-zero time dilation for `hold` real
 * seconds and request an engine screenshot (Saved/Screenshots/.../TDDeath_<class>_<n>.png), so the death explosion
 * can be seen and captured (also with the MCP CaptureGameViewport while it holds). Logs each catch:
 * "TD.Dev.CaptureOnDeath: caught <victim> (<class>) ...". The command is compiled out of Shipping.
 */
UCLASS()
class UNREALGAMEJAM_API UTDDevDeathCapture : public UObject
{
	GENERATED_BODY()

public:
	/** Arms on World's event bus (replaces an armed capture). Returns the capture object, or null without a bus. */
	static UTDDevDeathCapture* Arm(UWorld* World, const FString& ClassFilter, int32 Count, float HoldSeconds, bool bRequestScreenshots = true);
	/** Disarms and restores time dilation if a hold is running. */
	static void Disarm();
	/** The armed capture (tests), or null. */
	static UTDDevDeathCapture* GetActive();

	/**
	 * Kills the live hostile closest to the screen centre (on screen, in front of the camera) through the normal damage
	 * path, so its real death (explosion, cues, events) plays where the camera looks. IdFilter (an enemy spec id or class
	 * name, "" = any) narrows the choice. Returns the victim, or null. Console: TD.Dev.KillCentral [id] (non-shipping).
	 */
	static AActor* KillCentral(UWorld* World, const FString& IdFilter);
	/** "ShardFighter" for an enemy ship, else the class name (logs). */
	static FString DescribeVictim(const AActor* Victim);

	/** Time dilation during a hold. */
	static constexpr float HoldDilation = 0.03f;

	int32 GetCaught() const { return Caught; }
	int32 GetRemaining() const { return Remaining; }
	bool IsHolding() const { return bHolding; }
	/** Ends the current hold now (tests; the ticker does it after HoldSeconds of real time). */
	void EndHold();

	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);

private:
	bool Matches(const AActor* Victim) const;
	void Unbind();

	FString Filter;
	int32 Remaining = 0;
	int32 Caught = 0;
	float HoldSeconds = 3.f;
	bool bScreenshots = true;
	bool bHolding = false;
	float DilationBefore = 1.f;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<class UTDEventBus> Bus;
	FTSTicker::FDelegateHandle HoldTicker;
};
