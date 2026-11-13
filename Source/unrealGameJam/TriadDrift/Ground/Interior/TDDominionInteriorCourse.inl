// Triad Drift - the Dominion interior on foot (ENG-29 interior, ENG-30 core, ENG-31 escape run), built from code like
// the planets, in the empty L_Dominion_Interior map. Included at the end of TDPlanetCourse.cpp (it shares the course's
// spawn and finish helpers). Owner: Teammate 5.
//
// Layout (course-local, +X forward, floor top at z = 0, a 400 cm grid; kit art SM_DI_* in
// Gameplay/Props/DominionInterior when it exists, else readable placeholder blocks):
//   Hangar (24 x 24 m) -> deflect corridor (40 m x 4 m) -> arena (24 x 24 m) -> link -> Warden room (20 x 20 m)
//   -> link -> core chamber (32 x 32 m, the reactor in the middle) ; the escape run goes back to the hangar.
// Story (storyline.md act 4): every door opens to the pilot's ID; one console per room plays a recording.

namespace TDInteriorImpl
{
	static const FLinearColor Hull(0.018f, 0.018f, 0.022f);     // gloss black
	static const FLinearColor Strip(1.f, 0.08f, 0.04f);         // red light strips
	static const FLinearColor Deck(0.04f, 0.04f, 0.05f);
	static const FLinearColor CoreGlow(1.f, 0.35f, 0.15f);
	static const float WallH = 600.f;
	static const float Grid = 400.f;

	static const FName Corridor(TEXT("Obj_DI_Corridor"));
	static const FName Arena(TEXT("Obj_DI_Arena"));
	static const FName Warden(TEXT("Obj_DI_Warden"));
	static const FName Core(TEXT("Obj_DI_Core"));
	static const FName Escape(TEXT("Obj_DI_Escape"));
}

const TArray<FName>& ATDPlanetCourse::InteriorObjectiveIds()
{
	using namespace TDInteriorImpl;
	static const TArray<FName> Ids = { Corridor, Arena, Warden, Core, Escape };
	return Ids;
}

FTransform ATDPlanetCourse::GetInteriorStart() const
{
	// The first room not done yet; the hangar when nothing is.
	for (const FName& Id : InteriorObjectiveIds())
	{
		const ATDObjective* O = FindObjective(Id);
		if (O && !O->IsComplete())
		{
			if (const FTransform* At = InteriorEntries.Find(Id))
			{
				return *At * GetActorTransform();
			}
			break;
		}
	}
	const FTransform* Hangar = InteriorEntries.Find(NAME_None);
	return (Hangar ? *Hangar : FTransform(FVector(-600.f, 0.f, 100.f))) * GetActorTransform();
}

