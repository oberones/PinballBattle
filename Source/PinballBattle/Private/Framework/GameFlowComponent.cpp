#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "PinballBattle.h"
#include "Framework/PinballPlayerController.h"
#include "Framework/PinballScoringComponent.h"
#include "Data/CabinetDefinition.h"
#include "Data/MiniGameDefinition.h"
#include "Pinball/PinballTable.h"
#include "Pinball/TableSessionComponent.h"
#include "Pinball/MinigameTriggerComponent.h"
#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "Engine/World.h"

// Transition commits run before physics, never inside a collision callback.
UGameFlowComponent::UGameFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UGameFlowComponent::InitializeProjection(APinballGameStateBase* InGameState)
{
    if (!ensureMsgf(Cast<APinballGameModeBase>(GetOwner()) && IsValid(InGameState),
        TEXT("Flow requires a project GameMode owner and GameState projection")))
    {
        return;
    }

    GameState = InGameState;
    GameState->SessionState.FlowState = CurrentState;
    UE_LOG(LogPinballBattle, Log, TEXT("Flow initialized: BOOT"));
}

// Extend the original session edges with the guarded generic round trip and recovery return.
bool UGameFlowComponent::IsLegalTransition(EArcadeGameFlowState From, EArcadeGameFlowState Next)
{
    using E = EArcadeGameFlowState;
    return (From == E::BOOT && Next == E::ATTRACT) ||
        (From == E::ATTRACT && Next == E::PINBALL_READY) ||
        (From == E::PINBALL_READY && Next == E::PINBALL_PLAYING) ||
        (From == E::PINBALL_PLAYING && Next == E::BALL_LOST) ||
        (From == E::BALL_LOST && (Next == E::PINBALL_READY || Next == E::GAME_OVER)) ||
        (From == E::GAME_OVER && Next == E::PINBALL_READY) ||
        (From == E::PINBALL_PLAYING && Next == E::MINIGAME_TRANSITION) ||
        (From == E::MINIGAME_TRANSITION && (Next == E::MINIGAME_PLAYING || Next == E::PINBALL_PLAYING || Next == E::PINBALL_READY)) ||
        (From == E::MINIGAME_PLAYING && (Next == E::MINIGAME_RESULTS || Next == E::MINIGAME_TRANSITION)) ||
        (From == E::MINIGAME_RESULTS && Next == E::MINIGAME_TRANSITION);
}

// Use the single local controller already selected by the GameMode.
APinballPlayerController* UGameFlowComponent::GetController() const
{ return Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController()); }

// Boot has a bounded active-time deadline; streaming completion never advances flow by callback.
void UGameFlowComponent::BeginMiniGameBoot()
{
    auto* Mode = Cast<APinballGameModeBase>(GetOwner());
    const auto* Cabinet = Mode ? Mode->GetCabinet() : nullptr;
    if (!Cabinet || Cabinet->MiniGames.IsEmpty()) { TransitionTo(EArcadeGameFlowState::ATTRACT); return; }
    if (bInvalidObjectives) { bBootFailed = true; GameState->PublishSession(); return; }
    if (bBootFailed) GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->ResetPreload();
    bBootLoading = GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->Preload(Cabinet->MiniGames);
    bBootFailed = !bBootLoading; BootSeconds = 0;
    GameState->PublishSession();
}

// Duplicate authored identities invalidate boot rather than ambiguously sharing one live objective state.
bool UGameFlowComponent::RegisterObjective(UMinigameTriggerComponent* Trigger)
{
    if (!Trigger || Trigger->ObjectiveId.IsNone() || Objectives.Contains(Trigger->ObjectiveId))
    { bInvalidObjectives = true; return false; }
    Objectives.Add(Trigger->ObjectiveId, Trigger); return true;
}

// Exiting is necessary but insufficient: the active-time return guard still controls availability.
void UGameFlowComponent::NotifyObjectiveExit(UMinigameTriggerComponent* Trigger)
{
    if (!GameState || !Trigger || Objectives.FindRef(Trigger->ObjectiveId).Get() != Trigger) return;
    if (auto* Objective = GameState->SessionState.ObjectiveStates.Find(Trigger->ObjectiveId)) Objective->bExited = true;
}

