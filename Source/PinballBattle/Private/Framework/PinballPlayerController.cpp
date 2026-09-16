#include "Framework/PinballPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Pinball/PinballControlPawn.h"
#include "Pinball/PinballTable.h"
#include "Framework/PinballGameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "PinballBattle.h"
#include "Framework/PinballGameStateBase.h"
#include "Data/CabinetDefinition.h"
#include "UI/PinballPresentationWidget.h"
#include "Kismet/KismetSystemLibrary.h"

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
    if (Table)
    {
        ConfigureTable(Table);
    }
    else if (GetPawn())
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
    if (Table)
    {
        ConfigureTable(Table);
    }
    else if (IsLocalController() && InPawn)
    {
        SetViewTarget(InPawn);
    }
}

void APinballPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelActions();
    if (ControlsWidget) ControlsWidget->RemoveFromParent();
    if (Presentation) Presentation->RemoveFromParent();
    if (auto* State = GetWorld()->GetGameState<APinballGameStateBase>())
        State->OnSessionChanged.RemoveDynamic(this, &ThisClass::RefreshPresentation);
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

void APinballPlayerController::ConfigureTable(APinballTable* InTable)
{
    Table = InTable;
    if (!IsLocalController() || !Table) return;
    if (APinballControlPawn* ControlPawn = Cast<APinballControlPawn>(GetPawn())) ControlPawn->SetTable(Table);
    SetViewTarget(Table);
    const auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (auto* State = GetWorld()->GetGameState<APinballGameStateBase>())
        State->OnSessionChanged.AddUniqueDynamic(this, &ThisClass::RefreshPresentation);
    if (Mode && Mode->IsPracticeMode() && !ControlsWidget && Table->ControlsWidgetClass)
    {
        ControlsWidget = CreateWidget<UUserWidget>(this, Table->ControlsWidgetClass);
        if (ControlsWidget) ControlsWidget->AddToViewport();
    }
}

void APinballPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
    if (!ensure(Input && LeftFlipperAction && RightFlipperAction && PlungerAction)) return;
    if (PauseAction) Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ThisClass::RequestPauseIntent);
    Input->BindAction(LeftFlipperAction, ETriggerEvent::Started, this, &ThisClass::LeftPressed);
    Input->BindAction(LeftFlipperAction, ETriggerEvent::Completed, this, &ThisClass::LeftReleased);
    Input->BindAction(LeftFlipperAction, ETriggerEvent::Canceled, this, &ThisClass::LeftReleased);
    Input->BindAction(RightFlipperAction, ETriggerEvent::Started, this, &ThisClass::RightPressed);
    Input->BindAction(RightFlipperAction, ETriggerEvent::Completed, this, &ThisClass::RightReleased);
    Input->BindAction(RightFlipperAction, ETriggerEvent::Canceled, this, &ThisClass::RightReleased);
    Input->BindAction(PlungerAction, ETriggerEvent::Started, this, &ThisClass::PlungerPressed);
    Input->BindAction(PlungerAction, ETriggerEvent::Completed, this, &ThisClass::PlungerReleased);
    Input->BindAction(PlungerAction, ETriggerEvent::Canceled, this, &ThisClass::CancelActions);
}

void APinballPlayerController::CancelActions()
{
    if (APinballControlPawn* ControlPawn = Cast<APinballControlPawn>(GetPawn())) ControlPawn->CancelActions();
}

void APinballPlayerController::OnUnPossess()
{
    CancelActions();
    Super::OnUnPossess();
}

void APinballPlayerController::LeftPressed() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->SetLeftHeld(true); }
void APinballPlayerController::LeftReleased() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->SetLeftHeld(false); }
void APinballPlayerController::RightPressed() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->SetRightHeld(true); }
void APinballPlayerController::RightReleased() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->SetRightHeld(false); }
void APinballPlayerController::PlungerPressed() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->BeginPlunger(); }
void APinballPlayerController::PlungerReleased() { if (auto* P = Cast<APinballControlPawn>(GetPawn())) P->ReleasePlunger(); }

void APinballPlayerController::SetGameplayInput(bool bEnabled)
{
    CancelActions();
    if (auto* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        FModifyContextOptions Options;
        Options.bForceImmediately = true;
        Options.bIgnoreAllPressedKeysUntilRelease = true;
        Input->RemoveMappingContext(PinballMappingContext, Options);
        if (bEnabled) Input->AddMappingContext(PinballMappingContext, 0, Options);
        // Preserve the common Escape action's ongoing press; flushing it would retrigger pause on resume.
        Input->RequestRebuildControlMappings(Options, EInputMappingRebuildType::Rebuild);
    }
}

void APinballPlayerController::RefreshPresentation(const FSessionState& State)
{
    if (!IsLocalController() || State.FlowState == LastPresentedState) return;
    const auto Previous = LastPresentedState;
    LastPresentedState = State.FlowState;
    const bool bGameplay = State.FlowState == EArcadeGameFlowState::PINBALL_READY || State.FlowState == EArcadeGameFlowState::PINBALL_PLAYING;
    // A normal launch preserves held flippers; menu boundaries require a fresh press.
    if (!(Previous == EArcadeGameFlowState::PINBALL_READY && State.FlowState == EArcadeGameFlowState::PINBALL_PLAYING))
        SetGameplayInput(bGameplay);
    const auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    const auto* Cabinet = Mode ? Mode->GetCabinet() : nullptr;
    if (!Cabinet) return;
    TSubclassOf<UPinballPresentationWidget> Class = Cabinet->HUDWidget;
    if (State.FlowState == EArcadeGameFlowState::ATTRACT) Class = Cabinet->StartWidget;
    if (State.FlowState == EArcadeGameFlowState::PAUSED) Class = Cabinet->PauseWidget;
    if (State.FlowState == EArcadeGameFlowState::GAME_OVER) Class = Cabinet->GameOverWidget;
    if (!Presentation || Presentation->GetClass() != Class)
    {
        if (Presentation) Presentation->RemoveFromParent();
        Presentation = CreateWidget<UPinballPresentationWidget>(this, Class);
        if (Presentation) Presentation->AddToViewport(10);
    }
    bShowMouseCursor = !bGameplay;
    if (bGameplay) SetInputMode(FInputModeGameOnly());
    else
    {
        FInputModeGameAndUI InputMode;
        if (Presentation) InputMode.SetWidgetToFocus(Presentation->TakeWidget());
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
}

void APinballPlayerController::RequestStartIntent()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>()) Mode->RequestNewSession();
}

void APinballPlayerController::RequestPauseIntent()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>()) Mode->RequestTogglePause(this);
}

void APinballPlayerController::RequestQuitIntent()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>()) Mode->InvalidateSession();
    CancelActions();
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
