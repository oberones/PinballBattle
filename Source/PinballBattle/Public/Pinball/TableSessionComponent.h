#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TableSessionComponent.generated.h"

class UPrimitiveComponent;

/** Explicit table body manifest; suspension is added when the minigame contract needs it. */
UCLASS()
class PINBALLBATTLE_API UTableSessionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTableSessionComponent();
    void RegisterBody(UPrimitiveComponent* Body);
    void UnregisterBody(UPrimitiveComponent* Body);
    const TArray<TWeakObjectPtr<UPrimitiveComponent>>& GetBodies() const { return Bodies; }
private:
    UPROPERTY(Transient) TArray<TWeakObjectPtr<UPrimitiveComponent>> Bodies;
};
