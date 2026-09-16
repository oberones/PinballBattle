#include "Framework/PinballPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "PinballBattle.h"

APinballPlayerController::APinballPlayerController()
{
    bAutoManageActiveCameraTarget = false;
}

void APinballPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController())
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (!ensureMsgf(Input && CommonMappingContext && PinballMappingContext,
        TEXT("Configure both Enhanced Input contexts on the controller Blueprint")))
    {
        return;
    }

    FModifyContextOptions Options;
    Options.bForceImmediately = true;
    Options.bIgnoreAllPressedKeysUntilRelease = true;
    Input->AddMappingContext(CommonMappingContext, 100, Options);
    Input->AddMappingContext(PinballMappingContext, 0, Options);
    SetInputMode(FInputModeGameOnly());
    if (GetPawn())
    {
        SetViewTarget(GetPawn());
    }
    UE_LOG(LogPinballBattle, Log, TEXT("Foundation input ready: Common=%s (%d mappings), Pinball=%s (%d mappings), Pawn=%s, View=%s"),
        *CommonMappingContext->GetName(), CommonMappingContext->GetMappings().Num(),
        *PinballMappingContext->GetName(), PinballMappingContext->GetMappings().Num(),
        *GetNameSafe(GetPawn()), *GetNameSafe(GetViewTarget()));
}

void APinballPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (IsLocalController() && InPawn)
    {
        SetViewTarget(InPawn);
    }
}

void APinballPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (CommonMappingContext)
        {
            Input->RemoveMappingContext(CommonMappingContext);
        }
        if (PinballMappingContext)
        {
            Input->RemoveMappingContext(PinballMappingContext);
        }
    }
    Super::EndPlay(EndPlayReason);
}
