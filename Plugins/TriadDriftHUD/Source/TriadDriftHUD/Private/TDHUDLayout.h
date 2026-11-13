#pragma once

#include "CoreMinimal.h"

namespace TDHUD
{
// Design-space bounds. Projected targeting geometry stays on the actual target;
// only its text/health stack moves to a clear neighbouring location.
inline bool TargetInfoIsClear(const FVector2D& P,bool bMessageVisible,bool bKillVisible)
{
    const FVector2D Size(196,100);
    if(!FMath::IsFinite(P.X) || !FMath::IsFinite(P.Y) || P.X<110 || P.Y<190 || P.X+Size.X>1810 || P.Y+Size.Y>750) return false;
    auto Overlaps=[&](double X,double Y,double W,double H)
    { return P.X<X+W && P.X+Size.X>X && P.Y<Y+H && P.Y+Size.Y>Y; };
    if(Overlaps(900,480,120,120) || Overlaps(66,724,268,238) || Overlaps(1560,696,294,304)) return false;
    if(bMessageVisible && Overlaps(704,640,512,43)) return false;
    if(bKillVisible && Overlaps(760,584,400,44)) return false;
    return true;
}

inline FVector2D TargetInfoAnchor(const FVector2D& Target,bool bMessageVisible,bool bKillVisible)
{
    const FVector2D Candidates[]={Target+FVector2D(92,-50),Target+FVector2D(-288,-50),Target+FVector2D(-98,-186),Target+FVector2D(-98,92)};
    for(const FVector2D& Candidate:Candidates)
        if(TargetInfoIsClear(Candidate,bMessageVisible,bKillVisible)) return Candidate;
    // A fixed clear lane handles extreme/invalid projections without drawing over
    // the aim point, combat alerts, cards or objective panel.
    return FVector2D(1230,210);
}

inline FString RecoveryLabel(float Seconds)
{
    if(!FMath::IsFinite(Seconds) || Seconds<=0) return TEXT("AWAITING RECOVERY");
    if(Seconds>3600) return TEXT("RECOVERY IN >1 HOUR");
    return FString::Printf(TEXT("RECOVERY IN %d s"),FMath::CeilToInt(Seconds));
}
}
