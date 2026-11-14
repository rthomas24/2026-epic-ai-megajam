// Triad Drift - the Dominion interior on foot (ENG-29 interior, ENG-30 core, ENG-31 escape run), built from code like
// the planets, in the empty L_Dominion_Interior map. Included at the end of TDPlanetCourse.cpp (it shares the course's
// spawn and finish helpers). Owner: Teammate 5.
//
// Layout (course-local, +X forward, floor top at z = 0, a 400 cm grid; kit art SM_DI_* in
// Gameplay/Props/DominionInterior when it exists, else readable placeholder blocks):
//   Hangar (24 x 24 m) -> deflect corridor (40 m x 4 m) -> arena (24 x 24 m) -> link -> Warden room (20 x 20 m)
//   -> link -> core chamber (32 x 32 m, the reactor in the middle) ; the escape run goes back to the hangar.
// Story (storyline.md act 4): every door opens to the pilot's ID; one console per room plays a recording.

ATDPlanetCourse::FTDOnFieldAnchorDisabled ATDPlanetCourse::OnFieldAnchorDisabled;
ATDPlanetCourse::FTDOnFinalRecord ATDPlanetCourse::OnFinalRecordStarted;
ATDPlanetCourse::FTDOnInteriorMoment ATDPlanetCourse::OnInteriorMoment;

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
	// The walkable rebuild from the Blender kit (structures builder, eng/struct 8ce5c3fb: Props/Kit/Dominion, same names,
	// sizes, pivots and openings) wins; the earlier pieces in Props/DominionInterior (the reactor's among them) otherwise.
	auto KitMesh = [](const TCHAR* Name) -> UStaticMesh*
	{
		for (const TCHAR* Folder : { TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Dominion"), TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior") })
		{
			const FString Package = FString::Printf(TEXT("%s/%s"), Folder, Name);
			if (FindPackage(nullptr, *Package) || FPackageName::DoesPackageExist(Package))
			{
				if (UStaticMesh* M = TDPropArt::Load(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *Package, Name))))) { return M; }
			}
		}
		return nullptr;
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
			D->DoorMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Dominion/SM_DI_Door"))
				? TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Dominion/SM_DI_Door.SM_DI_Door") : TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Door.SM_DI_Door")));
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
		L->ComponentTags.Add(TEXT("TD.InteriorLight")); // the environment builder's alarm and collapse lighting finds them (ART-13)
		L->RegisterComponent();
	};
	// One story console per room (storyline.md act 4: a recording in each room, older and older-sounding voices).
	// The kit console's screen faces -X, where its user stands: Yaw turns that side toward the room.
	auto StoryConsole = [this](const FVector& At, float Yaw, const TCHAR* Room) -> ATDInteractableActor*
	{
		// The origin is the box centre: 65 cm up so the 130 cm console stands on the deck. A recording never competes
		// with a fight (story 07-09, b3): it answers only with no crewman within 15 m.
		ATDSurveyTerminal* C = SpawnProp<ATDSurveyTerminal>(At + FVector(0.f, 0.f, 65.f), FRotator(0.f, Yaw, 0.f));
		if (!C)
		{
			return nullptr;
		}
		C->bAnnounceRecords = false;
		C->Prompt = LOCTEXT("StoryConsole", "Play the recording");
		C->HoldTime = 0.f;
		C->bSingleUse = true;
		C->ConsoleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/DominionInterior/SM_DI_Console.SM_DI_Console")));
		TDCourseImpl::FinishProp(C);
		C->SetActorScale3D(FVector(1.f, 2.f, 130.f / 120.f)); // the kit console's 60 x 120 x 130 box (the cube is 60 x 60 x 120)
		UTDStatics::TintPlaceholder(C->Mesh, FLinearColor(0.3f, 0.85f, 1.f), 1.5f);
		C->OnInteracted.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStoryConsole);
		StoryConsoles.Add(C, FName(*FString::Printf(TEXT("A4_Recording_%s"), Room)));
		return C;
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
	CoreChamberBox = FBox(W(FVector(L2X1, -KY, -200.f)), W(FVector(KX1, KY, 1900.f)));

	// --- Shell ----------------------------------------------------------------------------------------
	Room(HX0, HX1, HY, 2, false, true);
	Kit(TEXT("SM_DI_Hangar_Pad"), FVector(-200.f, 0.f, 0.5f), 0.f, FBox(FVector(-600.f, -600.f, -40.f), FVector(600.f, 600.f, 0.f))); // where the ship sits
	// The ship the player docked with stands on the pad (the boarding pilot's own; the escape run returns to it).
	if (const UTDShipVisuals* ShipArt = UTDShipLibrary::LoadVisuals(UTDShipLibrary::Get(ATDFirstPersonCharacter::GetOnFootPilotShip(this))))
	{
		// As ATDShip::RebuildVisuals builds it: the hull mesh (or, parts only, the first part as the fuselage) plus every
		// visual part on its socket (the Rift's portal ring is a part: Ryan saw the empty ring, Nov 13).
		auto MakePart = [this](UStaticMesh* Mesh, const TArray<TSoftObjectPtr<UMaterialInterface>>& Mats, USceneComponent* Parent, FName Socket, const FTransform& Rel, bool bCollide)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Dressing);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Mesh);
			C->SetupAttachment(Parent, Socket);
			C->SetRelativeTransform(Rel);
			for (int32 i = 0; i < Mats.Num(); ++i)
			{
				if (UMaterialInterface* M = Mats[i].LoadSynchronous()) { C->SetMaterial(i, M); }
			}
			if (bCollide) { C->SetCollisionProfileName(TEXT("BlockAll")); }
			else { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
			C->CanCharacterStepUpOn = ECB_No;
			C->RegisterComponent();
			Dressing->AddInstanceComponent(C);
			return C;
		};
		UStaticMesh* HullArt = ShipArt->Mesh.IsNull() ? nullptr : ShipArt->Mesh.LoadSynchronous();
		UStaticMesh* FirstPart = nullptr;
		for (const FTDShipVisualPart& Spec : ShipArt->Parts) { if (!FirstPart && !Spec.Mesh.IsNull()) { FirstPart = Spec.Mesh.LoadSynchronous(); } }
		if (UStaticMesh* Base = HullArt ? HullArt : FirstPart)
		{
			// A pivot on the pad, nose to the bay; the base mesh's bottom rests on the pad.
			USceneComponent* Pivot = NewObject<USceneComponent>(Dressing);
			Pivot->SetMobility(EComponentMobility::Movable);
			Pivot->SetupAttachment(Dressing->GetRootComponent());
			Pivot->SetRelativeLocationAndRotation(FVector(-200.f, 0.f, 1.f - Base->GetBoundingBox().Min.Z), FRotator(0.f, 180.f, 0.f));
			Pivot->RegisterComponent();
			Dressing->AddInstanceComponent(Pivot);
			UStaticMeshComponent* Primary = HullArt ? MakePart(HullArt, ShipArt->MaterialOverrides, Pivot, NAME_None, FTransform::Identity, true) : nullptr;
			ParkedShip = Primary;
			ParkedParts.Reset();
			for (const FTDShipVisualPart& Spec : ShipArt->Parts)
			{
				UStaticMesh* PartMesh = Spec.Mesh.IsNull() ? nullptr : Spec.Mesh.LoadSynchronous();
				if (!PartMesh) { continue; }
				const bool bSocket = Spec.ParentSocket != NAME_None && Primary && Primary->DoesSocketExist(Spec.ParentSocket);
				UStaticMeshComponent* Part = MakePart(PartMesh, Spec.MaterialOverrides, bSocket ? Primary : Pivot, bSocket ? Spec.ParentSocket : NAME_None,
					Spec.RelativeTransform, !Primary); // parts only: the fuselage part collides
				ParkedParts.Add(Part);
				if (!Primary) { Primary = Part; ParkedShip = Part; }
			}
		}
	}
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
	if (WardenIn)
	{
		WardenIn->AccessText = LOCTEXT("DIWardenDoor", "ACCESS GRANTED · WREN · KATO · MOSS"); // story 07-09: the security system knows all three
	}
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
	// The hangar start (T4's docking travels to "Hangar"): beside the parked ship, facing the door with nothing in the
	// way (T7, Nov 14: at (-484, 0) the pilot stood behind the Rift with the hull between them and the door).
	FVector HangarStart(-600.f, 0.f, 40.f);
	{
		FBox ShipBox(ForceInit);
		for (const TObjectPtr<UStaticMeshComponent>& Part : ParkedParts)
		{
			if (Part) { ShipBox += Part->Bounds.GetBox().TransformBy(GetActorTransform().Inverse()); }
		}
		if (ParkedShip) { ShipBox += ParkedShip->Bounds.GetBox().TransformBy(GetActorTransform().Inverse()); }
		if (ShipBox.IsValid)
		{
			HangarStart = FVector(ShipBox.GetCenter().X, FMath::Max(ShipBox.Min.Y - 250.f, -HY + 200.f), 40.f);
		}
	}
	Entry(NAME_None, HangarStart, 0.f);
	// Kato on arrival in the hangar (first visit: the story fires once per run).
	GetWorldTimerManager().SetTimerForNextTick([this]() { UTDStorySubsystem::PlaySequence(this, TEXT("HangarArrival")); });

	// 1. The deflect lesson: two troopers at the corridor's far end, behind low cover, firing down the lane.
	ATDObjective* Lesson = MakeObjective(Corridor, LOCTEXT("DICorridorTitle", "Reach the Command Archive"), ETDAbilityId::None, ETDShipId::Rift, 2);
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
	ATDEncounterVolume* Wave1 = Encounter(Fight, 0, ArenaMid, ArenaExtent, { ETDCrewType::Grunt, ETDCrewType::Grunt, ETDCrewType::Grunt }, false, 800.f);
	ATDEncounterVolume* Wave2 = Encounter(Fight, 1, ArenaMid, ArenaExtent, { ETDCrewType::Trooper, ETDCrewType::Trooper, ETDCrewType::Heavy }, false, 900.f);
	ATDEncounterVolume* Wave3 = Encounter(Fight, 2, ArenaMid, ArenaExtent, { ETDCrewType::Drone, ETDCrewType::Drone, ETDCrewType::Grunt }, false, 900.f);
	// Director's mercy on the deck too (T7, Nov 14: 17-18 takes here with no easing at all): take 2 the crew at 75 %
	// health, take 3 one fewer (the last listed: wave 2's Heavy), take 5 two fewer.
	for (ATDEncounterVolume* E : { Wave1, Wave2, Wave3 })
	{
		if (!E) { continue; }
		FTDRetakeMercyTier M2; M2.FromTake = 2; M2.HealthScale = 0.75f;
		FTDRetakeMercyTier M3; M3.FromTake = 3; M3.HealthScale = 0.75f; M3.DropCrew = 1;
		FTDRetakeMercyTier M5; M5.FromTake = 5; M5.HealthScale = 0.6f; M5.DropCrew = 2;
		E->MercyTiers = { M2, M3, M5 };
	}
	StepEncounters.Add({ Fight, 0, Wave1 });
	StepEncounters.Add({ Fight, 1, Wave2 });
	StepEncounters.Add({ Fight, 2, Wave3 });
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
	ATDObjective* Reactor = MakeObjective(Core, LOCTEXT("DICoreTitle", "The Reactor"), ETDAbilityId::None, ETDShipId::Rift, 2);
	AddTrailhead(Reactor, FVector(L2X1 + 500.f, 0.f, 0.f), CoreGlow, LOCTEXT("DICore", "THE REACTOR"));
	Trailheads.Last().After = Gate;
	ReactorDoor = CoreIn; // opens when the final record has played (TickFinalRecord), not on the fight's start
	{
		ATDEncounterVolume* Drones = Encounter(Reactor, INDEX_NONE, CoreAt + FVector(0.f, 0.f, 700.f), FVector(10.f),
			{ ETDCrewType::Drone, ETDCrewType::Drone }, false, 1400.f);
		if (Drones)
		{
			FTDRetakeMercyTier DroneMercy3; DroneMercy3.FromTake = 3; DroneMercy3.DropCrew = 1;
			Drones->MercyTiers = { DroneMercy3 };
			StepEncounters.Add({ Reactor, 0, Drones });
		}
	}
	ATDStepCounter* Conduits = SpawnProp<ATDStepCounter>(CoreAt);
	Conduits->LinkedObjective = Reactor;
	Conduits->LinkedStep = 0;
	Conduits->Required = 4;
	Conduits->BaseText = LOCTEXT("DIConduits", "BREAK THE REACTOR CONDUITS");
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
		ReactorConduits.Add(C);
		if (UTDHealthComponent* H = UTDStatics::GetHealth(C))
		{
			H->OnDeath.AddUniqueDynamic(this, &ATDPlanetCourse::HandleConduitDown);
		}
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
	ReactorHeart = Heart;
	if (UTDHealthComponent* HH = UTDStatics::GetHealth(Heart))
	{
		HH->OnDamaged.AddUniqueDynamic(this, &ATDPlanetCourse::HandleHeartHit);
	}
	PlayReactorFX(TEXT("CoreIdle"), Heart->GetActorLocation(), Heart->GetRootComponent(), true);
	// The heart's death completes its step (live run, Nov 13: it broke and nothing happened; a counter of one).
	ATDStepCounter* HeartDone = SpawnProp<ATDStepCounter>(CoreAt + FVector(-760.f, 0.f, 0.f));
	HeartDone->LinkedObjective = Reactor;
	HeartDone->LinkedStep = 1;
	HeartDone->Required = 1;
	HeartDone->BaseText = LOCTEXT("DIHeart", "DESTROY THE REACTOR HEART");
	TDCourseImpl::FinishProp(HeartDone);
	HeartDone->WatchDestructible(Heart);
	Checkpoint(Reactor, 0, FVector(L2X1 + 300.f, 0.f, 0.f), 0.f);
	FinishObjective(Reactor, { LOCTEXT("DIK0", "BREAK THE REACTOR CONDUITS  0 / 4"), LOCTEXT("DIK1", "DESTROY THE REACTOR HEART") }, { Conduits, Heart });
	ReactorConduitCounter = Conduits;
	Entry(Core, FVector(L2X1 - 300.f, 0.f, 0.f), 0.f);
	FinalRecordConsole = StoryConsole(FVector(WX1 - 350.f, -WY + 350.f, 0.f), 0.f, TEXT("Core")); // mandatory: on the main route, after the Warden
	if (ATDInteractableActor* Rec = Cast<ATDInteractableActor>(FinalRecordConsole))
	{
		Rec->Prompt = LOCTEXT("FinalRecord", "Play the final record");
		Rec->bSingleUse = false; // a second press skips the record
	}

	// 5. The escape run: back to the ship against the clock, debris falling (a CUT restarts the run, not the fight).
	ATDObjective* Run = MakeObjective(Escape, LOCTEXT("DIEscapeTitle", "Get Out"), ETDAbilityId::None, ETDShipId::Rift, 1);
	ATDStepTrigger* Ship = SpawnProp<ATDStepTrigger>(FVector(-200.f, 0.f, 0.f));
	Ship->LinkedObjective = Run;
	Ship->LinkedStep = 0;
	TDCourseImpl::FinishProp(Ship);
	Ship->Box->SetBoxExtent(FVector(500.f, 500.f, 400.f));
	ATDCollapseRun* Collapse = EscapeCollapse = SpawnProp<ATDCollapseRun>(FVector((HX0 + KX1) * 0.5f, 0.f, 0.f));
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

	// Emergency strips (environment builder's UTDInteriorLightSubsystem, Lead Nov 14): every kit piece with a strip slot
	// (MI_KitD_Glow*, parent M_TD_DominionGlow) is tagged TD.EmergencyStrip and gets its own instance on that slot, so
	// the alarm can drive the strip glow per component. The parent has no EmergencyPulse parameter yet (a binary material
	// edit): its scalars today are GlowStrength (the glow, read the base from the instance) and PulseAmount.
	{
		// The kit pieces on the dressing actor, and the doors (SM_DI_Door's strip is its slot 1).
		TArray<UStaticMeshComponent*> Pieces;
		for (AActor* A : Spawned)
		{
			TArray<UStaticMeshComponent*> Own;
			if (A) { A->GetComponents(Own); Pieces.Append(Own); }
		}
		int32 Strips = 0;
		for (UStaticMeshComponent* C : Pieces)
		{
			bool bStrip = false;
			for (int32 i = 0; C && i < C->GetNumMaterials(); ++i)
			{
				const UMaterialInterface* M = C->GetMaterial(i);
				if (M && M->GetName().StartsWith(TEXT("MI_KitD_Glow")))
				{
					C->CreateDynamicMaterialInstance(i);
					bStrip = true;
				}
			}
			if (bStrip)
			{
				C->ComponentTags.AddUnique(EmergencyStripTag);
				++Strips;
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("Dominion interior: %d kit pieces carry emergency strips (%s)"), Strips, EmergencyStripTag);
	}
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
		UTDStorySubsystem::Fire(this, TEXT("G8_Command_ReactorLost"));
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
		UTDStorySubsystem::Fire(this, TEXT("G9_Kato_HangarConfirmed")); // may be cut by the travel; T4 can repeat it at the flight's start
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
	if (Interactable == FinalRecordConsole)
	{
		UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
		if (FinalRecordElapsed < 0.f)
		{
			// T3's Cinematic 08: the final record and the decision, voices over the shut reactor door (story 07-09).
			FinalRecordElapsed = 0.f;
			FinalRecordQuiet = 0.f;
			if (Story)
			{
				Story->OnSequenceFinished.AddUniqueDynamic(this, &ATDPlanetCourse::HandleStorySequenceFinished);
			}
			bFinalRecordSequence = UTDStorySubsystem::PlaySequence(this, TEXT("Cinematic08"));
			if (ATDInteractableActor* Rec = Cast<ATDInteractableActor>(Interactable))
			{
				Rec->Prompt = LOCTEXT("SkipRecord", "Skip the record");
			}
			OnFinalRecordStarted.Broadcast(Interactable);
		}
		else if (!bFinalRecordDone && Story && Story->IsSequencePlaying(TEXT("Cinematic08")))
		{
			Story->SkipSequence(); // OnSequenceFinished(Cinematic08, true) opens the door
		}
		return;
	}
	UTDStorySubsystem::Fire(this, *Id); // T3's transmissions: Nicholas's words and voices through Presentation
	if (*Id == TEXT("A4_Recording_Corridor"))
	{
		UTDStorySubsystem::Fire(this, TEXT("G7_Moss_MyVoice")); // queued after it (T3)
	}
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
	// Verdance, the survey archive (storyline.md section 16, reduced form; at the relay since decision 2): the transfer
	// starts, the disruption, data secured and the way home.
	// Gameplay 02 (T3's audit): reaching the research site, and the relay deck before its console.
	if (Id == TEXT("Obj_WreckCore") && StepIndex == 1) { Beat(TEXT("G2_Kato_OneLine")); }
	else if (Id == TEXT("Obj_Relay") && StepIndex == 1) { Beat(TEXT("G2_Moss_LightsOn")); }
	else if (Id == TEXT("Obj_Relay") && StepIndex == 2) { Beat(TEXT("G3_TransferStart")); }
	else if (Id == TEXT("Obj_Relay") && StepIndex == 3) { Beat(TEXT("G3_Disruption")); }
	else if (Id == TEXT("Obj_Relay") && StepIndex == 4)
	{
		Beat(TEXT("G3_DataSecured"));
		Beat(TEXT("G3_Command_Extraction"));
		// Cloak with the data secured, not at the pad 4 minutes later (T7's play, Nov 14; script 02). The objective's own
		// completion at the ship then finds it unlocked and does not announce it again.
		if (UTDGameInstance* GI = UTDGameInstance::Get(this); GI && !GI->IsRetakePending())
		{
			GI->UnlockAbility(Objective->RewardAbility, Objective->RewardShip);
		}
	}
	// Cinder (T3's ids): no echo on the route (story alignment a5 / a7); the only echo is in Cinematic 05 at the terminal.
	else if (Id == TEXT("Obj_Survey") && StepIndex == 1) { Beat(TEXT("G4_Kato_TerminalAhead")); }
	else if (Id == TEXT("Obj_Outpost") && StepIndex == 0) { Beat(TEXT("G5_Wren_ClearRoute")); }
	else if (Id == TEXT("Obj_Outpost") && StepIndex == 4) { Beat(TEXT("G5_Moss_Guardian")); }
	else if (Id == TEXT("Obj_Jammer") && StepIndex == 0) { Beat(TEXT("G5_Command_AnchorMarked")); }
	else if (Id == TEXT("Obj_Jammer") && StepIndex == 2) { Beat(TEXT("G5_Wren_BackToShips")); }
	// The Dominion (acts 4 and 5).
	else if (Id == Corridor && StepIndex == 0)
	{
		UTDStorySubsystem::PlaySequence(this, TEXT("FirstDoor")); // A4_DoorsKnowUs, C7_Kato_Archive, C7_Wren_Record (T3)
	}
	else if (Id == Warden && StepIndex == 0) { Beat(TEXT("G7_Kato_Authorisations")); } // the Warden door opens
	else if (Id == Core && StepIndex == 0) { Beat(TEXT("G8_Wren_ConduitsFirst")); OnInteriorMoment.Broadcast(TEXT("ReactorFightStarted"), this); }
	else if (Id == Core && StepIndex == 1)
	{
		Beat(TEXT("G8_Kato_HeartExposed"));
		if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->NotifyCoreExposed(); }
		PlayReactorFX(TEXT("ShieldDrop"), CoreShield ? CoreShield->GetComponentLocation() : (ReactorHeart ? ReactorHeart->GetActorLocation() : GetActorLocation()));
	}
	else if (Id == Escape && StepIndex == 0) { Beat(TEXT("A5_Escape")); Beat(TEXT("G9_Moss_HoldingWayOut")); OnInteriorMoment.Broadcast(TEXT("EscapeStarted"), this);
		APawn* Runner = UGameplayStatics::GetPlayerPawn(this, 0);
		PlayReactorFX(TEXT("EscapeRunLoop"), Runner ? Runner->GetActorLocation() : GetActorLocation(), Runner ? Runner->GetRootComponent() : nullptr, true); }
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
	if (Id == TDInteriorImpl::Core)
	{
		OnInteriorMoment.Broadcast(TEXT("ReactorFightEnded"), this); // the heart's death completes the core
		StopReactorLoop(TEXT("CoreIdle"));
		PlayReactorFX(TEXT("CoreDestroyed"), ReactorHeart ? ReactorHeart->GetActorLocation() : GetActorLocation());
	}
	if (Id == TDInteriorImpl::Escape) { StopReactorLoop(TEXT("EscapeRunLoop")); }
	if (Id == TEXT("Obj_WreckCore")) { Beat(TEXT("A2_DeviceFound")); }
	else if (Id == TEXT("Obj_Relay")) { Beat(TEXT("A2_Voices")); } // static, then human voices that should not be there
	else if (Id == TEXT("Obj_Outpost")) { Beat(TEXT("A3_CrystalsFound")); }
	else if (Id == TEXT("Obj_Jammer"))
	{
		Beat(TEXT("A3_PassageOpen"));
		OnFieldAnchorDisabled.Broadcast(); // T2: the pending approach marker
	}
	if ((Id == TEXT("Obj_WreckCore") || Id == TEXT("Obj_Relay")) && Done(TEXT("Obj_WreckCore")) && Done(TEXT("Obj_Relay")))
	{
		Beat(TEXT("A2_TrailToCinder")); // the data points to Cinder
	}
}

