#pragma once
#include "CoreMinimal.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "AlienAssaultRuntime.generated.h"
class AAssaultEnemy;
class AAssaultProjectile;
class AAssaultShipPawn;
class USoundBase;
class UAudioComponent;

/** Independent arcade rules; the shared lifecycle owns clock, results and cleanup. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAlienAssaultRuntime : public AMiniGameRuntimeBase
{
    GENERATED_BODY()
public:
    /** Set native defaults and an explicit overhead arena camera. */
    AAlienAssaultRuntime();
    /** Advance bounded spawn pacing only while the common lifecycle permits play. */
    virtual void Tick(float DeltaSeconds) override;
    /** Admit callbacks only from this active run, never a prior pooled instance. */
    bool Accepts(const FGuid& RunId) const;
    /** Spawn one registered projectile subject to the local fire interval. */
    void Fire();
    /** Count an owned live enemy once and emit local destruction feedback. */
    bool DestroyEnemy(AAssaultEnemy* Enemy, const FGuid& RunId);
    /** Consume one local life and protect the centered respawn using active time. */
    bool DamageShip(const FGuid& RunId);
    /** Move a formation member and sweep enemy contact against the ship. */
    void MoveEnemy(AAssaultEnemy* Enemy, float DeltaSeconds);
    /** Fire toward the current player position from an owned live enemy. */
    void EnemyFire(AAssaultEnemy* Enemy);
    /** Project local lives and controls without giving UI ownership of rules. */
    virtual FText GetLocalStatus() const override;
    /** Read local lives for presentation and acceptance checks. */
    int32 GetLives() const { return Lives; }
    /** Read accepted unique destruction count. */
    int32 GetDestroyedCount() const { return DestroyedCount; }
    /** Read the active-time protection deadline. */
    bool IsProtected() const { return GetElapsed() < ProtectedUntil; }
    UPROPERTY(EditDefaultsOnly) TSubclassOf<AAssaultEnemy> EnemyClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<AAssaultProjectile> ProjectileClass;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> DestructionSound;
    UPROPERTY(EditDefaultsOnly) float FireInterval = .14f;
    UPROPERTY(EditDefaultsOnly) float ProtectionSeconds = 2.f;
    UPROPERTY(EditDefaultsOnly) int32 LocalPointsPerObject = 100;
    UPROPERTY(EditDefaultsOnly) int32 LocalPointsPerSecond = 10;
    UPROPERTY(EditDefaultsOnly) float EnemyFireInterval = 2.4f;
protected:
    /** Validate local tuning and reset all run-scoped counters before play. */
    virtual bool OnInitializeRun_Implementation(const FMiniGameContext& Context) override;
    /** Populate an initial formation above the player. */
    virtual bool OnStartRun_Implementation() override;
    /** Build performance metrics for either normal timeout or life exhaustion. */
    virtual FMiniGameResult BuildResult_Implementation() const override;
    /** Route the common fresh Space action through the same fire-rate guard. */
    virtual void OnAction_Implementation() override;
    /** Stop owned audio and release local references before registry cleanup. */
    virtual void OnCleanupRun_Implementation() override;
private:
    /** Fill one vacant moving formation slot without disturbing surviving enemies. */
    bool SpawnEnemy(int32 SlotIndex);
    UPROPERTY(Transient) TArray<TObjectPtr<AAssaultEnemy>> Enemies;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> ActiveSound;
    int32 Lives = 0;
    int32 DestroyedCount = 0;
    double ProtectedUntil = 0;
    TArray<double> RespawnAt;
    double NextShot = 0;
};
