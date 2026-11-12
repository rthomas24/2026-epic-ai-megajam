// Triad Drift - presentation hooks, front end, cover and input bindings (ENG-21..24 review round). Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/Crew/TDCrewCover.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"

namespace TDPresentationTest
{
	inline ATDCrewCharacter* SpawnCrewWithVisuals(FTDTestWorld& W, ETDCrewType Type, const FVector& At, UTDEnemyVisuals* Visuals)
	{
		const FTransform Xf(FRotator(0.f, 180.f, 0.f), At);
		ATDCrewCharacter* C = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (C)
		{
			C->CrewType = Type;
			C->Visuals = Visuals;
			C->FinishSpawning(Xf);
		}
		return C;
	}
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewCues, "TriadDrift.Ground.Crew.PresentationCuesAndAnimBP", TD_TEST_FLAGS)
bool FTDCrewCues::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(GetTransientPackage());
	V->SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube")));
	V->AnimBPClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Script/Engine.AnimSingleNodeInstance")));
	ATDCrewCharacter* G = TDPresentationTest::SpawnCrewWithVisuals(W, ETDCrewType::Grunt, FVector(250.f, 0.f, 95.f), V);
	if (!TestNotNull(TEXT("grunt"), G)) { return false; }
	TArray<FName> Cues;
	G->OnCueRequested.AddLambda([&](ATDCrewCharacter*, FName N) { Cues.Add(N); });

	TestTrue(TEXT("skeletal mesh applied"), G->GetMesh()->IsVisible() && G->GetMesh()->GetSkeletalMeshAsset() != nullptr);
	TestFalse(TEXT("placeholder hidden"), G->IsPlaceholderVisible());
	TestEqual(TEXT("AnimBP class applied"), G->GetMesh()->GetAnimClass(), (UClass*)UAnimSingleNodeInstance::StaticClass());

	P->GetHealth()->SetInvulnerable(true);
	for (int32 i = 0; i < 60 * 4 && !Cues.Contains(TEXT("Telegraph")); ++i) { W.Tick(1.f / 60.f); }
	TestTrue(TEXT("Telegraph cue on the wind-up"), Cues.Contains(TEXT("Telegraph")));
	G->ApplyStagger(1.2f, P);
	G->ApplyStagger(1.2f, P);
	TestEqual(TEXT("Stagger cue once on entering the stagger state"), Cues.FilterByPredicate([](const FName& N) { return N == TEXT("Stagger"); }).Num(), 1);
	UTDStatics::ApplyTDDamage(G, 10.f, ETDDamageKind::Energy, P, P);
	TestTrue(TEXT("HullHit cue"), Cues.Contains(TEXT("HullHit")));
	G->GetHealth()->Kill(P, ETDDamageKind::Melee);
	TestTrue(TEXT("Death cue"), Cues.Contains(TEXT("Death")));

	ATDCrewCharacter* H = TDPresentationTest::SpawnCrewWithVisuals(W, ETDCrewType::Heavy, FVector(150.f, 400.f, 110.f), nullptr);
	TArray<FName> HeavyCues;
	H->OnCueRequested.AddLambda([&](ATDCrewCharacter*, FName N) { HeavyCues.Add(N); });
	H->SetActorRotation((P->GetActorLocation() - H->GetActorLocation()).Rotation());
	UTDStatics::ApplyTDDamage(H, 25.f, ETDDamageKind::Melee, P, P, H->GetActorLocation(), (H->GetActorLocation() - P->GetActorLocation()).GetSafeNormal());
	TestTrue(TEXT("ShieldHit cue when the shield takes a slash"), HeavyCues.Contains(TEXT("ShieldHit")));
	TestEqual(TEXT("shield took it"), TDGroundTest::Hull(H), 200.f, 0.1f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBladeCues, "TriadDrift.Ground.Blade.PresentationCuesMeshAndRetract", TD_TEST_FLAGS)
