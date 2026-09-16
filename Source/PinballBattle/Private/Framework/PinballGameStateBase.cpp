#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballScoringComponent.h"

APinballGameStateBase::APinballGameStateBase()
{
    Scoring = CreateDefaultSubobject<UPinballScoringComponent>(TEXT("Scoring"));
}

void APinballGameStateBase::PublishSession()
{
    check(SessionState.BallsRemaining >= 0 && SessionState.BallsRemaining <= 3);
    check(SessionState.Multiplier >= 1 && SessionState.Multiplier <= 10);
    OnSessionChanged.Broadcast(SessionState);
}
