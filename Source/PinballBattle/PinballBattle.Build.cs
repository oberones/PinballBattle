using UnrealBuildTool;

public class PinballBattle : ModuleRules
{
    // Include functional fixtures and editor-only latent automation support for phase acceptance.
    public PinballBattle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "FunctionalTesting"
        });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.Add("UnrealEd");
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate", "SlateCore", "PhysicsCore", "Niagara"
        });
    }
}
