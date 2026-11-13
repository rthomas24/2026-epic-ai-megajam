// Triad Drift - Squad (Teammate 3). The words of the barks (see header, dialogs.md §10).

#include "TriadDrift/Squad/TDBarkLines.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "UObject/GCObject.h"

#if WITH_DEV_AUTOMATION_TESTS
TWeakObjectPtr<UDataTable> FTDBarkLines::PresentationOverrideForTests;
TWeakObjectPtr<UDataTable> FTDBarkLines::FallbackOverrideForTests;
#endif

namespace TDBarkLinesPrivate
{
	static const TCHAR* FallbackPath = TEXT("/Game/TriadDrift/Gameplay/Characters/Barks/DT_BarksFallback.DT_BarksFallback");

	struct FRotation
	{
		int32 LastIndex = INDEX_NONE;
		TMap<int32, double> UsedAt;
	};
	struct FPick
	{
		FText Line;
		double At = -1000.0;
	};
	static TMap<FName, FRotation> Rotations;
	static TMap<FName, FPick> Picks;

	static UDataTable* Presentation()
	{
#if WITH_DEV_AUTOMATION_TESTS
		if (FTDBarkLines::PresentationOverrideForTests.IsValid() || FTDBarkLines::FallbackOverrideForTests.IsValid())
		{
			return FTDBarkLines::PresentationOverrideForTests.Get();
		}
#endif
		return UTDStatics::LoadPresentation<UDataTable>(TEXT("DT_Barks"));
	}

	class FTableKeepAlive : public FGCObject
	{
	public:
		TObjectPtr<UDataTable> Table = nullptr;
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override { Collector.AddReferencedObject(Table); }
		virtual FString GetReferencerName() const override { return TEXT("FTDBarkLines fallback table"); }
	};

