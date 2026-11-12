#include "TriadDrift/Core/TDDamageTypes.h"

ETDDamageKind UTDDamageType::KindOf(const UDamageType* DamageType)
{
	const UTDDamageType* Typed = Cast<UTDDamageType>(DamageType);
	return Typed ? Typed->Kind : ETDDamageKind::Environmental;
}

TSubclassOf<UDamageType> UTDDamageType::ClassFor(ETDDamageKind Kind)
{
	switch (Kind)
	{
	case ETDDamageKind::Energy: return UTDDamageType_Energy::StaticClass();
	case ETDDamageKind::Kinetic: return UTDDamageType_Kinetic::StaticClass();
	case ETDDamageKind::Explosive: return UTDDamageType_Explosive::StaticClass();
	case ETDDamageKind::Melee: return UTDDamageType_Melee::StaticClass();
	case ETDDamageKind::Ram: return UTDDamageType_Ram::StaticClass();
	default: return UTDDamageType_Environmental::StaticClass();
	}
}
