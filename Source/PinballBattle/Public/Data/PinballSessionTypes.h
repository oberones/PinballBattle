#pragma once

#include "CoreMinimal.h"
#include "PinballSessionTypes.generated.h"

/** Shared flow vocabulary. Gameplay transitions are introduced with their playable phases. */
UENUM(BlueprintType)
enum class EArcadeGameFlowState : uint8
{
    BOOT,
    ATTRACT,
    PINBALL_READY,
    PINBALL_PLAYING,
    MINIGAME_TRANSITION,
    MINIGAME_PLAYING,
    MINIGAME_RESULTS,
    BALL_LOST,
    GAME_OVER,
    PAUSED
};

class APinballBall;

/** Physical disposition is separate from the session's legal flow state. */
UENUM(BlueprintType)
enum class EBallDisposition : uint8 { Ready, Active, Suspended, Drained, Recovering };

/** One ball entitlement. Recovery moves its actor without allocating a new BallId. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FBallHandle
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid BallId;
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<APinballBall> Ball;
    UPROPERTY(BlueprintReadOnly) EBallDisposition Disposition = EBallDisposition::Ready;
};

/** Read-only session projection; GameMode and flow are its only writers. */
USTRUCT(BlueprintType)
struct PINBALLBATTLE_API FSessionState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SessionId;
    UPROPERTY(BlueprintReadOnly) FGuid CurrentBallId;
    UPROPERTY(BlueprintReadOnly) int64 Generation = 0;
    UPROPERTY(BlueprintReadOnly) int32 BallsRemaining = 3;
    UPROPERTY(BlueprintReadOnly) int32 Multiplier = 1;
    UPROPERTY(BlueprintReadOnly) EArcadeGameFlowState FlowState = EArcadeGameFlowState::BOOT;
    UPROPERTY(BlueprintReadOnly) EArcadeGameFlowState ResumeState = EArcadeGameFlowState::BOOT;
    UPROPERTY(BlueprintReadOnly) TMap<FName, bool> ObjectiveStates;
};
