#include "Data/MiniGameTypes.h"

// Validate snapshots independently of mutable assets, including every arithmetic bound.
bool FMiniGameContext::IsValid() const
{
    if (!SessionId.IsValid() || !RunId.IsValid() || MiniGameId.IsNone() || Generation <= 0 ||
        !FMath::IsFinite(DurationLimit) || DurationLimit <= 0 || DurationLimit > 45 ||
        StartingLives < 0 || StartingLives > 3 || SessionMultiplier < 1 || SessionMultiplier > 10 ||
        !FMath::IsFinite(MinimumResultMultiplier) || !FMath::IsFinite(MaximumResultMultiplier) ||
        MinimumResultMultiplier < 1 || MaximumResultMultiplier < MinimumResultMultiplier || MaximumResultMultiplier > 10 ||
        MaximumObjectives < 0 || MaximumObjectives > 1000000 || ArenaTransform.ContainsNaN() || ArenaExtent.ContainsNaN() ||
        ArenaExtent.GetMin() <= 0 || !FMath::IsFinite(BaseCap) || BaseCap <= 0 || BaseCap > 10000 || ProfileRevision.IsNone()) return false;
    for (const auto& Metric : MetricBounds)
        if (Metric.Key.IsNone() || !FMath::IsFinite(Metric.Value) || Metric.Value < 0 || Metric.Value > 1.e9) return false;
    if (MetricWeights.IsEmpty()) return false;
    for (const auto& Weight : MetricWeights)
        if (!MetricBounds.Contains(Weight.Key) || !FMath::IsFinite(Weight.Value) || Weight.Value < 0 || Weight.Value > 1.e9) return false;
    for (FName Key : FloorMetrics) if (!MetricWeights.Contains(Key)) return false;
    return true;
}

// Compare all identity fields before inspecting payloads or updating a terminal latch.
bool FMiniGameResult::Matches(const FMiniGameContext& C) const
{
    return SessionId == C.SessionId && RunId == C.RunId && MiniGameId == C.MiniGameId && Generation == C.Generation;
}

// Validate normal results strictly; technical failures use an explicit zero-valued factory.
bool FMiniGameResult::Validate(const FMiniGameContext& C) const
{
    if (!C.IsValid() || !Matches(C) || RawScore < 0 || ObjectivesCompleted < 0 || ObjectivesCompleted > C.MaximumObjectives ||
        !FMath::IsFinite(DurationSeconds) || DurationSeconds < 0 || DurationSeconds > C.DurationLimit + 1.e-6 ||
        static_cast<uint8>(EndReason) > static_cast<uint8>(EMiniGameEndReason::RuntimeFailed) ||
        static_cast<uint8>(PerformanceRating) > static_cast<uint8>(EPerformanceRating::Gold) ||
        !FMath::IsFinite(Multiplier) || (bHasMultiplier && (Multiplier < C.MinimumResultMultiplier || Multiplier > C.MaximumResultMultiplier))) return false;
    for (const auto& Metric : Metrics)
    {
        const double* Bound = C.MetricBounds.Find(Metric.Key);
        if (!Bound || !FMath::IsFinite(Metric.Value) || Metric.Value < 0 || Metric.Value > *Bound) return false;
    }
    for (const auto& Metric : C.MetricBounds) if (!Metrics.Contains(Metric.Key)) return false;
    return true;
}

// Preserve accepted identity while discarding any untrusted partial gameplay payload.
FMiniGameResult FMiniGameResult::Failure(const FMiniGameContext& C, EMiniGameEndReason Reason)
{
    FMiniGameResult R;
    R.SessionId = C.SessionId; R.RunId = C.RunId; R.MiniGameId = C.MiniGameId; R.Generation = C.Generation;
    R.EndReason = Reason;
    for (const auto& Metric : C.MetricBounds) R.Metrics.Add(Metric.Key, 0);
    return R;
}
