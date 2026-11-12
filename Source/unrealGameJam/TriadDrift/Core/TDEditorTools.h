// Triad Drift - small editor-only helpers for headless authoring scripts (Tools/*.py). Lead-owned.
// The functions exist in every build so Blueprint/Python bindings stay stable; outside the editor
// they do nothing and return false.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDEditorTools.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDEditorTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Switch World Partition streaming on or off for an editor world. Our maps keep World Partition
	 * (for One File Per Actor) but turn streaming off, so nothing placed far away ever unloads.
	 * Returns true when the world has a World Partition and the value was applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Editor")
	static bool SetWorldPartitionStreaming(UWorld* World, bool bEnableStreaming);

	/** True when the world is partitioned and streaming is enabled. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Editor")
	static bool IsWorldPartitionStreamingEnabled(const UWorld* World);
};
