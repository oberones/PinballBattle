#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PinballTuningData.generated.h"

class UPhysicalMaterial;

/** Centimetres, kilograms, seconds and degrees. Shared immutable physical configuration. */
UCLASS(BlueprintType)
class PINBALLBATTLE_API UPinballTuningData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Table") float TableScale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Table") float InclineDegrees = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ball") float BallRadius = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ball") float BallMass = .08f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ball") float MaxBallSpeed = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ball") float LinearDamping = .03f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ball") TObjectPtr<UPhysicalMaterial> PhysicalMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Plunger") float MaxChargeSeconds = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Plunger") float MinLaunchImpulse = 65.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Plunger") float MaxLaunchImpulse = 105.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Plunger") float ChargeExponent = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flippers") float FlipperTravelDegrees = 55.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flippers") float FlipperStrength = 80000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flippers") float FlipperDamping = 4000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flippers") float FlipperMaxTorque = 10000000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flippers") float FlipperMass = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bumpers") float BumperImpulse = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bumpers") float BumperCooldown = .1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contacts") float ContactSeparationTolerance = 1.f;
    // Phase 3 recovery contract: 10-second trap window; exempt launch/capture areas.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") float TrapWindowSeconds = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") float LowMotionSpeed = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") bool bExemptLaunchAndCaptureAreas = true;

    /** Reject nonfinite or inconsistent tuning before any body starts simulating. */
    bool Validate(FString& OutError) const;
    /** Convert a bounded hold duration into a mass-based launcher impulse. */
    float LaunchImpulse(float ChargeSeconds) const;
#if WITH_EDITOR
    /** Expose runtime tuning validation to Unreal's asset validation system. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
