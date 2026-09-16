#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pinball/TableSuspendParticipant.h"
#include "PinballFlipper.generated.h"

class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class UPinballTuningData;

UCLASS()
class PINBALLBATTLE_API APinballFlipper : public AActor, public ITableSuspendParticipant
{
    GENERATED_BODY()
public:
    /** Disable drive motors in addition to the table-owned body freeze. */
    virtual bool SuspendForMinigame(int64 Generation) override;
    /** Stage a neutral authored pose while motors and simulation remain disabled. */
    virtual bool PrepareRestore(int64 Generation) override;
    /** Reenable the configured neutral drive at the table's commit boundary. */
    virtual void CommitRestore(int64 Generation) override;
    APinballFlipper();
    void Configure(UPinballTuningData* Tuning);
    void SetHeld(bool bInHeld);
    UStaticMeshComponent* GetBody() const { return Body; }
    bool IsHeld() const { return bHeld; }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") bool bReverseDrive = false;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> Hinge;
    float HalfTravel = 27.5f;
    bool bHeld = false;
    bool bSuspended = false;
    FTransform NeutralTransform;
};
