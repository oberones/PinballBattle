#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidProjectile.generated.h"
/** Fast planar shot with a sphere sweep, finite lifetime and explicit run identity. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAsteroidProjectile : public AActor
{
    GENERATED_BODY()
public:
    /** Build a small original projectile visual without physics simulation. */
    AAsteroidProjectile();
    /** Sweep the whole flight segment and consume this shot on its first target. */
    virtual void Tick(float DeltaSeconds) override;
    FGuid RunId;
    FVector Velocity = FVector::ZeroVector;
private:
    float Remaining = 1.8f;
};
