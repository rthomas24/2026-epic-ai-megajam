// Triad Drift - World (Teammate 4). Story Act 1: the Dominion seen far off behind its distortion field, unreachable.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDDistortionField.generated.h"

class ATDDominion;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UNiagaraComponent;

/**
 * The Dominion before the passage opens: visible from wave 1, far off, behind a distortion field it cannot be
 * approached through. A mirage copy of the hull (the real ATDDominion stays hidden and untouchable, so nothing
 * about the assault changes) inside a field shell:
 * - Presentation: UTDDominionVisuals::DistortionFieldIdle (Niagara, attached) and / or DistortionFieldMaterial (a
 *   sphere shell); without either, an engineering stand-in (a slowly turning lattice of lights).
 * - A squad ship that reaches the shell is pushed back out, with the HUD line "DISTORTION FIELD - NO APPROACH" (a
 *   hyperdrive lock while a ship is near) and, the first time, the A1_FieldUnreachable transmission.
 * - When Cinder's field anchor falls the Dominion warps in: Collapse() plays DistortionFieldCollapse and the field
 *   goes away. From then on the four shield generators are the field that is left.
 */
UCLASS()
class UNREALGAMEJAM_API ATDDistortionField : public AActor
{
	GENERATED_BODY()

public:
	ATDDistortionField();

	static const FName LockId;

	/** Builds the mirage from the Dominion's visible meshes and sizes the field around it. */
	void Init(ATDDominion* InDominion);

	/** The passage is open: the collapse cue, then the field and the mirage are gone. */
	void Collapse();

	float GetFieldRadius() const { return FieldRadius; }
	bool IsCollapsed() const { return bCollapsed; }
	/** How many times a squad ship was pushed back (tests). */
	int32 GetRepelCount() const { return RepelCount; }
	bool IsWarningShown() const { return bWarning; }

	/** Field radius = this x the hull's half diagonal (at least twice the Dominion's shield radius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Field", meta = (ClampMin = "1"))
	float HullRadii = 1.6f;

	/** The HUD warning shows inside this multiple of the field radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Field", meta = (ClampMin = "1"))
	float WarningRadiusScale = 1.25f;

	/** Outward speed (cm/s) a ship is given when the field pushes it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Field", meta = (ClampMin = "0"))
	float RepelSpeed = 4000.f;

	/** Within this distance (cm) outside the shell, a ship heading in is turned away (smoothly: no camera snap). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Field", meta = (ClampMin = "0"))
	float DeflectBuffer = 15000.f;

	/** How fast (deg/s) the field turns a ship's nose away at the shell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Field", meta = (ClampMin = "0"))
	float DeflectTurnRate = 120.f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SetWarning(bool bOn);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Field")
	TObjectPtr<USceneComponent> Root;

	/** Engineering stand-in: lights on the shell. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Field")
	TObjectPtr<UInstancedStaticMeshComponent> Lattice;

	/** Sphere shell for DistortionFieldMaterial. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Field")
	TObjectPtr<UStaticMeshComponent> Shell;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Mirage;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> IdleFX;

	TWeakObjectPtr<ATDDominion> Dominion;
	float FieldRadius = 100000.f;
	float CollapseTime = -1.f;
	int32 RepelCount = 0;
	bool bCollapsed = false;
	bool bWarning = false;
	bool bToldUnreachable = false;
	/** Ships touching the shell now (a repel counts once per contact, not every frame). */
	TSet<TWeakObjectPtr<AActor>> Touching;
};
