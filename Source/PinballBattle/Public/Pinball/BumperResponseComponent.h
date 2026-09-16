#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Pinball/ScoringTargetComponent.h"
#include "BumperResponseComponent.generated.h"

class APinballTable;
class UStaticMeshComponent;

UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UBumperResponseComponent : public UScoringTargetComponent
{
    GENERATED_BODY()
public:
    /** Identify this contact producer as a powered bumper. */
    UBumperResponseComponent();
    /** Forget the prior game's coil activation so the first fresh contact can always impart an impulse. */
    void ResetForNewSession();
private:
    /** Fire the coil only on a new ring contact; preserve tangent motion and sphere spin. */
    virtual void OnQualifiedContact(APinballBall* Ball, const FHitResult& Hit) override;
    double LastImpulseTime = 0;
    bool bHasImpulse = false;
};

/** Simple reusable host; the response also works on other explicitly registered surfaces. */
UCLASS()
class PINBALLBATTLE_API APinballBumper : public AActor
{
    GENERATED_BODY()
public:
    /** Construct a simple cylinder, contact-driven coil and presentation-only flash/audio. */
    APinballBumper();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UStaticMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UBumperResponseComponent> Response;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UInteractionFeedbackComponent> Feedback;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> Flash;
};
