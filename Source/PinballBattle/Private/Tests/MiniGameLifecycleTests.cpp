#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/MiniGameTestRuntime.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiniGameLifecycleTest, "PinballBattle.MiniGame.Lifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Exercise production guards with real actors, including partial initialization and duplicate endings.
bool FMiniGameLifecycleTest::RunTest(const FString&)
{
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Runtime = World->SpawnActor<AMiniGameTestRuntime>();
    Runtime->ConfigurePawn(AMiniGameTestPawn::StaticClass());
    FMiniGameContext C; C.SessionId = FGuid::NewGuid(); C.RunId = FGuid::NewGuid(); C.MiniGameId = TEXT("Fixture");
    C.Generation = 1; C.ProfileRevision = TEXT("test"); C.MetricBounds.Add(TEXT("Actions"), 100); C.MetricWeights.Add(TEXT("Actions"), 500);
    TestTrue(TEXT("Valid context"), C.IsValid());
    TestFalse(TEXT("Dormant cannot start"), Runtime->StartMiniGame());
    Runtime->bFailInitialize = true;
    TestFalse(TEXT("Partial initialize rejects"), Runtime->Initialize(C));
    Runtime->Cleanup(); Runtime->Cleanup();
    TestNull(TEXT("Partial pawn cleaned"), Runtime->GetRunPawn());
    Runtime->bFailInitialize = false;
    for (int32 Cycle = 0; Cycle < 3; ++Cycle)
    {
        C.RunId = FGuid::NewGuid(); ++C.Generation;
        TestTrue(TEXT("Fresh initialized run"), Runtime->Initialize(C));
        TestFalse(TEXT("Repeated initialize rejected"), Runtime->Initialize(C));
        TestFalse(TEXT("Presentation is required"), Runtime->StartMiniGame());
        TestTrue(TEXT("Explicit presentation targets"), Runtime->SetPresentationReady(true));
        TestTrue(TEXT("Start once"), Runtime->StartMiniGame());
        TestFalse(TEXT("Duplicate start"), Runtime->StartMiniGame());
        auto R = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
        R.RunId = FGuid::NewGuid();
        TestFalse(TEXT("Stale end cannot latch"), Runtime->EndMiniGame(R));
        R.RunId = C.RunId; R.DurationSeconds = 30;
        TestTrue(TEXT("Current result valid"), R.Validate(C));
        R.DurationSeconds = 30.01; TestFalse(TEXT("Duration overflow"), R.Validate(C));
        R.DurationSeconds = 30; R.Metrics[TEXT("Actions")] = std::numeric_limits<double>::quiet_NaN();
        TestFalse(TEXT("Nonfinite metric rejected"), R.Validate(C));
        TestTrue(TEXT("Malformed current result ends as failure"), Runtime->EndMiniGame(R));
        TestFalse(TEXT("Duplicate end cannot publish"), Runtime->EndMiniGame(R));
        TestEqual(TEXT("Terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
        Runtime->Cleanup(); Runtime->Cleanup();
        TestEqual(TEXT("Reusable dormant root"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Dormant);
    }
    C.RunId = FGuid::NewGuid();
    Runtime->Initialize(C); Runtime->SetPresentationReady(true); Runtime->StartMiniGame();
    Runtime->Tick(100);
    TestEqual(TEXT("Clock clamps at duration"), Runtime->GetElapsed(), 30.);
    TestEqual(TEXT("Timeout owns terminal state"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    Runtime->Cleanup();
    C.RunId = FGuid::NewGuid(); Runtime->Initialize(C); Runtime->SetPresentationReady(true); Runtime->StartMiniGame();
    auto* Subsystem = World->GetSubsystem<UMinigameWorldSubsystem>();
    Subsystem->Active.Context = C; Subsystem->Active.Runtime = Runtime;
    int32 Cancellations = 0;
    Subsystem->OnRunCancelled.AddLambda([&Cancellations](const FMiniGameContext&, EMiniGameEndReason) { ++Cancellations; });
    TestFalse(TEXT("Stale cancellation rejected"), Subsystem->CancelRun(C.RunId, C.Generation + 1));
    TestTrue(TEXT("Current cancellation accepted"), Subsystem->CancelRun(C.RunId, C.Generation));
    TestTrue(TEXT("Repeated cancellation is idempotent"), Subsystem->CancelRun(C.RunId, C.Generation));
    TestEqual(TEXT("One cancellation notification"), Cancellations, 1);
    TestEqual(TEXT("Cancellation stops local lifecycle"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    Subsystem->CleanupRun(C.RunId, C.Generation); Subsystem->OnRunCancelled.Clear();
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
