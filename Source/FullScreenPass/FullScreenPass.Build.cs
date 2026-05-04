using UnrealBuildTool;
using System.IO;

public class FullScreenPass : ModuleRules
{
    public FullScreenPass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RHI",
                "Renderer",
                "Projects",
                "RenderCore"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
                Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal")
            }
        );
    }
}
