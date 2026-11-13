// Triad Drift - records UTDIntroDirector::OnIntroFinished in tests (N-039). Owner: Teammate 3.
// A UCLASS has to live in a header for UHT; it's tiny and inert outside tests.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TDIntroTestListener.generated.h"

UCLASS(Transient, NotBlueprintable, NotBlueprintType)
class UTDIntroTestListener : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UTDIntroDirector> Intro;
	int32 Count = 0;
	bool bLastPlayed = false;
	bool bLastSkipped = false;
	bool bStillPlayingWhenFired = true;

	UFUNCTION()
	void OnFinished(bool bPlayed, bool bSkipped)
	{
		++Count;
		bLastPlayed = bPlayed;
		bLastSkipped = bSkipped;
		bStillPlayingWhenFired = Intro.IsValid() && Intro->IsPlaying();
	}
};
