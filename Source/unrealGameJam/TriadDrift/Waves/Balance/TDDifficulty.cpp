// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/Balance/TDDifficulty.h"

UTDDifficultySettings::UTDDifficultySettings()
{
	// Story: forgiving, for players who want the story and the switching. Ace: for the jam judges who fly well.
	Story.EnemyHull = 0.75f;
	Story.EnemyDamage = 0.6f;
	Story.EnemyAimError = 1.6f;
	Story.WaveSize = 0.75f;

	Ace.EnemyHull = 1.25f;
	Ace.EnemyDamage = 1.3f;
	Ace.EnemyAimError = 0.6f;
	Ace.WaveSize = 1.25f;
}

ETDDifficulty& UTDDifficultySettings::CurrentRef()
{
	static ETDDifficulty Current = GetDefault<UTDDifficultySettings>()->DefaultDifficulty;
	return Current;
}

ETDDifficulty UTDDifficultySettings::GetCurrent()
{
	return CurrentRef();
}

void UTDDifficultySettings::SetCurrent(ETDDifficulty Difficulty)
{
	CurrentRef() = Difficulty;
}

const FTDDifficultyScale& UTDDifficultySettings::GetScaleFor(ETDDifficulty Difficulty)
{
	const UTDDifficultySettings* S = GetDefault<UTDDifficultySettings>();
	switch (Difficulty)
	{
	case ETDDifficulty::Story: return S->Story;
	case ETDDifficulty::Ace:   return S->Ace;
	default:                   return S->Standard;
	}
}

FTDDifficultyScale UTDDifficultySettings::GetScale()
{
	return GetScaleFor(GetCurrent());
}
