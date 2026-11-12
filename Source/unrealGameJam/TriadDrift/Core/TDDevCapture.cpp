#include "TriadDrift/Core/TDDevCapture.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TriadDrift/Core/TDTypes.h"
#include "UnrealClient.h"

bool UTDDevCapture::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const TCHAR* CommandLine = FCommandLine::Get();
	return FParse::Param(CommandLine, TEXT("TDCapture"))
		|| FCString::Stristr(CommandLine, TEXT("-TDShots=")) != nullptr
		|| FCString::Stristr(CommandLine, TEXT("-TDQuit=")) != nullptr
		|| FCString::Stristr(CommandLine, TEXT("-TDExec")) != nullptr;
#endif
}

void UTDDevCapture::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const TCHAR* CommandLine = FCommandLine::Get();

	FString Shots;
	if (FParse::Value(CommandLine, TEXT("-TDShots="), Shots, false))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","), true);
		for (const FString& Part : Parts)
		{
			ShotTimes.Add(FCString::Atof(*Part));
		}
		ShotTimes.Sort();
	}

	if (!FParse::Value(CommandLine, TEXT("-TDShotDir="), ShotDir, false))
	{
		ShotDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TDShots"));
	}
	ShotDir = FPaths::ConvertRelativePathToFull(ShotDir);
	IFileManager::Get().MakeDirectory(*ShotDir, true);

	if (!FParse::Value(CommandLine, TEXT("-TDQuit="), QuitTime))
	{
		QuitTime = ShotTimes.Num() > 0 ? ShotTimes.Last() + 1.f : -1.f;
	}

	FString Exec;
	if (FParse::Value(CommandLine, TEXT("-TDExec="), Exec, false))
	{
		TArray<FString> Parts;
		Exec.ParseIntoArray(Parts, TEXT(";"), true);
		for (const FString& Part : Parts)
		{
			FTimedCommand Command;
			Command.Time = 0.f;
			Command.Command = Part.TrimStartAndEnd();
			Commands.Add(Command);
		}
	}

	// Repeatable: -TDExecAt=4:"TD.KillAll"
	const TCHAR* Cursor = CommandLine;
	FString Token;
	while (FParse::Token(Cursor, Token, false))
	{
		static const FString Prefix(TEXT("-TDExecAt="));
		if (Token.StartsWith(Prefix, ESearchCase::IgnoreCase))
		{
			FString Rest = Token.Mid(Prefix.Len());
			FString TimeText, CommandText;
			if (Rest.Split(TEXT(":"), &TimeText, &CommandText))
			{
				FTimedCommand Command;
				Command.Time = FCString::Atof(*TimeText);
				Command.Command = CommandText.TrimQuotes().TrimStartAndEnd();
				Commands.Add(Command);
			}
		}
	}

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTDDevCapture::Tick));
	UE_LOG(LogTriadDrift, Display, TEXT("TDDevCapture: %d shots -> %s, quit at %.1f s, %d commands"),
		ShotTimes.Num(), *ShotDir, QuitTime, Commands.Num());
}

void UTDDevCapture::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

bool UTDDevCapture::Tick(float DeltaSeconds)
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World || !World->HasBegunPlay())
	{
		return true;
	}
	if (!bStarted)
	{
		bStarted = true;
		Elapsed = 0.f;
	}
	Elapsed += FMath::Min(DeltaSeconds, 0.25f);

	for (FTimedCommand& Command : Commands)
	{
		if (!Command.bDone && Elapsed >= Command.Time)
		{
			Command.bDone = true;
			UE_LOG(LogTriadDrift, Display, TEXT("TDDevCapture: exec '%s' at %.2f s"), *Command.Command, Elapsed);
			if (GEngine)
			{
				GEngine->Exec(World, *Command.Command);
			}
		}
	}

	while (NextShot < ShotTimes.Num() && Elapsed >= ShotTimes[NextShot])
	{
		const FString File = FPaths::Combine(ShotDir, FString::Printf(TEXT("shot_%02d_%04.1fs.png"), NextShot + 1, ShotTimes[NextShot]));
		FScreenshotRequest::RequestScreenshot(File, false, false);
		UE_LOG(LogTriadDrift, Display, TEXT("TDDevCapture: screenshot %s"), *File);
		++NextShot;
	}

	if (QuitTime >= 0.f && Elapsed >= QuitTime)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("TDDevCapture: quitting at %.2f s"), Elapsed);
		FPlatformMisc::RequestExit(false);
		return false;
	}
	return true;
}
