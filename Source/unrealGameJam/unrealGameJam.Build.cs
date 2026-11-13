// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class unrealGameJam : ModuleRules
{
	public unrealGameJam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"AssetRegistry",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara",
			"DeveloperSettings",
			"PhysicsCore",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.Add("AudioMixer"); // T1, issue #24: Quartz schedules the tutorial loop sample-aligned to the prelude's end
		PrivateDependencyModuleNames.AddRange(new string[] { "LevelSequence", "MovieScene" }); // ENG-33: UTDUIStyle::IntroSequence
		if (Target.bBuildEditor) { PrivateDependencyModuleNames.Add("UnrealEd"); } // T3 e2e PIE test
		PrivateDependencyModuleNames.Add("ProceduralMeshComponent"); // ENG-34: ATDPlanetTerrain chunk meshes (T4)

		PublicIncludePaths.AddRange(new string[] {
			"unrealGameJam",
			"unrealGameJam/Variant_Horror",
			"unrealGameJam/Variant_Horror/UI",
			"unrealGameJam/Variant_Shooter",
			"unrealGameJam/Variant_Shooter/AI",
			"unrealGameJam/Variant_Shooter/UI",
			"unrealGameJam/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
