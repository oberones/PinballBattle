#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Minigames/AsteroidField/AsteroidShipPawn.h"
#include "Minigames/AsteroidField/AsteroidObstacle.h"
#include "Data/ScoringProfile.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAsteroidRulesTest, "PinballBattle.Asteroid.Rules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Exercise actual actors and result conversion, including stale hits, deduplication and both endings.
bool FAsteroidRulesTest::RunTest(const FString&)
{
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Runtime = World->SpawnActor<AAsteroidFieldRuntime>();
    Runtime->ConfigurePawn(AAsteroidShipPawn::StaticClass());
    FMiniGameContext C; C.SessionId = FGuid::NewGuid(); C.RunId = FGuid::NewGuid(); C.MiniGameId = TEXT("AsteroidField");
    C.Generation = 1; C.ProfileRevision = TEXT("test"); C.ArenaExtent = FVector(800, 430, 1500);
    C.MetricBounds.Add(TEXT("ObjectsDestroyed"), 100); C.MetricWeights.Add(TEXT("ObjectsDestroyed"), 500);
    TestTrue(TEXT("Initialize"), Runtime->Initialize(C));
    TestFalse(TEXT("Damage before start"), Runtime->DamageShip(C.RunId));
    Runtime->SetPresentationReady(true); TestTrue(TEXT("Start"), Runtime->StartMiniGame());
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Ship = CastChecked<AAsteroidShipPawn>(Runtime->GetRunPawn());
    Controller->Possess(Ship);
    Ship->SetActorLocation(FVector(100, 100, 0)); Ship->Respawn();
    TestTrue(TEXT("Run reference survives possession owner change"), Ship->GetActorLocation().Equals(Runtime->GetActorLocation()));
    TestEqual(TEXT("Three local lives"), Runtime->GetLives(), 3);
    TestFalse(TEXT("Initial protection"), Runtime->DamageShip(C.RunId));
    AAsteroidObstacle* First = nullptr;
    for (TActorIterator<AAsteroidObstacle> It(World); It; ++It) { First = *It; break; }
    TestNotNull(TEXT("Initial targets"), First);
    if (First)
    {
        TestFalse(TEXT("Stale projectile"), First->Hit(FGuid::NewGuid()));
        TestTrue(TEXT("First hit accepted"), First->Hit(C.RunId));
        TestFalse(TEXT("Duplicate hit rejected"), First->Hit(C.RunId));
    }
    for (int32 Step = 0; Step < 20; ++Step)
    {
        Runtime->Tick(.7f);
        for (TActorIterator<AAsteroidObstacle> It(World); It; ++It)
            if (!It->bDestroyed) { It->Hit(C.RunId); break; }
    }
    TestTrue(TEXT("At least twenty targets available before timeout"), Runtime->GetDestroyedCount() >= 20);
    TestEqual(TEXT("Local informational points"), Runtime->GetLocalScore(), int64(Runtime->GetDestroyedCount()) * 100);
    auto Result = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
    double Bonus = 0; EPerformanceRating Rating;
    for (const int32 Count : {2, 20})
    {
        Result.Metrics[TEXT("ObjectsDestroyed")] = Count;
        TestTrue(TEXT("Profile conversion"), UScoringProfile::EvaluateMiniGame(Result, C, Bonus, Rating));
        TestEqual(TEXT("Low/high bonus"), Bonus, Count == 2 ? 1000. : 10000.);
    }
    TestTrue(TEXT("First damage"), Runtime->DamageShip(C.RunId));
    TestEqual(TEXT("One life charged"), Runtime->GetLives(), 2);
    TestFalse(TEXT("Repeated contact protected"), Runtime->DamageShip(C.RunId));
    Runtime->Tick(2.1f); TestTrue(TEXT("Second damage"), Runtime->DamageShip(C.RunId));
    Runtime->Tick(2.1f); TestTrue(TEXT("Final damage"), Runtime->DamageShip(C.RunId));
    TestEqual(TEXT("Early terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    TestTrue(TEXT("Earned points survive life loss"), Runtime->GetLocalScore() >= 2000);
    TestFalse(TEXT("Late damage"), Runtime->DamageShip(C.RunId));
    Runtime->Cleanup(); Runtime->Cleanup();
    C.RunId = FGuid::NewGuid(); ++C.Generation;
    TestTrue(TEXT("Fresh replay"), Runtime->Initialize(C)); Runtime->SetPresentationReady(true); Runtime->StartMiniGame();
    Runtime->Tick(31);
    TestEqual(TEXT("Timeout clamp"), Runtime->GetElapsed(), 30.);
    TestEqual(TEXT("Timeout terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    TestEqual(TEXT("Replay clears score"), Runtime->GetLocalScore(), int64(0));
    Runtime->Cleanup(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
