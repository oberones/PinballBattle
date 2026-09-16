using UnrealBuildTool;

public class PinballBattle : ModuleRules
{
    public PinballBattle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate", "SlateCore", "PhysicsCore", "Niagara"
        });
    }
}
