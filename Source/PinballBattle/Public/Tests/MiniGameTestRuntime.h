#pragma once
#include "CoreMinimal.h"
#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "GameFramework/Pawn.h"
#include "MiniGameTestRuntime.generated.h"

/** Independent Space-action fixture using the same runtime contract as later real minigames. */
UCLASS()
class PINBALLBATTLE_API AMiniGameTestRuntime : public AMiniGameRuntimeBase
{
    GENERATED_BODY()
public:
    /** Configure a visible original primitive arena for the framework proof. */
    AMiniGameTestRuntime();
    /** Reset local progress on every initialization; optionally inject a partial-init failure. */
    virtual bool OnInitializeRun_Implementation(const FMiniGameContext& Context) override;
    /** Permit deterministic start failure without changing shared lifecycle code. */
    virtual bool OnStartRun_Implementation() override;
    /** Count accepted fresh actions locally without touching the pinball score. */
    virtual void OnAction_Implementation() override;
    /** Emit the fixture's metric schema and local informational score. */
    virtual FMiniGameResult BuildResult_Implementation() const override;
    UPROPERTY(EditAnywhere) bool bFailInitialize = false;
    UPROPERTY(EditAnywhere) bool bFailStart = false;
    /** Expose fixture action count for held-key acceptance checks. */
    int32 GetActionCount() const { return Actions; }
private:
    int32 Actions = 0;
};
UCLASS()
class PINBALLBATTLE_API AMiniGameTestPawn : public APawn
{
    GENERATED_BODY()
public:
    /** Create an explicitly possessed, stationary primitive pawn for the Space-action stub. */
    AMiniGameTestPawn();
};
