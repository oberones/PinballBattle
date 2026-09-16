#include "Data/ScoringProfile.h"

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
    return true;
}