// Identity and physical reentry are checked before closing all competing table events atomically.
bool UGameFlowComponent::RequestObjective(UMinigameTriggerComponent* Trigger, FGuid SessionId, FGuid BallId, FGuid EventId)
{
    auto* Mode = Cast<APinballGameModeBase>(GetOwner());
    auto* Table = Mode ? Mode->GetTable() : nullptr;
    if (!GameState || !Table || !Trigger || Trigger->Table != Table || !Trigger->Definition || !EventId.IsValid() ||
        CurrentState != EArcadeGameFlowState::PINBALL_PLAYING || Transition.Phase != ETransitionPhase::None ||
        Objectives.FindRef(Trigger->ObjectiveId).Get() != Trigger ||
        ObjectiveProtectionRemaining > 0 || !Trigger->IsArmed() || !Table->CanEmitEvent(Table->GetBall()) ||
        GameState->SessionState.SessionId != SessionId || Table->GetBallHandle().BallId != BallId ||
        !Mode->GetCabinet()->MiniGames.Contains(Trigger->Definition)) return false;
    Transition = FTransitionRecord(); Definition = Trigger->Definition; ActiveTrigger = Trigger;
    auto* Subsystem = GetWorld()->GetSubsystem<UMinigameWorldSubsystem>();
    Subsystem->OnRunCancelled.RemoveAll(this);
    Subsystem->OnRunCancelled.AddUObject(this, &ThisClass::HandleRunCancelled);
    auto& C = Transition.Context; C.SessionId = SessionId; C.RunId = FGuid::NewGuid();
    C.Generation = ++GameState->SessionState.Generation; C.MiniGameId = Definition->MiniGameId;
    C.DurationLimit = Definition->DurationSeconds; C.StartingLives = Definition->LocalLives;
    C.SessionMultiplier = GameState->SessionState.Multiplier; C.MetricBounds = Definition->MetricBounds;
    C.MinimumResultMultiplier = Definition->MinimumResultMultiplier; C.MaximumResultMultiplier = Definition->MaximumResultMultiplier;
    C.MaximumObjectives = Definition->MaximumObjectives; C.ArenaTransform = Definition->ArenaTransform; C.ArenaExtent = Definition->ArenaExtent;
    if (!GameState->Scoring->CaptureMiniGameProfile(Definition->ScoringProfileKey, C) || !C.IsValid())
    { Transition = FTransitionRecord(); Definition = nullptr; ActiveTrigger.Reset(); return false; }
    Transition.BallId = BallId; Transition.ObjectiveId = Trigger->ObjectiveId;
    Trigger->MarkAccepted(); Table->SecureForMiniGame();
    Transition.Phase = ETransitionPhase::Securing;
    auto& Objective = GameState->SessionState.ObjectiveStates.FindOrAdd(Trigger->ObjectiveId);
    Objective.MiniGameId = C.MiniGameId; Objective.bAvailable = false; Objective.bExited = false; Objective.LastRunId = C.RunId;
    TransitionTo(EArcadeGameFlowState::MINIGAME_TRANSITION);
    UE_LOG(LogPinballBattle, Display, TEXT("Minigame accepted session=%s ball=%s run=%s generation=%lld"),
        *SessionId.ToString(), *BallId.ToString(), *C.RunId.ToString(), C.Generation);
    return true;
}

// Internal phase publication drives UI even when the public transition state stays unchanged.
void UGameFlowComponent::SetPhase(ETransitionPhase Phase)
{
    Transition.Phase = Phase; Transition.PhaseSeconds = 0;
    if (GameState) GameState->PublishSession();
}

// Human waiting has no deadline; its confirming key never reaches runtime gameplay.
bool UGameFlowComponent::ConfirmMiniGame()
{
    if (CurrentState != EArcadeGameFlowState::MINIGAME_TRANSITION || Transition.Phase != ETransitionPhase::AwaitingConfirmation ||
        !Runtime.IsValid() || !GetController()) return false;
    if (!GetController()->EnableMiniGameInput(Definition) || !Runtime->SetPresentationReady(true))
    { Recover(EMiniGameEndReason::StartFailed); return false; }
    Transition.Phase = ETransitionPhase::Playing;
    TransitionTo(EArcadeGameFlowState::MINIGAME_PLAYING);
    if (!Runtime->StartMiniGame()) { Recover(EMiniGameEndReason::StartFailed); return false; }
    return true;
}

