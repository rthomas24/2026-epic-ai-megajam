// #211: close survey visuals independently of dialogue, without also triggering a gameplay action.
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/UI/TDScanMonitorWidget.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScanDismissStages, "TriadDrift.UI.ScanMonitor.Dismiss.KeyboardSuppressesStagesUntilSequenceEnd", TD_TEST_FLAGS)
bool FTDScanDismissStages::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("host"), Host)) { return false; }
	Host->bAddToViewport = false;
	TestFalse(TEXT("no scan to dismiss yet"), UTDUIHostSubsystem::DismissScanMonitor(W.GetWorld()));
	TestTrue(TEXT("Verdance scan shown"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 1));
	UTDScanMonitorWidget* Monitor = Cast<UTDScanMonitorWidget>(Host->GetScanMonitor());
	if (!TestNotNull(TEXT("monitor"), Monitor)) { return false; }
	W.Tick(); W.Tick();
	TestEqual(TEXT("keyboard close hint"), Monitor->GetDismissKey(), FString(TEXT("X")));
	FTDTestWorld::InjectKey(PC, EKeys::X, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::X, false);
	W.Tick();
	TestTrue(TEXT("real X input starts visual dismissal"), Monitor->IsHiding());
	TestFalse(TEXT("a later dialogue stage cannot reopen the scan"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 2));
	TestFalse(TEXT("repeated close is harmless"), UTDUIHostSubsystem::DismissScanMonitor(W.GetWorld()));
	Monitor->Advance(UTDScanMonitorWidget::FadeSeconds);
	Host->Tick(0.f);
	TestNull(TEXT("closed visual removed"), Host->GetScanMonitor());
	TestFalse(TEXT("dismissal survives widget removal"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 3));

	// The existing story end calls HideScanMonitor even after the widget has faded out.
	UTDUIHostSubsystem::HideScanMonitor(W.GetWorld());
	TestTrue(TEXT("next Cinder scan can open"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 4));
	TestTrue(TEXT("Cinder visual can also be dismissed"), UTDUIHostSubsystem::DismissScanMonitor(W.GetWorld()));
	TestFalse(TEXT("Cinder's later map stage stays closed"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 5));
	UTDUIHostSubsystem::HideScanMonitor(W.GetWorld());
	TestTrue(TEXT("programmatic sequence end resets the next display"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScanDismissPad, "TriadDrift.UI.ScanMonitor.Dismiss.ControllerCloseConsumesBAndRestoresDodge", TD_TEST_FLAGS)
bool FTDScanDismissPad::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Pilot = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 200.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Pilot, ATDSpacePlayerController::StaticClass()));
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("pilot"), Pilot) || !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("host"), Host)) { return false; }
	Host->bAddToViewport = false;
	W.Tick(); W.Tick();
	TestTrue(TEXT("Cinder scan shown"), UTDUIHostSubsystem::ShowScanMonitor(W.GetWorld(), 4));
	UTDScanMonitorWidget* Monitor = Cast<UTDScanMonitorWidget>(Host->GetScanMonitor());
	if (!TestNotNull(TEXT("monitor"), Monitor)) { return false; }
	W.Tick(); W.Tick();
	FTDTestWorld::InjectAxis(PC, EKeys::Gamepad_LeftX, .8f);
	W.Tick();
	FTDTestWorld::InjectAxis(PC, EKeys::Gamepad_LeftX, 0.f);
	TestEqual(TEXT("controller close hint follows actual input"), Monitor->GetDismissKey(), FString(TEXT("B")));
	const int32 ChargesBefore = Pilot->GetDashCharges();
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Right, true);
	W.Tick();
	TestTrue(TEXT("B closes the scan"), Monitor->IsHiding());
	TestEqual(TEXT("B close does not spend a dodge charge"), Pilot->GetDashCharges(), ChargesBefore);
	TestFalse(TEXT("B close does not also start a dodge"), Pilot->IsDashing());
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Right, false);
	W.Tick();
	Monitor->Advance(UTDScanMonitorWidget::FadeSeconds);
	Host->Tick(0.f);
	W.Tick(); W.Tick();
	TestNull(TEXT("dismissed scan gone"), Host->GetScanMonitor());
	// Positive control: after the scan closes its temporary mapping must no longer intercept B.
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Right, true);
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::Gamepad_FaceButton_Right, false);
	TestTrue(TEXT("B restores its real gameplay dodge after closing"), Pilot->GetDashCharges() < ChargesBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScanDismissStory, "TriadDrift.UI.ScanMonitor.Dismiss.DialogueContinuesAndNextSequenceResets", TD_TEST_FLAGS)
bool FTDScanDismissStory::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("host"), Host) || !TestNotNull(TEXT("story"), Story)) { return false; }
	Host->bAddToViewport = false;
	TestTrue(TEXT("actual Verdance sequence starts"), UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic03")));
	for (int32 Step = 0; Step < 300 && !Host->GetScanMonitor(); ++Step) { W.Advance(.1f); }
	UTDScanMonitorWidget* Monitor = Cast<UTDScanMonitorWidget>(Host->GetScanMonitor());
	if (!TestNotNull(TEXT("story line naturally shows the scan"), Monitor)) { return false; }
	const FTDTransmissionView Before = Story->GetCurrentTransmission();
	TestTrue(TEXT("dialogue is active with subtitle text"), Before.bActive && !Before.Text.IsEmpty());
	TestTrue(TEXT("manual visual dismissal"), UTDUIHostSubsystem::DismissScanMonitor(W.GetWorld()));
	TestTrue(TEXT("story sequence is still running"), Story->IsSequencePlaying(TEXT("Cinematic03")));
	TestEqual(TEXT("current spoken/subtitled line was not skipped"), Story->GetCurrentTransmission().Id, Before.Id);
	TestEqual(TEXT("subtitle text was not removed"), Story->GetCurrentTransmission().Text.ToString(), Before.Text.ToString());
	Monitor->Advance(UTDScanMonitorWidget::FadeSeconds);
	Host->Tick(0.f);
	bool bStayedClosed = Host->GetScanMonitor() == nullptr;
	for (int32 Step = 0; Step < 1200 && Story->IsSequencePlaying(TEXT("Cinematic03")); ++Step)
	{
		W.Advance(.1f);
		bStayedClosed &= Host->GetScanMonitor() == nullptr;
	}
	TestTrue(TEXT("all remaining dialogue stages stay visually dismissed"), bStayedClosed);
	TestFalse(TEXT("dialogue eventually completes normally"), Story->IsSequencePlaying(TEXT("Cinematic03")));
	TestTrue(TEXT("actual next Cinder sequence starts"), UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic05")));
	for (int32 Step = 0; Step < 300 && !Host->GetScanMonitor(); ++Step) { W.Advance(.1f); }
	UTDScanMonitorWidget* Cinder = Cast<UTDScanMonitorWidget>(Host->GetScanMonitor());
	TestTrue(TEXT("sequence-end cleanup lets the Cinder scan appear"), Cinder && Cinder->GetStage() >= 4);
	return true;
}
#endif
