#include "Data/ScoringProfile.h"
#include <cmath>

UScoringProfile::UScoringProfile()
{
    for (const auto Category : {EScoringCategory::Target, EScoringCategory::Bumper, EScoringCategory::Lane})
    {
        FTableScoreRule Rule;
        Rule.Category = Category;
        Rule.BasePoints = Category == EScoringCategory::Target ? 100 : Category == EScoringCategory::Bumper ? 50 : 500;
        Categories.Add(Rule);
    }
}

// Validate both table categories and configurable minigame profiles before capturing a session.
bool UScoringProfile::Validate(FString& Error) const
{
    if (ProfileId.IsNone() || Revision.IsNone() || MinimumMultiplier != 1 || MaximumMultiplier != 10 || Categories.IsEmpty())
    { Error = TEXT("Scoring requires profile/revision, categories and multiplier range 1..10."); return false; }
    TSet<EScoringCategory> Seen;
    for (const FTableScoreRule& Rule : Categories)
    {
        if (static_cast<uint8>(Rule.Category) > static_cast<uint8>(EScoringCategory::Lane) || Seen.Contains(Rule.Category) ||
            !FMath::IsFinite(Rule.BasePoints) || Rule.BasePoints < 0 || Rule.BasePoints > 1.e9)
        { Error = TEXT("Categories must be unique and base points finite in 0..1,000,000,000."); return false; }
        Seen.Add(Rule.Category);
    }
    TSet<FName> Profiles;
    for (const auto& Rule : MiniGameRules)
    {
        if (Rule.ProfileKey.IsNone() || Profiles.Contains(Rule.ProfileKey) || Rule.Weights.IsEmpty() ||
            !FMath::IsFinite(Rule.BaseCap) || Rule.BaseCap <= 0 || Rule.BaseCap > 10000)
        { Error = TEXT("Invalid or duplicate minigame profile."); return false; }
        Profiles.Add(Rule.ProfileKey);
        for (const auto& Weight : Rule.Weights)
            if (Weight.Key.IsNone() || !FMath::IsFinite(Weight.Value) || Weight.Value < 0 || Weight.Value > 1.e9)
            { Error = TEXT("Invalid minigame weight."); return false; }
        for (FName Key : Rule.FloorMetrics) if (!Rule.Weights.Contains(Key)) return false;
    }
    return true;
}

// Apply metric-specific flooring before weighting, then compute a normalized presentation rating.
bool UScoringProfile::EvaluateMiniGame(const FMiniGameResult& R, const FMiniGameContext& C, double& Base, EPerformanceRating& Rating)
{
    if (!R.Validate(C)) return false;
    Base = 0; Rating = EPerformanceRating::None;
    if (R.EndReason == EMiniGameEndReason::StartFailed || R.EndReason == EMiniGameEndReason::Cancelled ||
        R.EndReason == EMiniGameEndReason::RuntimeFailed) return true;
    for (const auto& Weight : C.MetricWeights)
    {
        const double Value = R.Metrics.FindChecked(Weight.Key);
        Base += (C.FloorMetrics.Contains(Weight.Key) ? std::floor(Value) : Value) * Weight.Value;
        if (!FMath::IsFinite(Base)) return false;
    }
    Base = FMath::Clamp(Base, 0., C.BaseCap);
    Rating = Base == 0 ? EPerformanceRating::None : Base < C.BaseCap / 3 ? EPerformanceRating::Bronze :
        Base < C.BaseCap * 2 / 3 ? EPerformanceRating::Silver : EPerformanceRating::Gold;
    return true;
}
