#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseBlastZone.generated.h"
class USphereComponent;
class UStaticMeshComponent;

/** Finite local-time blast; the threat/runtime terminal gate deduplicates overlapping zones. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API ADefenseBlastZone : public AActor
{
    GENERATED_BODY()
public:
    /** Construct a query volume and flat readable blast disk below missiles. */
    ADefenseBlastZone();
    /** Arm a fresh zone and immediately test threats already at the destination. */
    void Configure(const FGuid& InRunId, float Radius, float Lifetime);
    /** Expire in active time and intercept threats entering the live zone. */
    virtual void Tick(float DeltaSeconds) override;
    bool IsActive() const { return Remaining > 0; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
private:
    /** Query owned threats only; no contact can credit two blasts. */
    void InterceptOverlaps();
    float Remaining = 0;
    float Duration = 1;
    float MaximumRadius = 125;
};
