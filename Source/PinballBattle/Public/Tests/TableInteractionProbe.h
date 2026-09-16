#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "InputCoreTypes.h"
#include "TableInteractionProbe.generated.h"

class APinballTable;
class APinballGameModeBase;
class APinballPlayerController;

/** Development-only opt-in: mapped natural play, then explicitly separated fault/shot fixtures. */
UCLASS(NotBlueprintable)
class PINBALLBATTLE_API ATableInteractionProbe : public AActor
{
    GENERATED_BODY()
public:
    /** Observe after physics and after lane producers have processed the current sample. */
    ATableInteractionProbe();
    /** Enable only with the command-line validation switch, never in Shipping. */
    virtual void BeginPlay() override;
    /** Run bounded natural cycles followed by labelled recovery and collision fixtures. */
    virtual void Tick(float DeltaSeconds) override;
private:
    /** Inject ordinary keyboard events through the production Enhanced Input path. */
    void Key(FKey Input, bool bPressed);
    /** Fail/pass once, persist evidence and terminate this standalone validation process. */
    void Finish(bool bSuccess, const FString& Reason);
    /** Check all typed identity fields and record unique event/source coverage. */
    UFUNCTION() void ObserveEvent(const FScoringEvent& Event);
    /** Position a fixture ball in table coordinates, clearing spin and stale motion. */
    void Place(const FVector& LocalPosition, const FVector& LocalVelocity, bool bSimulate = true);
    /** Start a timed adversarial fixture without mixing its events into natural coverage. */
    void StartFixture(int32 Index);
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<APinballGameModeBase> Mode;
    UPROPERTY(Transient) TObjectPtr<APinballPlayerController> Controller;
    TSet<FGuid> EventIds;
    TMap<FGuid, int32> SourceCounts;
    TMap<FGuid, int32> NaturalSources;
    TMap<FGuid, int64> LastSequences;
    FGuid SavedBallId;
    FGuid SavedSessionId;
    FGuid ExpectedSource;
    FDrainEvent StaleDrain;
    FVector PrimaryPosition;
    FVector BackupPosition;
    float Clock = 0;
    float StageStart = 0;
    float FixtureStart = 0;
    int32 Stage = 0;
    int32 Cycles = 0;
    int32 TargetCycles = 8;
    int32 Fixture = -1;
    int32 BeforeEvents = 0;
    int32 BeforeRecovery = 0;
    bool bLeft = false;
    bool bRight = false;
    bool bFinished = false;
};
