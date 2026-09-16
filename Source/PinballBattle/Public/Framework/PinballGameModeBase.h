#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/ScoringTypes.h"
#include "PinballGameModeBase.generated.h"

class UGameFlowComponent;
class APinballTable;
class APinballBall;
class UCabinetDefinition;

UCLASS()
class PINBALLBATTLE_API APinballGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    /** Select shared framework classes and the sole flow-writing component. */
    APinballGameModeBase();
    /** Bind the world-local read-only flow projection. */
    virtual void InitGameState() override;
    /** Validate the registered cabinet/table, then show Start or begin explicit test practice. */
    virtual void StartPlay() override;
    /** Cancel the replacement timer and actuator input on world teardown. */
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    /** Accept exactly one explicitly referenced table in this world. */
    bool RegisterTable(APinballTable* InTable);
    /** Permit a fresh launch only while the current ball is ready. */
    bool CanLaunch() const;
    /** Report whether flow is in the active pinball phase. */
    bool CanPlay() const;
    /** Accept one launch and advance flow plus physical entitlement together. */
    bool RequestLaunch(float Impulse);
    /** Consume the active ball once, update three-ball accounting and replace or end the game. */
    bool RequestDrain(APinballBall* Ball);
    /** Reject stale/malformed drain identities before invoking the once-only drain boundary. */
    bool RequestDrainEvent(const FDrainEvent& Event);
    /** Expose the registered table without searching actors by names. */
    APinballTable* GetTable() const { return Table; }
    /** Start or restart only from the corresponding menu state, using a fresh session identity. */
    bool RequestNewSession();
    /** Freeze or resume the current playable phase through native world pause. */
    bool RequestTogglePause(APlayerController* Controller);
    /** Invalidate all callbacks before stopping timers/actors on Quit or teardown. */
    void InvalidateSession();
    /** Supply content-owned presentation to the controller without a core theme dependency. */
    const UCabinetDefinition* GetCabinet() const { return Cabinet; }
    /** Keep repeat-ball practice explicit and restricted to test map configuration. */
    bool IsPracticeMode() const { return bPracticeMode; }

protected:
    // Enabled only by test content; production cabinets use the same lifecycle with three balls.
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Development") bool bPracticeMode = false;
    UPROPERTY(EditDefaultsOnly, Category="Pinball") TObjectPtr<UCabinetDefinition> Cabinet;

    /** Expose read-only flow access to Blueprint presentation. */
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    UGameFlowComponent* GetGameFlow() const { return GameFlow; }

private:
    /** Route typed events to the sole scoring owner with authoritative identity and phase context. */
    UFUNCTION() void HandleTableScore(const FScoringEvent& Event);
    /** Spawn only after the prior actor is removed and BALL_LOST is still authoritative. */
    void ReplaceDrainedBall();
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    FTimerHandle ReplacementTimer;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGameFlowComponent> GameFlow;
};
