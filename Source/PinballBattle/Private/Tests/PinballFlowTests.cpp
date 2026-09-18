#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameStateBase.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPinballFlowTest, "PinballBattle.Flow.EdgesAndPause",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Exercise the actual flow writer and its projection, including illegal/nested overlay requests. */
bool FPinballFlowTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    auto* State = World->SpawnActor<APinballGameStateBase>();
    auto* Flow = NewObject<UGameFlowComponent>();
    Flow->GameState = State;
    using E = EArcadeGameFlowState;
    TestFalse(TEXT("Cannot skip boot"), Flow->TransitionTo(E::PINBALL_PLAYING));
    TestTrue(TEXT("Boot readiness"), Flow->TransitionTo(E::ATTRACT));
    TestFalse(TEXT("Attract cannot pause"), Flow->SetPaused(true));
    TestTrue(TEXT("Start"), Flow->TransitionTo(E::PINBALL_READY));
    for (int32 I = 0; I < 3; ++I)
    {
        TestTrue(TEXT("Pause stores ready"), Flow->SetPaused(true));
        TestEqual(TEXT("Projection remembers exact phase"), State->GetSessionState().ResumeState, E::PINBALL_READY);
        TestFalse(TEXT("Pause overlay cannot nest"), Flow->SetPaused(true));
        TestFalse(TEXT("Launch edge closed while paused"), Flow->TransitionTo(E::PINBALL_PLAYING));
        TestTrue(TEXT("Resume"), Flow->SetPaused(false));
        TestFalse(TEXT("No duplicate continuation"), Flow->SetPaused(false));
    }
    TestTrue(TEXT("Launch"), Flow->TransitionTo(E::PINBALL_PLAYING));
    TestTrue(TEXT("Objective closes pinball"), Flow->TransitionTo(E::MINIGAME_TRANSITION));
    for (E Phase : {E::MINIGAME_TRANSITION, E::MINIGAME_PLAYING, E::MINIGAME_RESULTS})
    {
        if (Phase != E::MINIGAME_TRANSITION) TestTrue(TEXT("Round phase edge"), Flow->TransitionTo(Phase));
        Flow->Transition.PhaseSeconds = 1.25;
        TestTrue(TEXT("Pause each round phase"), Flow->SetPaused(true));
        TestFalse(TEXT("No duplicate overlay"), Flow->SetPaused(true));
        TestFalse(TEXT("No return while paused"), Flow->TransitionTo(E::PINBALL_PLAYING));
        TestEqual(TEXT("Phase clock preserved"), Flow->Transition.PhaseSeconds, 1.25);
        TestTrue(TEXT("Resume once"), Flow->SetPaused(false));
        TestEqual(TEXT("Exact phase restored"), Flow->GetCurrentState(), Phase);
        TestFalse(TEXT("No second continuation"), Flow->SetPaused(false));
    }
    TestTrue(TEXT("Return begins"), Flow->TransitionTo(E::MINIGAME_TRANSITION));
    TestTrue(TEXT("Return commits"), Flow->TransitionTo(E::PINBALL_PLAYING));
    TestTrue(TEXT("Moving-phase pause"), Flow->SetPaused(true));
    TestFalse(TEXT("Drain rejected during pause"), Flow->TransitionTo(E::BALL_LOST));
    TestTrue(TEXT("Resume exact playing phase"), Flow->SetPaused(false));
    TestEqual(TEXT("Resume projection"), State->GetFlowState(), E::PINBALL_PLAYING);
    TestTrue(TEXT("Drain"), Flow->TransitionTo(E::BALL_LOST));
    TestFalse(TEXT("Atomic drain cannot be interrupted by pause"), Flow->SetPaused(true));
    TestFalse(TEXT("Duplicate drain"), Flow->TransitionTo(E::BALL_LOST));
    TestTrue(TEXT("Terminal edge"), Flow->TransitionTo(E::GAME_OVER));
    TestFalse(TEXT("Final state cannot launch"), Flow->TransitionTo(E::PINBALL_PLAYING));
    TestTrue(TEXT("Restart ready"), Flow->TransitionTo(E::PINBALL_READY));
    World->DestroyWorld(false);
    return true;
}
#endif
