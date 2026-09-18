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
#include "Framework/GameFlowComponent.h"
#include "Data/MiniGameDefinition.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include "Framework/Application/SlateApplication.h"

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

// Minigame swaps set their own explicit view target instead of reconfiguring the pinball table.
void APinballPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (Table && !bModeSwap)
    {
        ConfigureTable(Table);
    }
    else if (IsLocalController() && InPawn)
    {
        SetViewTarget(InPawn);
    }
}

// Remove all controller-owned contexts and presentation subscriptions during teardown.
void APinballPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisableMiniGameInput();
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

// Register the persistent pawn and explicit camera once, retaining a return fallback across run possession.
void APinballPlayerController::ConfigureTable(APinballTable* InTable)
{
    Table = InTable;
    if (!IsLocalController() || !Table) return;
    if (APinballControlPawn* ControlPawn = Cast<APinballControlPawn>(GetPawn())) { ControlPawn->SetTable(Table); PersistentPinballPawn = ControlPawn; }
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

// Public state plus internal transition phase select screens; clocks and awards remain in flow.
void APinballPlayerController::RefreshPresentation(const FSessionState& State)
{
    if (!IsLocalController() || (State.FlowState == LastPresentedState && State.FlowState != EArcadeGameFlowState::MINIGAME_TRANSITION && State.FlowState != EArcadeGameFlowState::BOOT)) return;
    const auto Previous = LastPresentedState;
    LastPresentedState = State.FlowState;
    const bool bGameplay = State.FlowState == EArcadeGameFlowState::PINBALL_READY || State.FlowState == EArcadeGameFlowState::PINBALL_PLAYING;
    // A normal launch preserves held flippers; menu boundaries require a fresh press.
    if (!(Previous == EArcadeGameFlowState::PINBALL_READY && State.FlowState == EArcadeGameFlowState::PINBALL_PLAYING))
        SetGameplayInput(bGameplay);
    const auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    const auto* Cabinet = Mode ? Mode->GetCabinet() : nullptr;
    if (!Cabinet) return;
    const auto* Flow = Mode->FindComponentByClass<UGameFlowComponent>();
    TSubclassOf<UPinballPresentationWidget> Class = Cabinet->HUDWidget;
    if (State.FlowState == EArcadeGameFlowState::ATTRACT) Class = Cabinet->StartWidget;
    if (State.FlowState == EArcadeGameFlowState::PAUSED) Class = Cabinet->PauseWidget;
    if (State.FlowState == EArcadeGameFlowState::GAME_OVER) Class = Cabinet->GameOverWidget;
    if (State.FlowState == EArcadeGameFlowState::MINIGAME_TRANSITION || State.FlowState == EArcadeGameFlowState::BOOT) Class = Cabinet->InstructionsWidget;
    if (State.FlowState == EArcadeGameFlowState::MINIGAME_TRANSITION && Flow->GetTransition().Phase == ETransitionPhase::RecoveryMenu) Class = Cabinet->RecoveryWidget;
    if (State.FlowState == EArcadeGameFlowState::BOOT && Flow->HasBootFailed()) Class = Cabinet->RecoveryWidget;
    if (State.FlowState == EArcadeGameFlowState::MINIGAME_PLAYING) Class = Cabinet->MiniGameHUDWidget;
    if (State.FlowState == EArcadeGameFlowState::MINIGAME_RESULTS) Class = Cabinet->ResultsWidget;
    if (!Class) Class = Cabinet->HUDWidget;
    if (!Presentation || Presentation->GetClass() != Class)
    {
        if (Presentation) Presentation->RemoveFromParent();
        Presentation = CreateWidget<UPinballPresentationWidget>(this, Class);
        if (Presentation) Presentation->AddToViewport(10);
    }
    if (State.FlowState == EArcadeGameFlowState::PAUSED) DisableMiniGameInput();
    if (Previous == EArcadeGameFlowState::PAUSED && State.FlowState == EArcadeGameFlowState::MINIGAME_PLAYING)
        EnableMiniGameInput(GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->GetActiveRun().Definition);
    bShowMouseCursor = !bGameplay && State.FlowState != EArcadeGameFlowState::MINIGAME_PLAYING;
    if (bGameplay || State.FlowState == EArcadeGameFlowState::MINIGAME_PLAYING) SetInputMode(FInputModeGameOnly());
    else
    {
        FInputModeGameAndUI InputMode;
        if (Presentation) InputMode.SetWidgetToFocus(Presentation->TakeWidget());
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
}

// Capture occurs before possession/context mutation, retaining weak persistent targets.
bool APinballPlayerController::CaptureMiniGameMode(int64 Generation, FControllerModeSnapshot& S)
{
    if (!Table || !GetPawn() || !GetViewTarget()) return false;
    S.Pawn = GetPawn(); S.ViewTarget = GetViewTarget(); S.GameplayContext = PinballMappingContext;
    S.bShowCursor = bShowMouseCursor; S.Generation = Generation; ModeGeneration = Generation;
    if (FSlateApplication::IsInitialized()) S.Focus = FSlateApplication::Get().GetUserFocusedWidget(0);
    CancelActions(); SetGameplayInput(false);
    return true;
}
// A camera cut avoids blending across the spatial gap between table and arena.
bool APinballPlayerController::SwitchToMiniGame(AMiniGameRuntimeBase* Runtime)
{
    if (!IsValid(Runtime) || !Runtime->HasPresentationTargets()) return false;
    TGuardValue<bool> Guard(bModeSwap, true); MiniGameRuntime = Runtime; Possess(Runtime->GetRunPawn()); SetViewTarget(Runtime);
    bFreshMiniGameInput = false; bSpaceWasDown = IsInputKeyDown(EKeys::SpaceBar); bEnterWasDown = IsInputKeyDown(EKeys::Enter);
    return GetPawn() == Runtime->GetRunPawn() && GetViewTarget() == Runtime;
}
// Immediate Enhanced Input rebuild suppresses held Boolean keys; PlayerTick also requires neutral axes.
bool APinballPlayerController::EnableMiniGameInput(const UMiniGameDefinition* D)
{
    auto* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent);
    if (!Input || !Enhanced || !D || !D->InputContext.Get() || !D->ActionInput.Get() || !MiniGameRuntime.IsValid() || GetPawn() != MiniGameRuntime->GetRunPawn()) return false;
    DisableMiniGameInput(); MiniGameMappingContext = D->InputContext.Get();
    FModifyContextOptions Options; Options.bForceImmediately = true; Options.bIgnoreAllPressedKeysUntilRelease = true;
    Input->AddMappingContext(MiniGameMappingContext, 0, Options); bFreshMiniGameInput = false;
    MiniGameActionBinding = Enhanced->BindAction(D->ActionInput.Get(), ETriggerEvent::Started, this, &ThisClass::MiniGameActionPressed).GetHandle();
    return Input->HasMappingContext(MiniGameMappingContext);
}
// Common mappings remain installed so Escape/menu work in every phase.
void APinballPlayerController::DisableMiniGameInput()
{
    if (MiniGameActionBinding) if (auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent)) Enhanced->RemoveBindingByHandle(MiniGameActionBinding);
    MiniGameActionBinding = 0;
    if (auto* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        if (MiniGameMappingContext) Input->RemoveMappingContext(MiniGameMappingContext);
    MiniGameMappingContext = nullptr; bFreshMiniGameInput = false;
}
// Releasing the arena's pawn and camera is a prerequisite for registry cleanup.
bool APinballPlayerController::RestoreMiniGameMode(const FControllerModeSnapshot& S)
{
    if (!IsValid(Table) || S.Generation != ModeGeneration) return false;
    APawn* ReturnPawn = S.Pawn.IsValid() ? S.Pawn.Get() : PersistentPinballPawn.Get();
    if (!ReturnPawn)
    {
        const auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
        const auto* Cabinet = Mode ? Mode->GetCabinet() : nullptr;
        if (!Cabinet || !Cabinet->ControlPawnClass) return false;
        auto* Replacement = GetWorld()->SpawnActor<APinballControlPawn>(Cabinet->ControlPawnClass, Table->GetActorTransform());
        if (!Replacement) return false;
        Replacement->SetTable(Table); PersistentPinballPawn = Replacement; ReturnPawn = Replacement;
    }
    DisableMiniGameInput(); TGuardValue<bool> Guard(bModeSwap, true);
    Possess(ReturnPawn); SetViewTarget(S.ViewTarget.IsValid() ? S.ViewTarget.Get() : Table.Get());
    bShowMouseCursor = S.bShowCursor; MiniGameRuntime.Reset();
    if (S.Focus.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetUserFocus(0, S.Focus.Pin());
    return GetPawn() == ReturnPawn && GetViewTarget() != nullptr;
}
// The visible status is a projection of flow and runtime progress, never a source of timing truth.
void APinballPlayerController::UpdateMiniGameStatus(const FText& Status)
{
    if (Presentation && LastPresentedState != EArcadeGameFlowState::PAUSED) Presentation->ShowMiniGameStatus(Status);
}
// Poll physical releases to consume confirmation and block held Space/arrows across both boundaries.
void APinballPlayerController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    auto* Flow = Mode ? Mode->FindComponentByClass<UGameFlowComponent>() : nullptr;
    const bool Space = IsInputKeyDown(EKeys::SpaceBar), Enter = IsInputKeyDown(EKeys::Enter);
    const bool AxesNeutral = AreMiniGameAxesNeutral();
    const bool Neutral = !Space && !Enter && AxesNeutral;
    if (Flow && Flow->GetCurrentState() != EArcadeGameFlowState::PAUSED)
    {
        if (Neutral) bFreshMiniGameInput = true;
        if (bFreshMiniGameInput && ((Space && !bSpaceWasDown) || (Enter && !bEnterWasDown)))
        {
            if (AxesNeutral && Flow->GetTransition().Phase == ETransitionPhase::AwaitingConfirmation) { bFreshMiniGameInput = false; Flow->ConfirmMiniGame(); }
        }
    }
    bSpaceWasDown = Space; bEnterWasDown = Enter;
}
// Confirmation never calls this handler; only the installed minigame Enhanced Input context can.
void APinballPlayerController::MiniGameActionPressed()
{
    if (IsMiniGameInputReady() && MiniGameRuntime.IsValid()) MiniGameRuntime->SubmitAction();
}
// Unlike Boolean held-key suppression, analog axes require an explicit physical-neutral observation.
bool APinballPlayerController::AreMiniGameAxesNeutral() const
{
    if (IsInputKeyDown(EKeys::Left) || IsInputKeyDown(EKeys::Right) || IsInputKeyDown(EKeys::Down) || IsInputKeyDown(EKeys::Up)) return false;
    if (MiniGameMappingContext) for (const auto& Mapping : MiniGameMappingContext->GetMappings())
        if (Mapping.Action && Mapping.Action->ValueType != EInputActionValueType::Boolean &&
            FMath::Abs(GetInputAnalogKeyState(Mapping.Key)) > .01f) return false;
    return true;
}
// Explicit recovery retry is separate from restart and cannot silently discard progress.
void APinballPlayerController::RequestRetryIntent()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>()) Mode->FindComponentByClass<UGameFlowComponent>()->RetryReturn();
}

void APinballPlayerController::RequestStartIntent()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>())
    {
        FText Failure;
        if (!Mode->RequestNewSession(&Failure) && !Failure.IsEmpty() && Presentation)
            Presentation->ShowStartFailure(Failure);
    }
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
