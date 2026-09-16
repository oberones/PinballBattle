#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/PinballTuningData.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPinballTuningTest, "PinballBattle.Practice.TuningValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPinballTuningTest::RunTest(const FString&)
{
    UPinballTuningData* Tuning = NewObject<UPinballTuningData>();
    Tuning->PhysicalMaterial = NewObject<UPhysicalMaterial>(Tuning);
    FString Error;
    TestTrue(TEXT("Valid physical defaults"), Tuning->Validate(Error));
    TestEqual(TEXT("Short charge starts at minimum impulse"), Tuning->LaunchImpulse(0), Tuning->MinLaunchImpulse);
    TestEqual(TEXT("Full charge is bounded even after a long hold"), Tuning->LaunchImpulse(100), Tuning->MaxLaunchImpulse);
    TestTrue(TEXT("Short and full launches are distinct"), Tuning->LaunchImpulse(.1f) < Tuning->LaunchImpulse(1.25f));
    Tuning->BallMass = 0;
    TestFalse(TEXT("Zero mass rejected"), Tuning->Validate(Error));
    Tuning->BallMass = .08f;
    Tuning->FlipperStrength = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite physical drive rejected"), Tuning->Validate(Error));
    Tuning->FlipperStrength = 80000;
    Tuning->MaxLaunchImpulse = Tuning->MinLaunchImpulse - 1;
    TestFalse(TEXT("Inverted impulse range rejected"), Tuning->Validate(Error));
    Tuning->MaxLaunchImpulse = 105;
    Tuning->LinearDamping = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN damping rejected"), Tuning->Validate(Error));
    TestEqual(TEXT("NaN charge cannot launch"), Tuning->LaunchImpulse(Tuning->LinearDamping), 0.f);
    Tuning->LinearDamping = .03f;
    Tuning->bExemptLaunchAndCaptureAreas = false;
    TestFalse(TEXT("Recovery exclusions preserved"), Tuning->Validate(Error));
    return true;
}
#endif
