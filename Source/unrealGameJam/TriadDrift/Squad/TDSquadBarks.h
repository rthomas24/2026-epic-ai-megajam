// Triad Drift - Squad (Teammate 3). Squad bark triggers (ART-11 voice lines, Nick's audio).
//
// Gameplay asks for a line with an id + speaker through ITDGameFlow::RequestBark (which re-broadcasts
// UTDEventBus::OnBarkRequested for the audio / subtitle side). The limiter keeps it from ever
// spamming: a per-bark cooldown, a global minimum gap between lines, and a cap per time window.
// High-priority lines (a ship going down, a squad wipe) skip the global gap but still respect their
// own cooldown and the window cap. Real time, so slow-mo never bunches lines up.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDSquadBarks.generated.h"

/** Bark ids (Nick's DT_Barks rows key on these; see docs/eng/squad.md). */
namespace TDBarkIds
{
	inline const FName SwitchIn(TEXT("Bark_SquadSwitchIn"));        // speaker = the pilot you switched to
	inline const FName ShipDownRift(TEXT("Bark_SquadShipDownRift")); // speaker = a squadmate reacting
	inline const FName ShipDownLancer(TEXT("Bark_SquadShipDownLancer"));
	inline const FName ShipDownBastion(TEXT("Bark_SquadShipDownBastion"));
	inline const FName Respawn(TEXT("Bark_SquadRespawn"));          // speaker = the pilot back in
	inline const FName FocusAck(TEXT("Bark_SquadFocusAck"));        // speaker = a wingman acknowledging
	inline const FName SwitchStrike(TEXT("Bark_SquadSwitchStrike")); // speaker = the pilot switched to: "Strike's hot!"
	inline const FName WingmanKill(TEXT("Bark_SquadWingmanKill"));  // speaker = the wingman who scored
	inline const FName LowHull(TEXT("Bark_SquadLowHull"));          // speaker = the pilot in trouble
	inline const FName SquadWipe(TEXT("Bark_SquadWipe"));           // speaker = the last pilot down
	// ENG-43, dialogs.md's (new, T3) rows.
	inline const FName PlayerKill(TEXT("Bark_PlayerKill"));   // a wingman reacts to the player's kill, 1 in 3 (Flavour)
	inline const FName MultiKill(TEXT("Bark_MultiKill"));     // 3 player kills within 4 s (High)
	inline const FName ShieldDown(TEXT("Bark_ShieldDown"));   // that pilot's shield breaks (High)
	inline const FName ShieldBack(TEXT("Bark_ShieldBack"));   // full again after a break (Flavour)
	inline const FName Breather(TEXT("Bark_Breather"));       // 4 s into the gap between waves (Flavour)
	inline const FName OnMySix(TEXT("Bark_OnMySix"));         // an enemy holds behind the flown ship for 3 s (High)
	inline const FName CmdRetake(TEXT("Bark_Cmd_Retake"));    // Command, first line of a retake, 1 in 2
	inline const FName AbilityUnlocked(TEXT("Bark_AbilityUnlocked_<Ability>")); // that ship's pilot (the row per pilot holds their ability's line)
	inline const FName CmdObjectiveNew(TEXT("Bark_Cmd_ObjectiveNew"));   // Command, a new objective in space
	inline const FName CmdObjectiveDone(TEXT("Bark_Cmd_ObjectiveDone")); // Command, an objective in space completes

	UNREALGAMEJAM_API FName ShipDownFor(ETDShipId Ship);

	/**
	 * "Kato, engaging high left": the line a wingman calls as it enters a fight, picked by where IT is
	 * seen from the player's camera. Ids: Bark_SquadEngage[High|Low](Ahead|Left|Right|Behind), 12 in all
	 * (e.g. Bark_SquadEngageAhead, Bark_SquadEngageHighLeft, Bark_SquadEngageLowBehind). Speaker = that wingman.
	 * High / Low beyond 25 degrees of elevation; Ahead within 35 degrees of the view, Behind beyond 145.
	 */
	UNREALGAMEJAM_API FName EngageCallFor(const FTransform& Camera, const FVector& Where);
	/** "Wren" / "Kato" / "Moss" (the speaker names RequestBark expects). */
	UNREALGAMEJAM_API FName SpeakerFor(ETDShipId Ship);
}

/** Normal lines respect everything; High skips the global gap; Critical (squad wipe: once per take)
 *  also skips the window cap. Every line still respects its own cooldown. */
UENUM(BlueprintType)
enum class ETDBarkPriority : uint8
{
	Normal,
	High,
	Critical,
	/** Banter and colour (dialogs.md §2): only after FlavourQuietSeconds with no line, never stacked. */
	Flavour
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDBarkTuning
{
	GENERATED_BODY()

	/** Minimum real seconds between any two squad lines (normal priority). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (Units = "s"))
	float GlobalMinGapSeconds = 2.5f;

	/** At most this many lines per window... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (ClampMin = "1"))
	int32 MaxPerWindow = 3;

	/** ...of this many real seconds (also caps high-priority lines). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (Units = "s"))
	float WindowSeconds = 10.f;

	/** A Flavour line needs this many real seconds of radio silence first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (Units = "s"))
	float FlavourQuietSeconds = 8.f;

	/** Cooldown per bark id when not listed in CooldownById. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (Units = "s"))
	float DefaultCooldownSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks")
	TMap<FName, float> CooldownById;

	/** Low-hull line when a ship's hull falls below this... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHullFraction = 0.3f;

	/** ...and only again after it has been repaired above this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Barks", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHullRearmFraction = 0.55f;

	FTDBarkTuning();
};

/** The limiter: pure logic, no world. */
struct UNREALGAMEJAM_API FTDBarkLimiter
{
	/** True if the line may play now (and records it). */
	bool TryConsume(const FTDBarkTuning& Tuning, FName BarkId, ETDBarkPriority Priority, double RealNow);
	void Reset();

	TMap<FName, double> LastById;
	TArray<double> Recent; // real times of recent lines, oldest first
	double LastAny = -1000.0;
};
