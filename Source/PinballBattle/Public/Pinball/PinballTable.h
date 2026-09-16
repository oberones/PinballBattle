#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "PinballTable.generated.h"

class APinballBall;
class APinballFlipper;
class APinballPlunger;
class APinballBumper;
class APinballDrain;
class UCameraComponent;
class UTableSessionComponent;
class UPinballTuningData;
class UUserWidget;
class UBoxComponent;
class APinballScoringTarget;
class APinballLane;

UCLASS()
class PINBALLBATTLE_API APinballTable : public AActor
{
    GENERATED_BODY()
public:
    /** Construct explicit spawn, release, camera and recovery-volume references. */
    APinballTable();
    /** Validate/configure the complete registered physical inventory before play. */
    bool InitializeTable(FString& OutError);
    /** Allocate one fresh ball entitlement only when no previous ball actor remains. */
    bool SpawnReadyBall();
    /** Unregister and destroy the current ball before a replacement can be created. */
    void RemoveBall();
    /** Return actuators to neutral without firing a cancelled plunger. */
    void CancelActions();
    /** Check actor validity and explicit table ownership. */
    bool IsCurrentBall(const APinballBall* Ball) const;
    /** Return the sole owned ball actor, which is never the possessed pawn. */
    APinballBall* GetBall() const { return CurrentBall; }
    /** Expose the explicit physical-body manifest for lifecycle validation. */
    UTableSessionComponent* GetSession() const { return Session; }
    /** Return the inclined playfield normal for planar actuator impulses. */
    FVector GetTableNormal() const { return GetActorUpVector(); }
    /** Expose a read-only entitlement snapshot; recovery retains these IDs. */
    const FBallHandle& GetBallHandle() const { return BallHandle; }
    /** Return the invalidation epoch used to discard pre-recovery contact/lane progress. */
    int64 GetPhysicalEpoch() const { return PhysicalEpoch; }
    /** Open the active-ball disposition after the GameMode accepts a launch. */
    void NotifyBallLaunched();
    /** Independently gate score, actuator and drain producers by phase, identity and recovery. */
    bool CanEmitEvent(const APinballBall* Ball) const;
    /** Stamp a category event with the current session, entitlement and physical epoch. */
    FScoringEvent MakeScoringEvent(FGuid SourceId, EScoringCategory Category, int64 Sequence) const;
    /** Publish typed producer events without calculating or mutating score. */
    void PublishScoringEvent(const FScoringEvent& Event);
    /** Check escape bounds and the ten-second low-motion window after each physics step. */
    virtual void Tick(float DeltaSeconds) override;
    /** Secure the same ball, then try collision-checked primary and backup release paths. */
    bool RecoverBall();
    /** Count successful relocations for development evidence, not scoring. */
    int32 GetRecoveryCount() const { return RecoveryCount; }
    /** Expose accepted interaction counts to the temporary practice presentation. */
    int32 GetInteractionCount(EScoringCategory Category) const { return InteractionCounts[static_cast<uint8>(Category)]; }
    UPROPERTY(BlueprintAssignable) FTableScoringEvent OnScoringEvent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UPinballTuningData> Tuning;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") TSubclassOf<APinballBall> BallClass;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballFlipper> LeftFlipper;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballFlipper> RightFlipper;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballPlunger> Plunger;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TArray<TObjectPtr<APinballBumper>> Bumpers;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballDrain> Drain;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TArray<TObjectPtr<APinballScoringTarget>> Targets;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TArray<TObjectPtr<APinballLane>> Lanes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") bool bRequireCompleteInventory = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recovery") TObjectPtr<UBoxComponent> EscapeBounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recovery") TObjectPtr<UBoxComponent> LaunchExemption;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Recovery") TArray<TObjectPtr<UBoxComponent>> CaptureExemptions;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recovery") TObjectPtr<USceneComponent> PrimaryReturn;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recovery") TObjectPtr<USceneComponent> BackupReturn;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") FVector ReturnVelocity = FVector(0, -220, 0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSubclassOf<UUserWidget> ControlsWidgetClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<USceneComponent> BallSpawn;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UCameraComponent> TableCamera;
protected:
    /** Register this explicit table with the world-local GameMode. */
    virtual void BeginPlay() override;
    /** Cancel owned actuators and remove the ball during world teardown. */
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    /** Reject occupied release spheres and swept exit paths, including trigger/drain volumes. */
    bool IsSafeRelease(const FVector& Position, const FVector& Velocity) const;
    /** Test an oriented volume using the ball centre in unscaled local coordinates. */
    bool IsInside(const UBoxComponent* Volume, const FVector& Position) const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTableSessionComponent> Session;
    UPROPERTY(Transient) TObjectPtr<APinballBall> CurrentBall;
    UPROPERTY(Transient) FBallHandle BallHandle;
    int64 PhysicalEpoch = 0;
    double EventProtectionUntil = 0;
    double RecoveryRetryAt = 0;
    float LowMotionSeconds = 0;
    FVector TrapAnchor = FVector::ZeroVector;
    int32 RecoveryCount = 0;
    int32 InteractionCounts[3] = {0, 0, 0};
};
