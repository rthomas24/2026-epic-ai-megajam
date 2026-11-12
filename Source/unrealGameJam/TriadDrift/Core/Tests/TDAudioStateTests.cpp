// Automation tests for the audio live state. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Core.AudioState

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDAudioState.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestActors.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace TDAudioStateTest
{
	FTDShipHUD MakeCard(ETDShipId Ship, bool bPlayer, float Health = 1.f, bool bAlive = true)
	{
		FTDShipHUD Card;
		Card.Ship = Ship;
		Card.bPlayerControlled = bPlayer;
		Card.Health01 = Health;
		Card.bAlive = bAlive;
		return Card;
	}

	/** A squad stand-in and a possessed pawn: what "flying a ship" looks like to the audio state. */
	ATDTestHUDSource* MakeSpaceScene(FTDTestWorld& W)
	{
		ATDTestHUDSource* Source = W.Spawn<ATDTestHUDSource>();
		APawn* Pawn = W.Spawn<APawn>();
		W.SpawnPlayerController(Pawn);
		if (Source)
		{
			Source->Cards = { MakeCard(ETDShipId::Rift, false), MakeCard(ETDShipId::Lancer, true), MakeCard(ETDShipId::Bastion, false) };
		}
		return Source;
	}

	void StartWave(ATDTestHUDSource* Source, float Intensity, int32 Alive, int32 Total)
	{
		Source->Wave.WaveNumber = 3;
		Source->Wave.bWaveActive = true;
		Source->Wave.bInBreather = false;
		Source->Wave.Intensity01 = Intensity;
		Source->Wave.EnemiesAlive = Alive;
		Source->Wave.EnemiesTotal = Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAudioStateFrontEndIsSilent, "TriadDrift.Core.AudioState.FrontEndIsSilent", TD_TEST_FLAGS)
bool FTDAudioStateFrontEndIsSilent::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDAudioStateSubsystem* Audio = UTDAudioStateSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("audio state exists in a game world"), Audio))
	{
		return false;
	}
	W.Advance(1.f);
	const FTDAudioState S = Audio->GetState();
	TestEqual(TEXT("no pawn, no squad: front end"), S.Scene, ETDAudioScene::FrontEnd);
	TestEqual(TEXT("music stays at 0"), S.MusicIntensity01, 0.f);
	TestEqual(TEXT("combat stays at 0"), S.Combat01, 0.f);
	TestEqual(TEXT("no danger"), S.Danger01, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAudioStateReadsThePlayersShip, "TriadDrift.Core.AudioState.ReadsThePlayersShip", TD_TEST_FLAGS)
bool FTDAudioStateReadsThePlayersShip::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDTestHUDSource* Source = TDAudioStateTest::MakeSpaceScene(W);
	UTDAudioStateSubsystem* Audio = UTDAudioStateSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("source"), Source) || !TestNotNull(TEXT("audio state"), Audio))
	{
		return false;
	}
	Source->Speed01 = 0.7f;
	Source->HyperdriveCharge01 = 0.4f;
	Source->MissileWarning = ETDMissileWarning::Tracking;
	Source->Cards[0].bAlive = false;                 // Rift is down
	Source->Cards[1].Health01 = 0.14f;               // the player's Lancer is badly hurt
	Source->Cards[1].Shield01 = 0.f;
	Source->Cards[1].Boost01 = 0.25f;
	Source->Cards[1].Heat01 = 0.9f;

	W.Advance(1.5f);   // includes the half-second search for the squad
	const FTDAudioState S = Audio->GetState();
	TestEqual(TEXT("flying: space"), S.Scene, ETDAudioScene::Space);
	TestEqual(TEXT("the player's ship, not a wingman's"), S.ActiveShip, ETDShipId::Lancer);
	TestEqual(TEXT("speed"), S.Speed01, 0.7f, 0.001f);
	TestEqual(TEXT("boost of the player's ship"), S.Boost01, 0.25f, 0.001f);
	TestEqual(TEXT("heat of the player's ship"), S.Heat01, 0.9f, 0.001f);
	TestEqual(TEXT("hull of the player's ship"), S.Health01, 0.14f, 0.001f);
	TestEqual(TEXT("shield of the player's ship"), S.Shield01, 0.f, 0.001f);
	TestEqual(TEXT("two of three alive"), S.SquadAlive, 2);
	TestEqual(TEXT("hyperdrive charge"), S.HyperdriveCharge01, 0.4f, 0.001f);
	TestEqual(TEXT("missile warning"), S.MissileWarning, ETDMissileWarning::Tracking);
	// 14% hull: (0.35 - 0.14) / 0.35 = 0.6
	TestEqual(TEXT("danger follows low hull"), S.Danger01, 0.6f, 0.02f);

	Source->Cards[1].Health01 = 1.f;
	W.Advance(1.f);
	TestEqual(TEXT("danger clears when repaired"), Audio->GetDanger01(), 0.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAudioStateMusicFollowsTheFight, "TriadDrift.Core.AudioState.MusicFollowsTheFight", TD_TEST_FLAGS)
