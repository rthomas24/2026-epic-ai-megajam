import os
os.chdir(r'C:\Users\ryant\Documents\myRepos\game-jam\.wt\t3\Source\unrealGameJam\TriadDrift\Director')
NL = chr(10)
CR = chr(13) + chr(10)


def patch(p, pairs):
    s = open(p, encoding='utf-8', newline='').read()
    crlf = CR in s
    for a, b in pairs:
        if crlf:
            a = a.replace(NL, CR)
            b = b.replace(NL, CR)
        assert s.count(a) == 1, (p, a[:70], s.count(a))
        s = s.replace(a, b)
    open(p, 'w', encoding='utf-8', newline='').write(s)


patch('TDDirectorSubsystem.h', [('''	FAtmosphereShot Atmosphere;
''', '''	FAtmosphereShot Atmosphere;
	/** #111: the escape's look-back at the Dominion's final blast (once per Dominion; real-time end). */
	TWeakObjectPtr<class ATDDominion> WatchedDominion;
	bool bEscapeLookBackDone = false;
	double EscapeLookBackEnd = -1.0;
	void UpdateEscapeLookBack(double Now);
public:
	/** Seconds of the escape look-back shot (#111) and its time scale (the reactor shots' slow motion). */
	float EscapeLookBackSeconds = 2.8f;
	float EscapeLookBackDilation = 0.3f;
private:
''')])

patch('TDDirectorSubsystem.cpp', [
('''	LastTickRealTime = Now;
	EnsureDirectorWidget();''', '''	LastTickRealTime = Now;
	UpdateEscapeLookBack(Now);
	EnsureDirectorWidget();'''),
('''bool UTDDirectorSubsystem::IsAtmosphereShotActive(const UObject* WorldContext)
{''', '''void UTDDirectorSubsystem::UpdateEscapeLookBack(double Now)
{
	// #111 (Nick's Session48): flying the escape straight out, the player never saw the Dominion go up. When the escape
	// is clear, the camera turns round for EscapeLookBackSeconds: ahead of the flown ship, looking back past it at the
	// final blast, in slow motion; then the chase camera again. Only on a clear escape (a timed-out take keeps its CUT);
	// input is never blocked (the ship flies on under the shot); the ClearSpace lines play over it.
	if (EscapeLookBackEnd > 0.0)
	{
		if (Now >= EscapeLookBackEnd)
		{
			EscapeLookBackEnd = -1.0;
			EndAtmosphereShot(this, true);
			UE_LOG(LogTriadDrift, Log, TEXT("Director: escape look-back ends"));
		}
		return;
	}
	if (bEscapeLookBackDone)
	{
		return;
	}
	ATDDominion* Dom = WatchedDominion.Get();
	if (!Dom)
	{
		Dom = ATDDominion::Get(this);
		WatchedDominion = Dom;
	}
	if (!Dom || !Dom->IsEscapeClear())
	{
		return;
	}
	bEscapeLookBackDone = true;
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	ATDShip* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	if (!Ship)
	{
		return;
	}
	const TWeakObjectPtr<ATDShip> WeakShip(Ship);
	const FVector Blast = Dom->GetActorLocation();
	const bool bStarted = RequestAtmosphereShot(this, Ship, /*bExit*/ true, EscapeLookBackSeconds, [WeakShip, Blast](float Alpha)
	{
		const ATDShip* S = WeakShip.Get();
		if (!S)
		{
			return FTransform::Identity;
		}
		// Ahead of the ship, a little above and to the side, drifting back over the shot; looking past the ship at the blast.
		const FTransform ShipT = S->GetActorTransform();
		const FVector Fwd = ShipT.GetUnitAxis(EAxis::X);
		const FVector Up = ShipT.GetUnitAxis(EAxis::Z);
		const FVector Right = ShipT.GetUnitAxis(EAxis::Y);
		const FVector Cam = ShipT.GetLocation() + Fwd * FMath::Lerp(2600.f, 1800.f, Alpha) + Up * 520.f + Right * 420.f;
		const FVector Look = FMath::Lerp(ShipT.GetLocation(), Blast, 0.08f); // the ship in the foreground, the blast beyond
		return FTransform((Look - Cam).Rotation(), Cam).GetRelativeTransform(ShipT);
	});
	if (bStarted)
	{
		EscapeLookBackEnd = Now + EscapeLookBackSeconds;
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(GetWorld()))
		{
			Feel->RequestSlowMo(EscapeLookBackSeconds, EscapeLookBackDilation);
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("Director: escape look-back at the final blast %s (%.1f s at %.2fx)"), bStarted ? TEXT("starts") : TEXT("refused"),
		EscapeLookBackSeconds, EscapeLookBackDilation);
}

bool UTDDirectorSubsystem::IsAtmosphereShotActive(const UObject* WorldContext)
{'''),
])
s = open('TDDirectorSubsystem.cpp', encoding='utf-8', newline='').read()
nl = CR if CR in s else NL
for inc in ('#include "TriadDrift/World/TDDominion.h"', '#include "TriadDrift/Squad/TDSquadManager.h"', '#include "TriadDrift/Flight/TDShip.h"'):
    if inc not in s:
        s = s.replace('#include "TriadDrift/Core/TDStatics.h"', '#include "TriadDrift/Core/TDStatics.h"' + nl + inc, 1)
open('TDDirectorSubsystem.cpp', 'w', encoding='utf-8', newline='').write(s)
print('ok')
