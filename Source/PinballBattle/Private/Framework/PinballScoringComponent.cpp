#include "Framework/PinballScoringComponent.h"
#include "Data/ScoringProfile.h"
#include "PinballBattle.h"
#include <cmath>

bool UPinballScoringComponent::CalculateAward(double BasePoints, int32 Multiplier, int64 Total, int64& Points, int64& NewTotal)
{
    if (!FMath::IsFinite(BasePoints) || BasePoints < 0 || Multiplier < 1 || Multiplier > 10 || Total < 0) return false;
    const double Product = std::floor(BasePoints * Multiplier);
    // 2^63 is exactly representable as double; casting that boundary to int64 would be undefined.
    if (!FMath::IsFinite(Product) || Product >= 9223372036854775808.0) return false;
    const int64 Award = static_cast<int64>(Product);
    if (Total > MAX_int64 - Award) return false;
    Points = Award;
    NewTotal = Total + Award;
    return true;
}

bool UPinballScoringComponent::ResetSession(FGuid SessionId, const UScoringProfile* Profile)
{
    FPreparedSession Prepared;
    if (!PrepareSession(SessionId, Profile, Prepared)) return false;
    CommitSession(MoveTemp(Prepared));
    PublishReset();
    return true;
}

// Copy all profile values so editing assets cannot reinterpret accepted gameplay.
bool UPinballScoringComponent::PrepareSession(FGuid SessionId, const UScoringProfile* Profile, FPreparedSession& Prepared)
{
    FString Error;
    if (!SessionId.IsValid() || !Profile || !Profile->Validate(Error)) return false;
    Prepared.SessionId = SessionId;
    Prepared.CategoryPoints.Reset();
    for (const FTableScoreRule& Rule : Profile->Categories) Prepared.CategoryPoints.Add(Rule.Category, Rule.BasePoints);
    Prepared.MinimumMultiplier = Profile->MinimumMultiplier;
    Prepared.MaximumMultiplier = Profile->MaximumMultiplier;
    Prepared.ProfileRevision = Profile->Revision;
    Prepared.MiniGameRules = Profile->MiniGameRules;
    return true;
}

// Replace both event and run ledgers only after the new session has been prepared.
void UPinballScoringComponent::CommitSession(FPreparedSession&& Prepared)
{
    ActiveSession = Prepared.SessionId;
    CategoryPoints = MoveTemp(Prepared.CategoryPoints);
    MinimumMultiplier = Prepared.MinimumMultiplier;
    MaximumMultiplier = Prepared.MaximumMultiplier;
    ProfileRevision = Prepared.ProfileRevision;
    MiniGameRules = MoveTemp(Prepared.MiniGameRules);
    FinalizedRuns.Reset();
    AcceptedEvents.Reset();
    SourceSequences.Reset();
    SequenceBall.Invalidate();
    TotalScore = 0;
    LatestAward = FScoreAward();
    LatestAward.SessionId = ActiveSession;
}

// Resolve a requested profile explicitly; a missing profile never falls back to another game's rules.
bool UPinballScoringComponent::CaptureMiniGameProfile(FName Key, FMiniGameContext& C) const
{
    for (const auto& Rule : MiniGameRules) if (Rule.ProfileKey == Key)
    {
        C.MetricWeights = Rule.Weights; C.FloorMetrics = Rule.FloorMetrics;
        C.BaseCap = Rule.BaseCap; C.ProfileRevision = ProfileRevision;
        return true;
    }
    return false;
}

// The ledger write precedes delegates, preventing reentrant or repeated results from awarding twice.
bool UPinballScoringComponent::EvaluateAndAwardMiniGame(const FMiniGameResult& R, const FMiniGameContext& C, FScoreAward& Award)
{
    if (ActiveSession != C.SessionId || !C.IsValid() || !R.Matches(C) || FinalizedRuns.Contains(C.RunId)) return false;
    double Base = 0; EPerformanceRating Rating;
    const bool Valid = UScoringProfile::EvaluateMiniGame(R, C, Base, Rating);
    if (!Valid) Base = 0;
    int64 Points = 0, NewTotal = TotalScore;
    const double ResultMultiplier = Valid && R.bHasMultiplier ? R.Multiplier : 1;
    if (!CalculateAward(Base * ResultMultiplier, C.SessionMultiplier, TotalScore, Points, NewTotal))
    { Base = 0; Points = 0; NewTotal = TotalScore; UE_LOG(LogPinballBattle, Warning, TEXT("Rejected overflowing minigame award.")); }
    Award = FScoreAward(); Award.AwardId = FGuid::NewGuid(); Award.SessionId = ActiveSession;
    Award.RunId = C.RunId; Award.BasePoints = static_cast<int64>(std::floor(Base));
    Award.EffectiveMultiplier = C.SessionMultiplier; Award.AwardedPoints = Points;
    Award.NewTotal = NewTotal; Award.ProfileRevision = C.ProfileRevision;
    FinalizedRuns.Add(C.RunId, Award); TotalScore = NewTotal; LatestAward = Award;
    OnScoreChanged.Broadcast(Award);
    OnBonusAwarded.Broadcast(Award);
    return true;
}

void UPinballScoringComponent::PublishReset()
{
    OnScoreChanged.Broadcast(LatestAward);
}

bool UPinballScoringComponent::SubmitTableScore(const FScoringEvent& Event, const FSessionState& State, const FBallHandle& Ball, int64 Epoch)
{
    const double* Base = CategoryPoints.Find(Event.Category);
    if (!ActiveSession.IsValid() || Event.SessionId != ActiveSession || State.SessionId != ActiveSession ||
        Ball.SessionId != ActiveSession || Event.BallId != State.CurrentBallId || Event.BallId != Ball.BallId ||
        !Event.BallId.IsValid() || !Event.EventId.IsValid() || !Event.SourceId.IsValid() || Event.Sequence <= 0 ||
        Ball.Disposition != EBallDisposition::Active || State.FlowState != EArcadeGameFlowState::PINBALL_PLAYING ||
        Event.ObservedPhase != State.FlowState || Event.PhaseEpoch != Epoch || !Base || AcceptedEvents.Contains(Event.EventId) ||
        State.Multiplier < MinimumMultiplier || State.Multiplier > MaximumMultiplier || State.BallsRemaining < 1 || State.BallsRemaining > 3)
        return false;
    if (SequenceBall == Event.BallId && Event.Sequence <= SourceSequences.FindRef(Event.SourceId)) return false;
    int64 Points, NewTotal;
    if (!CalculateAward(*Base, State.Multiplier, TotalScore, Points, NewTotal))
    { UE_LOG(LogPinballBattle, Warning, TEXT("Rejected overflowing table award.")); return false; }
    if (SequenceBall != Event.BallId) { SourceSequences.Reset(); SequenceBall = Event.BallId; }
    SourceSequences.Add(Event.SourceId, Event.Sequence);
    AcceptedEvents.Add(Event.EventId);
    TotalScore = NewTotal;
    LatestAward.AwardId = FGuid::NewGuid();
    LatestAward.SessionId = ActiveSession;
    LatestAward.EventId = Event.EventId;
    LatestAward.BasePoints = static_cast<int64>(std::floor(*Base));
    LatestAward.EffectiveMultiplier = State.Multiplier;
    LatestAward.AwardedPoints = Points;
    LatestAward.NewTotal = TotalScore;
    LatestAward.ProfileRevision = ProfileRevision;
    OnScoreChanged.Broadcast(LatestAward);
    return true;
}
