// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
#include "TriadDrift/Perf/TDAssetCache.h"

#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

namespace
{
	struct FEntry
	{
		TWeakObjectPtr<UObject> Asset;
		bool bMissing = false;
	};
	TMap<FString, FEntry>& Entries()
	{
		static TMap<FString, FEntry> Map;
		return Map;
	}
	int32 GLoads = 0;
}

UObject* TDAssetCache::Find(const FString& Path, UClass* Class)
{
	check(IsInGameThread());
	if (Path.IsEmpty())
	{
		return nullptr;
	}
	FEntry& E = Entries().FindOrAdd(Path);
	if (UObject* Hit = E.Asset.Get())
	{
		return Hit;
	}
	if (E.bMissing)
	{
		return nullptr;
	}
	++GLoads;
	UObject* Loaded = StaticLoadObject(Class ? Class : UObject::StaticClass(), nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	E.Asset = Loaded;
	E.bMissing = Loaded == nullptr;
	return Loaded;
}

int32 TDAssetCache::GetLoadCount()
{
	return GLoads;
}

void TDAssetCache::Reset()
{
	Entries().Reset();
}
