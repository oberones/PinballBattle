#include "Misc/AutomationTest.h"
#include "Pinball/InteractionState.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContactEpisodeTest, "PinballBattle.Interactions.ContactEpisodes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Exercise callback repeats, contact hysteresis, invalid geometry and new ball identity. */
bool FContactEpisodeTest::RunTest(const FString& Parameters)
{
    FContactEpisode Contact;
    const FGuid Ball = FGuid::NewGuid();
    TestTrue(TEXT("First hit"), Contact.TryBegin(Ball));
    for (int32 I = 0; I < 100; ++I)
    {
        Contact.ObserveSeparation(12.5f, 12.f, 1.f);
        TestFalse(TEXT("Long contact never repeats"), Contact.TryBegin(Ball));
    }
    Contact.ObserveSeparation(-1, 12, 1);
    TestFalse(TEXT("Failed collision query cannot rearm"), Contact.TryBegin(Ball));
    Contact.ObserveSeparation(14, 12, 1);
    TestTrue(TEXT("Separated re-hit"), Contact.TryBegin(Ball));
    TestTrue(TEXT("New entitlement"), Contact.TryBegin(FGuid::NewGuid()));
    TestFalse(TEXT("Invalid identity"), Contact.TryBegin(FGuid()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLaneTraversalTest, "PinballBattle.Interactions.DirectedLane",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Check ordered travel, reverse/incomplete approaches, high-speed crossing and departure. */
bool FLaneTraversalTest::RunTest(const FString& Parameters)
{
    FLaneTraversal Lane;
    const FVector Extent(45, 90, 40);
    TestFalse(TEXT("Entry only"), Lane.Advance(FVector(0,-130,0), FVector(0,0,0), Extent, 12));
    TestTrue(TEXT("Forward exit"), Lane.Advance(FVector(0,0,0), FVector(0,95,0), Extent, 12));
    TestFalse(TEXT("Dwelling exit"), Lane.Advance(FVector(0,95,0), FVector(0,95,0), Extent, 12));
    Lane.Advance(FVector(0,95,0), FVector(0,150,0), Extent, 12);
    TestFalse(TEXT("Reverse never awards"), Lane.Advance(FVector(0,150,0), FVector(0,-150,0), Extent, 12));
    Lane.Advance(FVector(0,-150,0), FVector(0,-150,0), Extent, 12);
    TestTrue(TEXT("Fast full crossing cannot skip gates"), Lane.Advance(FVector(0,-150,0), FVector(0,150,0), Extent, 12));
    Lane.Reset();
    Lane.Advance(FVector(0,-130,0), FVector(0,0,0), Extent, 12);
    Lane.Advance(FVector(0,0,0), FVector(80,0,0), Extent, 12);
    TestFalse(TEXT("Side departure cancels progress"), Lane.Advance(FVector(80,0,0), FVector(0,150,0), Extent, 12));
    Lane.Reset();
    TestFalse(TEXT("Starting inside is incomplete"), Lane.Advance(FVector(0,0,0), FVector(0,150,0), Extent, 12));
    Lane.Reset();
    TestFalse(TEXT("Crossing above lane"), Lane.Advance(FVector(0,-150,100), FVector(0,150,100), Extent, 12));
    return true;
}
#endif
