#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidObstacle.generated.h"
class USphereComponent;
class UStaticMeshComponent;
/** A single run-owned drifting target; terminal hits latch before any notification. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAsteroidObstacle : public AActor
{
    GENERATED_BODY()
public:
    /** Construct simple query collision and a faceted placeholder rock. */
    AAsteroidObstacle();
    /** Drift and rebound only while the owning identity remains active. */
    virtual void Tick(float DeltaSeconds) override;
    /** Latch a projectile hit before asking the runtime to count it. */
    bool Hit(const FGuid& ShotRun);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
    FVector Velocity = FVector::ZeroVector;
    bool bDestroyed = false;
};
