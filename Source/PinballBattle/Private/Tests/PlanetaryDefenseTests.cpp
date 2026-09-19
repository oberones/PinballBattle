#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseAimPawn.h"
#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Minigames/PlanetaryDefense/DefenseColony.h"
#include "Minigames/PlanetaryDefense/DefenseInterceptor.h"
#include "Minigames/PlanetaryDefense/DefenseBlastZone.h"
#include "Data/ScoringProfile.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlanetaryDefenseRulesTest, "PinballBattle.Defense.Rules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Exercise real actors, transformed aiming, terminal races and monotonic profile conversion.
bool FPlanetaryDefenseRulesTest::RunTest(const FString&)
{
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Runtime = World->SpawnActor<APlanetaryDefenseRuntime>();
    Runtime->SetActorTransform(FTransform(FRotator(0, 35, 0), FVector(20000, 1200, 50)));
    Runtime->ConfigurePawn(ADefenseAimPawn::StaticClass());
    FMiniGameContext C; C.SessionId = FGuid::NewGuid(); C.RunId = FGuid::NewGuid(); C.MiniGameId = TEXT("PlanetaryDefense");
    C.Generation = 1; C.ProfileRevision = TEXT("test"); C.StartingLives = 0; C.ArenaExtent = FVector(800, 430, 1500);
    C.MetricBounds = {{TEXT("ThreatsDestroyed"), 100}, {TEXT("StructuresSurviving"), 3}};
    C.MetricWeights = {{TEXT("ThreatsDestroyed"), 250}, {TEXT("StructuresSurviving"), 1000}};
    TestTrue(TEXT("Initialize"), Runtime->Initialize(C));
    TestEqual(TEXT("Three colonies before start"), Runtime->GetSurvivorCount(), 3);
    TestFalse(TEXT("No pre-start fire"), Runtime->FireAt(FVector::ZeroVector));
    auto* Aim = CastChecked<ADefenseAimPawn>(Runtime->GetRunPawn());
    const FTransform Transform = Runtime->GetActorTransform();
    TestTrue(TEXT("Transformed ray intersects plane"), Aim->AimFromRay(Transform.TransformPosition(FVector(200, 100, 1000)), Transform.TransformVectorNoScale(FVector(0, 0, -1))));
    TestTrue(TEXT("Aim in local plane"), Aim->GetLocalAim().Equals(FVector(200, 100, 0), .01));
    const FVector Previous = Aim->GetLocalAim();
    TestFalse(TEXT("Reject parallel ray"), Aim->AimFromRay(FVector::ZeroVector, FVector::ForwardVector));
    TestFalse(TEXT("Reject behind-camera ray"), Aim->AimFromRay(Transform.TransformPosition(FVector(0, 0, 1000)), FVector::UpVector));
    TestTrue(TEXT("Invalid ray retains target"), Previous.Equals(Aim->GetLocalAim()));
    TestTrue(TEXT("Clamp outside pointer"), Aim->AimFromRay(Transform.TransformPosition(FVector(10000, -10000, 1000)), FVector(0, 0, -1)));
    TestTrue(TEXT("Clamp protects colony band"), Aim->GetLocalAim().X < 800 && Aim->GetLocalAim().Y > -300);
    Runtime->SetPresentationReady(true); TestTrue(TEXT("Start"), Runtime->StartMiniGame());
    TestTrue(TEXT("First shot"), Runtime->FireAt(FVector(0, 100, 0)));
    TestFalse(TEXT("Rate limit duplicate press/hold"), Runtime->FireAt(FVector(0, 100, 0)));
    TArray<ADefenseThreat*> Initial;
    TArray<ADefenseColony*> Colonies;
    for (TActorIterator<ADefenseThreat> It(World); It; ++It) Initial.Add(*It);
    for (TActorIterator<ADefenseColony> It(World); It; ++It) Colonies.Add(*It);
    TestEqual(TEXT("Three initial threats"), Initial.Num(), 3);
    if (Initial.Num() == 3 && Colonies.Num() == 3)
    {
        TestFalse(TEXT("Stale interception"), Initial[0]->Intercept(FGuid::NewGuid()));
        Initial[0]->SetActorLocation(Runtime->GetActorLocation());
        TestTrue(TEXT("First live blast"), Runtime->Detonate(Runtime->GetActorLocation(), C.RunId));
        TestEqual(TEXT("Real overlap intercepts threat"), Runtime->GetDestroyedCount(), 1);
        TestTrue(TEXT("Second overlapping blast"), Runtime->Detonate(Runtime->GetActorLocation(), C.RunId));
        TestEqual(TEXT("Overlapping blast query credits only once"), Runtime->GetDestroyedCount(), 1);
        TestFalse(TEXT("Overlapping zones cannot double credit"), Initial[0]->Intercept(C.RunId));
        TestFalse(TEXT("Intercepted threat cannot impact"), Colonies[0]->Impact(Initial[0], C.RunId));
        TestTrue(TEXT("One hit destroys colony"), Colonies[0]->Impact(Initial[1], C.RunId));
        TestFalse(TEXT("Impact cannot become interception"), Initial[1]->Intercept(C.RunId));
        TestFalse(TEXT("Duplicate colony loss"), Colonies[0]->Impact(Initial[2], C.RunId));
        TestTrue(TEXT("Second colony loss"), Colonies[1]->Impact(Initial[2], C.RunId));
        Runtime->Tick(.6f);
        for (TActorIterator<ADefenseThreat> It(World); It; ++It)
            if (!It->IsResolved()) { Colonies[2]->Impact(*It, C.RunId); break; }
        TestEqual(TEXT("All colonies lost"), Runtime->GetSurvivorCount(), 0);
        TestEqual(TEXT("Zero colonies does not end round"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Playing);
        TestTrue(TEXT("Can still shoot without colonies"), Runtime->FireAt(FVector(0, 100, 0)));
    }
    // A real interceptor reaches its exact destination and creates a zone with a finite lifetime.
    for (TActorIterator<ADefenseInterceptor> It(World); It; ++It) It->Tick(2.f);
    int32 Blasts = 0;
    for (TActorIterator<ADefenseBlastZone> It(World); It; ++It)
    { ++Blasts; It->Tick(2.f); TestFalse(TEXT("Expired blast inactive"), It->IsActive()); }
    TestTrue(TEXT("Destination detonates"), Blasts > 0);
    for (int32 Step = 0; Step < 48; ++Step)
    {
        Runtime->Tick(.55f);
        for (TActorIterator<ADefenseThreat> It(World); It; ++It) if (!It->IsResolved()) It->Intercept(C.RunId);
    }
    TestTrue(TEXT("Twenty-eight threats available in time"), Runtime->GetDestroyedCount() >= 28);
    TestEqual(TEXT("No survivor points before end"), Runtime->GetLocalScore(), int64(Runtime->GetDestroyedCount()) * 100);
    Runtime->Tick(5.f);
    TestEqual(TEXT("Zero colonies full thirty seconds"), Runtime->GetElapsed(), 30.);
    TestEqual(TEXT("Timeout terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    TestFalse(TEXT("Late shot rejected"), Runtime->FireAt(FVector::ZeroVector));
    Runtime->Cleanup(); Runtime->Cleanup();
    C.RunId = FGuid::NewGuid(); ++C.Generation;
    TestTrue(TEXT("Clean replay"), Runtime->Initialize(C)); Runtime->SetPresentationReady(true); Runtime->StartMiniGame();
    TestEqual(TEXT("Replay restores all colonies"), Runtime->GetSurvivorCount(), 3);
    TestEqual(TEXT("Replay clears performance"), Runtime->GetLocalScore(), int64(0));
    Runtime->Tick(31);
    TestEqual(TEXT("Survivor timeout"), Runtime->GetElapsed(), 30.);
    TestEqual(TEXT("Survivor points only at timeout"), Runtime->GetLocalScore(), int64(1500));
    auto Result = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
    double Bonus = 0; EPerformanceRating Rating;
    for (int32 Survivors = 0; Survivors <= 3; ++Survivors)
    {
        double Last = 0;
        for (int32 Count = 0; Count <= 40; ++Count)
        {
            Result.Metrics[TEXT("ThreatsDestroyed")] = Count; Result.Metrics[TEXT("StructuresSurviving")] = Survivors;
            TestTrue(TEXT("Profile conversion valid"), UScoringProfile::EvaluateMiniGame(Result, C, Bonus, Rating));
            TestEqual(TEXT("Configured monotonic reward"), Bonus, FMath::Min(10000., Count * 250. + Survivors * 1000.));
            TestTrue(TEXT("More interceptions never reduce reward"), Bonus >= Last); Last = Bonus;
            if (Count == 4 && Survivors == 0) TestEqual(TEXT("Low example"), Bonus, 1000.);
            if (Count == 28 && Survivors == 3) TestEqual(TEXT("High example"), Bonus, 10000.);
        }
    }
    Runtime->Cleanup(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
