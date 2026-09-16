#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/SessionProbeBudget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionProbeBudgetTest, "PinballBattle.Validation.SessionBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify that permitted slow balls fit overall bounds and shortened deadlines expire consistently. */
bool FSessionProbeBudgetTest::RunTest(const FString&)
{
    FSessionProbeBudget Budget;
    TestTrue(TEXT("Default nine-ball scenario is valid"), Budget.IsValid());
    TestEqual(TEXT("Nine slow balls plus menu allowance"), Budget.RequiredSeconds(), 645.);
    TestFalse(TEXT("Nine 65-second balls cannot exceed the overall deadline"),
        FSessionProbeBudget::HasExpired(585, Budget.TotalSeconds));
    Budget.TotalSeconds = 420;
    TestFalse(TEXT("Old global deadline rejected"), Budget.IsValid());
    Budget.Sessions = 4;
    Budget.TotalSeconds = 645;
    TestFalse(TEXT("More sessions require more time"), Budget.IsValid());
    Budget.SetupAndMenuSeconds = 20 * Budget.Sessions;
    Budget.TotalSeconds = Budget.RequiredSeconds();
    TestTrue(TEXT("Derived four-session limit accepted"), Budget.IsValid());
    TestFalse(TEXT("Exact shortened limit is allowed"), FSessionProbeBudget::HasExpired(.25, .25));
    TestTrue(TEXT("Shortened deadline expires immediately after limit"), FSessionProbeBudget::HasExpired(.251, .25));
    Budget.PerBallSeconds = -1;
    TestFalse(TEXT("Invalid per-ball override rejected"), Budget.IsValid());
    return true;
}
#endif