	static UDataTable* Fallback()
	{
#if WITH_DEV_AUTOMATION_TESTS
		if (FTDBarkLines::PresentationOverrideForTests.IsValid() || FTDBarkLines::FallbackOverrideForTests.IsValid())
		{
			return FTDBarkLines::FallbackOverrideForTests.Get();
		}
#endif
		// Held by a GC referencer for the session (T2 found a weak pointer here: the first GC freed the table and every
		// bark went silent). Tried again only while it is missing, and at most once a second.
		static FTableKeepAlive* Keep = new FTableKeepAlive(); // never deleted: outlives shutdown order
		static double LastTry = -1000.0;
		const double Now = FPlatformTime::Seconds();
		if (!Keep->Table && Now - LastTry >= 1.0)
		{
			LastTry = Now;
			Keep->Table = LoadObject<UDataTable>(nullptr, FallbackPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
		}
		return Keep->Table;
	}

	static FName RowName(FName BarkId, FName Speaker)
	{
		return FName(*FString::Printf(TEXT("%s__%s"), *BarkId.ToString(), *Speaker.ToString()));
	}

	static const FTDBarkRow* Find(UDataTable* Table, FName BarkId, FName Speaker)
	{
		if (!Table || Table->GetRowStruct() != FTDBarkRow::StaticStruct())
		{
			return nullptr;
		}
		if (const FTDBarkRow* Row = Table->FindRow<FTDBarkRow>(RowName(BarkId, Speaker), TEXT("Barks"), false))
		{
			return Row;
		}
		// The 12 engage calls share one row with {dir} in its lines.
		if (BarkId.ToString().StartsWith(TEXT("Bark_SquadEngage")))
		{
			return Table->FindRow<FTDBarkRow>(RowName(TEXT("Bark_SquadEngage<Dir>"), Speaker), TEXT("Barks"), false);
		}
		return nullptr;
	}

	/** The dev HUD shows a bark's line only when there is one (never the raw id). */
	static bool ResolveForHUD(const UObject* WorldContext, FName BarkId, FName Speaker, FString& Out)
	{
		FText Line;
		if (!FTDBarkLines::PickLine(WorldContext, BarkId, Speaker, Line))
		{
			return false;
		}
		Out = Line.ToString();
		return true;
	}

	static FDelayedAutoRegisterHelper RegisterResolver(EDelayedRegisterRunPhase::EndOfEngineInit, []()
	{
		ATDDebugHUD::BarkLineResolver = &ResolveForHUD;
	});
}

const FTDBarkRow* FTDBarkLines::FindRow(FName BarkId, FName Speaker)
{
	using namespace TDBarkLinesPrivate;
	if (const FTDBarkRow* Row = Find(Presentation(), BarkId, Speaker))
	{
		return Row; // Nicholas's row wins
	}
	return Find(Fallback(), BarkId, Speaker);
}

FString FTDBarkLines::DirectionOf(FName BarkId)
{
	FString S = BarkId.ToString();
	if (!S.RemoveFromStart(TEXT("Bark_SquadEngage")))
	{
		return FString();
	}
	FString Out;
	for (int32 i = 0; i < S.Len(); ++i)
	{
		if (i > 0 && FChar::IsUpper(S[i]))
		{
			Out.AppendChar(TEXT(' '));
		}
		Out.AppendChar(FChar::ToLower(S[i]));
	}
	return Out;
}

bool FTDBarkLines::PickLine(const UObject* WorldContext, FName BarkId, FName Speaker, FText& OutLine)
{
	using namespace TDBarkLinesPrivate;
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const double Now = World ? World->GetRealTimeSeconds() : FPlatformTime::Seconds();
	const FName Key = RowName(BarkId, Speaker);
	if (const FPick* P = Picks.Find(Key); P && Now - P->At >= 0.0 && Now - P->At < 3.0)
	{
		OutLine = P->Line; // one pick per request: every reader sees the same line
		return true;
	}
	const FTDBarkRow* Row = FindRow(BarkId, Speaker);
	if (!Row || Row->Lines.Num() == 0)
	{
		return false;
	}
	// Rotation: never the same line twice in a row; not reused within 60 s; otherwise the one used longest ago.
	FRotation& R = Rotations.FindOrAdd(Key);
	int32 Best = INDEX_NONE;
	double BestAge = -1.0;
	for (int32 i = 0; i < Row->Lines.Num(); ++i)
	{
		if (Row->Lines.Num() > 1 && i == R.LastIndex)
		{
			continue;
		}
		const double* Used = R.UsedAt.Find(i);
		const double Age = Used ? Now - *Used : 1e9;
		if (Age > BestAge)
		{
			BestAge = Age;
			Best = i;
		}
	}
	if (Best == INDEX_NONE)
	{
		Best = 0;
	}
	R.LastIndex = Best;
	R.UsedAt.Add(Best, Now);
	FString Line = Row->Lines[Best];
	const FString Dir = DirectionOf(BarkId);
	if (!Dir.IsEmpty())
	{
		FString Cap = Dir;
		Cap[0] = FChar::ToUpper(Cap[0]);
		Line.ReplaceInline(TEXT("{dir}"), *Dir, ESearchCase::CaseSensitive);
		Line.ReplaceInline(TEXT("{Dir}"), *Cap, ESearchCase::CaseSensitive);
	}
	OutLine = FText::FromString(Line);
	Picks.Add(Key, { OutLine, Now });
	return true;
}

TArray<TArray<TPair<FName, FName>>> FTDBarkLines::GetBanters()
{
	using namespace TDBarkLinesPrivate;
	// Bark_Banter<N>_<k>__<Speaker>: collect from both tables (Nicholas's rows replace ours with the same name).
	TMap<int32, TMap<int32, TPair<FName, FName>>> ByExchange;
	for (UDataTable* Table : { Fallback(), Presentation() })
	{
		if (!Table || Table->GetRowStruct() != FTDBarkRow::StaticStruct())
		{
			continue;
		}
		for (const TPair<FName, uint8*>& It : Table->GetRowMap())
		{
			const FTDBarkRow* Row = reinterpret_cast<const FTDBarkRow*>(It.Value);
			FString Rest = Row ? Row->Id.ToString() : FString();
			FString NStr, KStr;
			if (Rest.RemoveFromStart(TEXT("Bark_Banter")) && Rest.Split(TEXT("_"), &NStr, &KStr))
			{
				const int32 N = FCString::Atoi(*NStr);
				const int32 K = FCString::Atoi(*KStr);
				if (N > 0 && K > 0)
				{
					ByExchange.FindOrAdd(N).Add(K, TPair<FName, FName>(Row->Id, Row->Speaker));
				}
			}
		}
	}
	ByExchange.KeySort(TLess<int32>());
	TArray<TArray<TPair<FName, FName>>> Out;
	for (TPair<int32, TMap<int32, TPair<FName, FName>>>& Ex : ByExchange)
	{
		Ex.Value.KeySort(TLess<int32>());
		TArray<TPair<FName, FName>>& Steps = Out.AddDefaulted_GetRef();
		for (const TPair<int32, TPair<FName, FName>>& S : Ex.Value)
		{
			Steps.Add(S.Value);
		}
	}
	return Out;
}

void FTDBarkLines::ResetForTests()
{
	TDBarkLinesPrivate::Rotations.Reset();
	TDBarkLinesPrivate::Picks.Reset();
}
