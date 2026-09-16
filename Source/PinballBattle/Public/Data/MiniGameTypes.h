#pragma once
#include "CoreMinimal.h"
#include "MiniGameTypes.generated.h"

UENUM(BlueprintType)
enum class EMiniGameLifecycleState : uint8 { Dormant, Initialized, Playing, Ended };
UENUM(BlueprintType)
enum class EMiniGameEndReason : uint8 { TimedOut, LivesExhausted, StartFailed, Cancelled, RuntimeFailed };
UENUM(BlueprintType)
enum class EPerformanceRating : uint8 { None, Bronze, Silver, Gold };

/** Copied configuration; a runtime never receives a mutable session or score service. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FMiniGameContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid RunId;
    UPROPERTY(BlueprintReadOnly) FName MiniGameId;
    UPROPERTY(BlueprintReadOnly) int64 Generation = 0;
    UPROPERTY(BlueprintReadOnly) double DurationLimit = 30;
    UPROPERTY(BlueprintReadOnly) int32 StartingLives = 3;
    UPROPERTY(BlueprintReadOnly) int32 Seed = 0;
    UPROPERTY(BlueprintReadOnly) int32 SessionMultiplier = 1;
    UPROPERTY(BlueprintReadOnly) double MinimumResultMultiplier = 1;
    UPROPERTY(BlueprintReadOnly) double MaximumResultMultiplier = 1;
    UPROPERTY(BlueprintReadOnly) int32 MaximumObjectives = 10000;
    UPROPERTY(BlueprintReadOnly) FTransform ArenaTransform;
    UPROPERTY(BlueprintReadOnly) FVector ArenaExtent = FVector(500);
    UPROPERTY(BlueprintReadOnly) TMap<FName, double> MetricBounds;
    UPROPERTY(BlueprintReadOnly) TMap<FName, double> MetricWeights;
    UPROPERTY(BlueprintReadOnly) TSet<FName> FloorMetrics;
    UPROPERTY(BlueprintReadOnly) double BaseCap = 10000;
    UPROPERTY(BlueprintReadOnly) FName ProfileRevision;
    /** Check identity and bounded copied configuration before allocating any run resources. */
    bool IsValid() const;
};

/** Terminal value published once by the guarded base, with no caller-selected pinball bonus. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FMiniGameResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid RunId;
    UPROPERTY(BlueprintReadOnly) FName MiniGameId;
    UPROPERTY(BlueprintReadOnly) int64 Generation = 0;
    UPROPERTY(BlueprintReadOnly) EMiniGameEndReason EndReason = EMiniGameEndReason::TimedOut;
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) int64 RawScore = 0;
    UPROPERTY(BlueprintReadOnly) int32 ObjectivesCompleted = 0;
    UPROPERTY(BlueprintReadOnly) double DurationSeconds = 0;
    UPROPERTY(BlueprintReadOnly) EPerformanceRating PerformanceRating = EPerformanceRating::None;
    UPROPERTY(BlueprintReadOnly) bool bHasMultiplier = false;
    UPROPERTY(BlueprintReadOnly) double Multiplier = 1;
    UPROPERTY(BlueprintReadOnly) TMap<FName, double> Metrics;
    /** Match the whole accepted identity, not merely the run ID. */
    bool Matches(const FMiniGameContext& Context) const;
    /** Reject malformed values and missing configured metrics without coercing positive rewards. */
    bool Validate(const FMiniGameContext& Context) const;
    /** Construct an identity-correct zero result for cancellation and technical failures. */
    static FMiniGameResult Failure(const FMiniGameContext& Context, EMiniGameEndReason Reason);
};