// ------------------------------------------------------------------------------------------------
// The final record (story 07-09, b2): mandatory, at the reactor's threshold, combat held
// ------------------------------------------------------------------------------------------------

bool ATDPlanetCourse::IsFieldAnchorDisabled(const UObject* WorldContextObject)
{
	const UTDGameInstance* GI = UTDGameInstance::Get(WorldContextObject);
	return GI && GI->GetProgressRef().CompletedObjectives.Contains(FName(TEXT("Obj_Jammer")));
}

float ATDPlanetCourse::GetEscapeSecondsLeft() const
{
	return EscapeCollapse && EscapeCollapse->IsRunning() ? EscapeCollapse->GetTimeLeft() : -1.f;
}

void ATDPlanetCourse::UpdateConduitMarker()
{
	// T7 (Nov 14): the conduits carried no marker, only the step counter at the heart. While the conduits are the step,
	// the marker is the nearest one still standing (the counter once none is).
	ATDObjective* Reactor = FindObjective(TDInteriorImpl::Core);
	if (!Reactor || !Reactor->IsActive() || Reactor->GetCurrentStepIndex() != 0 || !Reactor->Steps.IsValidIndex(0))
	{
		return;
	}
	FVector Pilot = GetActorLocation();
	for (TActorIterator<ATDFirstPersonCharacter> It(GetWorld()); It; ++It) { if (!It->IsDead()) { Pilot = It->GetActorLocation(); break; } }
	AActor* Best = nullptr;
	for (const TWeakObjectPtr<AActor>& W : ReactorConduits)
	{
		AActor* C = W.Get();
		const UTDHealthComponent* H = C && !C->IsActorBeingDestroyed() ? UTDStatics::GetHealth(C) : nullptr;
		if (H && H->IsAlive() && (!Best || FVector::DistSquared(Pilot, C->GetActorLocation()) < FVector::DistSquared(Pilot, Best->GetActorLocation())))
		{
			Best = C;
		}
	}
	Reactor->Steps[0].Waypoint = Best ? Best : ReactorConduitCounter.Get();
}

