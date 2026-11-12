// Triad Drift - late-bound calls into other areas' classes (sandbox glue). Owner: Teammate 5.
//
// The sandbox game mode and cheats must compile before T3's squad manager and T4's wave director
// merge, so they resolve classes by path and call UFUNCTIONs by name. Once those classes are on main
// these calls keep working unchanged; a missing class or function just logs and returns false.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

namespace TDFlowReflection
{
	/** Loads a class by path (null if it doesn't exist yet). */
	UClass* ResolveClass(const FSoftClassPath& Path);

	/** Calls a UFUNCTION with no parameters. Ignores its return value. */
	bool Call(UObject* Target, FName Function);

	/** Calls a UFUNCTION whose first parameter is an int32 (e.g. StartWave(int32)). */
	bool CallInt(UObject* Target, FName Function, int32 Value);

	/** Calls a UFUNCTION whose first parameter is an FTransform (e.g. SpawnSquad(FTransform)). */
	bool CallTransform(UObject* Target, FName Function, const FTransform& Value);

	/** Calls a no-parameter UFUNCTION that returns a UObject* (e.g. GetActiveShip()). */
	UObject* CallReturnObject(UObject* Target, FName Function);

	/** Returns true if the object's class has the UFUNCTION. */
	bool HasFunction(const UObject* Target, FName Function);
}
