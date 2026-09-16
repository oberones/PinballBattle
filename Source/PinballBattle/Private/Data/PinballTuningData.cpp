#include "Data/PinballTuningData.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/DataValidation.h"

bool UPinballTuningData::Validate(FString& OutError) const
{
    const float PositiveValues[] = {TableScale, InclineDegrees, BallRadius, BallMass, MaxBallSpeed,
        MaxChargeSeconds, MinLaunchImpulse, MaxLaunchImpulse, ChargeExponent, FlipperTravelDegrees,
        FlipperStrength, FlipperDamping, FlipperMaxTorque, FlipperMass, BumperImpulse,
        BumperCooldown, TrapWindowSeconds, LowMotionSpeed};
    for (const float Value : PositiveValues)
    {
        if (!FMath::IsFinite(Value) || Value <= 0.f)
        {
            OutError = TEXT("Physical dimensions, masses, speeds, drives and durations must be positive and finite.");
            return false;
        }
    }
    if (!FMath::IsFinite(LinearDamping) || LinearDamping < 0.f || InclineDegrees >= 30.f ||
        FlipperTravelDegrees >= 90.f || MaxLaunchImpulse < MinLaunchImpulse ||
        MaxLaunchImpulse / BallMass > MaxBallSpeed || TrapWindowSeconds != 10.f ||
        !bExemptLaunchAndCaptureAreas || !PhysicalMaterial ||
        !FMath::IsFinite(PhysicalMaterial->Friction) || PhysicalMaterial->Friction < 0.f ||
        !FMath::IsFinite(PhysicalMaterial->Restitution) || PhysicalMaterial->Restitution < 0.f ||
        PhysicalMaterial->Restitution > 1.f)
    {
        OutError = TEXT("Invalid damping, incline, travel, impulse bounds, recovery policy or physical material.");
        return false;
    }
    OutError.Reset();
    return true;
}

float UPinballTuningData::LaunchImpulse(float ChargeSeconds) const
{
    if (!FMath::IsFinite(ChargeSeconds) || !FMath::IsFinite(MaxChargeSeconds) || MaxChargeSeconds <= 0.f)
    {
        return 0.f;
    }
    return FMath::Lerp(MinLaunchImpulse, MaxLaunchImpulse,
        FMath::Pow(FMath::Clamp(ChargeSeconds / MaxChargeSeconds, 0.f, 1.f), ChargeExponent));
}

#if WITH_EDITOR
EDataValidationResult UPinballTuningData::IsDataValid(FDataValidationContext& Context) const
{
    FString Error;
    if (!Validate(Error))
    {
        Context.AddError(FText::FromString(Error));
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif
