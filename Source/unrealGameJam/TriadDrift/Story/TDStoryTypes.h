// Triad Drift - story transmissions: the data (storyline.md, "The Paradox"). Owner: Teammate 3.
//
// Every story beat is a transmission with an id. Gameplay fires the id; the words, voice and timing come from this
// table. The C++ defaults are plain placeholder lines (they state the beat and never reveal the twist before act 4);
// Nicholas's Presentation asset DA_Story_Transmissions (a UTDTransmissionSet) replaces any entry by id, field by
// field: a line without a Sound keeps the placeholder's empty sound, a Sound with no Text keeps the placeholder text.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TDStoryTypes.generated.h"

class USoundBase;

/** Who speaks. The Future* voices are heard only from act 4 / the finale (storyline.md §4). */
UENUM(BlueprintType)
enum class ETDSpeaker : uint8
{
	Control,
	Wren,
	Kato,
	Moss,
	Unknown,
	FutureWren,
	FutureKato,
	FutureMoss,
	Command,
	PatrolSeven,
	/** The pilot on foot aboard the Dominion: whoever flew the ship that docked (Nicholas, issue #29). Resolved when the
	 *  line goes on air (UTDStorySubsystem::ResolveSpeaker); the two wingmen speak from outside. */
	Boarder
};

/** How the line is presented (and how its audio should be treated). */
UENUM(BlueprintType)
enum class ETDTransmissionStyle : uint8
{
	Clean,	// squad radio, Control
	Static,	// a broken or intercepted signal
	Echo	// a voice that should not be there
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTransmission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	ETDSpeaker Speaker = ETDSpeaker::Control;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (MultiLine = "true"))
	FText Text;

	/** Optional voice line (2D). Without it the line is subtitle-only and the bark hook still fires (OnBarkRequested). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	TSoftObjectPtr<USoundBase> Sound;

	/**
	 * A sound shared by several lines, by key in UTDTransmissionSet::SharedSounds (used when Sound is unset). The
	 * opening's "Don't-" and the finale's must be the identical processed audio (First Contact brief): both use "Dont".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	FName SharedSound;

	/** Seconds on screen. 0 = from the text's length (2.5 to 7 s), or the sound's duration when longer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	ETDTransmissionStyle Style = ETDTransmissionStyle::Clean;

	/** Plays every time it is fired (default: once per run). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	bool bRepeatable = false;
};

/** Nicholas's words and voices: DA_Story_Transmissions in the Presentation folder. Entries replace the defaults by id. */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDTransmissionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmissions", meta = (TitleProperty = "Id"))
	TArray<FTDTransmission> Entries;

	/** Sounds several lines share, by key (FTDTransmission::SharedSound): "Dont" = the one processed "Don't-" fragment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmissions")
	TMap<FName, TSoftObjectPtr<USoundBase>> SharedSounds;
};

/** What the HUD draws for the transmission on air (all real time). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTransmissionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	ETDSpeaker Speaker = ETDSpeaker::Control;

	/** "WREN", "CONTROL", "UNKNOWN SIGNAL" ... */
	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	FText SpeakerTag;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	ETDTransmissionStyle Style = ETDTransmissionStyle::Clean;

	/** The speaker's colour (squad colours; white for Control, red-white for Unknown). */
	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	FLinearColor Color = FLinearColor::White;

	/** Fade in / out (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	float Alpha = 0.f;

	/** ENG-45: false when the player turned subtitles off. A HUD that draws transmissions draws no text then; the voice,
	 *  the timing and the queue are unchanged. */
	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	bool bShowSubtitle = true;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	float Progress01 = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Transmission")
	float TimeLeft = 0.f;
};
