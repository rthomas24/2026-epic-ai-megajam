// Secondary muzzles (T4 / the Lead: the Rodin interceptor's missiles leave from Missile_0/1 under the wings). Teammate 2.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Muzzles

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDMuzzleTests
{
	/** A transient copy of the engine cube with the named sockets at the given local spots. */
	UStaticMesh* MeshWithSockets(const TArray<TPair<FName, FVector>>& Sockets)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube)
		{
			return nullptr;
		}
		UStaticMesh* Mesh = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage());
		Mesh->Sockets.Reset();
		for (const TPair<FName, FVector>& S : Sockets)
		{
			UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(Mesh);
			Socket->SocketName = S.Key;
			Socket->RelativeLocation = S.Value;
			Mesh->AddSocket(Socket);
		}
		return Mesh;
	}

	UTDWeaponComponent* ShipWith(FTDTestWorld& W, UStaticMesh* Mesh)
	{
		AActor* Ship = W.Spawn<AActor>(FVector::ZeroVector);
		UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Ship, TEXT("Body"));
		Ship->SetRootComponent(Body);
		Body->SetStaticMesh(Mesh);
		Body->RegisterComponent();
		UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Ship, TEXT("Weapons"));
		Weapons->RegisterComponent();
		return Weapons;
	}
}

using namespace TDMuzzleTests;

// With Missile_N sockets the secondary slot leaves from them (all of them, in order) and the primary keeps Muzzle_N;
// without them the secondary shares Muzzle_N exactly as before; prefix None turns it off.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMuzzleSecondarySockets, "TriadDrift.Combat.Muzzles.SecondaryFromMissileSockets", TD_TEST_FLAGS)
bool FTDMuzzleSecondarySockets::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	const FVector Gun0(200.f, -50.f, 0.f), Gun1(200.f, 50.f, 0.f), Mis0(40.f, -120.f, -30.f), Mis1(40.f, 120.f, -30.f);
	const TStrongObjectPtr<UStaticMesh> Both(MeshWithSockets({ { TEXT("Muzzle_0"), Gun0 }, { TEXT("Muzzle_1"), Gun1 }, { TEXT("Missile_0"), Mis0 }, { TEXT("Missile_1"), Mis1 } }));
	const TStrongObjectPtr<UStaticMesh> GunsOnly(MeshWithSockets({ { TEXT("Muzzle_0"), Gun0 }, { TEXT("Muzzle_1"), Gun1 } }));
	if (!TestTrue(TEXT("test meshes"), Both.IsValid() && GunsOnly.IsValid()))
	{
		return false;
	}
	TArray<FTransform> Out;

	UTDWeaponComponent* Rodin = ShipWith(W, Both.Get());
	Rodin->GetMuzzleTransforms(Out, ETDWeaponSlot::Primary);
	TestTrue(TEXT("primary: the two gun muzzles"), Out.Num() == 2 && Out[0].GetLocation().Equals(Gun0, 1.f) && Out[1].GetLocation().Equals(Gun1, 1.f));
	Rodin->GetMuzzleTransforms(Out, ETDWeaponSlot::Secondary);
	TestTrue(TEXT("secondary: Missile_0 and Missile_1, in order"), Out.Num() == 2 && Out[0].GetLocation().Equals(Mis0, 1.f) && Out[1].GetLocation().Equals(Mis1, 1.f));
	Rodin->GetMuzzleTransforms(Out);
	TestTrue(TEXT("the slot-less call is unchanged (guns)"), Out.Num() == 2 && Out[0].GetLocation().Equals(Gun0, 1.f));
	Rodin->SecondaryMuzzlePrefix = NAME_None;
	Rodin->GetMuzzleTransforms(Out, ETDWeaponSlot::Secondary);
	TestTrue(TEXT("prefix None: secondary back on the guns"), Out.Num() == 2 && Out[0].GetLocation().Equals(Gun0, 1.f));

	UTDWeaponComponent* Old = ShipWith(W, GunsOnly.Get());
	Old->GetMuzzleTransforms(Out, ETDWeaponSlot::Secondary);
	TestTrue(TEXT("no Missile_ sockets: secondary shares the guns, as before"), Out.Num() == 2 && Out[0].GetLocation().Equals(Gun0, 1.f));
	return true;
}

#endif
