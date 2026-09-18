#include "Data/CabinetDefinition.h"
#include "Data/ScoringProfile.h"
#include "Data/PinballTuningData.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballControlPawn.h"
#include "Camera/CameraComponent.h"
#include "UI/PinballPresentationWidget.h"
#include "Data/MiniGameDefinition.h"

// Development cabinets may select zero/one games; final cabinet validation still requires all three.
bool UCabinetDefinition::Validate(const APinballTable* Table, const APawn* Pawn, FString& Error) const
{
    if (CabinetId.IsNone() || DisplayTitle.IsEmpty() || PersistentMap.IsNull() || !TableClass || !ControlPawnClass ||
        !ScoringProfile || !Tuning || InitialBalls != 3 || ReturnTriggerProtectionSeconds != 1 || ResultsPresentationSeconds != 3 ||
        (!bDevelopmentWithoutMinigames && MiniGames.Num() != 3) || !StartWidget || !HUDWidget || !PauseWidget || !GameOverWidget)
    { Error = TEXT("Incomplete development cabinet: requires three balls, 1s protection, 3s results and explicit content references."); return false; }
    if (!Table || !Table->IsA(TableClass) || Table->Tuning != Tuning || !IsValid(Table->TableCamera) ||
        Table->TableCamera->GetOwner() != Table || !Pawn || !Pawn->IsA(ControlPawnClass))
    { Error = TEXT("Registered table, explicit table camera, tuning or control pawn does not match cabinet."); return false; }
    TSet<FName> IDs;
    for (const auto& Definition : MiniGames)
    {
        if (!Definition || !Definition->Validate(Error) || IDs.Contains(Definition->MiniGameId) ||
            !InstructionsWidget || !MiniGameHUDWidget || !ResultsWidget || !RecoveryWidget) return false;
        IDs.Add(Definition->MiniGameId);
        for (const auto& Other : MiniGames) if (Other && Other != Definition &&
            FBox::BuildAABB(Definition->ArenaTransform.GetLocation(), Definition->ArenaExtent).Intersect(
                FBox::BuildAABB(Other->ArenaTransform.GetLocation(), Other->ArenaExtent)))
        { Error = TEXT("Arena slots overlap."); return false; }
    }
    return ScoringProfile->Validate(Error) && Tuning->Validate(Error);
}
