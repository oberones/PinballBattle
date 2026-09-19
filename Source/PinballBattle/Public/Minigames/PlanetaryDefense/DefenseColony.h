#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseColony.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class ADefenseThreat;

/** One-hit colony owned by a single initialized run; destruction leaves visible debris. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API ADefenseColony : public AActor
{
    GENERATED_BODY()
public:
    /** Construct a query-only impact body and primitive colony dome. */
    ADefenseColony();
    /** Route a threat contact through the owning runtime's once-only gate. */
    bool Impact(ADefenseThreat* Threat, const FGuid& SourceRunId);
    /** Latch loss and disable further colony contacts. */
    void Lose();
    bool IsAlive() const { return bAlive; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    FGuid RunId;
private:
    bool bAlive = true;
};
