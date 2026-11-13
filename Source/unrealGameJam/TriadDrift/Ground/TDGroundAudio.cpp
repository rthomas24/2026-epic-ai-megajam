#include "TriadDrift/Ground/TDGroundAudio.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/StrongObjectPtr.h"

TSoftObjectPtr<USoundBase> TDGroundAudio::Default(const TCHAR* Relative)
{
	const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Audio/%s"), Relative);
	return TSoftObjectPtr<USoundBase>(FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package)));
}

namespace TDGroundAudioImpl
{
	static TWeakObjectPtr<const UTDGroundFX> Override;
	static bool bOverride = false;
	static TWeakObjectPtr<const UTDGroundFX> Cached;
	static bool bLooked = false;
	static TWeakObjectPtr<USoundBase> LastSound;
	static bool bLastFromCue = false;

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

const UTDGroundFX* TDGroundAudio::GroundFX()
{
	using namespace TDGroundAudioImpl;
	if (bOverride)
	{
		return Override.Get();
	}
	if (!bLooked || !Cached.IsValid())
	{
		// Looked up once (the presentation lookup searches folders); again only if it was collected.
		Cached = UTDStatics::LoadPresentation<UTDGroundFX>(TEXT("DA_Ground_FX"));
		bLooked = true;
	}
	return Cached.Get();
}

void TDGroundAudio::SetGroundFXOverride(const UTDGroundFX* FX)
{
	TDGroundAudioImpl::Override = FX;
	TDGroundAudioImpl::bOverride = FX != nullptr;
}

USoundBase* TDGroundAudio::GetLastResolvedSound() { return TDGroundAudioImpl::LastSound.Get(); }
bool TDGroundAudio::WasLastFromCue() { return TDGroundAudioImpl::bLastFromCue; }

const FTDFXCue* TDGroundAudio::CueFor(FName Event)
{
	const UTDGroundFX* FX = GroundFX();
	if (!FX)
	{
		return nullptr;
	}
	static const FName StepSoil(TEXT("StepSoil")), StepRock(TEXT("StepRock")), Dash(TEXT("Dash")), Jump(TEXT("Jump")), Land(TEXT("Land")),
		Hurt(TEXT("Hurt")), RifleShot(TEXT("RifleShot")), DroneHum(TEXT("DroneHum"));
	const FTDFXCue* Cue = Event == StepSoil ? &FX->StepSoil : Event == StepRock ? &FX->StepRock : Event == Dash ? &FX->Dash
		: Event == Jump ? &FX->Jump : Event == Land ? &FX->Land : Event == Hurt ? &FX->Hurt
		: Event == RifleShot ? &FX->CrewRifleShot : Event == DroneHum ? &FX->DroneHum : nullptr;
	return Cue && !Cue->IsEmpty() ? Cue : nullptr;
}

void TDGroundAudio::Play(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Fallback, const FVector& At, bool b2D,
	TArray<FName>& Log, FName Event)
{
	using namespace TDGroundAudioImpl;
	Record(Log, Event);
	if (const FTDFXCue* Cue = CueFor(Event))
	{
		USoundBase* CueSound = Cue->Sound.IsNull() ? nullptr : Load(Cue->Sound);
		LastSound = CueSound;
		bLastFromCue = true;
		if (!WorldContext)
		{
			return;
		}
		if (b2D)
		{
			if (CueSound)
			{
				UGameplayStatics::PlaySound2D(WorldContext, CueSound); // the player's own hurt is not placed in the world
			}
			return;
		}
		UTDStatics::PlayCue(WorldContext, *Cue, At); // sound, VFX and shake as the slot sets them
		return;
	}
	USoundBase* S = Load(Fallback);
	LastSound = S;
	bLastFromCue = false;
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

UAudioComponent* TDGroundAudio::PlayAttached(const TSoftObjectPtr<USoundBase>& Fallback, USceneComponent* AttachTo, TArray<FName>& Log, FName Event)
{
	using namespace TDGroundAudioImpl;
	Record(Log, Event);
	if (const FTDFXCue* Cue = CueFor(Event))
	{
		LastSound = Cue->Sound.IsNull() ? nullptr : Load(Cue->Sound);
		bLastFromCue = true;
		UAudioComponent* Audio = nullptr;
		if (AttachTo)
		{
			UTDStatics::PlayCueAttachedLoop(*Cue, AttachTo, NAME_None, Audio);
		}
		return Audio;
	}
	USoundBase* S = Load(Fallback);
	LastSound = S;
	bLastFromCue = false;
	return S && AttachTo ? UGameplayStatics::SpawnSoundAttached(S, AttachTo) : nullptr;
}
