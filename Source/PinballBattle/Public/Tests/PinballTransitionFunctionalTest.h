#pragma once
#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "Data/TransitionTypes.h"
#include "Pinball/TableSuspendParticipant.h"
#include "Components/ActorComponent.h"
#include "PinballTransitionFunctionalTest.generated.h"
class AMinigameObjective;
class APinballBall;
class AMiniGameTestRuntime;
class UCameraComponent;

/** Injects an unacknowledged restore without weakening the shipping participant contract. */
UCLASS()
class UTransitionTestParticipant : public UActorComponent, public ITableSuspendParticipant
{
    GENERATED_BODY()
public:
    /** Hold restoration until the fixture simulates an explicit successful repair. */
    virtual bool PrepareRestore(int64 Generation) override { return !bRejectRestore && Generation > 0; }
    bool bRejectRestore = false;
};

/** Rendered fixture exercises the shipping flow against a real table and streamed arena. */
UCLASS()
class PINBALLBATTLE_API APinballTransitionFunctionalTest : public AFunctionalTest
{
    GENERATED_BODY()
public:
    /** Enable opt-in validation ticks, including observation while the game is paused. */
    APinballTransitionFunctionalTest();
    /** Start only for the command-line probe or an explicit functional-test request. */
    virtual void BeginPlay() override;
    /** Reset counters and the wall-clock safety deadline for one acceptance run. */
    virtual void StartTest() override;
    /** Observe real body/score/session invariants across ten normal and injected-failure cycles. */
    virtual void Tick(float DeltaSeconds) override;
    /** Record one explicit pass/fail and exit only an opt-in standalone probe. */
    void Complete(bool Success, const FString& Message);
    /** Expose final status to the latent Automation wrapper. */
    bool HasCompleted() const { return bCompleted; }
    /** Expose final pass/fail without treating process exit as proof. */
    bool DidPass() const { return bPassed; }
    UPROPERTY(EditAnywhere) TObjectPtr<AMinigameObjective> Objective;
private:
    /** Inject physical key transitions through the real controller input stack. */
    void Key(FKey Input, bool Pressed);
    /** Pause synchronously at acceptance so even the one-frame Securing phase is covered. */
    UFUNCTION() void ObservePhase(const FSessionState& State);
    double StartedAt = 0;
    double PauseAt = 0;
    double SavedClock = 0;
    int32 Cycle = 0;
    int32 Stage = 0;
    int64 BaselineScore = 0;
    int32 Balls = 0;
    FGuid SessionId;
    FGuid BallId;
    FVector FrozenPosition;
    TSet<ETransitionPhase> PausedPhases;
    bool bProbe = false;
    bool bCompleted = false;
    bool bPassed = false;
    bool bFrozen = false;
    bool bInjected = false;
    double EntryWait = 0;
    int32 InputStep = 0;
    UPROPERTY() TObjectPtr<AMiniGameTestRuntime> FixtureRoot;
    UPROPERTY() TObjectPtr<UCameraComponent> SavedCamera;
    FTransform PrimaryReturn;
    FTransform BackupReturn;
    UPROPERTY() TObjectPtr<UTransitionTestParticipant> StalledParticipant;
    FTimerHandle ActiveTableTimer;
    FTimerHandle PausedTableTimer;
    int32 TimerCallbacks = 0;
    bool bInstructionShot = false;
    bool bResultsShot = false;
    bool bTimeoutActions = false;
    bool bNearTimeoutPause = false;
    double SavedRunClock = 0;
};
