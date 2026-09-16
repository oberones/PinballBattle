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
    GameState->FlowState = CurrentState;
    UE_LOG(LogPinballBattle, Log, TEXT("Flow initialized: BOOT"));
}

bool UGameFlowComponent::TransitionTo(EArcadeGameFlowState Next)
{
    using E = EArcadeGameFlowState;
    const bool bAllowed = (CurrentState == E::BOOT && Next == E::ATTRACT) ||
        (CurrentState == E::ATTRACT && Next == E::PINBALL_READY) ||
        (CurrentState == E::PINBALL_READY && Next == E::PINBALL_PLAYING) ||
        (CurrentState == E::PINBALL_PLAYING && Next == E::BALL_LOST) ||
        (CurrentState == E::BALL_LOST && Next == E::PINBALL_READY);
    if (!bAllowed || !GameState) return false;
    UE_LOG(LogPinballBattle, Log, TEXT("Flow: %s -> %s"),
        *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(Next));
    CurrentState = Next;
    GameState->FlowState = Next;
    return true;
}
