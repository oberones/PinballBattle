#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseThreat.generated.h"
class USphereComponent;
class UStaticMeshComponent;

/** A single descending threat resolves as either an interception or an impact, never both. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API ADefenseThreat : public AActor
{
    GENERATED_BODY()
public:
    /** Create the sweep target and a readable incoming missile silhouette. */
    ADefenseThreat();
    /** Sweep descending motion through active blasts and colonies in contact order. */
    virtual void Tick(float DeltaSeconds) override;
    /** Ask the owning runtime to count this threat once. */
    bool Intercept(const FGuid& SourceRunId);
    /** Close contacts immediately and leave a brief active-time interception flash. */
    void Resolve(bool Intercepted);
    bool IsResolved() const { return bResolved; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
    FVector Velocity = FVector::ZeroVector;
private:
    bool bResolved = false;
    float FlashRemaining = 0;
};
