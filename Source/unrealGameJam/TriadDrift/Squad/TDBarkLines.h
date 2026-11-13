// Triad Drift - Squad (Teammate 3). The words of the barks (dialogs.md §10).
//
// Bark ids are fired by gameplay (TDBarkIds, ITDGameFlow::RequestBark); this is where an id + speaker becomes a line:
//  - the rows come from Nicholas's DT_Barks (Presentation folder) when it has the row, else from the engineering fallback
//    DT_BarksFallback (Gameplay/Characters/Barks), generated from dialogs.md by Tools/td_barks_from_dialogs.py and
//    Tools/td-barks-import.ps1. Row name "<BarkId>__<Speaker>", row struct FTDBarkRow;
//  - each row has several lines: the same line never plays twice in a row, and a line is not reused within 60 s;
//  - the engage calls (Bark_SquadEngage<Dir>) fill {dir} / {Dir} from the id ("high left");
//  - one pick per request: every reader (the dev HUD, T2's subtitles) asking within 3 s gets the same line.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TriadDrift/Squad/TDSquadBarks.h"
#include "TDBarkLines.generated.h"

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDBarkRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	FName Id;

	/** Wren, Kato, Moss or Command. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	FName Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	ETDBarkPriority Priority = ETDBarkPriority::Normal;

	/** The alternatives; one is picked per play. {dir} / {Dir} / {n} are filled by code. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	TArray<FString> Lines;
};

struct UNREALGAMEJAM_API FTDBarkLines
{
	/** The line for this bark now (picked once, then the same for 3 s). False: no line, show nothing. */
	static bool PickLine(const UObject* WorldContext, FName BarkId, FName Speaker, FText& OutLine);

	/** The row (Nicholas's first, then the fallback), or null. */
	static const FTDBarkRow* FindRow(FName BarkId, FName Speaker);

	/** "Bark_SquadEngageHighLeft" -> "high left" (the {dir} of an engage call); empty for other ids. */
	static FString DirectionOf(FName BarkId);

	/** Banter exchanges in the table, in order: each a list of (id, speaker) steps (dialogs.md §9). */
	static TArray<TArray<TPair<FName, FName>>> GetBanters();

	/** Forget rotation and picks (tests). */
	static void ResetForTests();

#if WITH_DEV_AUTOMATION_TESTS
	/** Tests: use these instead of DT_Barks / DT_BarksFallback. */
	static TWeakObjectPtr<UDataTable> PresentationOverrideForTests;
	static TWeakObjectPtr<UDataTable> FallbackOverrideForTests;
#endif
};
