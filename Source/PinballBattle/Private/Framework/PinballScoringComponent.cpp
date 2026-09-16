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
    return true;
}

void UPinballScoringComponent::CommitSession(FPreparedSession&& Prepared)
{
    ActiveSession = Prepared.SessionId;
    CategoryPoints = MoveTemp(Prepared.CategoryPoints);
    MinimumMultiplier = Prepared.MinimumMultiplier;
    MaximumMultiplier = Prepared.MaximumMultiplier;
    ProfileRevision = Prepared.ProfileRevision;
    AcceptedEvents.Reset();
    SourceSequences.Reset();
    SequenceBall.Invalidate();
    TotalScore = 0;
    LatestAward = FScoreAward();
    LatestAward.SessionId = ActiveSession;
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
