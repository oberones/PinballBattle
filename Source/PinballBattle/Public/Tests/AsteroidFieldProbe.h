#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidFieldProbe.generated.h"
class AMinigameObjective;
/** Opt-in rendered acceptance harness using the production transition and real projectiles. */
UCLASS()
class PINBALLBATTLE_API AAsteroidFieldProbe : public AActor
{
    GENERATED_BODY()
public:
    /** Enable the observer tick, including native-pause observation. */
    AAsteroidFieldProbe();
    /** Start only under the explicit command-line acceptance flag. */
    virtual void BeginPlay() override;
    /** Drive two round trips while asserting session, input, damage and scoring invariants. */
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere) TObjectPtr<AMinigameObjective> Objective;
private:
    /** Inject a physical key transition through the actual Enhanced Input stack. */
    void Key(FKey Key, bool Pressed);
    /** Emit an explicit result marker and terminate only the opted-in standalone process. */
    void Finish(bool Passed, const FString& Detail);
    double Started = 0;
    double Wait = 0;
    double PauseStarted = 0;
    double PausedClock = 0;
    int32 Stage = 0;
    int32 Cycle = 0;
    int32 DamageCount = 0;
    int64 Baseline = 0;
    FGuid SessionId;
    FGuid BallId;
    FVector FrozenBall;
    bool bEnabled = false;
    bool bPaused = false;
    bool bInputChecked = false;
    bool bRightTurnChecked = false;
    bool bReboundChecked = false;
};
