// Triad Drift - engineering placeholder HUD (dev builds). Lead-owned.
// Draws crosshair, squad cards, wave info, target brackets and speed with the Canvas so the game is
// playable before Nick's real HUD exists. It installs itself only when the player has the stock
// AHUD; any real HUD class set by a game mode wins. Toggle with the console variable TD.DebugHUD.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDDebugHUD.generated.h"

UCLASS()
class UNREALGAMEJAM_API ATDDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Set by the UI host (TriadDrift/UI) while the real space HUD is on screen in World; the dev HUD then draws nothing there. */
	static void SetRealHUDActive(const UWorld* World, bool bActive);
	static bool IsRealHUDActive(const UWorld* World);

private:
	void DrawSquad(float& Y);
	void DrawWave();
	void DrawTargets();
	void DrawObjective();
	void DrawWingmen();
	void DrawDamageIndicators();

	UFUNCTION()
	void HandleHitConfirmed(AActor* Target, bool bKill);

	UFUNCTION()
	void HandleBarkRequested(FName BarkId, FName Speaker);
	void DrawCrosshair();
	void DrawFlight();
	void DrawFull();
	void DrawSupplementText(bool bPrompt, bool bHyperdriveBlock, bool bAltitude);
	void DrawLine2D(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness = 1.f);
	void DrawBracket(const FVector2D& Center, float HalfSize, const FLinearColor& Color);
	AActor* FindHUDDataActor();

	TWeakObjectPtr<AActor> CachedHUDDataActor;
	float NextSearchTime = 0.f;

	/** Hit marker around the crosshair: real seconds, so slow motion does not stretch it. */
	double HitMarkerUntil = 0.0;
	bool bLastHitWasKill = false;

	/** Stand-in subtitle for squad barks until voice and real subtitles exist. */
	FString BarkLine;
	double BarkUntil = 0.0;
};

/** Installs ATDDebugHUD for local players that still have the stock HUD. Not created in Shipping. */
UCLASS()
class UNREALGAMEJAM_API UTDDebugHUDSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	float TimeUntilCheck = 0.f;
};
