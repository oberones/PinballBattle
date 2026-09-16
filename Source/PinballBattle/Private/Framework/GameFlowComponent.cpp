#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "PinballBattle.h"

UGameFlowComponent::UGameFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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

bool UGameFlowComponent::IsLegalTransition(EArcadeGameFlowState From, EArcadeGameFlowState Next)
{
    using E = EArcadeGameFlowState;
    return (From == E::BOOT && Next == E::ATTRACT) ||
        (From == E::ATTRACT && Next == E::PINBALL_READY) ||
        (From == E::PINBALL_READY && Next == E::PINBALL_PLAYING) ||
        (From == E::PINBALL_PLAYING && Next == E::BALL_LOST) ||
        (From == E::BALL_LOST && (Next == E::PINBALL_READY || Next == E::GAME_OVER)) ||
        (From == E::GAME_OVER && Next == E::PINBALL_READY);
}

bool UGameFlowComponent::TransitionTo(EArcadeGameFlowState Next)
{
    if (!IsLegalTransition(CurrentState, Next) || !GameState) return false;
    UE_LOG(LogPinballBattle, Log, TEXT("Flow: %s -> %s"),
        *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(Next));
    CurrentState = Next;
    GameState->SessionState.FlowState = Next;
    GameState->PublishSession();
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
