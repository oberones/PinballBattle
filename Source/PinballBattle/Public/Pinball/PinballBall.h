#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pinball/TableSuspendParticipant.h"
#include "PinballBall.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPinballTuningData;

UCLASS()
class PINBALLBATTLE_API APinballBall : public AActor, public ITableSuspendParticipant
{
    GENERATED_BODY()
public:
    /** A consumed/unlaunched ball cannot participate in a minigame transaction. */
    virtual bool CaptureState(int64 Generation) override;
    /** Recreate an existing active entitlement without firing a launch impulse. */
    void RestoreActiveEntitlement();
    /** Construct the CCD sphere root and noncolliding visual shell. */
    APinballBall();
    /** Apply validated dimensions, mass, material and damping before launch. */
    void Configure(UPinballTuningData* InTuning);
    /** Start this entitlement once and apply the configured mass-based plunger impulse. */
    bool Launch(const FVector& Direction, float Impulse);
    /** Add only the impulse that fits the speed budget, preserving tangential motion and spin. */
    void AddBoundedImpulse(const FVector& Impulse);
    /** Atomically close collision and launch eligibility on the first accepted drain. */
    bool MarkDrained();
    /** Report whether this ball is launched and has not been consumed by a drain. */
    bool IsLaunched() const { return bLaunched && !bDrained; }
    /** Expose the single Chaos body for table registration and safe recovery. */
    USphereComponent* GetBody() const { return Body; }
    /** Return raw collision callback count for diagnostics, never for score calculation. */
    int32 GetContactCount() const { return ContactCount; }
    /** Bound solver-generated extreme speed after physics without imposing planar motion. */
    virtual void Tick(float DeltaSeconds) override;
private:
    /** Count physical notifications independently of deduplicated gameplay events. */
    UFUNCTION() void OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(Transient) TObjectPtr<UPinballTuningData> Tuning;
    bool bLaunched = false;
    bool bDrained = false;
    int32 ContactCount = 0;
};
