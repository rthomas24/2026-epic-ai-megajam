// Triad Drift - destructible space field contract (Teammate 2). See header.

#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "GameFramework/Actor.h"

FTDFieldDamage::FOnFieldHit FTDFieldDamage::OnPlayerFieldHit;

namespace TDFieldDamagePrivate
{
	static TArray<TWeakObjectPtr<AActor>> Fields;
}

void FTDFieldDamage::Register(AActor* Field)
{
	if (Field && Field->Implements<UTDFieldDamageable>())
	{
		TDFieldDamagePrivate::Fields.AddUnique(Field);
	}
}

void FTDFieldDamage::Unregister(AActor* Field)
{
	TDFieldDamagePrivate::Fields.Remove(Field);
}

void FTDFieldDamage::GetFields(const UWorld* World, TArray<AActor*>& Out)
{
	Out.Reset();
	TDFieldDamagePrivate::Fields.RemoveAll([](const TWeakObjectPtr<AActor>& F) { return !F.IsValid(); });
	for (const TWeakObjectPtr<AActor>& F : TDFieldDamagePrivate::Fields)
	{
		if (F->GetWorld() == World)
		{
			Out.Add(F.Get());
		}
	}
}
