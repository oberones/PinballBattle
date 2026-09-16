#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PinballGameModeBase.generated.h"

class UGameFlowComponent;
class APinballTable;
class APinballBall;

UCLASS()
class PINBALLBATTLE_API APinballGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    APinballGameModeBase();
    virtual void InitGameState() override;
    virtual void StartPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool RegisterTable(APinballTable* InTable);
    bool CanLaunch() const;
    bool CanPlay() const;
    bool RequestLaunch(float Impulse);
    bool RequestDrain(APinballBall* Ball);
    APinballTable* GetTable() const { return Table; }

protected:
    // Enabled only by test content. The shared lifecycle owns practice and future sessions.
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Development") bool bPracticeMode = false;

    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    UGameFlowComponent* GetGameFlow() const { return GameFlow; }

private:
    void ReplaceDrainedBall();
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    FTimerHandle ReplacementTimer;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGameFlowComponent> GameFlow;
};
