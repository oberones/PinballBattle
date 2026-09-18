#pragma once
#include "CoreMinimal.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "AsteroidFieldRuntime.generated.h"
class AAsteroidObstacle;
class AAsteroidProjectile;
class AAsteroidShipPawn;
class USoundBase;
class UAudioComponent;

/** Independent arcade rules; the shared lifecycle owns clock, results and cleanup. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAsteroidFieldRuntime : public AMiniGameRuntimeBase
{
    GENERATED_BODY()
public:
    /** Set native defaults and an explicit overhead arena camera. */
    AAsteroidFieldRuntime();
    /** Advance bounded spawn pacing only while the common lifecycle permits play. */
    virtual void Tick(float DeltaSeconds) override;
    /** Admit callbacks only from this active run, never a prior pooled instance. */
    bool Accepts(const FGuid& RunId) const;
    /** Spawn one registered projectile subject to the local fire interval. */
    void Fire();
    /** Count an owned live obstacle once and emit local destruction feedback. */
    bool DestroyObstacle(AAsteroidObstacle* Obstacle, const FGuid& RunId);
    /** Consume one local life and protect the centered respawn using active time. */
    bool DamageShip(const FGuid& RunId);
    /** Sweep in the local plane, reflecting at the visible arena boundaries. */
    void MoveBounded(AActor* Actor, FVector& Velocity, float Radius, float DeltaSeconds);
    /** Project local lives and controls without giving UI ownership of rules. */
    virtual FText GetLocalStatus() const override;
    /** Read local lives for presentation and acceptance checks. */
    int32 GetLives() const { return Lives; }
    /** Read accepted unique destruction count. */
    int32 GetDestroyedCount() const { return DestroyedCount; }
    /** Read the active-time protection deadline. */
    bool IsProtected() const { return GetElapsed() < ProtectedUntil; }
    UPROPERTY(EditDefaultsOnly) TSubclassOf<AAsteroidObstacle> ObstacleClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<AAsteroidProjectile> ProjectileClass;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> DestructionSound;
    UPROPERTY(EditDefaultsOnly) float SpawnInterval = .65f;
    UPROPERTY(EditDefaultsOnly) float FireInterval = .14f;
    UPROPERTY(EditDefaultsOnly) float ProtectionSeconds = 2.f;
    UPROPERTY(EditDefaultsOnly) int32 LocalPointsPerObject = 100;
protected:
    /** Validate local tuning and reset all run-scoped counters before play. */
    virtual bool OnInitializeRun_Implementation(const FMiniGameContext& Context) override;
    /** Populate an initial ring of targets without placing one at the spawn. */
    virtual bool OnStartRun_Implementation() override;
    /** Build performance metrics for either normal timeout or life exhaustion. */
    virtual FMiniGameResult BuildResult_Implementation() const override;
    /** Route the common fresh Space action through the same fire-rate guard. */
    virtual void OnAction_Implementation() override;
    /** Stop owned audio and release local references before registry cleanup. */
    virtual void OnCleanupRun_Implementation() override;
private:
    /** Add one drifting rock with identity and level ownership already established. */
    bool SpawnObstacle();
    UPROPERTY(Transient) TArray<TObjectPtr<AAsteroidObstacle>> Obstacles;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> ActiveSound;
    FRandomStream Random;
    int32 Lives = 0;
    int32 DestroyedCount = 0;
    double ProtectedUntil = 0;
    double NextSpawn = 0;
    double NextShot = 0;
};