void ATDPlanetCourse::TickCoreChamber()
{
	if (!IsInterior() || !CoreChamberBox.IsValid)
	{
		return;
	}
	UpdateConduitMarker();
	bool bInside = false;
	for (TActorIterator<ATDFirstPersonCharacter> It(GetWorld()); It && !bInside; ++It)
	{
		bInside = !It->IsDead() && CoreChamberBox.IsInsideXY(It->GetActorLocation()); // the on-foot pilot (crew are not first-person characters)
	}
	if (bInside && !bPlayerInCoreChamber)
	{
		OnInteriorMoment.Broadcast(TEXT("CoreChamberEntered"), this); // once per crossing in; a retake outside re-arms it
	}
	bPlayerInCoreChamber = bInside;
}

void ATDPlanetCourse::TickFinalRecord(float DeltaSeconds)
{
	if (!IsInterior() || bFinalRecordDone || FinalRecordElapsed < 0.f)
	{
		return;
	}
	FinalRecordElapsed += DeltaSeconds;
	// The door shows the playback while it waits (the Lead's (a)), counting up.
	const int32 Second = FMath::FloorToInt(FinalRecordElapsed);
	if (ReactorDoor && Second != ShownPlaybackSecond)
	{
		ShownPlaybackSecond = Second;
		ReactorDoor->ShowStatusText(FText::Format(LOCTEXT("Playback", "ARCHIVE PLAYBACK · {0} s"), Second));
	}
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
	bool bWaiting = false;
	if (bFinalRecordSequence && Story)
	{
		bWaiting = Story->IsSequencePlaying(TEXT("Cinematic08")); // OnSequenceFinished normally opens it first
	}
	else
	{
		// No sequence (a missing id): wait for the queue to play out instead.
		bWaiting = Story && (Story->GetCurrentTransmission().bActive || Story->GetQueueLength() > 0);
	}
	FinalRecordQuiet = bWaiting ? 0.f : FinalRecordQuiet + DeltaSeconds;
	if ((FinalRecordElapsed > 1.f && FinalRecordQuiet > 0.75f) || FinalRecordElapsed > FinalRecordMaxSeconds)
	{
		OpenReactorDoor();
	}
}

