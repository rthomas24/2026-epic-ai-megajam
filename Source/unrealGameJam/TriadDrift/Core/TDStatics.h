// Triad Drift - shared helpers: teams, damage, presentation cues, game-feel time dilation. Lead-owned.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDStatics.generated.h"

class ITDGameFlow;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UNiagaraComponent;
class USceneComponent;
class UTDHealthComponent;

UCLASS()
class UNREALGAMEJAM_API UTDStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UTDStatics();

	// ------------------------------------------------------------------ teams
	/** ITDTeam on the actor, else its UTDHealthComponent::DefaultTeam, else Neutral. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static ETDTeam GetTeam(const AActor* Actor);

	/** True only for Player vs Enemy (either way). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static bool AreTeamsHostile(ETDTeam A, ETDTeam B);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static bool AreHostile(const AActor* A, const AActor* B);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	static UTDHealthComponent* GetHealth(const AActor* Actor);

	// ------------------------------------------------------------------ damage
	/**
	 * THE way to deal damage. Friendly fire is off: the hit is dropped when the source team
	 * (Causer's team if it has one, else Instigator's) equals the target's team.
	 * Returns the damage actually dealt.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Damage")
	static float ApplyTDDamage(AActor* Target, float Amount, ETDDamageKind Kind, AActor* Instigator, AActor* Causer,
		FVector HitLocation = FVector::ZeroVector, FVector HitDirection = FVector::ZeroVector, bool bIgnoreShield = false);

	/**
	 * Splash damage to every live hostile of SourceTeam within Radius of Origin (measured to the
	 * actor's bounds). bFalloff scales linearly to 25% at the edge. Returns the number of actors hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Damage", meta = (WorldContext = "WorldContextObject"))
	static int32 ApplyTDRadialDamage(const UObject* WorldContextObject, FVector Origin, float Radius, float Amount,
		ETDDamageKind Kind, ETDTeam SourceTeam, AActor* Instigator, AActor* Causer, bool bFalloff = false,
		AActor* IgnoreActor = nullptr);

	// ------------------------------------------------------------------ flow
	/** The game flow (UTDGameInstance), or null in worlds that run a plain game instance. */
	static ITDGameFlow* GetGameFlow(const UObject* WorldContextObject);

	// ------------------------------------------------------------------ presentation (all null-safe)
	/**
	 * One-shot cue at a world location. The Niagara component comes from the engine pool and is
	 * released automatically: use the return value this frame only, never store it.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX", meta = (WorldContext = "WorldContextObject"))
	static UNiagaraComponent* PlayCue(const UObject* WorldContextObject, const FTDFXCue& Cue, FVector Location,
		FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX")
	static UNiagaraComponent* PlayCueAttached(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket = NAME_None);

	/**
	 * Attached cue for loops (charge-up, engine, beam): also hands back the audio component so the
	 * caller can stop both halves. Neither is pooled; the caller owns stopping them
	 * (Deactivate / FadeOut). Either value can be null when the cue leaves that half empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX")
	static UNiagaraComponent* PlayCueAttachedLoop(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket,
		UAudioComponent*& OutAudio);

	/**
	 * Placeholder look for engine BasicShapes meshes until real art is assigned: a flat colour on
	 * every material slot. EmissiveBoost > 0 uses an unlit glowing material (engine glows, eyes,
	 * projectiles) at Color * EmissiveBoost. Returns the instance on slot 0. Never call this on a
	 * mesh that has real art: check the visuals asset first.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX")
	static UMaterialInstanceDynamic* TintPlaceholder(UMeshComponent* Mesh, FLinearColor Color, float EmissiveBoost = 0.f);

	/**
	 * The one unlit glow material for placeholders: /Game/TriadDrift/Gameplay/Materials/M_TD_PlaceholderGlow.
	 * Emissive = the vector parameter "Color". It works on instanced static meshes, skeletal meshes and
	 * Niagara mesh particles and is always cooked. Use it (through a dynamic instance) for anything that
	 * must read from every direction: stars, shots, flashes, eyes, markers. The engine's
	 * EmissiveMeshMaterial has no instanced-mesh usage and falls back to grey in a cooked game.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|FX")
	static UMaterialInterface* GetPlaceholderGlowMaterial();

	/** Squad colour code: Rift magenta, Lancer cyan, Bastion amber. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|FX")
	static FLinearColor ShipColor(ETDShipId Ship);

	/** Team colour code: player green-white, enemy red-orange, neutral grey. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|FX")
	static FLinearColor TeamColor(ETDTeam Team);

	/**
	 * The one way gameplay loads a presentation data asset by its agreed name
	 * (docs/visual/Presentation-Asset-Paths.md), e.g. "DA_Rift_Visuals" from
	 * /Game/TriadDrift/Presentation/. Returns null quietly when the asset has not been authored yet
	 * (no log warning in packaged builds) or is not of ExpectedClass. Misses are remembered: for the
	 * whole session in packaged builds, for 10 s in the editor so new assets are picked up.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX")
	static UObject* LoadPresentationAsset(const FString& AssetName, TSubclassOf<UObject> ExpectedClass);

	template <typename T>
	static T* LoadPresentation(const FString& AssetName)
	{
		return Cast<T>(LoadPresentationAsset(AssetName, T::StaticClass()));
	}

	/** "/Game/TriadDrift/Presentation": the flat folder that is always cooked. */
	static FString GetPresentationFolder();

	/** True in the editor / dev builds when placeholder debug drawing should stand in for missing art. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|FX")
	static bool ShouldDrawPlaceholders();
private:
	/** Hard reference held by the class default object, so the cooker includes the material. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> PlaceholderGlowMaterial;
};

/**
 * Game-feel time control: hit-stop and slow motion share one owner of global time dilation so they
 * never fight. Timers run on real (undilated) time.
 */
UCLASS()
class UNREALGAMEJAM_API UTDFeelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDFeelSubsystem* Get(const UObject* WorldContextObject);

	/** Brief freeze on impact. Defaults: 40 ms at 5% speed (features.md ENG-05). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void RequestHitStop(float RealSeconds = 0.04f, float Dilation = 0.05f);

	/** Cinematic slow motion (Director's Cut money shots). The slowest active request wins. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void RequestSlowMo(float RealSeconds, float Dilation = 0.3f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void CancelAll();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feel")
	float GetCurrentDilation() const { return CurrentDilation; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feel")
	bool IsSlowMoActive() const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void Apply();

	double HitStopEnd = 0.0;
	float HitStopDilation = 1.f;
	double SlowMoEnd = 0.0;
	float SlowMoDilation = 1.f;
	float CurrentDilation = 1.f;
};