bool FTDBladeCues::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	ATDCrewDummy* D = TDGroundTest::SpawnDummy(W, FVector(130.f, 0.f, 95.f));
	UTDEnergyBladeComponent* B = P ? P->GetBlade() : nullptr;
	if (!TestNotNull(TEXT("blade"), B)) { return false; }
	TestFalse(TEXT("placeholder blade until art"), P->IsBladeMeshFromArt());

	UTDBladeFX* FX = NewObject<UTDBladeFX>(GetTransientPackage());
	FX->BladeMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	FX->BladeColor = FLinearColor(1.f, 0.f, 1.f);
	B->BladeFX = FX;
	P->ApplyBladeVisuals();
	TestTrue(TEXT("BladeMesh replaces the placeholder"), P->IsBladeMeshFromArt());
	TestEqual(TEXT("blade mesh"), P->GetBladeMesh()->GetStaticMesh()->GetName(), FString(TEXT("Sphere")));

	TArray<FName> Cues;
	B->OnCueRequested.AddLambda([&](FName N) { Cues.Add(N); });
	B->LightAttack();
	TestTrue(TEXT("SwingLight cue"), Cues.Contains(TEXT("SwingLight")));
	TestTrue(TEXT("Hit cue"), Cues.Contains(TEXT("Hit")));
	W.Advance(1.f);
	B->SetBlockHeld(true);
	W.Advance(0.4f);
	D->MeleeAttack(P);
	TestTrue(TEXT("Block cue"), Cues.Contains(TEXT("Block")));
	W.Advance(3.f);
	TestTrue(TEXT("GuardBreak cue"), Cues.Contains(TEXT("GuardBreak")));
	B->SetBlockHeld(false);

	UTDStatics::ApplyTDDamage(P, 500.f, ETDDamageKind::Energy, D, D);
	TestTrue(TEXT("Retract cue on death"), Cues.Contains(TEXT("Retract")));
	TestTrue(TEXT("blade put away"), B->IsRetracted());
	TestFalse(TEXT("blade mesh hidden"), P->GetBladeMesh()->IsVisible());
	TestFalse(TEXT("no swings while retracted"), B->LightAttack());
	P->RespawnAt(FTransform(FVector(0.f, 0.f, 95.f)));
	TestTrue(TEXT("Ignite cue on respawn"), Cues.Contains(TEXT("Ignite")));
	TestTrue(TEXT("blade mesh visible again"), P->GetBladeMesh()->IsVisible());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFrontEndWidgetsTest, "TriadDrift.Flow.FrontEndWidgetsFromStyle", TD_TEST_FLAGS)
bool FTDFrontEndWidgetsTest::RunTest(const FString& Parameters)
{
	using EKind = FTDFrontEndWidgets::EKind;
	UClass* WidgetClass = UUserWidget::StaticClass();
	UTDUIStyle* Style = NewObject<UTDUIStyle>(GetTransientPackage());
	TestNull(TEXT("nothing when override and style are both empty"), FTDFrontEndWidgets::Resolve(nullptr, Style, EKind::MainMenu));
	TestNull(TEXT("nothing with no style at all"), FTDFrontEndWidgets::Resolve(nullptr, nullptr, EKind::Loading));
	Style->MainMenuWidgetClass = TSoftClassPtr<UUserWidget>(WidgetClass);
	TestEqual(TEXT("style's menu widget when no override"), FTDFrontEndWidgets::Resolve(nullptr, Style, EKind::MainMenu), WidgetClass);
	TestNull(TEXT("loading stays empty"), FTDFrontEndWidgets::Resolve(nullptr, Style, EKind::Loading));
	Style->LoadingWidgetClass = TSoftClassPtr<UUserWidget>(WidgetClass);
	TestEqual(TEXT("style's loading widget"), FTDFrontEndWidgets::Resolve(nullptr, Style, EKind::Loading), WidgetClass);
	UClass* Override = UUserWidget::StaticClass();
	TestEqual(TEXT("override wins"), FTDFrontEndWidgets::Resolve(Override, nullptr, EKind::MainMenu), Override);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewCover, "TriadDrift.Ground.Crew.TrooperUsesCoverPeeksAndRelocates", TD_TEST_FLAGS)