// Result identity and lifecycle are revalidated independently of the runtime's terminal guard.
void UGameFlowComponent::HandleMiniGameEnded(const FMiniGameResult& R)
{
    if (!GameState || CurrentState != EArcadeGameFlowState::MINIGAME_PLAYING || Transition.bTerminal ||
        !R.Matches(Transition.Context) || GameState->SessionState.Generation != R.Generation ||
        !Runtime.IsValid() || Runtime->GetLifecycle() != EMiniGameLifecycleState::Ended) return;
    if (!R.Validate(Transition.Context) || R.EndReason == EMiniGameEndReason::RuntimeFailed ||
        R.EndReason == EMiniGameEndReason::StartFailed || R.EndReason == EMiniGameEndReason::Cancelled)
    { Recover(R.Validate(Transition.Context) ? R.EndReason : EMiniGameEndReason::RuntimeFailed); return; }
    Transition.bTerminal = true;
    GameState->Scoring->EvaluateAndAwardMiniGame(R, Transition.Context, Transition.Award);
    if (GetController()) GetController()->DisableMiniGameInput();
    GameState->SessionState.ObjectiveStates.FindOrAdd(Transition.ObjectiveId).bCompleted = true;
    Transition.Phase = ETransitionPhase::Results; Transition.PhaseSeconds = 0;
    TransitionTo(EArcadeGameFlowState::MINIGAME_RESULTS);
}

// Advance only active time; paused streaming can update residency but never execute continuation work.
void UGameFlowComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Dt, TickType, ThisTick);
    if (!GameState || CurrentState == EArcadeGameFlowState::PAUSED || GetWorld()->IsPaused()) return;
    auto* Mode = Cast<APinballGameModeBase>(GetOwner());
    auto* Subsystem = GetWorld()->GetSubsystem<UMinigameWorldSubsystem>();
    if (PendingCancellation.IsSet())
    {
        const auto Reason = PendingCancellation.GetValue(); PendingCancellation.Reset(); Recover(Reason);
    }
    if (bBootLoading)
    {
        if (GetController()) GetController()->UpdateMiniGameStatus(GetMiniGameStatus());
        BootSeconds += Dt; bool Ready = true;
        for (const auto& D : Mode->GetCabinet()->MiniGames) Ready &= Subsystem->IsReady(D);
        if (Ready) { bBootLoading = false; TransitionTo(EArcadeGameFlowState::ATTRACT); }
        else if (BootSeconds >= 30) { bBootLoading = false; bBootFailed = true; GameState->PublishSession(); }
        return;
    }
    if (bBootFailed) { if (GetController()) GetController()->UpdateMiniGameStatus(GetMiniGameStatus()); return; }
    ObjectiveProtectionRemaining = FMath::Max(0., ObjectiveProtectionRemaining - Dt);
    if (CurrentState == EArcadeGameFlowState::PINBALL_PLAYING && ObjectiveProtectionRemaining == 0)
        for (auto& Entry : GameState->SessionState.ObjectiveStates) Entry.Value.bAvailable = Entry.Value.bExited;
    if (Transition.Phase == ETransitionPhase::None || Transition.Phase == ETransitionPhase::RecoveryMenu) return;
    Transition.PhaseSeconds += Dt;
    if (Transition.Phase == ETransitionPhase::Securing || Transition.Phase == ETransitionPhase::Preparing)
    {
        Transition.WatchdogSeconds += Dt;
        if (Transition.WatchdogSeconds >= 5) { Recover(EMiniGameEndReason::StartFailed); return; }
    }
    switch (Transition.Phase)
    {
    case ETransitionPhase::Securing:
        if (!GetController() || !GetController()->CaptureMiniGameMode(Transition.Context.Generation, Transition.Controller) ||
            !Mode->GetTable()->GetSession()->CaptureAndSuspend(Transition.Context.Generation, Transition.Table))
        { Recover(EMiniGameEndReason::StartFailed); break; }
        SetPhase(ETransitionPhase::Preparing); break;
    case ETransitionPhase::Preparing:
        Runtime = Subsystem->Prepare(Definition, Transition.Context);
        if (!Runtime.IsValid() || !GetController()->SwitchToMiniGame(Runtime.Get())) { Recover(EMiniGameEndReason::StartFailed); break; }
        Runtime->OnMiniGameEnded.AddUniqueDynamic(this, &ThisClass::HandleMiniGameEnded);
        SetPhase(ETransitionPhase::AwaitingConfirmation); break;
    case ETransitionPhase::Playing:
        if (!Runtime.IsValid() || !Runtime->HasPresentationTargets()) Recover(EMiniGameEndReason::RuntimeFailed);
        break;
    case ETransitionPhase::Results:
        if (Transition.PhaseSeconds >= Mode->GetCabinet()->ResultsPresentationSeconds)
        { TransitionTo(EArcadeGameFlowState::MINIGAME_TRANSITION, false); SetPhase(ETransitionPhase::Returning); }
        break;
    case ETransitionPhase::Recovering:
        if (Transition.PhaseSeconds >= 2) SetPhase(ETransitionPhase::Returning);
        break;
    case ETransitionPhase::Returning:
        if (!TryReturn() && Transition.PhaseSeconds >= 5) SetPhase(ETransitionPhase::RecoveryMenu);
        break;
    default: break;
    }
    if (GetController()) GetController()->UpdateMiniGameStatus(GetMiniGameStatus());
}

