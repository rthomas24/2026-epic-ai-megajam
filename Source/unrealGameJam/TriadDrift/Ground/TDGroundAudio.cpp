#include "TriadDrift/Ground/TDGroundAudio.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

TSoftObjectPtr<USoundBase> TDGroundAudio::Default(const TCHAR* Relative)
{
	const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Audio/%s"), Relative);
	return TSoftObjectPtr<USoundBase>(FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package)));
}

namespace TDGroundAudioImpl
{
	static void Record(TArray<FName>& Log, FName Event)
	{
		if (Log.Num() >= 32)
		{
			Log.RemoveAt(0);
		}
		Log.Add(Event);
	}

	static USoundBase* Load(const TSoftObjectPtr<USoundBase>& Sound)
	{
		if (Sound.IsNull())
		{
			return nullptr;
		}
		if (USoundBase* Loaded = Sound.Get())
		{
			return Loaded;
		}
		// Quietly: the fallback set may not be imported in every checkout.
		const FString Package = Sound.ToSoftObjectPath().GetLongPackageName();
		return FPackageName::DoesPackageExist(Package) ? Sound.LoadSynchronous() : nullptr;
	}
}

void TDGroundAudio::Play(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& At, bool b2D,
	TArray<FName>& Log, FName Event)
{
	TDGroundAudioImpl::Record(Log, Event);
	USoundBase* S = TDGroundAudioImpl::Load(Sound);
	if (!S || !WorldContext)
	{
		return;
	}
	if (b2D)
	{
		UGameplayStatics::PlaySound2D(WorldContext, S);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, S, At);
	}
}

UAudioComponent* TDGroundAudio::PlayAttached(const TSoftObjectPtr<USoundBase>& Sound, USceneComponent* AttachTo, TArray<FName>& Log, FName Event)
{
	TDGroundAudioImpl::Record(Log, Event);
	USoundBase* S = TDGroundAudioImpl::Load(Sound);
	return S && AttachTo ? UGameplayStatics::SpawnSoundAttached(S, AttachTo) : nullptr;
}