bool FTDCrewCover::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-900.f, 0.f, 95.f));
	AActor* Block = TDGroundTest::SpawnBox(W, FVector(200.f, 0.f, 100.f), FVector(200.f, 300.f, 200.f));
	UTDCrewCoverSubsystem* Cover = UTDCrewCoverSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("player"), P) || !TestNotNull(TEXT("cover"), Cover)) { return false; }
	Cover->RegisterCoverActor(Block);
	TestEqual(TEXT("4 cover points per block"), Cover->Num(), 4);
	P->GetHealth()->SetInvulnerable(true);

	const FTransform Xf(FRotator(0.f, 180.f, 0.f), FVector(500.f, 700.f, 95.f));
	ATDCrewCharacter* Tr = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Tr->CrewType = ETDCrewType::Trooper;
	Tr->FinishSpawning(Xf);
	ATDCrewAIController* AI = Cast<ATDCrewAIController>(Tr->GetController());
	if (!TestNotNull(TEXT("trooper AI"), AI)) { return false; }
	int32 Bursts = 0;
	bool bPeeked = false;
	Tr->OnAttackReleased.AddLambda([&](ATDCrewCharacter*, ETDCrewAttack A) { Bursts += (A == ETDCrewAttack::Burst) ? 1 : 0; });

	bool bReachedCover = false;
	for (int32 i = 0; i < 60 * 12; ++i)
	{
		W.Tick(1.f / 60.f);
		bReachedCover |= AI->IsInCover() && !AI->IsPeeking();
		bPeeked |= AI->IsPeeking();
	}
	TestTrue(TEXT("trooper reached cover"), bReachedCover);
	const FTDCoverPoint* Spot = Cover->GetPoint(AI->GetCoverIndex());
	TestTrue(TEXT("its spot is behind the block from the player (+X face)"), Spot && Spot->Normal.X > 0.5f);
	TestTrue(TEXT("peeked out"), bPeeked);
	TestTrue(TEXT("fired from cover"), Bursts >= 1);

	// Flank it: walk the player round to the trooper's side of the block.
	const int32 Before = AI->GetCoverRelocations();
	P->SetActorLocation(FVector(1300.f, 0.f, 95.f));
	W.Advance(1.f);
	TestTrue(TEXT("flanked -> relocates"), AI->GetCoverRelocations() > Before);

	// Destroy the cover: the trooper must not keep hiding behind nothing.
	W.Advance(4.f);
	const int32 Before2 = AI->GetCoverRelocations();
	const bool bWasInCover = AI->GetCoverIndex() != INDEX_NONE;
	Block->Destroy();
	W.Advance(1.f);
	TestTrue(TEXT("cover destroyed -> leaves it"), AI->GetCoverIndex() == INDEX_NONE);
	TestTrue(TEXT("counted as a relocation if it was in cover"), !bWasInCover || AI->GetCoverRelocations() > Before2);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundBindings, "TriadDrift.Ground.InputBindingsForEveryAction", TD_TEST_FLAGS)
bool FTDGroundBindings::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* C = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	if (!TestNotNull(TEXT("character"), C)) { return false; }
	UEnhancedInputComponent* IC = NewObject<UEnhancedInputComponent>(C);
	C->SetupPlayerInputComponent(IC);
	const UInputMappingContext* Ctx = C->GetGroundInputContext();
	if (!TestNotNull(TEXT("mapping context built in C++"), Ctx)) { return false; }

	struct FExpect { const TCHAR* Action; FKey Key; };
	const FExpect Expected[] = {
		{ TEXT("IA_Ground_Move"), EKeys::W }, { TEXT("IA_Ground_Move"), EKeys::A }, { TEXT("IA_Ground_Move"), EKeys::S }, { TEXT("IA_Ground_Move"), EKeys::D },
		{ TEXT("IA_Ground_Look"), EKeys::Mouse2D }, { TEXT("IA_Ground_Jump"), EKeys::SpaceBar }, { TEXT("IA_Ground_Sprint"), EKeys::LeftShift },
		{ TEXT("IA_Ground_Dash"), EKeys::Q }, { TEXT("IA_Ground_Crouch"), EKeys::LeftControl }, { TEXT("IA_Ground_Attack"), EKeys::LeftMouseButton },
		{ TEXT("IA_Ground_Block"), EKeys::RightMouseButton }, { TEXT("IA_Ground_Interact"), EKeys::E }, { TEXT("IA_Ground_Ping"), EKeys::Tab },
		{ TEXT("IA_Ground_Jump"), EKeys::Gamepad_FaceButton_Bottom }, { TEXT("IA_Ground_Attack"), EKeys::Gamepad_RightTrigger },
	};
	for (const FExpect& E : Expected)
	{
		const FEnhancedActionKeyMapping* Found = Ctx->GetMappings().FindByPredicate([&](const FEnhancedActionKeyMapping& M)
		{
			return M.Action && M.Action->GetFName() == FName(E.Action) && M.Key == E.Key;
		});
		TestNotNull(FString::Printf(TEXT("%s mapped to %s"), E.Action, *E.Key.ToString()), Found);
	}
	// Every action in the context has at least one event binding on the input component.
	TSet<const UInputAction*> Bound;
	for (const TUniquePtr<FEnhancedInputActionEventBinding>& B : IC->GetActionEventBindings())
	{
		Bound.Add(B->GetAction());
	}
	TSet<const UInputAction*> Mapped;
	for (const FEnhancedActionKeyMapping& M : Ctx->GetMappings())
	{
		Mapped.Add(M.Action);
	}
	TestEqual(TEXT("10 ground actions"), Mapped.Num(), 10);
	for (const UInputAction* A : Mapped)
	{
		TestTrue(FString::Printf(TEXT("%s is bound to a handler"), *GetNameSafe(A)), Bound.Contains(A));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS