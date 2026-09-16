#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ScoringTypes.h"
#include "Data/MiniGameTypes.h"
#include "ScoringProfile.generated.h"

/** A category entry is an array element so duplicate authoring can be rejected explicitly. */
USTRUCT(BlueprintType)
struct FTableScoreRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EScoringCategory Category = EScoringCategory::Target;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double BasePoints = 100;
};

USTRUCT(BlueprintType)
struct FMiniGameScoreRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ProfileKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, double> Weights;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSet<FName> FloorMetrics;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double BaseCap = 10000;
};

/** Immutable-at-session-start table scoring configuration. */
UCLASS(BlueprintType)
class PINBALLBATTLE_API UScoringProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    /** Supply the initial target/bumper/lane values as editable asset defaults. */
    UScoringProfile();
    /** Reject missing identity, duplicate categories and nonfinite, negative or unbounded values. */
    bool Validate(FString& Error) const;
    /** Evaluate copied metric weights and rating without accessing or modifying the central total. */
    static bool EvaluateMiniGame(const FMiniGameResult& Result, const FMiniGameContext& Context, double& Base, EPerformanceRating& Rating);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMiniGameScoreRule> MiniGameRules;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ProfileId = TEXT("Table");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Revision = TEXT("1");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTableScoreRule> Categories;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumMultiplier = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumMultiplier = 10;
};