bool FTDAudioStateMusicFollowsTheFight::RunTest(const FString& Parameters)
{
	// The same sequence at three frame rates must land on the same numbers.
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Step = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("[%d fps] "), FMath::RoundToInt(Fps));
		FTDTestWorld W;
		ATDTestHUDSource* Source = TDAudioStateTest::MakeSpaceScene(W);
		UTDAudioStateSubsystem* Audio = UTDAudioStateSubsystem::Get(W.GetWorld());
		UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
		if (!TestNotNull(*(Tag + TEXT("source")), Source) || !TestNotNull(*(Tag + TEXT("audio state")), Audio) || !TestNotNull(*(Tag + TEXT("bus")), Bus))
		{
			return false;
		}

		// Cruising, no wave.
		W.Advance(3.f, Step);
		TestEqual(*(Tag + TEXT("cruising settles at 0.15")), Audio->GetMusicIntensity01(), 0.15f, 0.01f);
		TestEqual(*(Tag + TEXT("no enemies, no combat")), Audio->GetCombat01(), 0.f, 0.001f);

		// A full wave starts: 12 enemies at top escalation.
		TDAudioStateTest::StartWave(Source, 1.f, 12, 12);
		W.Advance(0.5f, Step);
		const float AfterHalfSecond = Audio->GetMusicIntensity01();
		TestTrue(*(Tag + TEXT("music rises, but no faster than 0.5 per second")), AfterHalfSecond > 0.3f && AfterHalfSecond < 0.15f + 0.27f);
		W.Advance(3.f, Step);
		// Combat target 0.35 + 0.25 = 0.6; music target 0.4 + 0.35 + 0.15 * 0.6 = 0.84.
		TestEqual(*(Tag + TEXT("combat with 12 enemies")), Audio->GetCombat01(), 0.6f, 0.01f);
		TestEqual(*(Tag + TEXT("music in a full wave")), Audio->GetMusicIntensity01(), 0.84f, 0.01f);

		// A kill: the fight reads hotter for a few seconds.
		Bus->BroadcastHitConfirmed(nullptr, true);
		W.Advance(0.5f, Step);
		TestTrue(*(Tag + TEXT("a kill is remembered")), Audio->GetState().SecondsSinceKill < 0.6f);
		TestTrue(*(Tag + TEXT("a kill heats the fight")), Audio->GetCombat01() > 0.7f);

		// Slow motion: reported, and the music still moves in real time.
		UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
		if (TestNotNull(*(Tag + TEXT("feel")), Feel))
		{
			Feel->RequestSlowMo(1.5f, 0.3f);
			Source->Wave.bWaveActive = false;
			Source->Wave.bInBreather = true;
			Source->Wave.EnemiesAlive = 0;
			const float Before = Audio->GetMusicIntensity01();
			W.Advance(1.f, Step);
			const FTDAudioState Slow = Audio->GetState();
			TestTrue(*(Tag + TEXT("slow motion reported")), Slow.bSlowMotion);
			TestTrue(*(Tag + TEXT("time dilation reported")), Slow.TimeDilation < 0.5f);
			TestEqual(*(Tag + TEXT("music falls 0.2 per REAL second in slow motion")), Before - Slow.MusicIntensity01, 0.2f, 0.03f);
			W.Advance(1.f, Step);
			TestFalse(*(Tag + TEXT("slow motion ends")), Audio->IsSlowMotion());
		}

		// Breather settles.
		W.Advance(5.f, Step);
		TestEqual(*(Tag + TEXT("breather settles at 0.25")), Audio->GetMusicIntensity01(), 0.25f, 0.01f);

		// The Dominion assault never drops below 0.8, even between enemy groups.
		Bus->BroadcastPhaseChanged(ETDGamePhase::DominionAssault);
		W.Advance(3.f, Step);
		TestEqual(*(Tag + TEXT("phase read from the bus")), Audio->GetState().Phase, ETDGamePhase::DominionAssault);
		TestTrue(*(Tag + TEXT("Dominion assault floor")), Audio->GetMusicIntensity01() >= 0.79f);

		// "CUT!": the music drops out until the next take starts.
		Bus->BroadcastSceneCut(TEXT("Scene_Test"), 2, 1.5f);
		W.Advance(0.1f, Step);
		TestTrue(*(Tag + TEXT("scene cut reported")), Audio->GetState().bSceneCut);
		TestEqual(*(Tag + TEXT("cut pulls the music target down")), Audio->GetMusicIntensityTarget(), 0.1f, 0.001f);
		Bus->BroadcastTakeStarted(TEXT("Scene_Test"), 2);
		W.Advance(0.1f, Step);
		TestFalse(*(Tag + TEXT("next take clears the cut")), Audio->GetState().bSceneCut);

		// Victory.
		Bus->BroadcastPhaseChanged(ETDGamePhase::Victory);
		W.Advance(4.f, Step);
		TestEqual(*(Tag + TEXT("victory settles at 0.3")), Audio->GetMusicIntensity01(), 0.3f, 0.01f);
	}
	return true;
}

