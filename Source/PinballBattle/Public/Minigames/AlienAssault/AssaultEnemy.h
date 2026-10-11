#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssaultEnemy.generated.h"
class USphereComponent;
class UStaticMeshComponent;
/** A single run-owned formation member; terminal hits latch before any notification. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAssaultEnemy : public AActor
{
    GENERATED_BODY()
public:
    /** Construct simple query collision and a flattened saucer silhouette. */
    AAssaultEnemy();
    /** Move and fire only while the owning identity remains active. */
    virtual void Tick(float DeltaSeconds) override;
    /** Latch a projectile hit before asking the runtime to count it. */
    bool Hit(const FGuid& ShotRun);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
    FVector FormationPosition = FVector::ZeroVector;
    int32 SlotIndex = INDEX_NONE;
    double NextShot = 0;
    bool bDestroyed = false;
};
