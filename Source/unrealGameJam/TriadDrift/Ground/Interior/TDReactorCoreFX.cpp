// Triad Drift - the reactor fight's and the escape's effect slots. Owner: Teammate 5.

#include "TriadDrift/Ground/Interior/TDReactorCoreFX.h"

#include "TriadDrift/Core/TDStatics.h"

const UTDReactorCoreFX* UTDReactorCoreFX::Get()
{
	return UTDStatics::LoadPresentation<UTDReactorCoreFX>(TEXT("DA_ReactorCore_FX"));
}
