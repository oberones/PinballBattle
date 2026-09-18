#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/PinballTransitionFunctionalTest.h"
#include "Editor.h"
#include "EngineUtils.h"

/** Latent wrapper waits for the actual PIE fixture rather than replacing its shipping flow. */
class FWaitForPinballRoundTrips : public IAutomationLatentCommand
{
public:
    /** Retain the test reporter and a bounded real-time timeout. */
    explicit FWaitForPinballRoundTrips(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    /** Start one explicitly typed fixture and report its final invariant result. */
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(TEXT("Transition fixture timed out")); return true; }
        if (!GEditor || !GEditor->PlayWorld) return false;
        for (TActorIterator<APinballTransitionFunctionalTest> It(GEditor->PlayWorld); It; ++It)
        {
            if (It->HasCompleted()) { if (!It->DidPass()) Test->AddError(TEXT("Transition fixture failed")); return true; }
            if (!It->IsRunning()) It->RunTest();
            return false;
        }
        Test->AddError(TEXT("Transition map has no functional fixture")); return true;
    }
private:
    FAutomationTestBase* Test;
    double Started;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPinballTransitionTest, "PinballBattle.Transition.RoundTrips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
// Open the authored fixture, run the real PIE round trips, and release the play world afterward.
bool FPinballTransitionTest::RunTest(const FString&)
{
    AutomationOpenMap(TEXT("/Game/Tests/Maps/L_TransitionTest"));
    // AutomationOpenMap already schedules PIE through the editor map-load delegate.
    // Starting another session here would abort the first fixture and contaminate its report.
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForPinballRoundTrips(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
