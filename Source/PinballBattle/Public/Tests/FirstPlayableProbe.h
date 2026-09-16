#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "FirstPlayableProbe.generated.h"

class APinballGameModeBase;
class APinballPlayerController;
class APinballTable;
class APinballBall;
class UPrimitiveComponent;

/** Opt-in development observer in the test map. Drives normal keyboard mappings and Chaos. */
UCLASS(NotBlueprintable)
class PINBALLBATTLE_API AFirstPlayableProbe : public AActor
{
    GENERATED_BODY()
public:
    AFirstPlayableProbe();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void Key(FKey Key, bool bPressed);
    void Finish(bool bSuccess, const FString& Reason);
    UFUNCTION() void ObserveHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* Other, FVector NormalImpulse, const FHitResult& Hit);
    UPROPERTY(Transient) TObjectPtr<APinballGameModeBase> Mode;
    UPROPERTY(Transient) TObjectPtr<APinballPlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    TWeakObjectPtr<APinballBall> ObservedBall;
    TMap<TWeakObjectPtr<UPrimitiveComponent>, double> LastContacts;
    float Clock = 0;
    float StageStart = 0;
    float LastSample = 0;
    float MaxTravel = 0;
    float LaunchSpeed = 0;
    float PeakSpeed = 0;
    float ShortLaunchSpeed = 0;
    float FullLaunchSpeed = 0;
    float SampledTime = 0;
    int32 SampledFrames = 0;
    float LeftMinAngle = 360;
    float LeftMaxAngle = -360;
    float RightMinAngle = 360;
    float RightMaxAngle = -360;
    int32 Stage = 0;
    int32 Cycles = 0;
    int32 TargetCycles = 7;
    int32 RelevantContacts = 0;
    int32 BumperContacts = 0;
    int32 FlipperContacts = 0;
    bool bLeft = false;
    bool bRight = false;
    bool bFinished = false;
};
