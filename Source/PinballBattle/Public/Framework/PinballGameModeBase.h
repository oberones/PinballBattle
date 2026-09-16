#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/ScoringTypes.h"
#include "PinballGameModeBase.generated.h"

class UGameFlowComponent;
class APinballTable;
class APinballBall;

UCLASS()
class PINBALLBATTLE_API APinballGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    /** Select shared framework classes and the sole flow-writing component. */
    APinballGameModeBase();
    /** Bind the world-local read-only flow projection. */
    virtual void InitGameState() override;
    /** Validate the explicitly registered table and start the configured practice lifecycle. */
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
    /** Consume the current playable ball once and queue one practice replacement. */
    bool RequestDrain(APinballBall* Ball);
    /** Reject stale/malformed drain identities before invoking the once-only drain boundary. */
    bool RequestDrainEvent(const FDrainEvent& Event);
    /** Expose the registered table without searching actors by names. */
    APinballTable* GetTable() const { return Table; }

protected:
    // Enabled only by test content. The shared lifecycle owns practice and future sessions.
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Development") bool bPracticeMode = false;

    /** Expose read-only flow access to Blueprint presentation. */
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    UGameFlowComponent* GetGameFlow() const { return GameFlow; }

private:
    /** Spawn only after the prior actor is removed and BALL_LOST is still authoritative. */
    void ReplaceDrainedBall();
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    FTimerHandle ReplacementTimer;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGameFlowComponent> GameFlow;
};
