// Triad Drift - Squad (Teammate 3). C-03 #1 "Switching is a cut": the data Nick's presentation needs
// to play a ship switch as a director's camera cut. The switch itself is instant (same frame);
// these are presentation hints only and never delay or lock player control.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDSwitchCut.generated.h"

class ATDShip;

/** Suggested timing for the cut presentation (C-03 #1). Tunable on ATDSquadManager::CutTiming. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDSwitchCutTiming
{
	GENERATED_BODY()

	/** The white "cut" flash, in frames (features.md: a 1-frame flash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0"))
	int32 FlashFrames = 1;

	/** Letterbox bars snap in over this long... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0", Units = "s"))
	float LetterboxInSeconds = 0.08f;

	/** ...hold... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0", Units = "s"))
	float LetterboxHoldSeconds = 0.45f;

	/** ...and slide out over this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0", Units = "s"))
	float LetterboxOutSeconds = 0.25f;

	/** Pilot name card ("WREN · RIFT") on screen time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0", Units = "s"))
	float NameCardSeconds = 1.4f;

	/** Rapid switches inside this window should play a lighter cut (no letterbox, card only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (ClampMin = "0", Units = "s"))
	float QuickCutWindowSeconds = 1.2f;
};

/** One ship switch, as a cut. Fired by ATDSquadManager::OnSwitchCut in the same frame as the switch. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDSwitchCut
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	ETDShipId NewShip = ETDShipId::Rift;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	ETDShipId OldShip = ETDShipId::Rift;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	TObjectPtr<ATDShip> NewShipActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	TObjectPtr<ATDShip> OldShipActor = nullptr;

	// ---- name card
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText PilotName;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText ShipName;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText RoleName;

	/** Ready-made "WREN · RIFT". */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText NameCardText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FLinearColor SquadColor = FLinearColor::White;

	// ---- context
	/** Forced by the active ship's death (auto-switch), not a player input. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	bool bAutoSwitch = false;

	/** This switch armed Switch Strike (+25%): worth a punchier cut. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	bool bSwitchStrike = false;

	/** Another switch happened within CutTiming.QuickCutWindowSeconds: play the light version. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	bool bQuickCut = false;

	/** 1-based switch count this squad spawn ("take"). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	int32 SwitchIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float WorldTimeSeconds = 0.f;

	// ---- cameras (world space, at the moment of the cut)
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FTransform FromCamera = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FTransform ToCamera = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FTDSwitchCutTiming Timing;
};