// The subsystem reports cancellation without owning flow; paused notifications are consumed once on resume.
void UGameFlowComponent::HandleRunCancelled(const FMiniGameContext& Context, EMiniGameEndReason Reason)
{
    if (!GameState || Context.RunId != Transition.Context.RunId || Context.SessionId != GameState->SessionState.SessionId ||
        Context.Generation != GameState->SessionState.Generation || Transition.Phase == ETransitionPhase::None) return;
    if (CurrentState == EArcadeGameFlowState::PAUSED) PendingCancellation = Reason;
    else Recover(Reason);
}

// Invalidate callbacks first; preserve an already awarded result if cleanup fails later.
void UGameFlowComponent::Recover(EMiniGameEndReason Reason)
{
    if (Transition.Phase == ETransitionPhase::Recovering || Transition.Phase == ETransitionPhase::RecoveryMenu) return;
    ++GameState->SessionState.Generation;
    if (!Transition.bTerminal)
    {
        Transition.bTerminal = true;
        GameState->Scoring->EvaluateAndAwardMiniGame(FMiniGameResult::Failure(Transition.Context, Reason), Transition.Context, Transition.Award);
    }
    if (GetController()) GetController()->DisableMiniGameInput();
    GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->CancelRun(Transition.Context.RunId, Transition.Context.Generation, Reason, false);
    if (CurrentState != EArcadeGameFlowState::MINIGAME_TRANSITION) TransitionTo(EArcadeGameFlowState::MINIGAME_TRANSITION, false);
    SetPhase(ETransitionPhase::Recovering);
}

// Release view/possession before destroying run actors, then stage restoration under closed gates.
bool UGameFlowComponent::TryReturn()
{
    auto* Mode = Cast<APinballGameModeBase>(GetOwner()); auto* Controller = GetController();
    auto* Table = Mode ? Mode->GetTable() : nullptr;
    if (!Table || !Controller || !Controller->RestoreMiniGameMode(Transition.Controller)) return false;
    if (!GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->CleanupRun(Transition.Context.RunId, Transition.Context.Generation)) return false;
    Runtime.Reset();
    if (!Table->GetSession()->PrepareRestore(Transition.Table)) return false;
    Table->GetSession()->CommitRestore(Transition.Table);
    Table->CommitMiniGameReturn();
    ObjectiveProtectionRemaining = Mode->GetCabinet()->ReturnTriggerProtectionSeconds;
    Transition = FTransitionRecord(); Definition = nullptr; ActiveTrigger.Reset();
    TransitionTo(EArcadeGameFlowState::PINBALL_PLAYING);
    return true;
}

