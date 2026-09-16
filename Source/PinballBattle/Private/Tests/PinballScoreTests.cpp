#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/ScoringProfile.h"
#include "Framework/PinballScoringComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPinballScoreTest, "PinballBattle.Score.TableAwards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Exercise the production score boundary with known arithmetic and adversarial identities/data. */
bool FPinballScoreTest::RunTest(const FString&)
{
    auto* Profile = NewObject<UScoringProfile>();
    auto* Score = NewObject<UPinballScoringComponent>();
    FSessionState State;
    State.SessionId = FGuid::NewGuid();
    State.CurrentBallId = FGuid::NewGuid();
    State.FlowState = EArcadeGameFlowState::PINBALL_PLAYING;
    FBallHandle Ball;
    Ball.SessionId = State.SessionId;
    Ball.BallId = State.CurrentBallId;
    Ball.Disposition = EBallDisposition::Active;
    FScoringEvent Event;
    Event.SessionId = State.SessionId;
    Event.BallId = Ball.BallId;
    Event.SourceId = FGuid::NewGuid();
    Event.ObservedPhase = State.FlowState;
    Event.PhaseEpoch = 4;
    TestFalse(TEXT("Missing profile rejected"), Score->ResetSession(State.SessionId, nullptr));
    for (int32 Multiplier : {1, 2})
    {
        TestTrue(TEXT("Reset captures profile"), Score->ResetSession(State.SessionId, Profile));
        State.Multiplier = Multiplier;
        Event.Sequence = 0;
        for (EScoringCategory Category : {EScoringCategory::Target, EScoringCategory::Bumper, EScoringCategory::Lane})
        {
            Event.Category = Category;
            Event.EventId = FGuid::NewGuid();
            ++Event.Sequence;
            TestTrue(TEXT("New category event awarded"), Score->SubmitTableScore(Event, State, Ball, 4));
            TestFalse(TEXT("Duplicate event rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
        }
        TestEqual(TEXT("650/1300 example"), Score->GetTotalScore(), int64(650 * Multiplier));
    }
    Event.EventId = FGuid::NewGuid();
    TestFalse(TEXT("Same episode with new event ID rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
    ++Event.Sequence;
    TestFalse(TEXT("Old epoch rejected"), Score->SubmitTableScore(Event, State, Ball, 5));
    State.FlowState = EArcadeGameFlowState::PAUSED;
    TestFalse(TEXT("Paused event rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
    State.FlowState = EArcadeGameFlowState::PINBALL_PLAYING;
    Event.SessionId = FGuid::NewGuid();
    TestFalse(TEXT("Old session rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
    Event.SessionId = State.SessionId;
    State.Multiplier = 11;
    TestFalse(TEXT("Out-of-range multiplier rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
    State.Multiplier = 1;
    Event.BallId = FGuid::NewGuid();
    TestFalse(TEXT("Stale ball rejected"), Score->SubmitTableScore(Event, State, Ball, 4));
    Event.BallId = Ball.BallId;
    Profile->Categories[0].BasePoints = 999;
    Event.Category = EScoringCategory::Target;
    TestTrue(TEXT("Captured profile survives live editing"), Score->SubmitTableScore(Event, State, Ball, 4));
    TestEqual(TEXT("Original captured 100 awarded"), Score->GetLatestAward().AwardedPoints, int64(100));
    FString Error;
    const FTableScoreRule Duplicate = Profile->Categories[0];
    Profile->Categories.Add(Duplicate);
    TestFalse(TEXT("Duplicate categories rejected"), Profile->Validate(Error));
    Profile->Categories.Pop();
    for (double Bad : {-1., std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 1.e20})
    {
        Profile->Categories[0].BasePoints = Bad;
        TestFalse(TEXT("Invalid profile values rejected"), Profile->Validate(Error));
    }
    int64 Points = 0, Total = 0;
    TestTrue(TEXT("Fractional base rounds after multiplying"), Score->CalculateAward(100.75, 2, 0, Points, Total));
    TestEqual(TEXT("Floor once = 201"), Points, int64(201));
    TestFalse(TEXT("Int64 addition overflow rejected"), Score->CalculateAward(100, 1, MAX_int64 - 99, Points, Total));
    TestFalse(TEXT("Int64 multiplication/conversion overflow rejected"), Score->CalculateAward(1.e18, 10, 0, Points, Total));
    TestFalse(TEXT("Negative total rejected"), Score->CalculateAward(100, 1, -1, Points, Total));
    return true;
}
#endif
