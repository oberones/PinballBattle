#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "Pinball/InteractionState.h"
#include "Pinball/TableSuspendParticipant.h"
#include "LaneProgressComponent.generated.h"

class APinballTable;
class UBoxComponent;
class UPointLightComponent;
class UInteractionFeedbackComponent;
class UStaticMeshComponent;

/** Nonblocking directed lane. Samples swept centre paths so a fast ball cannot skip a gate. */
UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API ULaneProgressComponent : public UActorComponent, public ITableSuspendParticipant
{
    GENERATED_BODY()
public:
    /** Discard traversal continuity across safe relocation, preventing a synthetic lane completion. */
    virtual void CommitRestore(int64 Generation) override;
    /** Evaluate traversal after table recovery and the current physics step. */
    ULaneProgressComponent();
    /** Bind the explicit oriented corridor, where local +Y is the scoring direction. */
    void Initialize(APinballTable* InTable, UBoxComponent* InCorridor);
    /** Identify the registered lane in diagnostic traversal coverage. */
    FGuid GetSourceId() const { return SourceId; }
    /** Advance entry/exit state only for the current playable ball and physical epoch. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    UPROPERTY(BlueprintAssignable) FTableScoringEvent OnScoringEvent;
    UPROPERTY(Transient) TObjectPtr<UInteractionFeedbackComponent> Feedback;
private:
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<UBoxComponent> Corridor;
    FLaneTraversal Traversal;
    FGuid SourceId;
    FGuid BallId;
    FVector Previous = FVector::ZeroVector;
    int64 Epoch = -1;
    int64 Sequence = 0;
    bool bHasPrevious = false;
};

UCLASS()
class PINBALLBATTLE_API APinballLane : public AActor
{
    GENERATED_BODY()
public:
    /** Build a nonblocking lane sensor and floor insert, leaving guide geometry to content. */
    APinballLane();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Corridor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULaneProgressComponent> Progress;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Insert;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UInteractionFeedbackComponent> Feedback;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> Flash;
};
