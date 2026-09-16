#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "MinigameTriggerComponent.generated.h"
class APinballTable;
class UMiniGameDefinition;
class UBoxComponent;

/** Physical exit/reentry latch plus identity-tagged objective requests. */
UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UMinigameTriggerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Sample physical occupancy after physics without relying on synthetic overlap callbacks. */
    UMinigameTriggerComponent();
    /** Bind the explicitly authored volume and reject missing objective/table configuration. */
    virtual void BeginPlay() override;
    /** Request once on entry and require a real departure after any accepted run. */
    virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Tick) override;
    /** Read the physical rearm latch; flow separately enforces its active-time return guard. */
    bool IsArmed() const { return bArmed; }
    /** Consume this traversal after flow atomically accepts it. */
    void MarkAccepted() { bArmed = false; }
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<APinballTable> Table;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UMiniGameDefinition> Definition;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ObjectiveId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Volume;
private:
    bool bWasInside = false;
    bool bArmed = true;
};
UCLASS()
class PINBALLBATTLE_API AMinigameObjective : public AActor
{
    GENERATED_BODY()
public:
    /** Create a visible primitive objective with an explicitly connected trigger component. */
    AMinigameObjective();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Volume;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UMinigameTriggerComponent> Trigger;
};
