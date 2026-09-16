#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/ScoringProfile.h"
#include "Framework/PinballScoringComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPinballRestartTest, "PinballBattle.Flow.RestartScoreLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify terminal score lock and two resets reject all prior-session award callbacks. */
bool FPinballRestartTest::RunTest(const FString&)
{
    auto* Score = NewObject<UPinballScoringComponent>();
    auto* Profile = NewObject<UScoringProfile>();
    FScoringEvent Previous;
    for (int32 Run = 0; Run < 3; ++Run)
    {
        FSessionState State;
        State.SessionId = FGuid::NewGuid();
        State.CurrentBallId = FGuid::NewGuid();
        State.Generation = Run + 1;
        State.FlowState = EArcadeGameFlowState::PINBALL_PLAYING;
        FBallHandle Ball;
        Ball.SessionId = State.SessionId;
        Ball.BallId = State.CurrentBallId;
        Ball.Disposition = EBallDisposition::Active;
        TestTrue(TEXT("New session accepted"), Score->ResetSession(State.SessionId, Profile));
        TestEqual(TEXT("Reset total"), Score->GetTotalScore(), int64(0));
        TestEqual(TEXT("Initial balls"), State.BallsRemaining, 3);
        TestEqual(TEXT("Initial multiplier"), State.Multiplier, 1);
        TestFalse(TEXT("Previous callback rejected"), Score->SubmitTableScore(Previous, State, Ball, 1));
        FScoringEvent Event;
        Event.SessionId = State.SessionId;
        Event.BallId = State.CurrentBallId;
        Event.EventId = FGuid::NewGuid();
        Event.SourceId = FGuid::NewGuid();
        Event.Sequence = 1;
        Event.PhaseEpoch = 1;
        Event.ObservedPhase = State.FlowState;
        TestTrue(TEXT("Fresh award"), Score->SubmitTableScore(Event, State, Ball, 1));
        State.FlowState = EArcadeGameFlowState::GAME_OVER;
        State.BallsRemaining = 0;
        Event.EventId = FGuid::NewGuid();
        ++Event.Sequence;
        TestFalse(TEXT("Final score locked"), Score->SubmitTableScore(Event, State, Ball, 1));
        TestEqual(TEXT("Final score unchanged"), Score->GetTotalScore(), int64(100));
        Previous = Event;
    }
    return true;
}
#endif
