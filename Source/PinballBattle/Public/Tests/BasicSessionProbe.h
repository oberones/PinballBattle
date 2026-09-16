#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "InputCoreTypes.h"
#include "Tests/SessionProbeBudget.h"
#include "BasicSessionProbe.generated.h"

class APinballGameModeBase;
class APinballPlayerController;
class APinballGameStateBase;
class APinballTable;

/** Opt-in rendered acceptance of the real cabinet, input, physics, menus and session lifecycle. */
UCLASS(NotBlueprintable)
class PINBALLBATTLE_API ABasicSessionProbe : public AActor
{
    GENERATED_BODY()
public:
    /** Tick through native pause so real-time observations can verify a frozen world. */
    ABasicSessionProbe();
    /** Enable this development harness only for an explicit command-line request. */
    virtual void BeginPlay() override;
    /** Drive three scored sessions, three moving-ball pauses, two restarts and production Quit. */
    virtual void Tick(float DeltaSeconds) override;
private:
    /** Inject a press/release through the controller's real Enhanced Input path. */
    void Key(FKey Input, bool bPressed);
    /** Hit-test a visible menu button and route a real Slate pointer press/release to it. */
    bool ClickMenuButton(FName Name);
    /** Send Enter through the focused Slate widget instead of invoking a controller intent. */
    bool ConfirmMenu();
    /** Run isolated spawn-failure, retry, publication and bumper-cooldown fixtures on the real cabinet. */
    void TickRemediation(double Age);
    /** Check reset publication while callbacks can synchronously inspect every authoritative owner. */
    UFUNCTION() void ObserveResetSession(const FSessionState& Snapshot);
    /** Check that score-reset notifications reference the committed ready session. */
    UFUNCTION() void ObserveResetScore(const FScoreAward& Award);
    /** Obstruct the actual spawn, verify preserved terminal state, then retry through the controller. */
    bool CheckFailedStartAndRetry();
    /** Inject one qualified surface contact while observing the production coil and score pipeline. */
    void StrikeTestBumper();
    /** Begin a new timed harness step using wall time so pause cannot stall validation. */
    void Step(int32 Next);
    /** Record bounded acceptance evidence and exit through the actual Quit intent on success. */
    void Finish(bool bSuccess, const FString& Reason);
    /** Capture the rendered table and UI to a named local evidence image. */
    void Screenshot(const FString& Name);
    UPROPERTY(Transient) TObjectPtr<APinballGameModeBase> Mode;
    UPROPERTY(Transient) TObjectPtr<APinballPlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<APinballGameStateBase> State;
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    int32 Stage = 0;
    int32 Sessions = 0;
    int32 Drains = 0;
    int32 Pauses = 0;
    int32 Launches = 0;
    double StartedAt = 0;
    double StepAt = 0;
    double PlayingAt = 0;
    double PausedWorldTime = 0;
    int64 PausedScore = 0;
    int64 LockedScore = 0;
    FTransform PausedBall;
    FTransform PausedLeft;
    FTransform PausedRight;
    FVector PausedVelocity;
    FGuid PreviousSession;
    FGuid PreviousBall;
    FScoringEvent StaleScore;
    FDrainEvent StaleDrain;
    bool bFinished = false;
    FSessionProbeBudget Budget;
    bool bRemediation = false;
    bool bObservingReset = false;
    bool bResetObservationFailed = false;
    int32 ResetScoreNotifications = 0;
    int32 ResetReadyNotifications = 0;
    float OriginalBumperCooldown = 0;
};
