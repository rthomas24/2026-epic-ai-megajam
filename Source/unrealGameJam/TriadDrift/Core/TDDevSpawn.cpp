// Triad Drift - dev-only console commands for presentation review in a running game (Lead, Core).
// Asked for by Nick's side (MESSAGEBOARD.md N-022): load and spawn a Blueprint actor class by path in a
// game-only session, where the stock "summon" cannot load an unloaded Blueprint. Not in Shipping.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDDevSpawn, Log, All);

namespace
{
	const FName DevSpawnTag(TEXT("TD.Dev.Spawned"));

	// TD.Dev.SpawnClass <ClassPath> [Tag] [X Y Z]
	// ClassPath: a Blueprint generated class, e.g. /Game/Folder/BP_Thing.BP_Thing_C (the _C is added when missing).
	void SpawnClass(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1)
		{
			UE_LOG(LogTDDevSpawn, Display, TEXT("TD.Dev.SpawnClass <ClassPath> [Tag] [X Y Z]: load an actor class by path and spawn it"));
			return;
		}
		FString Path = Args[0];
		if (!Path.Contains(TEXT(".")))
		{
			// "/Game/Folder/BP_Thing" -> "/Game/Folder/BP_Thing.BP_Thing_C"
			FString Name;
			Path.Split(TEXT("/"), nullptr, &Name, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			Path = Path + TEXT(".") + Name + TEXT("_C");
		}
		else if (!Path.EndsWith(TEXT("_C")))
		{
			Path += TEXT("_C");
		}
		UClass* Class = Cast<UClass>(FSoftObjectPath(Path).TryLoad());
		if (!Class || !Class->IsChildOf(AActor::StaticClass()))
		{
			UE_LOG(LogTDDevSpawn, Warning, TEXT("TD.Dev.SpawnClass: FAILED, no actor class at %s (is the package mounted and cooked or present?)"), *Path);
			return;
		}
		FVector Location = FVector::ZeroVector;
		if (Args.Num() >= 5)
		{
			Location = FVector(FCString::Atod(*Args[2]), FCString::Atod(*Args[3]), FCString::Atod(*Args[4]));
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(Class, Location, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			UE_LOG(LogTDDevSpawn, Warning, TEXT("TD.Dev.SpawnClass: FAILED to spawn %s"), *Path);
			return;
		}
		Actor->Tags.AddUnique(DevSpawnTag);
		if (Args.Num() >= 2 && !Args[1].IsEmpty() && Args[1] != TEXT("-"))
		{
			Actor->Tags.AddUnique(FName(*Args[1]));
		}
		UE_LOG(LogTDDevSpawn, Display, TEXT("TD.Dev.SpawnClass: OK %s as %s at %s tags=%d (a %s-tagged actor switches the placeholder sky off within a second)"),
			*Path, *Actor->GetName(), *Location.ToCompactString(), Actor->Tags.Num(), *TDTags::PresentationSky.ToString());
	}

	// TD.Dev.DestroySpawned: remove everything TD.Dev.SpawnClass created in this world.
	void DestroySpawned(const TArray<FString>&, UWorld* World)
	{
		int32 Count = 0;
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(DevSpawnTag))
				{
					It->Destroy();
					++Count;
				}
			}
		}
		UE_LOG(LogTDDevSpawn, Display, TEXT("TD.Dev.DestroySpawned: %d actor(s) removed"), Count);
	}

	// TD.Dev.ListTagged <Tag>: name, class and location of every actor with that tag.
	void ListTagged(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1)
		{
			return;
		}
		const FName Tag(*Args[0]);
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				UE_LOG(LogTDDevSpawn, Display, TEXT("TD.Dev.ListTagged: %s class=%s at %s"), *It->GetName(), *It->GetClass()->GetPathName(),
					*It->GetActorLocation().ToCompactString());
				++Count;
			}
		}
		UE_LOG(LogTDDevSpawn, Display, TEXT("TD.Dev.ListTagged: %d actor(s) tagged %s"), Count, *Tag.ToString());
	}

	FAutoConsoleCommandWithWorldAndArgs CmdSpawnClass(TEXT("TD.Dev.SpawnClass"),
		TEXT("TD.Dev.SpawnClass <ClassPath> [Tag|-] [X Y Z]: load an actor Blueprint class by path and spawn it (dev builds)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnClass));
	FAutoConsoleCommandWithWorldAndArgs CmdDestroySpawned(TEXT("TD.Dev.DestroySpawned"),
		TEXT("Remove every actor created by TD.Dev.SpawnClass."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DestroySpawned));
	FAutoConsoleCommandWithWorldAndArgs CmdListTagged(TEXT("TD.Dev.ListTagged"),
		TEXT("TD.Dev.ListTagged <Tag>: list actors with a tag."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListTagged));
}

#endif // !UE_BUILD_SHIPPING
