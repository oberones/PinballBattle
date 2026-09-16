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
