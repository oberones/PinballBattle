#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Data/ScoringTypes.h"
#include "Pinball/InteractionState.h"
#include "ScoringTargetComponent.generated.h"

class APinballTable;
class APinballBall;
class UStaticMeshComponent;
class UPointLightComponent;
class UInteractionFeedbackComponent;
class UPhysicalMaterial;

/** Passive collision remains Chaos-owned; this component reports separated contact episodes. */
UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UScoringTargetComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Poll geometric separation after physics, never re-arm by elapsed cooldown. */
    UScoringTargetComponent();
    /** Bind an explicit table and simple collision surface, replacing any old binding. */
    void Initialize(APinballTable* InTable, UPrimitiveComponent* InSurface);
    /** Identify this registered producer in diagnostic event coverage. */
    FGuid GetSourceId() const { return SourceId; }
    /** Clear the latch after genuine sphere/surface separation or identity invalidation. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    /** Remove the hit binding before this producer goes away. */
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring") EScoringCategory Category = EScoringCategory::Target;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics") TObjectPtr<UPhysicalMaterial> SurfaceMaterial;
    UPROPERTY(BlueprintAssignable) FTableScoringEvent OnScoringEvent;
    UPROPERTY(Transient) TObjectPtr<UInteractionFeedbackComponent> Feedback;
protected:
    /** Optional actuator response for a new contact; passive targets do not add energy. */
    virtual void OnQualifiedContact(APinballBall* Ball, const FHitResult& Hit);
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<UPrimitiveComponent> Surface;
private:
    /** Validate phase/ball, suppress support contacts, then emit exactly one typed event. */
    UFUNCTION() void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
    FContactEpisode Episode;
    FGuid SourceId;
    int64 Sequence = 0;
    int64 Epoch = -1;
};

/** Blueprint-friendly target host with simple thick collision and explicit feedback wiring. */
UCLASS()
class PINBALLBATTLE_API APinballScoringTarget : public AActor
{
    GENERATED_BODY()
public:
    /** Construct a passive stand-up target, lamp and independent feedback component. */
    APinballScoringTarget();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UScoringTargetComponent> Scoring;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UInteractionFeedbackComponent> Feedback;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> Flash;
};
