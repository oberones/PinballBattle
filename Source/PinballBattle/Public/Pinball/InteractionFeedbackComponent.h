#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionFeedbackComponent.generated.h"

class UPointLightComponent;
class USoundBase;

/** Local presentation only: a short light pulse and original authored sound per accepted event. */
UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UInteractionFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Enable a fading presentation tick that never participates in score or physics. */
    UInteractionFeedbackComponent();
    /** Bind the host's explicit lamp; no component-name searches are required. */
    void Initialize(UPointLightComponent* InLight);
    /** Present one accepted interaction and restart its bounded flash envelope. */
    void Pulse();
    /** Fade using active world time, so native pause freezes the envelope. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") TObjectPtr<USoundBase> HitSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") float FlashSeconds = .16f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") FLinearColor Color = FLinearColor(.1f, .8f, 1.f);
private:
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
    float Remaining = 0;
};
