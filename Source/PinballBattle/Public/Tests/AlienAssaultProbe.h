#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/FlipperReturnCheck.h"
#include "Data/MiniGameTypes.h"
#include "AlienAssaultProbe.generated.h"
class AMinigameObjective;
class UReturnBallProbe;
/** Opt-in rendered acceptance harness using the production transition and real projectiles. */
UCLASS()
class PINBALLBATTLE_API AAlienAssaultProbe : public AActor
{
    GENERATED_BODY()
public:
    /** Enable the observer tick, including native-pause observation. */
    AAlienAssaultProbe();
    /** Start only under the explicit command-line acceptance flag. */
    virtual void BeginPlay() override;
    /** Drive two round trips while asserting session, input, damage and scoring invariants. */
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere) TObjectPtr<AMinigameObjective> Objective;
private:
    /** Observe the actual accepted terminal payload before the shared return cleans it. */
    UFUNCTION() void ObserveResult(const FMiniGameResult& Result);
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
    int32 ResultsSeen = 0;
    int32 FeedbackSeen = 0;
    int32 LastKills = 0;
    FMiniGameResult TerminalResult;
    int64 Baseline = 0;
    FGuid SessionId;
    FGuid BallId;
    FVector FrozenBall;
    FTransform SavedPrimaryReturn;
    bool bEnabled = false;
    bool bPaused = false;
    bool bInputChecked = false;
    bool bRightTurnChecked = false;
    bool bReboundChecked = false;
    FFlipperReturnCheck ReturnControls;
    UPROPERTY(Transient) TObjectPtr<UReturnBallProbe> ReturnBall;
};
