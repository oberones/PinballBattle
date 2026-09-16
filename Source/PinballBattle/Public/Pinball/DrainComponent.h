#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "DrainComponent.generated.h"

class APinballTable;

UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UDrainComponent : public UBoxComponent
{
    GENERATED_BODY()
public:
    /** Configure a query-only ball overlap volume; draining never alters physical rebound. */
    UDrainComponent();
    /** Assign the explicit table and a world-session source identity. */
    void Initialize(APinballTable* InTable) { Table = InTable; SourceId = FGuid::NewGuid(); }
private:
    /** Submit an identified request; GameMode closes the gate before destroying the ball. */
    UFUNCTION() void OnEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    FGuid SourceId;
};

UCLASS()
class PINBALLBATTLE_API APinballDrain : public AActor
{
    GENERATED_BODY()
public:
    /** Provide a reusable Blueprint host for the single drain trigger. */
    APinballDrain();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UDrainComponent> Drain;
};
