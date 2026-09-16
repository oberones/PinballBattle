#include "Data/CabinetDefinition.h"
#include "Data/ScoringProfile.h"
#include "Data/PinballTuningData.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballControlPawn.h"
#include "Camera/CameraComponent.h"
#include "UI/PinballPresentationWidget.h"

bool UCabinetDefinition::Validate(const APinballTable* Table, const APawn* Pawn, FString& Error) const
{
    if (CabinetId.IsNone() || DisplayTitle.IsEmpty() || PersistentMap.IsNull() || !TableClass || !ControlPawnClass ||
        !ScoringProfile || !Tuning || InitialBalls != 3 || ReturnTriggerProtectionSeconds != 1 || ResultsPresentationSeconds != 3 ||
        !bDevelopmentWithoutMinigames || !StartWidget || !HUDWidget || !PauseWidget || !GameOverWidget)
    { Error = TEXT("Incomplete development cabinet: requires three balls, 1s protection, 3s results and explicit content references."); return false; }
    if (!Table || !Table->IsA(TableClass) || Table->Tuning != Tuning || !IsValid(Table->TableCamera) ||
        Table->TableCamera->GetOwner() != Table || !Pawn || !Pawn->IsA(ControlPawnClass))
    { Error = TEXT("Registered table, explicit table camera, tuning or control pawn does not match cabinet."); return false; }
    return ScoringProfile->Validate(Error) && Tuning->Validate(Error);
}
