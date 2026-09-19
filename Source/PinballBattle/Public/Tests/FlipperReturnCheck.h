#pragma once
#include "CoreMinimal.h"
class APinballTable;
class APlayerController;

/** Rendered input/physics assertion shared by the real-minigame acceptance probes. */
struct FFlipperReturnCheck
{
    enum class EResult { Running, Passed, Failed };
    /** Press each actual arrow independently, measure travel, then verify neutral release. */
    EResult Tick(APinballTable* Table, APlayerController* Controller, float DeltaSeconds, FString& Failure);
    /** Start fresh after each round trip. */
    void Reset() { *this = FFlipperReturnCheck(); }
private:
    int32 Step = 0;
    float Age = 0;
    float LeftRest = 0;
    float RightRest = 0;
    float LeftTravel = 0;
    float RightTravel = 0;
};