// The placeholder glow material is a contract with every lane (and with the cook): it must exist,
// be unlit, carry a "Color" parameter, and be usable on instanced static meshes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCorePlaceholderGlowMaterial, "TriadDrift.Core.Placeholder.GlowMaterialContract", TD_TEST_FLAGS)
bool FTDCorePlaceholderGlowMaterial::RunTest(const FString& Parameters)
{
	UMaterialInterface* Glow = UTDStatics::GetPlaceholderGlowMaterial();
	if (!TestNotNull(TEXT("M_TD_PlaceholderGlow is loaded through the class default object"), Glow))
	{
		return false;
	}
	TestTrue(TEXT("it lives in the always-cooked gameplay folder"),
		Glow->GetPathName().StartsWith(TEXT("/Game/TriadDrift/Gameplay/")));
	const UMaterial* Base = Glow->GetMaterial();
	if (TestNotNull(TEXT("base material"), Base))
	{
		TestTrue(TEXT("unlit: reads the same from every direction"), Base->GetShadingModels().IsUnlit());
		TestTrue(TEXT("usable on instanced static meshes (stars) in a cooked game"), Base->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}
	FLinearColor Value;
	TestTrue(TEXT("has the Color parameter TintPlaceholder sets"),
		Glow->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Value));

	// TintPlaceholder with a boost must end up on this material, not on the engine's.
	FTDTestWorld W;
	AActor* Actor = W.Spawn<AActor>();
	UStaticMeshComponent* Mesh = Actor ? NewObject<UStaticMeshComponent>(Actor) : nullptr;
	if (TestNotNull(TEXT("mesh component"), Mesh))
	{
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Mesh->RegisterComponent();
		UMaterialInstanceDynamic* Instance = UTDStatics::TintPlaceholder(Mesh, FLinearColor::Red, 6.f);
		if (TestNotNull(TEXT("glow instance"), Instance))
		{
			TestTrue(TEXT("glow tint uses the project glow material"), Instance->GetBaseMaterial() == Glow->GetMaterial());
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
