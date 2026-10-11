#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssaultProjectile.generated.h"
/** Fast planar shot with a sphere sweep, finite lifetime and explicit run identity. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAssaultProjectile : public AActor
{
    GENERATED_BODY()
public:
    /** Build a small original projectile visual without physics simulation. */
    AAssaultProjectile();
    /** Sweep the whole flight segment and consume this shot on its first target. */
    virtual void Tick(float DeltaSeconds) override;
    FGuid RunId;
    FVector Velocity = FVector::ZeroVector;
    bool bEnemyShot = false;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Visual;
private:
    float Remaining = 5.f;
};
