using UnrealBuildTool;
public class TDAudioV1 : ModuleRules {
 public TDAudioV1(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","AudioMixer","Json","JsonUtilities"});
 }
}
