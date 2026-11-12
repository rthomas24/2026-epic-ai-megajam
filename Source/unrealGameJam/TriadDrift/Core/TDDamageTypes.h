// Triad Drift - one UDamageType per ETDDamageKind, for damage that arrives through the engine's
// ApplyDamage path. Gameplay code normally uses UTDStatics::ApplyTDDamage instead. Lead-owned.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDDamageTypes.generated.h"

UCLASS(Abstract)
class UNREALGAMEJAM_API UTDDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "TriadDrift|Damage")
	ETDDamageKind Kind = ETDDamageKind::Energy;

	/** Kind of an arbitrary damage type; non-TD damage types count as Environmental. */
	static ETDDamageKind KindOf(const UDamageType* DamageType);

	static TSubclassOf<UDamageType> ClassFor(ETDDamageKind Kind);
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Energy : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Energy() { Kind = ETDDamageKind::Energy; }
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Kinetic : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Kinetic() { Kind = ETDDamageKind::Kinetic; }
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Explosive : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Explosive() { Kind = ETDDamageKind::Explosive; }
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Melee : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Melee() { Kind = ETDDamageKind::Melee; }
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Ram : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Ram() { Kind = ETDDamageKind::Ram; }
};

UCLASS()
class UNREALGAMEJAM_API UTDDamageType_Environmental : public UTDDamageType
{
	GENERATED_BODY()
public:
	UTDDamageType_Environmental() { Kind = ETDDamageKind::Environmental; }
};
