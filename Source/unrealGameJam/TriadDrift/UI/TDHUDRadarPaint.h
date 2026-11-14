// Triad Drift - the flight HUD's radar (issue #160, Concept A: docs/eng/hud-concept/HUD-CONCEPT.md section 7; Teammate 6).
//
// One free function painted from FTDHUDModel inside UTDFlightHUDWidget::NativePaint (T2 adds the single call). Bottom-right
// in the 1920x1080 design frame: centre (1684, 886), radius 140 at every resolution (text floored at 12 px on
// screen), forward up, 1.5 km (the model's RadarRange) with rings at 1/3 and 2/3, the camera's 70 degree view cone, height
// stems (above: full, stem down; below: dimmed, stem up; level within 50 m: none), rim pins for big things only (objectives,
// gates, carriers, incoming groups and warp-ins), missiles flashing at 2.5 Hz, a pulsing arc on the rim for each announced
// warp-in and incoming group, and the wave header above it (WAVE n / N, pips, the next group's count and timer, or NEXT
// WAVE in a breather).
//
// Colours are the plugin's FTDHUDStyle numbers (FTDUIPalette), fonts CoreStyle only; every kind has its own shape, never
// colour alone. Nothing in Plugins/TriadDriftHUD is touched. No per-frame allocations of our own: one scratch point array
// reused, strings rebuilt only when the numbers they show change.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"

struct FTDUIPainter;
class FTDHUDModel;

/** What the radar needs beyond the model. */
struct FTDHUDRadarInput
{
	/** The flown ship (its colour for "you"). */
	ETDShipId Flown = ETDShipId::Rift;
	bool bHasFlown = true;
	/** The wave director's state (ITDHUDData / GetWaveInfo); WaveCount 0 hides the header. */
	FTDWaveInfo Wave;
	/** Seconds, for pulses and flashes (the widget's own clock). */
	float Time = 0.f;
};

/** Paints the radar and its wave header with P (the flight HUD's painter, already letterboxed and faded). */
UNREALGAMEJAM_API void TDPaintHUDRadar(const FTDUIPainter& P, const FTDHUDModel& Model, const FTDHUDRadarInput& In);

/** The radar's box in design space at the current scale (coverage tests and layout checks): plate plus header. */
UNREALGAMEJAM_API FBox2D TDHUDRadarBox(const FTDUIPainter& P);
