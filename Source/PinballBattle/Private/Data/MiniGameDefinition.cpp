#include "Data/MiniGameDefinition.h"

// Keep shipping rules strict while allowing a cabinet to select a single independent test definition.
bool UMiniGameDefinition::Validate(FString& Error) const
{
    if (MiniGameId.IsNone() || ScoringProfileKey.IsNone() || DisplayName.IsEmpty() || Instructions.IsEmpty() ||
        Map.IsNull() || RuntimeClass.IsNull() || PawnClass.IsNull() || InputContext.IsNull() || ActionInput.IsNull() || HUDClass.IsNull() ||
        DurationSeconds != 30 || (LocalLives != 0 && LocalLives != 3) ||
        MinimumResultMultiplier != 1 || MaximumResultMultiplier != 1 || MaximumObjectives < 0 || MaximumObjectives > 1000000 ||
        ArenaTransform.ContainsNaN() || ArenaExtent.ContainsNaN() || ArenaExtent.GetMin() <= 0 || MetricBounds.IsEmpty())
    { Error = TEXT("Minigame requires complete soft references, finite bounds, 30 seconds, zero/three lives and multiplier one."); return false; }
    for (const auto& Metric : MetricBounds)
        if (Metric.Key.IsNone() || !FMath::IsFinite(Metric.Value) || Metric.Value < 0 || Metric.Value > 1.e9)
        { Error = TEXT("Invalid minigame metric bound."); return false; }
    const FString MapPath = Map.ToSoftObjectPath().GetLongPackageName();
    if (MapPath.StartsWith(TEXT("/Game/Minigames/")))
    {
        FString Relative = MapPath.RightChop(16), GameFolder;
        if (!Relative.Split(TEXT("/"), &GameFolder, nullptr)) return false;
        const FString Prefix = TEXT("/Game/Minigames/") + GameFolder + TEXT("/");
        for (const FSoftObjectPath& Path : {RuntimeClass.ToSoftObjectPath(), PawnClass.ToSoftObjectPath(), InputContext.ToSoftObjectPath(), HUDClass.ToSoftObjectPath()})
            if (Path.GetLongPackageName().StartsWith(TEXT("/Game/Minigames/")) && !Path.GetLongPackageName().StartsWith(Prefix))
            { Error = TEXT("A minigame cannot reference a sibling minigame's content."); return false; }
    }
    return true;
}
