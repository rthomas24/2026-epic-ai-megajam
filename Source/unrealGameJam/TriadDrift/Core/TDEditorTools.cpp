#include "TriadDrift/Core/TDEditorTools.h"

#include "Engine/World.h"
#include "WorldPartition/WorldPartition.h"

bool UTDEditorTools::SetWorldPartitionStreaming(UWorld* World, bool bEnableStreaming)
{
#if WITH_EDITOR
	if (!IsValid(World))
	{
		return false;
	}
	UWorldPartition* Partition = World->GetWorldPartition();
	if (!Partition)
	{
		return false;
	}
	Partition->SetEnableStreaming(bEnableStreaming);
	Partition->MarkPackageDirty();
	return Partition->IsStreamingEnabled() == bEnableStreaming;
#else
	return false;
#endif
}

bool UTDEditorTools::IsWorldPartitionStreamingEnabled(const UWorld* World)
{
	if (!IsValid(World))
	{
		return false;
	}
	const UWorldPartition* Partition = World->GetWorldPartition();
	return Partition && Partition->IsStreamingEnabled();
}
