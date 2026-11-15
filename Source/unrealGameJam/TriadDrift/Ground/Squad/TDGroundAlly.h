// Triad Drift - the two other pilots on foot beside the player (#209, Ryan Nov 14). Owner: ground squad builder.
//
// A slim sibling of the on-foot pilot, not a copy of it: the same body (DA_Pilot_Visuals), the same C++ loop player and
// pilot clips, an energy blade in the pilot's own squad colour, AI-driven. It is deliberately NOT an
// ATDFirstPersonCharacter, so nothing that looks for "the player on foot" by class (encounter triggers, the core
// chamber, CUT, checkpoints, the camera) ever mistakes it for the player.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDGroundAlly.generated.h"

class UTDHealthComponent;
class UAnimSequenceBase;
class UStaticMeshComponent;
class UTDLoopBlendAnimInstance;
class ATDFirstPersonCharacter;
class UTDGroundSquadSubsystem;

UENUM()
enum class ETDGroundAllyState : uint8
{
	Follow,
	Fight,
	Down,
	Boarding
};

UCLASS()
class UNREALGAMEJAM_API ATDGroundAlly : public ACharacter, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDGroundAlly();

	// ------------------------------------------------------------------ tunables
	UPROPERTY(EditAnywhere, Category = "Ally|Health") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Health", meta = (Units = "s")) float HealthRegenDelay = 5.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Health") float HealthRegenPerSecond = 8.f;
	/** Down for this long, then up again at ReviveHealth01. */
	UPROPERTY(EditAnywhere, Category = "Ally|Health", meta = (Units = "s")) float DownSeconds = 8.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Health") float ReviveHealth01 = 0.5f;

	/** Share of the player's light-combo damage (the blade component's LightCombo numbers). */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight") float DamageScale = 0.6f;
	/** Enemies farther than this from the ally are not picked. */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight", meta = (Units = "cm")) float EngageRange = 1500.f;
	/** A fight is dropped when the player is farther than this (the ally comes back). */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight", meta = (Units = "cm")) float LeashRange = 2200.f;
	/** Pause between swings (random in the range); after the third hit of a combo, ComboRest. */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight") FVector2D SwingGap = FVector2D(0.7f, 1.2f);
	UPROPERTY(EditAnywhere, Category = "Ally|Fight", meta = (Units = "s")) float ComboRest = 1.6f;
	/** The hit lands this long into the swing clip (as the player's HitDelay). */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight", meta = (Units = "s")) float HitDelay = 0.12f;
	/** Chance a bolt arriving in the front arc is turned aside (no damage), and the rest between two. */
	UPROPERTY(EditAnywhere, Category = "Ally|Fight") float DeflectChance = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Ally|Fight", meta = (Units = "s")) float DeflectCooldown = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm/s")) float RunSpeed = 650.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm/s")) float SprintSpeed = 980.f;
	/** Farther than this from its slot the ally sprints. */
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm")) float SprintDistance = 550.f;
	/** Inside this of its slot the ally stands; it starts again past SlotLeave. */
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm")) float SlotArrive = 70.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm")) float SlotLeave = 170.f;
	/** Farther than this from the player, or stuck this long: put on a clear spot behind the player. */
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm")) float TeleportDistance = 3500.f;
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "s")) float StuckSeconds = 3.f;
	/** Kept clear on each side of the player's walked path and of the line to what the player is about to use. */
	UPROPERTY(EditAnywhere, Category = "Ally|Move", meta = (Units = "cm")) float PathClearance = 120.f;
	/** The body is hidden while the view camera is this close (never a suit filling the frame). */
	UPROPERTY(EditAnywhere, Category = "Ally|Look", meta = (Units = "cm")) float CameraHideDistance = 110.f;

	// ------------------------------------------------------------------ identity
	/** Wren, Kato or Moss. Call before or after BeginPlay. */
	void SetPilot(FName InPilot, int32 InSlot);
	FName GetPilot() const { return Pilot; }
	ETDShipId GetShipId() const;
	FLinearColor GetSquadColor() const;
	int32 GetSlot() const { return Slot; }

	// ------------------------------------------------------------------ state
	ETDGroundAllyState GetState() const { return State; }
	bool IsDown() const { return State == ETDGroundAllyState::Down; }
	/** 0..1 through the time on the ground (0 when standing). */
	float GetDown01() const { return IsDown() && DownSeconds > 0.f ? 1.f - FMath::Clamp(DownLeft / DownSeconds, 0.f, 1.f) : 0.f; }
	float GetHealth01() const;
	AActor* GetFightTarget() const { return FightTarget.Get(); }
	UTDHealthComponent* GetHealth() const { return Health; }
	UStaticMeshComponent* GetBladeBeam() const { return BladeBeam; }
	int32 GetSwingsLanded() const { return SwingsLanded; }
	int32 GetKills() const { return Kills; }
	int32 GetDeflects() const { return Deflects; }
	int32 GetTimesDown() const { return TimesDown; }
	int32 GetTeleports() const { return Teleports; }

	/** Down now (dev command, tests). */
	void GoDown();
	/** Standing, full health, on its slot by the player (a new take, a respawn). */
	void ResetBesideLeader();
	/** The player is climbing into the ship: run to them (the squad subsystem removes the ally when the hatch closes). */
	void BeginBoarding();
	/** The climb was called off (or it was the climb out): follow again. */
	void EndBoarding() { if (State == ETDGroundAllyState::Boarding) { State = ETDGroundAllyState::Follow; } }
	/** A clear spot behind the player, now. */
	void TeleportToLeader();

	// ITDTeam
	virtual ETDTeam GetTeam() const override { return ETDTeam::Player; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ally") TObjectPtr<UTDHealthComponent> Health;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BladeBeam;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SuitAccents;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UAnimSequenceBase>> Clips;

private:
	void ApplyLook();
	UAnimSequenceBase* Clip(std::initializer_list<const TCHAR*> Names);
	UTDLoopBlendAnimInstance* GetLoops() const;
	bool PlayShot(std::initializer_list<const TCHAR*> Names, float Rate, bool bUpperOnly, bool bHold = false);
	void UpdateBody(float DeltaSeconds);
	void UpdateCameraFade();

	UTDGroundSquadSubsystem* GetSquad() const;
	ATDFirstPersonCharacter* GetLeader() const;
	void TickFollow(float DeltaSeconds, ATDFirstPersonCharacter* Leader);
	void TickFight(float DeltaSeconds, ATDFirstPersonCharacter* Leader);
	void TickDown(float DeltaSeconds);
	void PickTarget(ATDFirstPersonCharacter* Leader);
	bool IsTargetUsable(const AActor* Target, const ATDFirstPersonCharacter* Leader, float Slack) const;
	bool HasClearLine(const AActor* To) const;
	/** Walk toward Goal (stops inside StopRadius). Returns true while it wants to move. */
	bool MoveToward(const FVector& Goal, float StopRadius, bool bSprint, float DeltaSeconds);
	/** A point off the player's path / interact line if the ally stands on it. */
	bool FindStepAside(const ATDFirstPersonCharacter* Leader, FVector& OutGoal) const;
	void StartSwing();
	void LandSwing();
	void GetUp();
	float FilterDamage(const FTDDamageInfo& Info);
	UFUNCTION() void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);
	void DropCrewTargetsOnMe();
	void FaceYaw(float Yaw, float DeltaSeconds, float Rate);

	FName Pilot = TEXT("Kato");
	int32 Slot = 0;
	bool bLookApplied = false;
	ETDGroundAllyState State = ETDGroundAllyState::Follow;

	TWeakObjectPtr<AActor> FightTarget;
	TWeakObjectPtr<AActor> IgnoredTarget;
	float IgnoreTargetLeft = 0.f;
	float SenseLeft = 0.f;
	float SwingCooldown = 0.f;
	float PendingHitLeft = -1.f;
	int32 ComboStep = 0;
	float DeflectLeft = 0.f;
	float DownLeft = 0.f;

	bool bHolding = true;      // standing on the slot (hysteresis)
	float StuckTime = 0.f;
	float AvoidLeft = 0.f;
	float AvoidSign = 0.f;
	float JumpLeft = 0.f;
	bool bBodyHidden = false;
	int32 LastGait = 0;

	int32 SwingsLanded = 0;
	int32 Kills = 0;
	int32 Deflects = 0;
	int32 TimesDown = 0;
	int32 Teleports = 0;
};
