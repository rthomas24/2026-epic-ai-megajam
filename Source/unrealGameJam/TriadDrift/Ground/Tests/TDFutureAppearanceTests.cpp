#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Ground/Crew/TDFutureCrew.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFutureAppearanceIsolation,
	"TriadDrift.Ground.FutureAppearance.SharedPilotMaterialIsolation", TD_TEST_FLAGS)
bool FTDFutureAppearanceIsolation::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Owner = W.Spawn<AActor>();
	USkeletalMeshComponent* OrdinaryPilot = NewObject<USkeletalMeshComponent>(Owner);
	if (!TestNotNull(TEXT("actual pilot visuals available"), TDFutureCrew::ApplyPilotLook(OrdinaryPilot, 90.f))) { return false; }
	UMaterialInterface* Shared = OrdinaryPilot->GetMaterial(0);
	FLinearColor OriginalTint, OriginalAccent;
	if (!Shared || !TestTrue(TEXT("authored suit tint exists"), Shared->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitTint")), OriginalTint))) { return false; }
	Shared->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitAccent")), OriginalAccent);
	UMaterialInterface* PreviousFuture = nullptr;
	for (FName Pilot : { FName(TEXT("Wren")), FName(TEXT("Kato")), FName(TEXT("Moss")) })
	{
		USkeletalMeshComponent* Future = NewObject<USkeletalMeshComponent>(Owner);
		TDFutureCrew::ApplyPilotLook(Future, 90.f);
		TDFutureCrew::ApplyFuturePalette(Future, Pilot);
		UMaterialInterface* Suit = Future->GetMaterial(0);
		TestNotNull(TEXT("future uses a dynamic instance"), Cast<UMaterialInstanceDynamic>(Suit));
		TestTrue(TEXT("future material isolated from ordinary pilot and other future"), Suit != Shared && Suit != PreviousFuture);
		FLinearColor Tint, Accent, SharedTint, SharedAccent;
		Suit->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitTint")), Tint);
		Suit->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitAccent")), Accent);
		TestTrue(TEXT("armor is dark neutral"), Tint.R < .04f && Tint.G < .04f && Tint.B < .04f);
		TestTrue(TEXT("visor retains this pilot role"), Accent.Equals(TDFutureCrew::ColorOf(Pilot)));
		Shared->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitTint")), SharedTint);
		Shared->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SuitAccent")), SharedAccent);
		TestTrue(TEXT("shared player armor unchanged"), SharedTint.Equals(OriginalTint));
		TestTrue(TEXT("shared player visor unchanged"), SharedAccent.Equals(OriginalAccent));
		TestTrue(TEXT("ordinary component still uses shared source"), OrdinaryPilot->GetMaterial(0) == Shared);
		PreviousFuture = Suit;
	}
	return true;
}
#endif
