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

		PrivateDependencyModuleNames.AddRange(new string[] { });

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