void ATDPlanetCourse::BuildInterior()
{
	using namespace TDInteriorImpl;

	// --- Shell helpers -------------------------------------------------------------------------------
	// A room: floor tiles on the grid (each its own kit tile), one ceiling slab, red strips along the walls.
	auto Floor = [this](float X0, float X1, float Y0, float Y1, float CeilingH)
	{
		for (float X = X0; X < X1 - 1.f; X += Grid)
		{
			for (float Y = Y0; Y < Y1 - 1.f; Y += Grid)
			{
				Platform(FVector(X + Grid * 0.5f, Y + Grid * 0.5f, 0.f), 0.f, FVector(Grid, Grid, 20.f), Deck, 0.f, TEXT("SM_DI_Room_Floor"));
			}
		}
		Block(EShape::Cube, FVector((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, CeilingH + 20.f), FVector(X1 - X0, Y1 - Y0, 40.f), Hull);
	};
	// A straight wall from A to B (axis-aligned) in 400 cm kit pieces, leaving a 400 cm gap at each doorway centre.
	auto Wall = [this](const FVector2D& A, const FVector2D& B, float H, const TArray<float>& Gaps)
	{
		const bool bAlongX = FMath::Abs(B.X - A.X) > FMath::Abs(B.Y - A.Y);
		const float L0 = bAlongX ? FMath::Min(A.X, B.X) : FMath::Min(A.Y, B.Y);
		const float L1 = bAlongX ? FMath::Max(A.X, B.X) : FMath::Max(A.Y, B.Y);
		const float Fixed = bAlongX ? A.Y : A.X;
		for (float L = L0; L < L1 - 1.f; L += Grid)
		{
			const float C = L + Grid * 0.5f;
			bool bGap = false;
			for (const float G : Gaps) { bGap |= FMath::Abs(C - G) < Grid * 0.5f; }
			if (bGap)
			{
				continue;
			}
			const FVector Center = bAlongX ? FVector(C, Fixed, H * 0.5f) : FVector(Fixed, C, H * 0.5f);
			Block(EShape::Cube, Center, FVector(60.f, Grid, H), Hull, 0.f, bAlongX ? FRotator(0.f, 90.f, 0.f) : FRotator::ZeroRotator, true, TEXT("SM_DI_Room_Wall"));
			// The red strip at shoulder height, both faces (placeholder look; the kit wall has its own).
			if (!ArtFor(TEXT("SM_DI_Room_Wall")))
			{
				Block(EShape::Cube, Center + FVector(0.f, 0.f, 50.f - H * 0.5f + 220.f), bAlongX ? FVector(Grid, 70.f, 10.f) : FVector(70.f, Grid, 10.f), Strip, 3.f, FRotator::ZeroRotator, false);
			}
		}
	};
	// A doorway in a wall across X at X: frame (two posts and a lintel, collision) and a door that knows the pilot.
	auto Doorway = [this](float X, float Y, bool bRecognise) -> ATDDoor*
	{
		Block(EShape::Cube, FVector(X, Y - 175.f, 300.f), FVector(60.f, 50.f, 600.f), Hull);
		Block(EShape::Cube, FVector(X, Y + 175.f, 300.f), FVector(60.f, 50.f, 600.f), Hull);
		Block(EShape::Cube, FVector(X, Y, 500.f), FVector(60.f, 300.f, 200.f), Hull, 0.f, FRotator::ZeroRotator, true, TEXT("SM_DI_Doorway"));
		ATDDoor* D = SpawnProp<ATDDoor>(FVector(X, Y, 0.f));
		if (D)
		{
			D->OpenHeight = 420.f;
			D->bRecognisesPilot = bRecognise;
			D->DoorMesh = TDPropArt::Default(TEXT("SM_DI_Door"));
			D->Mesh->SetRelativeScale3D(FVector(0.3f, 3.f, 4.f)); // 30 x 300 x 400 (the kit door), before BeginPlay
			D->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
			TDCourseImpl::FinishProp(D);
			UTDStatics::TintPlaceholder(D->Mesh, Strip, 0.6f);
		}
		return D;
	};
	auto Light = [this](const FVector& At, float Radius, const FLinearColor& Color, float Intensity)
	{
		if (!Dressing)
		{
			return;
		}
		UPointLightComponent* L = NewObject<UPointLightComponent>(Dressing);
		L->SetupAttachment(Dressing->GetRootComponent());
		L->SetRelativeLocation(At);
		L->SetAttenuationRadius(Radius);
		L->SetLightColor(Color);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Intensity);
		L->SetCastShadows(false); // many lights, cheap
		L->RegisterComponent();
	};
	// One story console per room (storyline.md act 4: a recording in each room, older and older-sounding voices).
	auto StoryConsole = [this](const FVector& At, float Yaw, const TCHAR* Room)
	{
		ATDInteractableActor* C = SpawnProp<ATDInteractableActor>(At, FRotator(0.f, Yaw, 0.f));
		if (!C)
		{
			return;
		}
		C->Prompt = LOCTEXT("StoryConsole", "Play the recording");
		C->HoldTime = 0.f;
		C->bSingleUse = true;
		C->ConsoleMesh = TDPropArt::Default(TEXT("SM_DI_Console"));
		TDCourseImpl::FinishProp(C);
		UTDStatics::TintPlaceholder(C->Mesh, FLinearColor(0.3f, 0.85f, 1.f), 1.5f);
		C->OnInteracted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStoryConsole);
		StoryConsoles.Add(C, FName(*FString::Printf(TEXT("A4_Recording_%s"), Room)));
	};
	auto Entry = [this](FName Id, const FVector& At, float Yaw)
	{
		InteriorEntries.Add(Id, FTransform(FRotator(0.f, Yaw, 0.f), At + FVector(0.f, 0.f, 100.f)));
	};

	// --- Coordinates ----------------------------------------------------------------------------------
	const float HX0 = -1200.f, HX1 = 1200.f, HY = 1200.f;        // hangar
	const float CX1 = 5200.f, CY = 200.f;                         // deflect corridor (4 m wide)
	const float AX1 = 7600.f, AY = 1200.f;                        // arena
	const float L1X1 = 8400.f;                                    // link
	const float WX1 = 10400.f, WY = 1000.f;                       // Warden room
	const float L2X1 = 11200.f;                                   // link
	const float KX1 = 14400.f, KY = 1600.f;                       // core chamber
	const FVector CoreAt((L2X1 + KX1) * 0.5f, 0.f, 0.f);

	// --- Shell ----------------------------------------------------------------------------------------
	Floor(HX0, HX1, -HY, HY, 1000.f);
	Platform(FVector(-200.f, 0.f, 0.f), 40.f, FVector(1200.f, 1200.f, 40.f), Deck, 0.f, TEXT("SM_DI_Hangar_Pad")); // where the ship sits
	Floor(HX1, CX1, -CY, CY, WallH);
	Floor(CX1, AX1, -AY, AY, 900.f);
	Floor(AX1, L1X1, -CY, CY, WallH);
	Floor(L1X1, WX1, -WY, WY, 900.f);
	Floor(WX1, L2X1, -CY, CY, WallH);
	Floor(L2X1, KX1, -KY, KY, 1600.f);

	// Side walls.
	Wall(FVector2D(HX0, -HY), FVector2D(HX1, -HY), 1000.f, {}); Wall(FVector2D(HX0, HY), FVector2D(HX1, HY), 1000.f, {});
	Wall(FVector2D(HX0, -HY), FVector2D(HX0, HY), 1000.f, {}); // the bay (T4's ship came in through here; sealed on foot)
	Wall(FVector2D(HX1, -CY), FVector2D(CX1, -CY), WallH, {}); Wall(FVector2D(HX1, CY), FVector2D(CX1, CY), WallH, {});
	Wall(FVector2D(CX1, -AY), FVector2D(AX1, -AY), 900.f, {}); Wall(FVector2D(CX1, AY), FVector2D(AX1, AY), 900.f, {});
	Wall(FVector2D(AX1, -CY), FVector2D(L1X1, -CY), WallH, {}); Wall(FVector2D(AX1, CY), FVector2D(L1X1, CY), WallH, {});
	Wall(FVector2D(L1X1, -WY), FVector2D(WX1, -WY), 900.f, {}); Wall(FVector2D(L1X1, WY), FVector2D(WX1, WY), 900.f, {});
	Wall(FVector2D(WX1, -CY), FVector2D(L2X1, -CY), WallH, {}); Wall(FVector2D(WX1, CY), FVector2D(L2X1, CY), WallH, {});
	Wall(FVector2D(L2X1, -KY), FVector2D(KX1, -KY), 1600.f, {}); Wall(FVector2D(L2X1, KY), FVector2D(KX1, KY), 1600.f, {});
	Wall(FVector2D(KX1, -KY), FVector2D(KX1, KY), 1600.f, {});
	// End walls with a doorway in the middle (y = 0); the corridors meet them there.
	auto EndWall = [&](float X, float HalfY, float H)
	{
		Wall(FVector2D(X, -HalfY), FVector2D(X, HalfY), H, { 0.f });
		Block(EShape::Cube, FVector(X, 0.f, (WallH + H) * 0.5f), FVector(60.f, Grid, H - WallH), Hull); // over the doorway
	};
	EndWall(HX1, HY, 1000.f);
	EndWall(CX1, AY, 900.f); EndWall(AX1, AY, 900.f);
	EndWall(L1X1, WY, 900.f); EndWall(WX1, WY, 900.f);
	EndWall(L2X1, KY, 1600.f);
	ATDDoor* HangarDoor = Doorway(HX1, 0.f, true);
	ATDDoor* ArenaIn = Doorway(CX1, 0.f, true);
	ATDDoor* ArenaOut = Doorway(AX1, 0.f, true);
	ATDDoor* WardenIn = Doorway(L1X1, 0.f, true);
	ATDDoor* WardenOut = Doorway(WX1, 0.f, true);
	ATDDoor* CoreIn = Doorway(L2X1, 0.f, true);

	// Lights: cold white overhead, red in the corridors, the core's orange glow.
	Light(FVector(0.f, 0.f, 850.f), 3000.f, FLinearColor(0.8f, 0.85f, 1.f), 4000.f);
	for (float X = HX1 + 600.f; X < CX1; X += 1200.f) { Light(FVector(X, 0.f, 520.f), 900.f, Strip, 900.f); }
	Light(FVector((CX1 + AX1) * 0.5f, 0.f, 780.f), 2600.f, FLinearColor(0.85f, 0.9f, 1.f), 4000.f);
	Light(FVector((L1X1 + WX1) * 0.5f, 0.f, 780.f), 2200.f, FLinearColor(1.f, 0.6f, 0.5f), 3000.f);
	Light(CoreAt + FVector(0.f, 0.f, 1300.f), 3800.f, CoreGlow, 8000.f);

	// --- The reactor core (the heart; the fight's art and phases come in step 4) ------------------------
	Block(EShape::Cylinder, CoreAt + FVector(0.f, 0.f, 600.f), FVector(800.f, 800.f, 1200.f), Hull, 0.f, FRotator::ZeroRotator, true, TEXT("SM_DI_Core"));
	Block(EShape::Sphere, CoreAt + FVector(0.f, 0.f, 700.f), FVector(500.f, 500.f, 500.f), CoreGlow, 4.f, FRotator::ZeroRotator, false);

	// --- Objectives, in order -------------------------------------------------------------------------
	Entry(NAME_None, FVector(-600.f, 0.f, 40.f), 0.f); // the hangar start (T4's docking travels to "Hangar")

	// 1. The deflect lesson: two troopers at the corridor's far end, behind low cover, firing down the lane.
	ATDObjective* Lesson = MakeObjective(Corridor, LOCTEXT("DICorridorTitle", "Into the Dominion"), ETDAbilityId::None, ETDShipId::Rift, 2);
	AddTrailhead(Lesson, FVector(600.f, 0.f, 0.f), Strip, LOCTEXT("DIHangarDoor", "THE HANGAR DOOR"));
	StepDoors.Add({ Lesson, 0, HangarDoor });
	for (const float Y : { -110.f, 110.f })
	{
		Block(EShape::Cube, FVector(CX1 - 700.f, Y, 55.f), FVector(60.f, 160.f, 110.f), Hull, 0.f, FRotator::ZeroRotator, true, TEXT("SM_DI_Cover_Low"));
	}
	ATDEncounterVolume* Lane = Encounter(Lesson, 0, FVector((HX1 + CX1) * 0.5f, 0.f, 200.f), FVector((CX1 - HX1) * 0.5f, CY, 400.f),
		{ ETDCrewType::Trooper, ETDCrewType::Trooper }, false, 0.f, { FVector(CX1 - 450.f, -100.f, 100.f), FVector(CX1 - 450.f, 100.f, 100.f) });
	StepEncounters.Add({ Lesson, 0, Lane });
	if (Lane) { Lane->DoorsToOpen.Add(ArenaIn); }
	ATDStepTrigger* IntoArena = SpawnProp<ATDStepTrigger>(FVector(CX1 + 500.f, 0.f, 0.f));
	IntoArena->LinkedObjective = Lesson;
	IntoArena->LinkedStep = 1;
	TDCourseImpl::FinishProp(IntoArena);
	IntoArena->Box->SetBoxExtent(FVector(300.f, 600.f, 400.f));
	Checkpoint(Lesson, 0, FVector(HX1 - 300.f, 0.f, 0.f), 0.f);
	FinishObjective(Lesson, { LOCTEXT("DIC0", "RETURN THEIR FIRE · BLOCK [RMB] AS IT HITS"), LOCTEXT("DIC1", "GO THROUGH") }, { nullptr, IntoArena });
	// No entry for the corridor: a Continue before it starts on the hangar floor.
	StoryConsole(FVector(-900.f, 900.f, 0.f), -45.f, TEXT("Hangar"));
	StoryConsole(FVector(HX1 + 900.f, 150.f, 0.f), -90.f, TEXT("Corridor"));

	// 2. The arena: three waves, one new threat each.
	ATDObjective* Fight = MakeObjective(Arena, LOCTEXT("DIArenaTitle", "Hold the Deck"), ETDAbilityId::None, ETDShipId::Rift, 3);
	AddTrailhead(Fight, FVector(CX1 + 900.f, 0.f, 0.f), Strip, LOCTEXT("DIArena", "THE FLIGHT DECK"));
	Trailheads.Last().After = Lesson;
	const FVector ArenaMid((CX1 + AX1) * 0.5f, 0.f, 200.f);
	const FVector ArenaExtent((AX1 - CX1) * 0.5f, AY, 500.f);
	StepEncounters.Add({ Fight, 0, Encounter(Fight, 0, ArenaMid, ArenaExtent, { ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Grunt }, false, 800.f) });
	StepEncounters.Add({ Fight, 1, Encounter(Fight, 1, ArenaMid, ArenaExtent, { ETDCrewType::Trooper, ETDCrewType::Trooper, ETDCrewType::Heavy }, false, 900.f) });
	StepEncounters.Add({ Fight, 2, Encounter(Fight, 2, ArenaMid, ArenaExtent, { ETDCrewType::Drone, ETDCrewType::Drone, ETDCrewType::Grunt }, false, 900.f) });
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector At(CX1 + 700.f + (i % 2) * 1000.f, (i < 2 ? -1 : 1) * 500.f, 55.f);
		Block(EShape::Cube, At, FVector(60.f, 200.f, 110.f), Hull, 0.f, FRotator(0.f, 90.f * (i % 2), 0.f), true, TEXT("SM_DI_Cover_Low"));
	}
	CompletionDoors.Add({ Fight, 0, ArenaOut });
	Checkpoint(Fight, 1, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	Checkpoint(Fight, 2, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	FinishObjective(Fight, { LOCTEXT("DIA0", "CLEAR THE DECK"), LOCTEXT("DIA1", "THEY'RE SENDING MORE"), LOCTEXT("DIA2", "LAST WAVE") }, {});
	Entry(Arena, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	StoryConsole(FVector(AX1 - 500.f, AY - 400.f, 0.f), -135.f, TEXT("Arena"));

	// 3. The Warden at the gate.
	ATDObjective* Gate = MakeObjective(Warden, LOCTEXT("DIWardenTitle", "The Warden's Gate"), ETDAbilityId::None, ETDShipId::Rift, 1);
	AddTrailhead(Gate, FVector(L1X1 + 400.f, 0.f, 0.f), Strip, LOCTEXT("DIWarden", "THE GATE"));
	Trailheads.Last().After = Fight;
	StepDoors.Add({ Gate, 0, WardenIn });
	ATDEncounterVolume* Boss = Encounter(Gate, 0, FVector((L1X1 + WX1) * 0.5f, 0.f, 200.f), FVector((WX1 - L1X1) * 0.5f, WY, 500.f), { ETDCrewType::Warden }, false, 0.f,
		{ FVector(WX1 - 500.f, 0.f, 120.f) });
	StepEncounters.Add({ Gate, 0, Boss });
	if (Boss)
	{
		FTDRetakeMercyTier M3; M3.FromTake = 3; M3.HealthScale = 0.75f; M3.MaxSummons = 1;
		FTDRetakeMercyTier M5; M5.FromTake = 5; M5.HealthScale = 0.5f; M5.MaxSummons = 0;
		Boss->MercyTiers = { M3, M5 };
	}
	CompletionDoors.Add({ Gate, 0, WardenOut });
	Checkpoint(Gate, 0, FVector(L1X1 + 300.f, 0.f, 0.f), 0.f);
	FinishObjective(Gate, { LOCTEXT("DIW0", "DEFEAT THE WARDEN") }, {});
	Entry(Warden, FVector(L1X1 - 300.f, 0.f, 0.f), 0.f);
	StoryConsole(FVector(WX1 - 400.f, WY - 350.f, 0.f), -135.f, TEXT("Warden"));

	// 4. The core: break the four shield conduits, then strike the heart (the orbs, the laser and the deflect-only
	// heart come in step 4 of the plan; this is the finishable skeleton).
	ATDObjective* Reactor = MakeObjective(Core, LOCTEXT("DICoreTitle", "The Reactor Core"), ETDAbilityId::None, ETDShipId::Rift, 2);
	AddTrailhead(Reactor, FVector(L2X1 + 500.f, 0.f, 0.f), CoreGlow, LOCTEXT("DICore", "THE CORE"));
	Trailheads.Last().After = Gate;
	StepDoors.Add({ Reactor, 0, CoreIn });
	ATDStepCounter* Conduits = SpawnProp<ATDStepCounter>(CoreAt);
	Conduits->LinkedObjective = Reactor;
	Conduits->LinkedStep = 0;
	Conduits->Required = 4;
	Conduits->BaseText = LOCTEXT("DIConduits", "BREAK THE SHIELD CONDUITS");
	TDCourseImpl::FinishProp(Conduits);
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = PI * 0.25f + i * PI * 0.5f;
		ATDShatterCrystal* C = SpawnProp<ATDShatterCrystal>(CoreAt + FVector(FMath::Cos(A) * 1100.f, FMath::Sin(A) * 1100.f, 0.f));
		C->LinkedObjective = Reactor;
		C->LinkedStep = 0;
		C->Color = Strip;
		C->MaxHealth = 80.f;
		C->CrystalMesh = TDPropArt::Default(TEXT("SM_DI_Conduit"));
		C->Mesh->SetStaticMesh(CylinderMesh);
		C->Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 3.f));
		C->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
		TDCourseImpl::FinishProp(C);
		Conduits->WatchDestructible(C);
	}
	ATDShatterCrystal* Heart = SpawnProp<ATDShatterCrystal>(CoreAt + FVector(-480.f, 0.f, 0.f));
	Heart->LinkedObjective = Reactor;
	Heart->LinkedStep = 1;
	Heart->Color = CoreGlow;
	Heart->MaxHealth = 150.f;
	Heart->Mesh->SetStaticMesh(SphereMesh);
	Heart->Mesh->SetRelativeScale3D(FVector(1.6f));
	Heart->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	TDCourseImpl::FinishProp(Heart);
	Checkpoint(Reactor, 0, FVector(L2X1 + 300.f, 0.f, 0.f), 0.f);
	FinishObjective(Reactor, { LOCTEXT("DIK0", "BREAK THE SHIELD CONDUITS  0 / 4"), LOCTEXT("DIK1", "STRIKE THE HEART") }, { Conduits, Heart });
	Entry(Core, FVector(L2X1 - 300.f, 0.f, 0.f), 0.f);
	StoryConsole(FVector(L2X1 + 400.f, -KY + 400.f, 0.f), 45.f, TEXT("Core"));

	// 5. The escape run: back to the ship against the clock, debris falling (a CUT restarts the run, not the fight).
	ATDObjective* Run = MakeObjective(Escape, LOCTEXT("DIEscapeTitle", "Get Out"), ETDAbilityId::None, ETDShipId::Rift, 1);
	ATDStepTrigger* Ship = SpawnProp<ATDStepTrigger>(FVector(-200.f, 0.f, 0.f));
	Ship->LinkedObjective = Run;
	Ship->LinkedStep = 0;
	TDCourseImpl::FinishProp(Ship);
	Ship->Box->SetBoxExtent(FVector(500.f, 500.f, 400.f));
	ATDCollapseRun* Collapse = SpawnProp<ATDCollapseRun>(FVector((HX0 + KX1) * 0.5f, 0.f, 0.f));
	Collapse->LinkedObjective = Run;
	Collapse->LinkedStep = 0;
	Collapse->Seconds = EscapeSeconds;
	Collapse->RunText = LOCTEXT("DIRun", "GET BACK TO YOUR SHIP · {0}");
	Collapse->DebrisExtent = FVector((KX1 - HX0) * 0.5f, 150.f, 0.f); // down the spine of the route
	TDCourseImpl::FinishProp(Collapse);
	Checkpoint(Run, 0, FVector(L2X1 + 300.f, 0.f, 0.f), 180.f);
	FinishObjective(Run, { LOCTEXT("DIE0", "GET BACK TO YOUR SHIP") }, { Ship });
	Entry(Escape, FVector(L2X1 + 300.f, 0.f, 0.f), 180.f);
	Routes.Add({ FVector(-600.f, 0.f, 0.f), CoreAt });
}

