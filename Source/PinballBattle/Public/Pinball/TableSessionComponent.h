#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/TransitionTypes.h"
#include "TableSessionComponent.generated.h"

class UPrimitiveComponent;

/** Explicit table body, participant and timer manifests for staged minigame suspension/restoration. */
UCLASS()
class PINBALLBATTLE_API UTableSessionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Maintain explicit manifests without ticking a second flow owner. */
    UTableSessionComponent();
    /** Add a body to the suspension barrier. */
    void RegisterBody(UPrimitiveComponent* Body);
    /** Remove a destroyed entitlement from the barrier. */
    void UnregisterBody(UPrimitiveComponent* Body);
    /** Expose registered bodies for diagnostics. */
    const TArray<TWeakObjectPtr<UPrimitiveComponent>>& GetBodies() const { return Bodies; }
    /** Register owned actors/components whose ticks and optional hooks must freeze. */
    void RegisterParticipant(UObject* Participant);
    /** Register an owned timer; only timers active at capture resume at commit. */
    void RegisterTimer(FTimerHandle Timer);
    /** Capture the complete manifest before cancelling actuators and disabling physics. */
    bool CaptureAndSuspend(int64 Generation, FTableSuspendSnapshot& Snapshot);
    /** Validate a safe release and stage body state with every producer still held. */
    bool PrepareRestore(FTableSuspendSnapshot& Snapshot);
    /** Resume prevalidated flags, velocities and previously active timers without latent work. */
    void CommitRestore(FTableSuspendSnapshot& Snapshot);
    /** Discard a failed transaction for an explicitly requested new session. */
    void ResetSuspension(FTableSuspendSnapshot& Snapshot);
private:
    UPROPERTY(Transient) TArray<TWeakObjectPtr<UPrimitiveComponent>> Bodies;
    UPROPERTY(Transient) TArray<TWeakObjectPtr<UObject>> Participants;
    TArray<FTimerHandle> Timers;
    int64 SuspendedGeneration = 0;
};
