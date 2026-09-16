#include "Framework/PinballGameModeBase.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballPlayerController.h"
#include "Pinball/PinballControlPawn.h"

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