void ATDPlanetCourse::OpenReactorDoor()
{
	if (bFinalRecordDone)
	{
		return;
	}
	bFinalRecordDone = true;
	if (ReactorDoor)
	{
		ReactorDoor->ClearStatusText();
		ReactorDoor->Open();
	}
	if (UTDStorySubsystem* Story = UTDStorySubsystem::Get(this))
	{
		Story->OnSequenceFinished.RemoveDynamic(this, &ATDPlanetCourse::HandleStorySequenceFinished);
	}
	if (ATDInteractableActor* Rec = Cast<ATDInteractableActor>(FinalRecordConsole))
	{
		Rec->bEnabled = false; // played
	}
	// T3's Cinematic 08 carries the decision lines now (no A5_Choice here).
}

void ATDPlanetCourse::HandleHeartHit(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	if (HullDamage > 0.f || ShieldDamage > 0.f)
	{
		PlayReactorFX(TEXT("HeartHit"), Info.HitLocation.IsNearlyZero() && Health && Health->GetOwner() ? Health->GetOwner()->GetActorLocation() : Info.HitLocation);
	}
}

void ATDPlanetCourse::PlayReactorFX(FName Slot, const FVector& At, USceneComponent* AttachTo, bool bLoop)
{
	ReactorFXLog.Add(Slot);
	const UTDReactorCoreFX* FX = UTDReactorCoreFX::Get();
	const FTDFXCue* Cue = !FX ? nullptr : Slot == TEXT("CoreIdle") ? &FX->CoreIdle : Slot == TEXT("ConduitBreak") ? &FX->ConduitBreak
		: Slot == TEXT("ShieldDrop") ? &FX->ShieldDrop : Slot == TEXT("HeartHit") ? &FX->HeartHit : Slot == TEXT("CoreDestroyed") ? &FX->CoreDestroyed
		: Slot == TEXT("EscapeRunLoop") ? &FX->EscapeRunLoop : Slot == TEXT("LaserSweep") ? &FX->LaserSweep : nullptr;
	if (bLoop)
	{
		StopReactorLoop(Slot);
		UAudioComponent* Audio = nullptr;
		UNiagaraComponent* N = Cue && !Cue->IsEmpty() && AttachTo ? UTDStatics::PlayCueAttachedLoop(*Cue, AttachTo, NAME_None, Audio) : nullptr;
		// The escape alarm: the run's own sound, else the engineering 2D loop (Nick's at the Presentation name wins).
		if (Slot == TEXT("EscapeRunLoop") && !Audio)
		{
			static const TCHAR* AlarmPath = TEXT("/Game/TriadDrift/Gameplay/Audio/Dominion/SW_Dominion_EscapeAlarm_01.SW_Dominion_EscapeAlarm_01");
			USoundBase* Alarm = FX && !FX->EscapeAlarm.IsNull() ? FX->EscapeAlarm.LoadSynchronous() : nullptr;
			if (!Alarm && FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Gameplay/Audio/Dominion/SW_Dominion_EscapeAlarm_01")))
			{
				Alarm = LoadObject<USoundBase>(nullptr, AlarmPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
			}
			Audio = Alarm ? UGameplayStatics::SpawnSound2D(this, Alarm, 1.f, 1.f, 0.f, nullptr, false, false) : nullptr;
		}
		if (N) { ReactorLoopFX.Add(Slot, N); }
		if (Audio) { ReactorLoopAudio.Add(Slot, Audio); }
		return;
	}
	if (Cue && !Cue->IsEmpty())
	{
		UTDStatics::PlayCue(this, *Cue, At);
	}
}

void ATDPlanetCourse::StopReactorLoop(FName Slot)
{
	if (TObjectPtr<UNiagaraComponent>* N = ReactorLoopFX.Find(Slot)) { if (*N) { (*N)->Deactivate(); } ReactorLoopFX.Remove(Slot); }
	if (TObjectPtr<UAudioComponent>* A = ReactorLoopAudio.Find(Slot)) { if (*A) { (*A)->FadeOut(0.5f, 0.f); } ReactorLoopAudio.Remove(Slot); }
}

void ATDPlanetCourse::HandleConduitDown(UTDHealthComponent* Health, const FTDDamageInfo& Info)
{
	++ConduitsDown;
	if (UTDGroundBarks* Barks = UTDGroundBarks::Get(this)) { Barks->NotifyConduitDown(); }
	PlayReactorFX(TEXT("ConduitBreak"), Health && Health->GetOwner() ? Health->GetOwner()->GetActorLocation() : GetActorLocation());
	UTDStorySubsystem::Fire(this, ConduitsDown == 1 ? TEXT("G8_Kato_ConduitDown") : TEXT("G8_Moss_KeepMoving")); // T3: the first, then the second
}

void ATDPlanetCourse::HandleStorySequenceFinished(FName SequenceId, bool bSkipped)
{
	if (SequenceId == TEXT("Cinematic08"))
	{
		OpenReactorDoor(); // finished or skipped (T3: it fires once)
	}
}
