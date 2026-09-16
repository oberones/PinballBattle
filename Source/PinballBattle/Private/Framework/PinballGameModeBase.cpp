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
    if (!bPracticeMode) return;
    FString Error;
    if (!Table || !Table->InitializeTable(Error) || !Table->SpawnReadyBall())
    {
        UE_LOG(LogPinballBattle, Error, TEXT("Practice boot failed: %s"), *Error);
        return;
    }
    GameFlow->TransitionTo(EArcadeGameFlowState::ATTRACT);
    GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_READY);
    if (APinballPlayerController* Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController()))
        Controller->ConfigureTable(Table);
    UE_LOG(LogPinballBattle, Log, TEXT("Practice ready: one ball; hold/release Down to launch, Left/Right to flip."));
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
    UE_LOG(LogPinballBattle, Log, TEXT("Launch accepted: impulse=%.3f"), Impulse);
    return true;
}

bool APinballGameModeBase::RequestDrain(APinballBall* Ball)
{
    if (!CanPlay() || !Table->IsCurrentBall(Ball) || !Ball->MarkDrained()) return false;
    GameFlow->TransitionTo(EArcadeGameFlowState::BALL_LOST);
    Table->CancelActions();
    UE_LOG(LogPinballBattle, Log, TEXT("Drain accepted once: contacts=%d"), Ball->GetContactCount());
    Table->RemoveBall();
    if (bPracticeMode)
        ReplacementTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ReplaceDrainedBall);
    return true;
}

void APinballGameModeBase::ReplaceDrainedBall()
{
    if (GameFlow->GetCurrentState() != EArcadeGameFlowState::BALL_LOST || !Table) return;
    if (!Table->SpawnReadyBall())
    {
        UE_LOG(LogPinballBattle, Error, TEXT("Replacement spawn failed; keeping BALL_LOST with gameplay closed."));
        return;
    }
    GameFlow->TransitionTo(EArcadeGameFlowState::PINBALL_READY);
}

void APinballGameModeBase::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ReplacementTimer);
    if (IsValid(Table)) Table->CancelActions();
    Super::EndPlay(Reason);
}
