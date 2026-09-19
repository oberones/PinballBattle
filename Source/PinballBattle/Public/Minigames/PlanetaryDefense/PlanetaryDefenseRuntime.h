#pragma once
#include "CoreMinimal.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "PlanetaryDefenseRuntime.generated.h"
class ADefenseThreat;
class ADefenseColony;
class ADefenseInterceptor;
class ADefenseBlastZone;
class UStaticMeshComponent;
class USoundBase;
class UAudioComponent;

/** Independent defense rules; common flow remains the only clock/end/award authority. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API APlanetaryDefenseRuntime : public AMiniGameRuntimeBase
{
    GENERATED_BODY()
public:
    /** Frame the local XY arena and author three explicit colony anchors. */
    APlanetaryDefenseRuntime();
    /** Pace threats against active time, including after the last colony is lost. */
    virtual void Tick(float DeltaSeconds) override;
    /** Reject stale, paused, initialized and terminal producers. */
    bool Accepts(const FGuid& RunId) const;
    /** Clamp a finite local target above the colonies and inside the visible arena. */
    FVector ClampAim(const FVector& Local) const;
    /** Launch unlimited shots through one active-time fire-rate guard. */
    bool FireAt(const FVector& LocalAim);
    /** Resolve an owned live threat once before emitting interception feedback. */
    bool InterceptThreat(ADefenseThreat* Threat, const FGuid& RunId);
    /** Resolve an impact once without awarding a destruction or ending the round. */
    bool ImpactColony(ADefenseThreat* Threat, ADefenseColony* Colony, const FGuid& RunId);
    /** Create a run-owned finite blast after an interceptor reaches or hits its destination. */
    bool Detonate(const FVector& WorldLocation, const FGuid& RunId);
    /** Return the explicit launcher position in world space. */
    FVector GetLauncherLocation() const;
    /** Project local counts and controls through the shared presentation. */
    virtual FText GetLocalStatus() const override;
    int32 GetSurvivorCount() const;
    int32 GetDestroyedCount() const { return DestroyedCount; }
    int32 GetSpawnedCount() const { return SpawnedCount; }
    int32 GetShotsFired() const { return ShotsFired; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Launcher;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Barrel;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<USceneComponent>> ColonyAnchors;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<ADefenseThreat> ThreatClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<ADefenseColony> ColonyClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<ADefenseInterceptor> InterceptorClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<ADefenseBlastZone> BlastClass;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> InterceptionSound;
    UPROPERTY(EditDefaultsOnly) float SpawnInterval = .55f;
    UPROPERTY(EditDefaultsOnly) float FireInterval = .22f;
    UPROPERTY(EditDefaultsOnly) float ThreatSpeed = 115;
    UPROPERTY(EditDefaultsOnly) float InterceptorSpeed = 1200;
    UPROPERTY(EditDefaultsOnly) float BlastRadius = 125;
    UPROPERTY(EditDefaultsOnly) float BlastLifetime = 1.4f;
    UPROPERTY(EditDefaultsOnly) int32 LocalPointsPerThreat = 100;
    UPROPERTY(EditDefaultsOnly) int32 LocalPointsPerSurvivor = 500;
protected:
    /** Validate tuning and create three dormant colonies for this run. */
    virtual bool OnInitializeRun_Implementation(const FMiniGameContext& Context) override;
    /** Place a valid cursor after camera readiness and seed three incoming threats. */
    virtual bool OnStartRun_Implementation() override;
    /** Report performance, with survivor points added only at the full timeout. */
    virtual FMiniGameResult BuildResult_Implementation() const override;
    /** Use the current deprojected target for the common fresh Space request. */
    virtual void OnAction_Implementation() override;
    /** Release sound and local references; the base destroys every run actor. */
    virtual void OnCleanupRun_Implementation() override;
private:
    /** Spawn a descending threat toward a colony anchor, retaining the run identity. */
    bool SpawnThreat();
    /** Stop all local audio during terminal freeze or cleanup. */
    void StopAudio();
    UPROPERTY(Transient) TArray<TObjectPtr<ADefenseThreat>> Threats;
    UPROPERTY(Transient) TArray<TObjectPtr<ADefenseColony>> Colonies;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Sounds;
    FRandomStream Random;
    int32 DestroyedCount = 0;
    int32 SpawnedCount = 0;
    int32 ShotsFired = 0;
    double NextSpawn = 0;
    double NextShot = 0;
};
