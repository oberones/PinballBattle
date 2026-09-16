#include "Framework/PinballGameModeBase.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballPlayerController.h"
#include "Pinball/PinballControlPawn.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballPlunger.h"
#include "PinballBattle.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Data/CabinetDefinition.h"
#include "Framework/PinballScoringComponent.h"

APinballGameModeBase::APinballGameModeBase()
{
    GameStateClass = APinballGameStateBase::StaticClass();
    PlayerControllerClass = APinballPlayerController::StaticClass();
    DefaultPawnClass = APinballControlPawn::StaticClass();
    GameFlow = CreateDefaultSubobject<UGameFlowComponent>(TEXT("GameFlow"));
}

void APinballGameModeBase::InitGameState()
{
    Super::InitGameState();
    GameFlow->InitializeProjection(GetGameState<APinballGameStateBase>());
}

bool APinballGameModeBase::RegisterTable(APinballTable* InTable)
{
    if (!IsValid(InTable) || (IsValid(Table) && Table != InTable))
    {
        UE_LOG(LogPinballBattle, Error, TEXT("Exactly one table must explicitly register with the GameMode."));
        return false;
    }
    Table = InTable;
    return true;
}

void APinballGameModeBase::StartPlay()
{
    Super::StartPlay(); // Table BeginPlay registers before boot prerequisites are inspected.
    FString Error;
    APlayerController* Player = GetWorld()->GetFirstPlayerController();
    if (!Table || !Table->InitializeTable(Error) || (!bPracticeMode &&
        (!Cabinet || !Cabinet->Validate(Table, Player ? Player->GetPawn() : nullptr, Error))))
    {
        UE_LOG(LogPinballBattle, Error, TEXT("Cabinet boot failed: %s"), *Error);
        return;
    }
    Table->OnScoringEvent.AddUniqueDynamic(this, &ThisClass::HandleTableScore);
    if (APinballPlayerController* Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController()))
        Controller->ConfigureTable(Table);
    GameFlow->TransitionTo(EArcadeGameFlowState::ATTRACT);
    if (bPracticeMode)
    {
        auto* State = GetGameState<APinballGameStateBase>();
        State->SessionState.SessionId = Table->GetBallHandle().SessionId;
        if (Table->SpawnReadyBall())
        {
            State->SessionState.CurrentBallId = Table->GetBallHandle().BallId;
            GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_READY);
        }
    }
}

bool APinballGameModeBase::CanLaunch() const
{
    return Table && Table->GetBall() && GameFlow->GetCurrentState() == EArcadeGameFlowState::PINBALL_READY;
}

bool APinballGameModeBase::CanPlay() const
{
    return Table && Table->GetBall() && GameFlow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING;
}

bool APinballGameModeBase::RequestLaunch(float Impulse)
{
    if (!CanLaunch() || !Table->GetBall()->Launch(Table->Plunger->GetLaunchDirection(), Impulse)) return false;
    GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_PLAYING);
    Table->NotifyBallLaunched();
    UE_LOG(LogPinballBattle, Log, TEXT("Launch accepted: impulse=%.3f"), Impulse);
    return true;
}

bool APinballGameModeBase::RequestDrain(APinballBall* Ball)
{
    if (!CanPlay() || !Table->CanEmitEvent(Ball) || !Ball->MarkDrained()) return false;
    GameFlow->TransitionTo(EArcadeGameFlowState::BALL_LOST);
    Table->CancelActions();
    UE_LOG(LogPinballBattle, Log, TEXT("Drain accepted once: contacts=%d"), Ball->GetContactCount());
    Table->RemoveBall();
    auto* State = GetGameState<APinballGameStateBase>();
    State->SessionState.CurrentBallId.Invalidate();
    if (!bPracticeMode) --State->SessionState.BallsRemaining;
    if (State->SessionState.BallsRemaining == 0)
        GameFlow->TransitionTo(EArcadeGameFlowState::GAME_OVER);
    else
    {
        State->PublishSession();
        const FGuid SessionId = State->SessionState.SessionId;
        const int64 Generation = State->SessionState.Generation;
        // A stale next-tick callback must never replace a ball in another session.
        ReplacementTimer = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, SessionId, Generation]()
        {
            const auto Current = GetGameState<APinballGameStateBase>()->GetSessionState();
            if (Current.SessionId == SessionId && Current.Generation == Generation) ReplaceDrainedBall();
        }));
    }
    return true;
}

