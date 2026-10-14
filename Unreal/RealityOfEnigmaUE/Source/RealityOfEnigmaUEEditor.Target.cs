using UnrealBuildTool;
public class RealityOfEnigmaUEEditorTarget:TargetRules { public RealityOfEnigmaUEEditorTarget(TargetInfo Target):base(Target) { Type=TargetType.Editor;DefaultBuildSettings=BuildSettingsVersion.Latest;IncludeOrderVersion=EngineIncludeOrderVersion.Latest;ExtraModuleNames.Add("RealityOfEnigmaUE"); } }
