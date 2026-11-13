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
	// The kit (lead/int 9e54fb8, ArtSource/EngProps/DominionInterior/README.md) is placed BY PIVOT at scale 1 with its own
	// collision: floor top at z = 0, 600 cm per storey, walls centred on their plane and tiling along their Y. A piece
	// whose mesh is missing falls back to a placeholder block of its size, so the layout always plays.
	auto KitMesh = [](const TCHAR* Name) -> UStaticMesh*
	{
		return TDPropArt::Load(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/%s.%s"), Name, Name))));
	};
	// Box: the piece's bounds from its pivot (README table), for the placeholder.
	auto Kit = [this, KitMesh](const TCHAR* Name, const FVector& At, float Yaw, const FBox& Box, bool bCollide = true) -> UStaticMeshComponent*
	{
		if (!Dressing)
		{
			return nullptr;
		}
		const FRotator Rot(0.f, Yaw, 0.f);
		UStaticMesh* M = KitMesh(Name);
		if (!M)
		{
			return Block(EShape::Cube, At + Rot.RotateVector(Box.GetCenter()), Box.GetSize(), Hull, 0.f, Rot, bCollide);
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Dressing);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(M);
		C->SetupAttachment(Dressing->GetRootComponent());
		C->SetRelativeLocationAndRotation(At, Rot);
		C->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		C->SetCanEverAffectNavigation(bCollide);
		C->RegisterComponent();
		Dressing->AddInstanceComponent(C);
		return C;
	};
	const FBox FloorBox(FVector(-200.f, -200.f, -20.f), FVector(200.f, 200.f, 0.f));
	const FBox CeilBox(FVector(-200.f, -200.f, 600.f), FVector(200.f, 200.f, 620.f));
	const FBox WallBox(FVector(-30.f, -200.f, 0.f), FVector(30.f, 200.f, 600.f));
	// A room on the grid: X0..X1, |Y| <= HalfY (HalfY an odd multiple of 200, so a doorway at y = 0 is one wall piece),
	// Storeys x 600 high. The end walls leave the ground-floor piece at y = 0 for a doorway when asked.
	auto Room = [&](float X0, float X1, float HalfY, int32 Storeys, bool bDoorAtX0, bool bDoorAtX1)
	{
		for (float X = X0 + 200.f; X < X1; X += Grid)
		{
			for (float Y = -HalfY + 200.f; Y < HalfY; Y += Grid)
			{
				Kit(TEXT("SM_DI_Room_Floor"), FVector(X, Y, 0.f), 0.f, FloorBox);
				Kit(TEXT("SM_DI_Room_Ceiling"), FVector(X, Y, (Storeys - 1) * 600.f), 0.f, CeilBox);
			}
		}
		for (int32 S = 0; S < Storeys; ++S)
		{
			const float Z = S * 600.f;
			for (float X = X0 + 200.f; X < X1; X += Grid)
			{
				Kit(TEXT("SM_DI_Room_Wall"), FVector(X, -HalfY, Z), 90.f, WallBox);
				Kit(TEXT("SM_DI_Room_Wall"), FVector(X, HalfY, Z), 90.f, WallBox);
			}
			for (float Y = -HalfY + 200.f; Y < HalfY; Y += Grid)
			{
				const bool bMiddle = S == 0 && FMath::Abs(Y) < 1.f;
				if (!(bMiddle && bDoorAtX0)) { Kit(TEXT("SM_DI_Room_Wall"), FVector(X0, Y, Z), 0.f, WallBox); }
				if (!(bMiddle && bDoorAtX1)) { Kit(TEXT("SM_DI_Room_Wall"), FVector(X1, Y, Z), 0.f, WallBox); }
			}
		}
	};
	// A corridor along X (clear width 340, height 600): one kit straight per cell.
	auto CorridorRun = [&](float X0, float X1)
	{
		const bool bKit = KitMesh(TEXT("SM_DI_Corridor_Straight")) != nullptr;
		for (float X = X0 + 200.f; X < X1; X += Grid)
		{
			if (bKit)
			{
				Kit(TEXT("SM_DI_Corridor_Straight"), FVector(X, 0.f, 0.f), 0.f, FloorBox);
				continue;
			}
			// Placeholder floor, walls and ceiling at the kit's inner faces.
			Block(EShape::Cube, FVector(X, 0.f, -10.f), FVector(Grid, Grid, 20.f), Deck);
			Block(EShape::Cube, FVector(X, -185.f, 300.f), FVector(Grid, 30.f, 600.f), Hull);
			Block(EShape::Cube, FVector(X, 185.f, 300.f), FVector(Grid, 30.f, 600.f), Hull);
			Block(EShape::Cube, FVector(X, 0.f, 610.f), FVector(Grid, Grid, 20.f), Hull);
		}
	};
	// A doorway on a cell seam across X, and its door at the same transform (it slides +Z by 400), knowing the pilot.
	auto Doorway = [&](float X, bool bRecognise) -> ATDDoor*
	{
		if (!KitMesh(TEXT("SM_DI_Doorway")))
		{
			Block(EShape::Cube, FVector(X, -175.f, 300.f), FVector(60.f, 50.f, 600.f), Hull);
			Block(EShape::Cube, FVector(X, 175.f, 300.f), FVector(60.f, 50.f, 600.f), Hull);
			Block(EShape::Cube, FVector(X, 0.f, 500.f), FVector(60.f, 300.f, 200.f), Hull);
		}
		else
		{
			Kit(TEXT("SM_DI_Doorway"), FVector(X, 0.f, 0.f), 0.f, WallBox);
		}
		ATDDoor* D = SpawnProp<ATDDoor>(FVector(X, 0.f, 0.f));
		if (D)
		{
			D->OpenHeight = 400.f;
			D->bRecognisesPilot = bRecognise;
			D->DoorMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Door.SM_DI_Door")));
			D->Mesh->SetRelativeScale3D(FVector(0.3f, 3.f, 4.f)); // the kit door's box, 30 x 300 x 400 (fitted exactly), before BeginPlay
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
	// The kit console's screen faces -X, where its user stands: Yaw turns that side toward the room.
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
		C->ConsoleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Console.SM_DI_Console")));
		TDCourseImpl::FinishProp(C);
		C->SetActorScale3D(FVector(0.6f, 1.2f, 1.3f)); // the kit console's 60 x 120 x 130 box
		UTDStatics::TintPlaceholder(C->Mesh, FLinearColor(0.3f, 0.85f, 1.f), 1.5f);
		C->OnInteracted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStoryConsole);
		StoryConsoles.Add(C, FName(*FString::Printf(TEXT("A4_Recording_%s"), Room)));
	};
	auto Entry = [this](FName Id, const FVector& At, float Yaw)
	{
		InteriorEntries.Add(Id, FTransform(FRotator(0.f, Yaw, 0.f), At + FVector(0.f, 0.f, 100.f)));
	};

	// --- Coordinates (cell seams on the 400 grid; half widths odd multiples of 200) ---------------------
	const float HX0 = -1200.f, HX1 = 1200.f, HY = 1400.f;        // hangar, 24 x 28 m, two storeys
	const float CX1 = 5200.f;                                     // deflect corridor, 40 m (clear width 3.4 m)
	const float AX1 = 7600.f, AY = 1400.f;                        // arena, 24 x 28 m
	const float L1X1 = 8400.f;                                    // link
	const float WX1 = 10400.f, WY = 1000.f;                       // Warden room, 20 x 20 m
	const float L2X1 = 11200.f;                                   // link
	const float KX1 = 14800.f, KY = 1800.f;                       // core chamber, 36 x 36 m, three storeys
	const float CY = 170.f;                                       // corridor clear half width
	const FVector CoreAt((L2X1 + KX1) * 0.5f, 0.f, 0.f);

	// --- Shell ----------------------------------------------------------------------------------------
	Room(HX0, HX1, HY, 2, false, true);
	Kit(TEXT("SM_DI_Hangar_Pad"), FVector(-200.f, 0.f, 0.5f), 0.f, FBox(FVector(-600.f, -600.f, -40.f), FVector(600.f, 600.f, 0.f))); // where the ship sits
	CorridorRun(HX1, CX1);
	Room(CX1, AX1, AY, 1, true, true);
	CorridorRun(AX1, L1X1);
	Room(L1X1, WX1, WY, 1, true, true);
	CorridorRun(WX1, L2X1);
	Room(L2X1, KX1, KY, 3, true, false);
	ATDDoor* HangarDoor = Doorway(HX1, true);
	ATDDoor* ArenaIn = Doorway(CX1, true);
	ATDDoor* ArenaOut = Doorway(AX1, true);
	ATDDoor* WardenIn = Doorway(L1X1, true);
	ATDDoor* WardenOut = Doorway(WX1, true);
	ATDDoor* CoreIn = Doorway(L2X1, true);

	// Lights: cold white overhead, red in the corridors, the core's orange glow.
	// (First capture, Nov 13: the corridor read solid red and the hangar floor washed out; the kit's own strips and
	// ceiling lights carry most of it.)
	Light(FVector(0.f, 0.f, 1000.f), 3200.f, FLinearColor(0.8f, 0.85f, 1.f), 1800.f);
	for (float X = HX1 + 600.f; X < CX1; X += 1200.f) { Light(FVector(X, 0.f, 520.f), 700.f, Strip, 180.f); }
	Light(FVector((CX1 + AX1) * 0.5f, 0.f, 520.f), 2600.f, FLinearColor(0.85f, 0.9f, 1.f), 2200.f);
	Light(FVector((L1X1 + WX1) * 0.5f, 0.f, 520.f), 2200.f, FLinearColor(1.f, 0.6f, 0.5f), 3000.f);
	Light(CoreAt + FVector(0.f, 0.f, 1500.f), 4200.f, CoreGlow, 8000.f);

	// Fixed exposure (live captures, Nov 13: auto-exposure in the dark corridors blew the kit out to solid red and the
	// room floors to white; with eye adaptation off it reads as designed, black with red strips). Exposure 1.0,
	// no adaptation, over the whole interior.
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (APostProcessVolume* PP = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), GetActorTransform(), Params))
		{
			PP->bUnbound = true;
			PP->Priority = 10.f;
			FPostProcessSettings& S = PP->Settings;
			S.bOverride_AutoExposureMethod = true;
			S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
			S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
			S.AutoExposureApplyPhysicalCameraExposure = false;
			S.bOverride_AutoExposureBias = true;
			S.AutoExposureBias = 0.f;
			Spawned.Add(PP);
			InteriorExposure = PP;
		}
	}

	// --- The reactor core: body, two rings, the shield (drops when the conduits break) -----------------
	Kit(TEXT("SM_DI_Core"), CoreAt, 0.f, FBox(FVector(-400.f, -400.f, 0.f), FVector(400.f, 400.f, 1200.f)));
	if (UStaticMeshComponent* RingA = Kit(TEXT("SM_DI_Core_Ring_A"), CoreAt + FVector(0.f, 0.f, 600.f), 0.f, FBox(FVector(-500.f, -500.f, -30.f), FVector(500.f, 500.f, 30.f)), false))
	{
		CoreRings.Add({ RingA, 14.f, 8.f });
	}
	if (UStaticMeshComponent* RingB = Kit(TEXT("SM_DI_Core_Ring_B"), CoreAt + FVector(0.f, 0.f, 600.f), 0.f, FBox(FVector(-575.f, -566.f, -24.f), FVector(575.f, 566.f, 24.f)), false))
	{
		CoreRings.Add({ RingB, -9.f, -12.f });
	}
	CoreShield = Kit(TEXT("SM_DI_Core_Shield"), CoreAt, 0.f, FBox(FVector(-640.f, -640.f, 0.f), FVector(640.f, 640.f, 1380.f)));
	if (CoreShield && !KitMesh(TEXT("SM_DI_Core_Shield")))
	{
		CoreShield->SetCollisionProfileName(TEXT("BlockAll"));
		UTDStatics::TintPlaceholder(CoreShield, CoreGlow, 0.5f);
	}

	// --- Objectives, in order -------------------------------------------------------------------------
	Entry(NAME_None, FVector(-600.f, 0.f, 40.f), 0.f); // the hangar start (T4's docking travels to "Hangar")

	// 1. The deflect lesson: two troopers at the corridor's far end, behind low cover, firing down the lane.
	ATDObjective* Lesson = MakeObjective(Corridor, LOCTEXT("DICorridorTitle", "Into the Dominion"), ETDAbilityId::None, ETDShipId::Rift, 2);
	AddTrailhead(Lesson, FVector(600.f, 0.f, 0.f), Strip, LOCTEXT("DIHangarDoor", "THE HANGAR DOOR"));
	StepDoors.Add({ Lesson, 0, HangarDoor });
	const FBox CoverBox(FVector(-30.f, -100.f, 0.f), FVector(30.f, 100.f, 110.f));
	Kit(TEXT("SM_DI_Cover_Low"), FVector(CX1 - 700.f, -70.f, 0.f), 0.f, CoverBox); // the troopers' cover; a 1.4 m gap on +Y
	// Spawn points are relative to the encounter volume's centre.
	const FVector LaneMid((HX1 + CX1) * 0.5f, 0.f, 200.f);
	ATDEncounterVolume* Lane = Encounter(Lesson, 0, LaneMid, FVector((CX1 - HX1) * 0.5f, CY, 400.f),
		{ ETDCrewType::Trooper, ETDCrewType::Trooper }, false, 0.f, { FVector(CX1 - 450.f, -100.f, 100.f) - LaneMid, FVector(CX1 - 450.f, 100.f, 100.f) - LaneMid });
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
	StoryConsole(FVector(-900.f, 900.f, 0.f), 180.f, TEXT("Hangar"));
	StoryConsole(FVector(CX1 + 250.f, -AY + 300.f, 0.f), 180.f, TEXT("Corridor")); // just inside the arena: the lane stays clear

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
		Kit(TEXT("SM_DI_Cover_Low"), At - FVector(0.f, 0.f, 55.f), 90.f * (i % 2), CoverBox);
	}
	CompletionDoors.Add({ Fight, 0, ArenaOut });
	Checkpoint(Fight, 1, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	Checkpoint(Fight, 2, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	FinishObjective(Fight, { LOCTEXT("DIA0", "CLEAR THE DECK"), LOCTEXT("DIA1", "THEY'RE SENDING MORE"), LOCTEXT("DIA2", "LAST WAVE") }, {});
	Entry(Arena, FVector(CX1 + 400.f, 0.f, 0.f), 0.f);
	StoryConsole(FVector(AX1 - 300.f, AY - 300.f, 0.f), 0.f, TEXT("Barracks")); // T3's id for this room

	// 3. The Warden at the gate.
	ATDObjective* Gate = MakeObjective(Warden, LOCTEXT("DIWardenTitle", "The Warden's Gate"), ETDAbilityId::None, ETDShipId::Rift, 1);
	AddTrailhead(Gate, FVector(L1X1 + 400.f, 0.f, 0.f), Strip, LOCTEXT("DIWarden", "THE GATE"));
	Trailheads.Last().After = Fight;
	StepDoors.Add({ Gate, 0, WardenIn });
	const FVector GateMid((L1X1 + WX1) * 0.5f, 0.f, 200.f);
	ATDEncounterVolume* Boss = Encounter(Gate, 0, GateMid, FVector((WX1 - L1X1) * 0.5f, WY, 500.f), { ETDCrewType::Warden }, false, 0.f,
		{ FVector(WX1 - 500.f, 0.f, 160.f) - GateMid }); // the Warden's capsule is 130 cm half height
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
		C->CrystalMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Conduit.SM_DI_Conduit")));
		C->Mesh->SetStaticMesh(CylinderMesh);
		C->Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 3.f));
		C->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
		TDCourseImpl::FinishProp(C);
		Conduits->WatchDestructible(C);
	}
	// Outside the kit core's base ring (live run, Nov 13: at -480 it sat buried inside the base, unreachable).
	ATDShatterCrystal* Heart = SpawnProp<ATDShatterCrystal>(CoreAt + FVector(-760.f, 0.f, 0.f));
	Heart->LinkedObjective = Reactor;
	Heart->LinkedStep = 1;
	Heart->Color = CoreGlow;
	Heart->MaxHealth = 150.f;
	Heart->Mesh->SetStaticMesh(SphereMesh);
	Heart->Mesh->SetRelativeScale3D(FVector(1.6f));
	Heart->Mesh->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	Heart->CrystalMesh.Reset(); // the glowing sphere until the heart's art lands (not a Cinder crystal)
	TDCourseImpl::FinishProp(Heart);
	// The heart's death completes its step (live run, Nov 13: it broke and nothing happened; a counter of one).
	ATDStepCounter* HeartDone = SpawnProp<ATDStepCounter>(CoreAt + FVector(-760.f, 0.f, 0.f));
	HeartDone->LinkedObjective = Reactor;
	HeartDone->LinkedStep = 1;
	HeartDone->Required = 1;
	HeartDone->BaseText = LOCTEXT("DIHeart", "STRIKE THE HEART");
	TDCourseImpl::FinishProp(HeartDone);
	HeartDone->WatchDestructible(Heart);
	Checkpoint(Reactor, 0, FVector(L2X1 + 300.f, 0.f, 0.f), 0.f);
	FinishObjective(Reactor, { LOCTEXT("DIK0", "BREAK THE SHIELD CONDUITS  0 / 4"), LOCTEXT("DIK1", "STRIKE THE HEART") }, { Conduits, Heart });
	Entry(Core, FVector(L2X1 - 300.f, 0.f, 0.f), 0.f);
	StoryConsole(FVector(L2X1 + 400.f, -KY + 400.f, 0.f), 180.f, TEXT("Core"));

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
	Collapse->DebrisMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Debris_A.SM_DI_Debris_A")));
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
	UTDStorySubsystem::Fire(this, *Id); // T3's transmissions: Nicholas's words and voices through Presentation
	// The kit console's screen (slot 2) lights once its recording has played.
	if (const ATDInteractableActor* Console = Cast<ATDInteractableActor>(Interactable))
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes(Console);
		for (UStaticMeshComponent* M : Meshes)
		{
			if (M && M != Console->Mesh && M->GetNumMaterials() > 2)
			{
				if (UMaterialInstanceDynamic* Screen = M->CreateDynamicMaterialInstance(2))
				{
					Screen->SetScalarParameterValue(TEXT("GlowStrength"), 9.f);
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Story beats (T3's UTDStorySubsystem, ids agreed Nov 13; once per run, queued, Nicholas's words)
// ------------------------------------------------------------------------------------------------

void ATDPlanetCourse::FireStepBeats(ATDObjective* Objective, int32 StepIndex)
{
	using namespace TDInteriorImpl;
	if (!Objective)
	{
		return;
	}
	const FName Id = Objective->ObjectiveId;
	auto Beat = [this](const TCHAR* Beat) { UTDStorySubsystem::Fire(this, FName(Beat)); };
	// Verdance, the survey archive (storyline.md section 16, reduced form): the transfer starts, the disruption, data secured.
	if (Id == TEXT("Obj_WreckCore") && StepIndex == 3) { Beat(TEXT("G3_TransferStart")); }
	else if (Id == TEXT("Obj_WreckCore") && StepIndex == 4) { Beat(TEXT("G3_Disruption")); }
	else if (Id == TEXT("Obj_WreckCore") && StepIndex == 5) { Beat(TEXT("G3_DataSecured")); }
	// Cinder: the enemy's transmissions echo the squad, one pilot at a time (act 3).
	else if (Id == TEXT("Obj_Outpost") && StepIndex == 0) { Beat(TEXT("A3_EchoWren")); }
	else if (Id == TEXT("Obj_Outpost") && StepIndex == 3) { Beat(TEXT("A3_EchoKato")); }
	else if (Id == TEXT("Obj_Jammer") && StepIndex == 0) { Beat(TEXT("A3_EchoMoss")); }
	// The Dominion (acts 4 and 5).
	else if (Id == Corridor && StepIndex == 0) { Beat(TEXT("A4_DoorsKnowUs")); } // the first door opens to the pilot's ID
	else if (Id == Core && StepIndex == 0) { Beat(TEXT("A5_Choice")); }
	else if (Id == Escape && StepIndex == 0) { Beat(TEXT("A5_Escape")); }
	// The core's shield is up while its conduits stand, and drops for the heart (a retake raises it again).
	if (Id == Core && CoreShield)
	{
		const bool bUp = StepIndex == 0;
		CoreShield->SetVisibility(bUp);
		CoreShield->SetCollisionEnabled(bUp ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

void ATDPlanetCourse::FireCompletionBeats(ATDObjective* Objective)
{
	if (!Objective)
	{
		return;
	}
	const FName Id = Objective->ObjectiveId;
	auto Beat = [this](const TCHAR* Beat) { UTDStorySubsystem::Fire(this, FName(Beat)); };
	auto Done = [this](const TCHAR* Other) { const ATDObjective* O = FindObjective(FName(Other)); return O && O->IsComplete(); };
	if (Id == TEXT("Obj_WreckCore")) { Beat(TEXT("A2_DeviceFound")); }
	else if (Id == TEXT("Obj_Relay")) { Beat(TEXT("A2_Voices")); } // static, then human voices that should not be there
	else if (Id == TEXT("Obj_Outpost")) { Beat(TEXT("A3_CrystalsFound")); }
	else if (Id == TEXT("Obj_Jammer")) { Beat(TEXT("A3_PassageOpen")); }
	if ((Id == TEXT("Obj_WreckCore") || Id == TEXT("Obj_Relay")) && Done(TEXT("Obj_WreckCore")) && Done(TEXT("Obj_Relay")))
	{
		Beat(TEXT("A2_TrailToCinder")); // the data points to Cinder
	}
}
