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
    UE_LOG(LogPinballBattle, Log, TEXT("Foundation flow initialized: BOOT; gameplay is not enabled."));
}
