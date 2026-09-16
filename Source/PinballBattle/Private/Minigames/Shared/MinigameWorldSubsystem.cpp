#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "Data/MiniGameDefinition.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"

// All soft references resolve before gameplay; callbacks only make residency observable.
bool UMinigameWorldSubsystem::Preload(const TArray<TObjectPtr<UMiniGameDefinition>>& Definitions)
{
    for (UMiniGameDefinition* Definition : Definitions)
    {
        FString Error;
        if (!Definition || !Definition->Validate(Error)) return false;
        if (Levels.Contains(Definition)) continue;
        TArray<FSoftObjectPath> Paths = { Definition->RuntimeClass.ToSoftObjectPath(), Definition->PawnClass.ToSoftObjectPath(),
            Definition->InputContext.ToSoftObjectPath(), Definition->ActionInput.ToSoftObjectPath(), Definition->HUDClass.ToSoftObjectPath() };
        AssetLoads.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths));
        bool Success = false;
        auto* Level = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(GetWorld(), Definition->Map,
            Definition->ArenaTransform.GetLocation(), Definition->ArenaTransform.Rotator(), Success);
        if (!Success || !Level) return false;
        Level->SetShouldBeLoaded(true); Level->SetShouldBeVisible(true);
        Levels.Add(Definition, Level);
    }
    return true;
}

// Boot retry replaces failed instances without allowing stale roots to satisfy readiness.
void UMinigameWorldSubsystem::ResetPreload()
{
    if (Active.Context.RunId.IsValid()) return;
    for (const auto& Entry : Levels) if (IsValid(Entry.Value))
    {
        Entry.Value->SetShouldBeVisible(false); Entry.Value->SetShouldBeLoaded(false);
        Entry.Value->SetIsRequestingUnloadAndRemoval(true);
    }
    Levels.Reset();
    for (auto& Handle : AssetLoads) if (Handle) Handle->CancelHandle();
    AssetLoads.Reset();
}

// A root can register before or after OnLevelShown; readiness checks both facts together.
void UMinigameWorldSubsystem::RegisterRoot(AMiniGameRuntimeBase* Root) { if (IsValid(Root)) Roots.AddUnique(Root); }
// Remove only the supplied root; level identity never relies on package/actor name heuristics.
void UMinigameWorldSubsystem::UnregisterRoot(AMiniGameRuntimeBase* Root) { Roots.Remove(Root); }

// Duplicate roots in one instance are an error instead of selecting an arbitrary actor.
AMiniGameRuntimeBase* UMinigameWorldSubsystem::ResolveRoot(const UMiniGameDefinition* Definition) const
{
    const auto* Handle = Levels.Find(Definition);
    if (!Handle || !IsValid(*Handle) || !(*Handle)->IsLevelLoaded() || !(*Handle)->IsLevelVisible()) return nullptr;
    AMiniGameRuntimeBase* Found = nullptr;
    for (const auto& Root : Roots) if (Root.IsValid() && Root->GetLevel() == (*Handle)->GetLoadedLevel())
    { if (Found) return nullptr; Found = Root.Get(); }
    return Found;
}

// Soft classes, input and UI must be resident in addition to a shown level and correct root class.
bool UMinigameWorldSubsystem::IsReady(const UMiniGameDefinition* D) const
{
    if (!D || !D->RuntimeClass.Get() || !D->PawnClass.Get() || !D->InputContext.Get() || !D->ActionInput.Get() || !D->HUDClass.Get()) return false;
    const auto* Root = ResolveRoot(D);
    return Root && Root->IsA(D->RuntimeClass.Get()) && IsValid(Root->Camera) &&
        Root->GetLifecycle() == EMiniGameLifecycleState::Dormant;
}

// Record ownership before Initialize so a failed hook can always be cleaned by the same transaction.
AMiniGameRuntimeBase* UMinigameWorldSubsystem::Prepare(UMiniGameDefinition* D, const FMiniGameContext& C)
{
    if (Active.Context.RunId.IsValid() || !C.IsValid() || !IsReady(D) || C.MiniGameId != D->MiniGameId) return nullptr;
    AMiniGameRuntimeBase* Root = ResolveRoot(D);
    Active.Context = C; Active.Definition = D; Active.Runtime = Root; Active.Streaming = Levels.FindChecked(D);
    Root->ConfigurePawn(D->PawnClass.Get());
    return Root->Initialize(C) ? Root : nullptr;
}

// Cancellation closes all owned producers while retaining the outgoing presentation targets.
bool UMinigameWorldSubsystem::CancelRun(FGuid RunId, int64 Generation, EMiniGameEndReason Reason, bool bNotifyFlow)
{
    if (!Active.Context.RunId.IsValid()) return true;
    if (Active.Context.RunId != RunId || Active.Context.Generation != Generation) return false;
    if (Active.bCancelled) return true;
    Active.bCancelled = true;
    if (Active.Runtime.IsValid())
    {
        Active.Runtime->Lifecycle = EMiniGameLifecycleState::Ended;
        Active.Runtime->StopProducers();
    }
    if (bNotifyFlow) OnRunCancelled.Broadcast(Active.Context, Reason);
    return true;
}

// Generation checks prevent old callbacks from cleaning a newer run on the same cached root.
bool UMinigameWorldSubsystem::CleanupRun(FGuid RunId, int64 Generation)
{
    if (!Active.Context.RunId.IsValid()) return true;
    if (Active.Context.RunId != RunId || Active.Context.Generation != Generation) return false;
    if (Active.Runtime.IsValid()) Active.Runtime->Cleanup();
    Active = FActiveMiniGameRun(); return true;
}

// No controller or table restoration is valid once the world is tearing down.
void UMinigameWorldSubsystem::Deinitialize()
{
    OnRunCancelled.Clear();
    if (Active.Runtime.IsValid()) Active.Runtime->Cleanup();
    Active = FActiveMiniGameRun(); Roots.Reset(); Levels.Reset();
    for (auto& Handle : AssetLoads) if (Handle) Handle->CancelHandle();
    AssetLoads.Reset(); Super::Deinitialize();
}
