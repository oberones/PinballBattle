#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/MiniGameTypes.h"
#include "Tests/FlipperReturnCheck.h"
#include "PlanetaryDefenseProbe.generated.h"
class AMinigameObjective;
class APlanetaryDefenseRuntime;
class UReturnBallProbe;

/** Opt-in rendered acceptance through the actual controller, sweeps, score and return transaction. */
UCLASS()
class PINBALLBATTLE_API APlanetaryDefenseProbe : public AActor
{
    GENERATED_BODY()
public:
    /** Observe post-physics and native pause without advancing gameplay. */
    APlanetaryDefenseProbe();
    /** Remain passive unless the standalone acceptance flag is present. */
    virtual void BeginPlay() override;
    /** Drive a defended run and an initially undefended run through the common harness. */
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere) TObjectPtr<AMinigameObjective> Objective;
private:
    /** Send keys through PlayerController and Enhanced Input. */
    void Key(FKey Input, bool Pressed);
    /** Position a real viewport cursor by world projection; gameplay performs deprojection. */
    bool PointAt(const FVector& WorldPosition);
    /** Emit an explicit pass/fail marker and exit only the opted-in process. */
    void Finish(bool Passed, const FString& Detail);
    /** Observe the immutable terminal result without changing flow or scoring. */
    UFUNCTION() void ReceiveResult(const FMiniGameResult& Result);
    bool bEnabled = false;
    bool bPaused = false;
    bool bSawPause = false;
    bool bEscapeReleased = false;
    bool bPauseReleased = false;
    bool bAimChecked = false;
    bool bScreenshot = false;
    int32 Stage = 0;
    int32 Cycle = 0;
    int32 PauseShots = 0;
    int32 LossKills = -1;
    int32 ResultCount = 0;
    bool bResultSuccess = false;
    double ResultDuration = 0;
    double Started = 0;
    double Wait = 0;
    double PauseStarted = 0;
    double PausedClock = 0;
    int64 Baseline = 0;
    FGuid SessionId;
    FGuid BallId;
    FVector FrozenBall;
    FTransform SavedPrimaryReturn;
    FVector FrozenThreat;
    TWeakObjectPtr<AActor> ObservedThreat;
    TWeakObjectPtr<APlanetaryDefenseRuntime> ObservedRuntime;
    FFlipperReturnCheck ReturnControls;
    UPROPERTY(Transient) TObjectPtr<UReturnBallProbe> ReturnBall;
};
