// Triad Drift - weapon FX cue log (Teammate 2).

#include "TriadDrift/Combat/TDCombatCueLog.h"

#include "Sound/SoundBase.h"
#include "TriadDrift/Core/TDStatics.h"

namespace TDCombatCueLogState
{
	bool bLogging = false;
	TArray<FTDCombatCuePlay>& Plays()
	{
		static TArray<FTDCombatCuePlay> List;
		return List;
	}
}

void FTDCombatCueLog::Play(const UObject* WorldContext, const FTDFXCue& Cue, ETDCombatCueKind Kind, const UObject* FXAsset, const FVector& Location,
	const FRotator& Rotation)
{
	if (TDCombatCueLogState::bLogging && !Cue.IsEmpty())
	{
		FTDCombatCuePlay& Entry = TDCombatCueLogState::Plays().AddDefaulted_GetRef();
		Entry.Kind = Kind;
		Entry.FXName = FXAsset ? FXAsset->GetFName() : NAME_None;
		Entry.bHasSound = !Cue.Sound.IsNull() && Cue.Sound.LoadSynchronous() != nullptr;
	}
	UTDStatics::PlayCue(WorldContext, Cue, Location, Rotation);
}

void FTDCombatCueLog::SetLogging(bool bOn)
{
	TDCombatCueLogState::bLogging = bOn;
	TDCombatCueLogState::Plays().Reset();
}

const TArray<FTDCombatCuePlay>& FTDCombatCueLog::Get()
{
	return TDCombatCueLogState::Plays();
}

const TCHAR* FTDCombatCueLog::KindName(ETDCombatCueKind Kind)
{
	switch (Kind)
	{
	case ETDCombatCueKind::Muzzle: return TEXT("Muzzle");
	case ETDCombatCueKind::ImpactHull: return TEXT("ImpactHull");
	case ETDCombatCueKind::ImpactShield: return TEXT("ImpactShield");
	case ETDCombatCueKind::ImpactWorld: return TEXT("ImpactWorld");
	case ETDCombatCueKind::Detonation: return TEXT("Detonation");
	case ETDCombatCueKind::Deflected: return TEXT("Deflected");
	}
	return TEXT("?");
}
