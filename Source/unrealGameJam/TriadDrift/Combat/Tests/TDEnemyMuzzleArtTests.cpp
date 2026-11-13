// Enemy shots leave the Null Armada art's Muzzle_N barrels (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Muzzle

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Tests/TDTestWorld.h"

// Live chase-camera stills could not show the barrels (the AI ships move too fast at gameplay range), so this checks the
// real thing directly: each art hull exposes Muzzle_0..N sockets and the weapon component fires from exactly those
// sockets (not its fallback nose offsets).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyMuzzlesFromArt, "TriadDrift.Combat.Muzzle.EnemyShotsLeaveTheArtBarrels", TD_TEST_FLAGS)
bool FTDEnemyMuzzlesFromArt::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	// The Hive Carrier is not here: its only attack launches fighters and its guns are separate ATDEnemyTurret actors, so
	// its hull fires nothing and carries no Muzzle_N sockets.
	const FName Ids[] = { TDEnemyIds::ShardFighter, TDEnemyIds::HookInterceptor, TDEnemyIds::WallGunship, TDEnemyIds::MaulBomber };
	for (const FName Id : Ids)
	{
		ATDEnemyShip* Ship = W.Spawn<ATDEnemyShip>(FVector(0.f, 0.f, 0.f), FRotator(0.f, 30.f, 0.f));
		if (!TestNotNull(Id.ToString(), Ship))
		{
			continue;
		}
		Ship->InitFromSpec(Id);
		W.Tick();
		// The hull with sockets (the art mesh), if any.
		const UStaticMeshComponent* Art = nullptr;
		int32 Sockets = 0;
		TInlineComponentArray<UStaticMeshComponent*> Meshes(Ship);
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->DoesSocketExist(TEXT("Muzzle_0")))
			{
				Art = Mesh;
				while (Sockets < 8 && Mesh->DoesSocketExist(FName(*FString::Printf(TEXT("Muzzle_%d"), Sockets))))
				{
					++Sockets;
				}
				break;
			}
		}
		const UTDWeaponComponent* Weapons = Ship->FindComponentByClass<UTDWeaponComponent>();
		if (!TestNotNull(Id.ToString() + TEXT(": weapon component"), Weapons))
		{
			continue;
		}
		TArray<FTransform> Muzzles;
		Weapons->GetMuzzleTransforms(Muzzles);
		AddInfo(FString::Printf(TEXT("%s: art %s, %d Muzzle_N sockets, %d muzzles fired from"), *Id.ToString(),
			Art ? *GetNameSafe(Art->GetStaticMesh()) : TEXT("none"), Sockets, Muzzles.Num()));
		if (!TestNotNull(Id.ToString() + TEXT(": the art hull has Muzzle_N sockets"), Art))
		{
			continue;
		}
		TestEqual(Id.ToString() + TEXT(": one muzzle per socket"), Muzzles.Num(), Sockets);
		for (int32 I = 0; I < FMath::Min(Muzzles.Num(), Sockets); ++I)
		{
			const FVector Socket = Art->GetSocketLocation(FName(*FString::Printf(TEXT("Muzzle_%d"), I)));
			TestTrue(FString::Printf(TEXT("%s: muzzle %d is the Muzzle_%d socket (%.1f cm off)"), *Id.ToString(), I, I, FVector::Dist(Socket, Muzzles[I].GetLocation())),
				Muzzles[I].GetLocation().Equals(Socket, 0.5f));
		}
		Ship->Destroy();
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
