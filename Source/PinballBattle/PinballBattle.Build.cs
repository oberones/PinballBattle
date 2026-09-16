using UnrealBuildTool;

public class PinballBattle : ModuleRules
{
    public PinballBattle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "UMG", "Slate", "SlateCore", "PhysicsCore", "Niagara"
        });
    }
}
