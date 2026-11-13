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
 * contains `class` (or whose enemy id does; case-insensitive; "any" matches every kill) slow the world for `hold` real
 * seconds (time dilation 0.2 by default) and request an engine screenshot (Saved/Screenshots/.../TDDeath_<class>_<n>.png), so the death explosion
 * can be seen and captured (also with the MCP CaptureGameViewport while it holds). Logs each catch:
 * "TD.Dev.CaptureOnDeath: caught <victim> (<class>) ...". The command is compiled out of Shipping.
 */
UCLASS()
class UNREALGAMEJAM_API UTDDevDeathCapture : public UObject
{
	GENERATED_BODY()

public:
	/** Arms on World's event bus (replaces an armed capture). Returns the capture object, or null without a bus. */
	static UTDDevDeathCapture* Arm(UWorld* World, const FString& ClassFilter, int32 Count, float HoldSeconds, bool bRequestScreenshots = true,
		float Dilation = DefaultHoldDilation);
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

	/** TD.Dev.SpawnAt: an enemy held still (no pilot, no drift) where screen point Screen01 (0..1) looks, Metres away. */
	static AActor* SpawnAtScreen(UWorld* World, FName EnemyId, const FVector2D& Screen01, float Metres);
	/** "ShardFighter" for an enemy ship, else the class name (logs). */
	static FString DescribeVictim(const AActor* Victim);

	/**
	 * Time dilation during a hold. 0.2: a hold of 6 real seconds plays 1.2 s of the death (live, 0.03 froze the frame before
	 * the enemies' tumble-then-explode sequence reached the explosion).
	 */
	static constexpr float DefaultHoldDilation = 0.2f;
	float GetHoldDilation() const { return HoldDilation; }

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
	float HoldDilation = DefaultHoldDilation;
	bool bScreenshots = true;
	bool bHolding = false;
	float DilationBefore = 1.f;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<class UTDEventBus> Bus;
	FTSTicker::FDelegateHandle HoldTicker;
};
