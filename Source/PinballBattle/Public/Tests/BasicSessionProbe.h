#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "InputCoreTypes.h"
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
};