bool APinballGameModeBase::RequestDrainEvent(const FDrainEvent& Event)
{
    if (!Table || !Event.EventId.IsValid() || !Event.SourceId.IsValid() ||
        Event.SessionId != Table->GetBallHandle().SessionId || Event.BallId != Table->GetBallHandle().BallId ||
        Event.PhaseEpoch != Table->GetPhysicalEpoch()) return false;
    return RequestDrain(Table->GetBall());
}

void APinballGameModeBase::ReplaceDrainedBall()
{
    if (GameFlow->GetCurrentState() != EArcadeGameFlowState::BALL_LOST || !Table) return;
    if (!Table->SpawnReadyBall())
    {
        UE_LOG(LogPinballBattle, Error, TEXT("Replacement spawn failed; keeping BALL_LOST with gameplay closed."));
        return;
    }
    GetGameState<APinballGameStateBase>()->SessionState.CurrentBallId = Table->GetBallHandle().BallId;
    GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_READY);
}

bool APinballGameModeBase::RequestNewSession()
{
    const auto Flow = GameFlow->GetCurrentState();
    if (bPracticeMode || !Cabinet || !Table || (Flow != EArcadeGameFlowState::ATTRACT && Flow != EArcadeGameFlowState::GAME_OVER)) return false;
    auto* State = GetGameState<APinballGameStateBase>();
    FString Error;
    if (!Cabinet->Validate(Table, GetWorld()->GetFirstPlayerController()->GetPawn(), Error)) return false;
    // Invalidate identities first, before notifications, timer cancellation or actor teardown.
    State->SessionState.SessionId = FGuid::NewGuid();
    ++State->SessionState.Generation;
    State->SessionState.CurrentBallId.Invalidate();
    State->SessionState.BallsRemaining = 3;
    State->SessionState.Multiplier = 1;
    State->SessionState.ResumeState = EArcadeGameFlowState::BOOT;
    State->SessionState.ObjectiveStates.Reset();
    GetWorldTimerManager().ClearTimer(ReplacementTimer);
    Table->ResetForNewSession(State->SessionState.SessionId);
    if (!State->Scoring->ResetSession(State->SessionState.SessionId, Cabinet->ScoringProfile) || !Table->SpawnReadyBall()) return false;
    State->SessionState.CurrentBallId = Table->GetBallHandle().BallId;
    return GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_READY);
}

void APinballGameModeBase::HandleTableScore(const FScoringEvent& Event)
{
    if (bPracticeMode || !CanPlay() || !Table->CanEmitEvent(Table->GetBall())) return;
    auto* State = GetGameState<APinballGameStateBase>();
    State->Scoring->SubmitTableScore(Event, State->SessionState, Table->GetBallHandle(), Table->GetPhysicalEpoch());
}

bool APinballGameModeBase::RequestTogglePause(APlayerController* Controller)
{
    if (!Controller || Controller != GetWorld()->GetFirstPlayerController()) return false;
    if (GameFlow->GetCurrentState() == EArcadeGameFlowState::PAUSED)
    {
        if (!ClearPause()) return false;
        return GameFlow->SetPaused(false);
    }
    if (!UGameFlowComponent::CanPause(GameFlow->GetCurrentState()) || !SetPause(Controller)) return false;
    Table->CancelActions();
    return GameFlow->SetPaused(true);
}

void APinballGameModeBase::InvalidateSession()
{
    if (auto* State = GetGameState<APinballGameStateBase>())
    {
        State->SessionState.SessionId.Invalidate();
        State->SessionState.CurrentBallId.Invalidate();
        ++State->SessionState.Generation;
    }
    GetWorldTimerManager().ClearTimer(ReplacementTimer);
    if (IsValid(Table))
    {
        Table->OnScoringEvent.RemoveDynamic(this, &ThisClass::HandleTableScore);
        Table->ResetForNewSession(FGuid());
    }
}

void APinballGameModeBase::EndPlay(const EEndPlayReason::Type Reason)
{
    InvalidateSession();
    Super::EndPlay(Reason);
}
