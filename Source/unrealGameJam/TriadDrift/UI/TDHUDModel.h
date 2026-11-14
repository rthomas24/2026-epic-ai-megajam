// Triad Drift - the data behind the richer flight HUD (issue #160; Teammate 2). Layout-free: the squad panel and the
// radar read this, whichever concept variant is drawn.
//
//   Squad[3]   per ship, fixed order Rift / Lancer / Bastion: hull, shield, down + respawn, signature / second power /
//              ability cooldowns and state, boost, the squad-wide overcharge, and "under fire" (1 on a hit, fading to 0
//              over UnderFireSeconds) for every ship, not only the flown one.
//   Contacts   the radar: at most MaxContacts entries in the flown ship's frame (X right, Y forward, metres; Up = above +
//              / below -), nearest first after the must-shows (missiles, objectives, incoming groups). Enemies with their
//              kind, missiles, wingmen, objective waypoints, pickups, portal gates, warp-in markers, and incoming wave
//              groups (T4's GetPendingGroups, read by name when it lands).
//
// Update() runs once per HUD tick and allocates nothing after the first frames (arrays keep their capacity).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

class AActor;
class UWorld;
struct FTDTargetMarker;

enum class ETDRadarType : uint8 { Enemy, Missile, Wingman, Objective, Pickup, Gate, WarpIn, Incoming };

struct UNREALGAMEJAM_API FTDRadarContact
{
	ETDRadarType Type = ETDRadarType::Enemy;
	/** Enemy: UTDFlightHUDWidget::KindOf. Wingman: ETDShipId. Pickup: ETDPickupType. */
	uint8 Kind = 0;
	/** Flown ship's frame, metres: X right, Y forward (forward is up on the radar). */
	FVector2D Plane = FVector2D::ZeroVector;
	/** Metres above (+) or below (-) the flown ship's wing plane. */
	float Up = 0.f;
	/** Straight-line metres. */
	float Range = 0.f;
	/** Beyond RadarRange: draw it on the rim. */
	bool bBeyond = false;
	/** An enemy attacking the flown ship, or a missile homing on it. */
	bool bThreat = false;
	/** Incoming group: seconds until it arrives and how many; enemy id. */
	float Seconds = 0.f;
	int32 Count = 0;
	FName Id;
};

struct UNREALGAMEJAM_API FTDSquadStatus
{
	ETDShipId Ship = ETDShipId::Rift;
	bool bValid = false;
	bool bFlown = false;
	bool bAlive = true;
	float Hull = 1.f;
	float Shield = 1.f;
	float RespawnSeconds = 0.f;
	/** Signature power (Q tap): 0 just used .. 1 ready; and whether it is running now. */
	float Power01 = 1.f;
	bool bPowerActive = false;
	/** Bastion's hold-Q shield dome. */
	bool bHasPower2 = false;
	float Power201 = 1.f;
	bool bPower2Active = false;
	/** The unlocked ability (E), None until awarded. */
	ETDAbilityId Ability = ETDAbilityId::None;
	float Ability01 = 1.f;
	bool bAbilityActive = false;
	float Boost01 = 1.f;
	/** Squad-wide overcharge (a pickup powers every ship's guns): 0..1 left and seconds left. */
	float Overcharge01 = 0.f;
	float OverchargeSeconds = 0.f;
	/** 1 on the frame a hit lands (hull or shield went down), fading to 0 over UnderFireSeconds. */
	float UnderFire01 = 0.f;
	/** Hull under the trouble line (a wingman to help). */
	bool bTrouble = false;
};

class UNREALGAMEJAM_API FTDHUDModel
{
public:
	static constexpr int32 MaxContacts = 48;
	static constexpr float UnderFireSeconds = 1.5f;
	static constexpr float TroubleHull = .35f;

	/** Radar scale: contacts further than this sit on the rim (bBeyond). */
	float RadarRange = 1500.f;
	/** Enemies and pickups further than this are not listed at all (objectives, gates and incoming groups always are). */
	float ListRange = 6000.f;

	FTDSquadStatus Squad[3];
	TArray<FTDRadarContact> Contacts;

	/**
	 * Once per HUD tick, from what the HUD already read from its source this frame (the source's getters return by value;
	 * asking twice would double their cost): squad cards, target markers, objective markers.
	 */
	void Update(const UWorld* World, const TArray<FTDShipHUD>& Cards, const TArray<FTDTargetMarker>& Markers,
		const TArray<AActor*>& Objectives, float Dt);

	/** Tests: number of contacts of a type. */
	int32 CountOf(ETDRadarType Type) const;

private:
	void UpdateSquad(const UWorld* World, const TArray<FTDShipHUD>& Cards, float Dt);
	void UpdateContacts(const UWorld* World, const TArray<FTDTargetMarker>& Markers, const TArray<AActor*>& Objectives, float Dt);
	void Add(ETDRadarType Type, const FVector& World, uint8 Kind = 0);

	float LastHealth[3] = { -1.f, -1.f, -1.f };
	float GateScanLeft = 0.f;
	TArray<TWeakObjectPtr<AActor>> Gates;
	TArray<AActor*> Scratch;
	FTransform Frame = FTransform::Identity;
	bool bHasFrame = false;
};
