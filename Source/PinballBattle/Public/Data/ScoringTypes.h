#pragma once
#include "CoreMinimal.h"
#include "Data/PinballSessionTypes.h"
#include "ScoringTypes.generated.h"

/** Producers describe an interaction; the central score service chooses its point value. */
UENUM(BlueprintType)
enum class EScoringCategory : uint8 { Target, Bumper, Lane };

/** Immutable contact/traversal identity, including the phase in which it was observed. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FScoringEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid BallId;
    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) FGuid SourceId;
    UPROPERTY(BlueprintReadOnly) EScoringCategory Category = EScoringCategory::Target;
    UPROPERTY(BlueprintReadOnly) int64 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) EArcadeGameFlowState ObservedPhase = EArcadeGameFlowState::BOOT;
    UPROPERTY(BlueprintReadOnly) int64 PhaseEpoch = 0;
};

/** Read-only output of the central score owner, never a producer-selected UI delta. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FScoreAward
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid AwardId;
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid RunId;
    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) int64 BasePoints = 0;
    UPROPERTY(BlueprintReadOnly) int32 EffectiveMultiplier = 1;
    UPROPERTY(BlueprintReadOnly) int64 AwardedPoints = 0;
    UPROPERTY(BlueprintReadOnly) int64 NewTotal = 0;
    UPROPERTY(BlueprintReadOnly) FName ProfileRevision;
};

/** Drain requests have identities but are never scoring categories. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FDrainEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid BallId;
    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) FGuid SourceId;
    UPROPERTY(BlueprintReadOnly) int64 PhaseEpoch = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTableScoringEvent, const FScoringEvent&, Event);
