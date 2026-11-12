using UnrealBuildTool;using System.Collections.Generic;
public class TDAudioV1EditorTarget : TargetRules {
 public TDAudioV1EditorTarget(TargetInfo Target):base(Target){Type=TargetType.Editor;DefaultBuildSettings=BuildSettingsVersion.Latest;IncludeOrderVersion=EngineIncludeOrderVersion.Latest;ExtraModuleNames.Add("TDAudioV1");}
}
