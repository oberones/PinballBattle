#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DefenseAimPawn.generated.h"
class APlanetaryDefenseRuntime;
class UInputAction;
class UStaticMeshComponent;

/** Absolute mouse targeting, independent of viewport resolution and arena slot transform. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API ADefenseAimPawn : public APawn
{
    GENERATED_BODY()
public:
    /** Construct a collision-free reticle that always shows the actual clamped destination. */
    ADefenseAimPawn();
    /** Update target before polling the held Enhanced Input fire value. */
    virtual void Tick(float DeltaSeconds) override;
    /** Bind action values without storing held flags across pause or possession. */
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    /** Store the immutable run link separately from the pawn's controller-owned Actor Owner. */
    void ConfigureRun(APlanetaryDefenseRuntime* Runtime);
    /** Project a valid initial target into the ready camera's viewport. */
    void InitializeCursor();
    /** Deproject the current cursor onto the arena plane; retain last valid aim on failure. */
    bool UpdateMouseAim();
    /** Transform a world ray to the arena plane and clamp it, rejecting parallel/behind rays. */
    bool AimFromRay(const FVector& Origin, const FVector& Direction);
    FVector GetLocalAim() const { return LocalAim; }
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> FireAction;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Reticle;
private:
    /** Update the reticle and launcher direction from the accepted target. */
    void PresentAim();
    UPROPERTY(Transient) TWeakObjectPtr<APlanetaryDefenseRuntime> RunRuntime;
    FGuid RunId;
    FVector LocalAim = FVector(0, 100, 0);
};