// An explicit retry restarts only the bounded return attempt, never the old minigame.
void UGameFlowComponent::RetryReturn()
{
    if (CurrentState == EArcadeGameFlowState::BOOT && bBootFailed) { BeginMiniGameBoot(); return; }
    if (Transition.Phase == ETransitionPhase::RecoveryMenu && CurrentState != EArcadeGameFlowState::PAUSED) SetPhase(ETransitionPhase::Returning);
}

// An explicit restart may discard preserved progress after callback generations have been invalidated.
void UGameFlowComponent::ResetForRecoveryRestart()
{
    auto* Mode = Cast<APinballGameModeBase>(GetOwner());
    if (Mode && Mode->GetTable()) Mode->GetTable()->GetSession()->ResetSuspension(Transition.Table);
    CancelForTeardown();
}

// Teardown invalidates the transaction before cleanup and deliberately skips return operations.
void UGameFlowComponent::CancelForTeardown()
{
    const auto C = Transition.Context; Transition = FTransitionRecord();
    PendingCancellation.Reset(); GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->OnRunCancelled.RemoveAll(this);
    if (C.RunId.IsValid()) GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->CleanupRun(C.RunId, C.Generation);
    Runtime.Reset(); Definition = nullptr; ActiveTrigger.Reset();
}

// Presentation consumes a read-only snapshot; flow alone owns countdowns and awards.
FText UGameFlowComponent::GetMiniGameStatus() const
{
    if (CurrentState == EArcadeGameFlowState::BOOT) return FText::FromString(bBootFailed ? TEXT("Arena loading failed. Retry or Quit.") : TEXT("Loading arenas..."));
    if (Transition.Phase == ETransitionPhase::AwaitingConfirmation) return FText::FromString(Definition->Instructions.ToString() + TEXT("\nRelease keys, then press SPACE or ENTER to start."));
    if (Transition.Phase == ETransitionPhase::Playing && Runtime.IsValid()) return FText::FromString(FString::Printf(TEXT("Time remaining: %.1f\nLocal score: %lld\nSPACE: action     ESCAPE: pause"), Transition.Context.DurationLimit - Runtime->GetElapsed(), Runtime->GetLocalScore()));
    if (Transition.Phase == ETransitionPhase::Results) return FText::FromString(FString::Printf(TEXT("Round complete\nBonus: %lld"), Transition.Award.AwardedPoints));
    if (Transition.Phase == ETransitionPhase::RecoveryMenu) return FText::FromString(TEXT("Unable to restore the table safely. Retry, Restart or Quit."));
    if (Transition.Phase == ETransitionPhase::Recovering) return FText::FromString(TEXT("The round could not continue. Returning to your ball..."));
    return FText::FromString(TEXT("Preparing..."));
}

bool UGameFlowComponent::TransitionTo(EArcadeGameFlowState Next, bool bPublish)
{
    if (!IsLegalTransition(CurrentState, Next) || !GameState) return false;
    UE_LOG(LogPinballBattle, Log, TEXT("Flow: %s -> %s"),
        *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(Next));
    CurrentState = Next;
    GameState->SessionState.FlowState = Next;
    if (bPublish) GameState->PublishSession();
    return true;
}

bool UGameFlowComponent::CanPause(EArcadeGameFlowState State)
{
    using E = EArcadeGameFlowState;
    return State == E::PINBALL_READY || State == E::PINBALL_PLAYING || State == E::MINIGAME_TRANSITION ||
        State == E::MINIGAME_PLAYING || State == E::MINIGAME_RESULTS;
}

bool UGameFlowComponent::SetPaused(bool bPaused)
{
    if (!GameState) return false;
    if (bPaused)
    {
        if (!CanPause(CurrentState)) return false;
        GameState->SessionState.ResumeState = CurrentState;
        CurrentState = EArcadeGameFlowState::PAUSED;
    }
    else
    {
        if (CurrentState != EArcadeGameFlowState::PAUSED || !CanPause(GameState->SessionState.ResumeState)) return false;
        CurrentState = GameState->SessionState.ResumeState;
        GameState->SessionState.ResumeState = EArcadeGameFlowState::BOOT;
    }
    GameState->SessionState.FlowState = CurrentState;
    GameState->PublishSession();
    return true;
}
