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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiniGameScoreTest, "PinballBattle.Score.MiniGameAwards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Use all three contract schemas to verify pure arithmetic, captured settings and finalization.
bool FMiniGameScoreTest::RunTest(const FString&)
{
    auto* Profile = NewObject<UScoringProfile>(); auto* Score = NewObject<UPinballScoringComponent>();
    const FGuid Session = FGuid::NewGuid(); Score->ResetSession(Session, Profile);
    FMiniGameContext C; C.SessionId = Session; C.Generation = 1; C.MiniGameId = TEXT("IndependentFixture"); C.ProfileRevision = TEXT("test");
    const TArray<TMap<FName, double>> Weights = {
        {{TEXT("ObjectsDestroyed"), 500}}, {{TEXT("ThreatsDestroyed"), 250}, {TEXT("StructuresSurviving"), 1000}},
        {{TEXT("EnemiesDestroyed"), 250}, {TEXT("SurvivalSeconds"), 100}} };
    const TArray<TMap<FName, double>> Low = {
        {{TEXT("ObjectsDestroyed"), 2}}, {{TEXT("ThreatsDestroyed"), 4}, {TEXT("StructuresSurviving"), 0}},
        {{TEXT("EnemiesDestroyed"), 4}, {TEXT("SurvivalSeconds"), 10.9}} };
    const TArray<TMap<FName, double>> High = {
        {{TEXT("ObjectsDestroyed"), 20}}, {{TEXT("ThreatsDestroyed"), 28}, {TEXT("StructuresSurviving"), 3}},
        {{TEXT("EnemiesDestroyed"), 28}, {TEXT("SurvivalSeconds"), 30}} };
    for (int32 Game = 0; Game < 3; ++Game)
    {
        C.MetricWeights = Weights[Game]; C.MetricBounds.Reset(); C.FloorMetrics.Reset();
        for (const auto& W : C.MetricWeights) C.MetricBounds.Add(W.Key, 100);
        if (Game == 2) C.FloorMetrics.Add(TEXT("SurvivalSeconds"));
        for (int32 Multiplier : {1, 2}) for (bool IsHigh : {false, true})
        {
            C.RunId = FGuid::NewGuid(); C.SessionMultiplier = Multiplier;
            auto R = FMiniGameResult::Failure(C, EMiniGameEndReason::LivesExhausted);
            R.Metrics = IsHigh ? High[Game] : Low[Game];
            FScoreAward Award;
            TestTrue(TEXT("Performance loss retains reward"), Score->EvaluateAndAwardMiniGame(R, C, Award));
            const int64 Expected = (IsHigh ? 10000 : Game == 2 ? 2000 : 1000) * Multiplier;
            TestEqual(TEXT("Contract low/high example"), Award.AwardedPoints, Expected);
            TestFalse(TEXT("Repeated result rejected"), Score->EvaluateAndAwardMiniGame(R, C, Award));
        }
        C.SessionMultiplier = 1; double Previous = -1;
        for (int32 Value = 0; Value <= 30; ++Value)
        {
            C.RunId = FGuid::NewGuid(); auto R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
            for (auto& Metric : R.Metrics) Metric.Value = Value;
            double Base; EPerformanceRating Rating;
            TestTrue(TEXT("Metric evaluation"), UScoringProfile::EvaluateMiniGame(R, C, Base, Rating));
            TestTrue(TEXT("Reward monotonicity"), Base >= Previous); Previous = Base;
        }
    }
    C.RunId = FGuid::NewGuid(); auto R = FMiniGameResult::Failure(C, EMiniGameEndReason::Cancelled); FScoreAward Award;
    TestTrue(TEXT("Cancellation finalized"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    TestEqual(TEXT("Cancellation zero"), Award.AwardedPoints, int64(0));
    TestFalse(TEXT("Zero cannot later award"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    C.RunId = FGuid::NewGuid(); R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut); R.Metrics.Reset();
    TestTrue(TEXT("Malformed run finalized"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    TestEqual(TEXT("Missing metrics zero"), Award.AwardedPoints, int64(0));
    C.RunId = FGuid::NewGuid(); R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut); R.Generation++;
    TestFalse(TEXT("Stale generation"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    Score->ResetSession(FGuid::NewGuid(), Profile); R.Generation = C.Generation;
    TestFalse(TEXT("Stale session"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    Score->ResetSession(C.SessionId, Profile);
    for (double Bad : {-1., 101., std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        C.RunId = FGuid::NewGuid(); R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
        R.Metrics.begin()->Value = Bad;
        TestTrue(TEXT("Invalid metric finalizes zero"), Score->EvaluateAndAwardMiniGame(R, C, Award));
        TestEqual(TEXT("Invalid metric no reward"), Award.AwardedPoints, int64(0));
    }
    C.RunId = FGuid::NewGuid(); R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
    R.bHasMultiplier = true; R.Multiplier = 2;
    TestFalse(TEXT("Definition rejects multiplier two"), R.Validate(C));
    R.bHasMultiplier = false; R.RawScore = -1; TestFalse(TEXT("Negative raw score rejected"), R.Validate(C));
    R.RawScore = 0; R.ObjectivesCompleted = C.MaximumObjectives + 1;
    TestFalse(TEXT("Objective bound enforced"), R.Validate(C));
    R.ObjectivesCompleted = 0; R.Metrics = High[2]; Score->TotalScore = MAX_int64 - 1;
    AddExpectedError(TEXT("Rejected overflowing minigame award"), EAutomationExpectedErrorFlags::Contains, 1);
    TestTrue(TEXT("Overflow finalized at zero"), Score->EvaluateAndAwardMiniGame(R, C, Award));
    TestEqual(TEXT("Overflow cannot wrap total"), Score->GetTotalScore(), MAX_int64 - 1);
    return true;
}
#endif
