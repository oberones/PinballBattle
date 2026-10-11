#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Minigames/AlienAssault/AssaultShipPawn.h"
#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Minigames/AlienAssault/AssaultProjectile.h"
#include "Data/ScoringProfile.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssaultRulesTest, "PinballBattle.Assault.Rules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Exercise actual actors and result conversion, including stale hits, deduplication and both endings.
bool FAssaultRulesTest::RunTest(const FString&)
{
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Runtime = World->SpawnActor<AAlienAssaultRuntime>();
    Runtime->ConfigurePawn(AAssaultShipPawn::StaticClass());
    FMiniGameContext C; C.SessionId = FGuid::NewGuid(); C.RunId = FGuid::NewGuid(); C.MiniGameId = TEXT("AlienAssault");
    C.Generation = 1; C.ProfileRevision = TEXT("test"); C.ArenaExtent = FVector(800, 430, 1500);
    C.MaximumObjectives = 120;
    C.MetricBounds.Add(TEXT("EnemiesDestroyed"), 120); C.MetricWeights.Add(TEXT("EnemiesDestroyed"), 250);
    C.MetricBounds.Add(TEXT("SurvivalSeconds"), 30); C.MetricWeights.Add(TEXT("SurvivalSeconds"), 100); C.FloorMetrics.Add(TEXT("SurvivalSeconds"));
    TestTrue(TEXT("Initialize"), Runtime->Initialize(C));
    TestFalse(TEXT("Damage before start"), Runtime->DamageShip(C.RunId));
    Runtime->SetPresentationReady(true); TestTrue(TEXT("Start"), Runtime->StartMiniGame());
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Ship = CastChecked<AAssaultShipPawn>(Runtime->GetRunPawn());
    Controller->Possess(Ship);
    Ship->SetActorLocation(FVector(100, 100, 0)); Ship->Respawn();
    TestTrue(TEXT("Run reference survives possession owner change"), Ship->GetActorLocation().Equals(Runtime->GetActorLocation() + FVector(0, -370, 0)));
    TestEqual(TEXT("Three local lives"), Runtime->GetLives(), 3);
    TestFalse(TEXT("Initial protection"), Runtime->DamageShip(C.RunId));
    const auto CountShots = [World]()
    {
        int32 Count = 0; for (TActorIterator<AAssaultProjectile> It(World); It; ++It) ++Count;
        return Count;
    };
    Runtime->Fire(); Runtime->Fire();
    TestEqual(TEXT("Fire rate admits only one simultaneous request"), CountShots(), 1);
    for (TActorIterator<AAssaultProjectile> It(World); It; ++It) It->Tick(6);
    TestEqual(TEXT("Finite projectile lifetime"), CountShots(), 0);
    AAssaultEnemy* First = nullptr;
    for (TActorIterator<AAssaultEnemy> It(World); It; ++It) { First = *It; break; }
    TestNotNull(TEXT("Initial targets"), First);
    const auto FindSlot = [World, &C](int32 Slot) -> AAssaultEnemy*
    {
        for (TActorIterator<AAssaultEnemy> It(World); It; ++It)
            if (It->RunId == C.RunId && !It->bDestroyed && It->SlotIndex == Slot) return *It;
        return nullptr;
    };
    Runtime->Tick(6.1f);
    TestTrue(TEXT("Surviving formation does not reset after six seconds"), First && FindSlot(First->SlotIndex) == First);
    if (First)
    {
        const int32 Slot = First->SlotIndex;
        auto* Survivor = FindSlot((Slot + 1) % 12);
        TestFalse(TEXT("Stale projectile"), First->Hit(FGuid::NewGuid()));
        TestTrue(TEXT("First hit accepted"), First->Hit(C.RunId));
        TestFalse(TEXT("Duplicate hit rejected"), First->Hit(C.RunId));
        Runtime->Tick(1.f);
        auto* Second = FindSlot((Slot + 2) % 12);
        TestTrue(TEXT("Second slot killed independently"), Second && Second->Hit(C.RunId));
        Runtime->Tick(1.99f);
        TestNull(TEXT("First slot still empty before three seconds"), FindSlot(Slot));
        const double BeforePause = Runtime->GetElapsed();
        World->GetWorldSettings()->SetPauserPlayerState(World->SpawnActor<APlayerState>());
        Runtime->Tick(10);
        TestEqual(TEXT("Respawn clock freezes while paused"), Runtime->GetElapsed(), BeforePause);
        TestNull(TEXT("Paused deadline cannot replace an alien"), FindSlot(Slot));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        Runtime->Tick(.02f);
        auto* Replacement = FindSlot(Slot);
        TestNotNull(TEXT("First slot returns after three active seconds"), Replacement);
        TestNull(TEXT("Second slot retains its own later deadline"), FindSlot((Slot + 2) % 12));
        if (Replacement)
        {
            TestTrue(TEXT("New actor occupies the same formation slot"), Replacement != First && Replacement->FormationPosition == First->FormationPosition);
            const FVector Expected = Replacement->FormationPosition + FVector(130 * FMath::Sin(Runtime->GetElapsed() * 1.5), -Runtime->GetElapsed() * 14, 0);
            TestTrue(TEXT("Replacement joins current formation motion"), Replacement->GetActorLocation().Equals(Expected, .01));
        }
        TestTrue(TEXT("Other survivors retain actor identity"), FindSlot((Slot + 1) % 12) == Survivor);
        Runtime->Tick(1.f);
        TestNotNull(TEXT("Second slot returns on its own deadline"), FindSlot((Slot + 2) % 12));
        TestEqual(TEXT("Respawning awards no points"), Runtime->GetDestroyedCount(), 2);
    }
    for (int32 Step = 0; Step < 3; ++Step)
    {
        Runtime->Tick(3.1f);
        for (TActorIterator<AAssaultEnemy> It(World); It; ++It)
            if (!It->bDestroyed) It->Hit(C.RunId);
    }
    TestTrue(TEXT("At least twenty-eight targets available before timeout"), Runtime->GetDestroyedCount() >= 28);
    TestEqual(TEXT("Local informational points"), Runtime->GetLocalScore(), int64(Runtime->GetDestroyedCount()) * 100);
    auto Result = FMiniGameResult::Failure(C, EMiniGameEndReason::TimedOut);
    double Bonus = 0; EPerformanceRating Rating;
    for (const int32 Count : {4, 28})
    {
        Result.Metrics[TEXT("EnemiesDestroyed")] = Count;
        Result.Metrics[TEXT("SurvivalSeconds")] = Count == 4 ? 10.9 : 30.;
        TestTrue(TEXT("Profile conversion"), UScoringProfile::EvaluateMiniGame(Result, C, Bonus, Rating));
        TestEqual(TEXT("Low/high bonus"), Bonus, Count == 4 ? 2000. : 10000.);
    }
    TestTrue(TEXT("First damage"), Runtime->DamageShip(C.RunId));
    TestEqual(TEXT("One life charged"), Runtime->GetLives(), 2);
    TestFalse(TEXT("Repeated contact protected"), Runtime->DamageShip(C.RunId));
    Runtime->Tick(2.1f);
    auto* Contact = Cast<AAssaultEnemy>(Runtime->SpawnRunActor(AAssaultEnemy::StaticClass(), Ship->GetActorTransform()));
    // Only registered formation members may cause contact damage.
    Contact->RunId = C.RunId;
    Runtime->MoveEnemy(Contact, .1f);
    TestEqual(TEXT("Unregistered enemy cannot damage"), Runtime->GetLives(), 2);
    auto* Shot = Cast<AAssaultProjectile>(Runtime->SpawnRunActor(AAssaultProjectile::StaticClass(),
        FTransform(FRotator::ZeroRotator, Ship->GetActorLocation() + FVector(0, 60, 0))));
    Shot->RunId = C.RunId; Shot->bEnemyShot = true; Shot->Velocity = FVector(0, -600, 0);
    Shot->Tick(.1f);
    TestEqual(TEXT("Swept hostile shot removes one life"), Runtime->GetLives(), 1);
    Runtime->Tick(2.1f); TestTrue(TEXT("Final damage"), Runtime->DamageShip(C.RunId));
    TestEqual(TEXT("Early terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    TestTrue(TEXT("Earned points survive life loss"), Runtime->GetLocalScore() >= 2800);
    TestFalse(TEXT("Late damage"), Runtime->DamageShip(C.RunId));
    Runtime->Cleanup(); Runtime->Cleanup();
    C.RunId = FGuid::NewGuid(); ++C.Generation;
    TestTrue(TEXT("Fresh replay"), Runtime->Initialize(C)); Runtime->SetPresentationReady(true); Runtime->StartMiniGame();
    Runtime->Tick(2.1f);
    for (TActorIterator<AAssaultEnemy> It(World); It; ++It)
    {
        if (It->RunId != C.RunId || It->bDestroyed) continue;
        It->SetActorLocation(Runtime->GetRunPawn()->GetActorLocation());
        Runtime->MoveEnemy(*It, .1f);
        TestEqual(TEXT("Owned enemy contact removes one life"), Runtime->GetLives(), 2);
        TestFalse(TEXT("Contact respawn immunity"), Runtime->DamageShip(C.RunId));
        break;
    }
    auto* PendingAtTimeout = FindSlot(0);
    TestTrue(TEXT("Schedule replacement before timeout"), PendingAtTimeout && PendingAtTimeout->Hit(C.RunId));
    Runtime->Tick(31);
    TestNull(TEXT("Timeout wins over overdue respawns"), FindSlot(0));
    Runtime->Tick(4);
    TestNull(TEXT("Terminal runtime cannot respawn"), FindSlot(0));
    TestEqual(TEXT("Timeout clamp"), Runtime->GetElapsed(), 30.);
    TestEqual(TEXT("Timeout terminal latch"), Runtime->GetLifecycle(), EMiniGameLifecycleState::Ended);
    TestEqual(TEXT("Replay adds only its own kill and survival points"), Runtime->GetLocalScore(), int64(400));
    const int32 ShotsAtEnd = CountShots(); Runtime->Fire();
    TestEqual(TEXT("Terminal fire is closed"), CountShots(), ShotsAtEnd);
    Runtime->Cleanup(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