void ATDPlanetCourse::HandleInteriorObjectiveCompleted(ATDObjective* Objective)
{
	using namespace TDInteriorImpl;
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!Objective)
	{
		return;
	}
	if (Objective->ObjectiveId == Core)
	{
		if (GI)
		{
			GI->GetMutableProgress().bReactorDestroyed = true; // T4 rebuilds the burning Dominion from this
		}
		// Every door opens for the way back (the changed route comes in step 5); the run starts after the core's moment.
		for (TActorIterator<ATDDoor> It(GetWorld()); It; ++It)
		{
			It->Open();
		}
		GetWorldTimerManager().SetTimer(EscapeTimer, this, &ATDPlanetCourse::StartEscape, 2.5f, false);
	}
	else if (Objective->ObjectiveId == Escape)
	{
		HandInteriorOver();
		return;
	}
	if (GI)
	{
		GI->SaveCheckpoint(); // a Continue starts at the next room
	}
}

void ATDPlanetCourse::StartEscape()
{
	StartObjectiveAt(TDInteriorImpl::Escape);
}

void ATDPlanetCourse::HandInteriorOver()
{
	if (bHandedOff)
	{
		return;
	}
	bHandedOff = true;
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!GI)
	{
		return;
	}
	// The contract with T4 (Nov 13): Phase Escape (it marks the core dead and the Dominion boarded), then the galaxy at
	// the hangar mouth, where T4's escape flight starts.
	GI->SetPhase(ETDGamePhase::Escape);
	GI->GetMutableProgress().ReturnPoint = TDReturnPoints::DominionHangarExit;
	if (bInteriorTravel)
	{
		GI->TravelTo(ETDDestination::Galaxy, TDReturnPoints::DominionHangarExit);
	}
}

void ATDPlanetCourse::HandleStoryConsole(AActor* Interactable, AActor* Interactor)
{
	const FName* Id = StoryConsoles.Find(Interactable);
	if (!Id)
	{
		return;
	}
	LastStoryBeat = *Id;
	// T3's transmission system plays the recording (Nicholas's words and voices through Presentation); until it is
	// in, the beat is logged so the hook can be checked.
	UE_LOG(LogTemp, Log, TEXT("TDDominionInterior: story beat %s"), *Id->ToString());
}
