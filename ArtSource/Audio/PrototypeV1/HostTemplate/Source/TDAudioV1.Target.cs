using UnrealBuildTool;using System.Collections.Generic;
public class TDAudioV1Target : TargetRules {
 public TDAudioV1Target(TargetInfo Target):base(Target){Type=TargetType.Game;DefaultBuildSettings=BuildSettingsVersion.Latest;IncludeOrderVersion=EngineIncludeOrderVersion.Latest;ExtraModuleNames.Add("TDAudioV1");}
}
