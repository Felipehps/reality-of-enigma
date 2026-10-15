using UnrealBuildTool;
public class RealityOfEnigmaUE:ModuleRules { public RealityOfEnigmaUE(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","Niagara","ProceduralMeshComponent","AudioMixer","Json","JsonUtilities"}); } }
