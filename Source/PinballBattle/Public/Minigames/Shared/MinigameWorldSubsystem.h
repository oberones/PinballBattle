#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/MiniGameTypes.h"
#include "MinigameWorldSubsystem.generated.h"
class UMiniGameDefinition;
class AMiniGameRuntimeBase;
class ULevelStreamingDynamic;
struct FStreamableHandle;
DECLARE_MULTICAST_DELEGATE_TwoParams(FMiniGameCancelled, const FMiniGameContext&, EMiniGameEndReason);

/** One accepted environment allocation; it owns no public flow state or global score. */
USTRUCT()
struct FActiveMiniGameRun
{
    GENERATED_BODY()
    UPROPERTY() FMiniGameContext Context;
    UPROPERTY() TObjectPtr<UMiniGameDefinition> Definition;
    UPROPERTY() TWeakObjectPtr<AMiniGameRuntimeBase> Runtime;
    UPROPERTY() TWeakObjectPtr<ULevelStreamingDynamic> Streaming;
    bool bCancelled = false;
};

/** Keeps small arenas resident and explicitly dormant in their world. */
UCLASS()
class PINBALLBATTLE_API UMinigameWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    /** Start asynchronous package and level-instance loads during cabinet boot. */
    bool Preload(const TArray<TObjectPtr<UMiniGameDefinition>>& Definitions);
    /** Discard failed residency only when no run owns an arena, allowing explicit boot retry. */
    void ResetPreload();
    /** Require loaded, visible, unique registered root and resolved soft classes/input/UI. */
    bool IsReady(const UMiniGameDefinition* Definition) const;
    /** Register an authored root by its actual level instance. */
    void RegisterRoot(AMiniGameRuntimeBase* Root);
    /** Remove a root on streaming/world teardown without resuming the table. */
    void UnregisterRoot(AMiniGameRuntimeBase* Root);
    /** Allocate at most one run and initialize only a resident dormant root. */
    AMiniGameRuntimeBase* Prepare(UMiniGameDefinition* Definition, const FMiniGameContext& Context);
    /** Clean the current generation after its controller has released the pawn and camera. */
    bool CleanupRun(FGuid RunId, int64 Generation);
    /** Close producers without destroying presentation targets before controller rollback. */
    bool CancelRun(FGuid RunId, int64 Generation, EMiniGameEndReason Reason = EMiniGameEndReason::Cancelled, bool bNotifyFlow = true);
    FMiniGameCancelled OnRunCancelled;
    /** Read the active record for validation without permitting mutation. */
    const FActiveMiniGameRun& GetActiveRun() const { return Active; }
    /** Invalidate and clean all run resources on world teardown. */
    virtual void Deinitialize() override;
private:
    friend class FMiniGameLifecycleTest;
    /** Find a unique root only among explicitly registered actors in this streaming instance. */
    AMiniGameRuntimeBase* ResolveRoot(const UMiniGameDefinition* Definition) const;
    UPROPERTY() TMap<TObjectPtr<UMiniGameDefinition>, TObjectPtr<ULevelStreamingDynamic>> Levels;
    UPROPERTY() TArray<TWeakObjectPtr<AMiniGameRuntimeBase>> Roots;
    UPROPERTY() FActiveMiniGameRun Active;
    TArray<TSharedPtr<FStreamableHandle>> AssetLoads;
};
