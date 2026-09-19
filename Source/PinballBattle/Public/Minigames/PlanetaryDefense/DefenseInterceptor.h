#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseInterceptor.generated.h"
class UStaticMeshComponent;

/** Fast swept shot with a fixed destination captured at launch; ammunition is unlimited. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API ADefenseInterceptor : public AActor
{
    GENERATED_BODY()
public:
    /** Construct an original bright projectile mesh without blocking friendly actors. */
    ADefenseInterceptor();
    /** Sweep the traveled segment and detonate once on contact or exact destination. */
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
    FVector Destination = FVector::ZeroVector;
    float Speed = 1200;
private:
    bool bDetonated = false;
};
