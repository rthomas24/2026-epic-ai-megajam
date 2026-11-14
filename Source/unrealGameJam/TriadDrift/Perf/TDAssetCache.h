// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
// T7 perf (#89): assets looked up by path string on hot paths (the crew's animation loop is chosen every frame, the
// pilot's and the crew's one-shots on every swing and death). LoadObject with a path string parses the path, searches
// memory and, for a path that does not exist, asks the file system again on every call. This keeps one answer per path
// for the session: a found asset (weakly, so a deleted test asset is not kept alive), or "missing", so a slot with no
// art costs nothing after its first look. The warm-up subsystem has usually streamed the asset in already.
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

namespace TDAssetCache
{
	/** The asset at Path (an object path), loading it only the first time it is asked for; null if it does not exist. */
	UNREALGAMEJAM_API UObject* Find(const FString& Path, UClass* Class);

	template <typename T>
	T* Find(const FString& Path)
	{
		return Cast<T>(Find(Path, T::StaticClass()));
	}

	/** Lookups that reached LoadObject (tests and the perf report). */
	UNREALGAMEJAM_API int32 GetLoadCount();
	/** Forget every answer (tests). */
	UNREALGAMEJAM_API void Reset();
}
