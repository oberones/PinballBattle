#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/PinballSessionTypes.h"
#include "PinballGameStateBase.generated.h"

class UPinballScoringComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPinballSessionChanged, const FSessionState&, State);

/** Read-only world-scoped projection. Only the GameMode's flow component may publish flow. */
UCLASS()
class PINBALLBATTLE_API APinballGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    /** Create the sole score owner on this world-scoped projection. */
    APinballGameStateBase();
    /** Return a copy so consumers cannot mutate authoritative session fields. */
    UFUNCTION(BlueprintPure) FSessionState GetSessionState() const { return SessionState; }
    /** Expose read-only score access and presentation delegates. */
    UFUNCTION(BlueprintPure) UPinballScoringComponent* GetScoring() const { return Scoring; }
    UPROPERTY(BlueprintAssignable) FPinballSessionChanged OnSessionChanged;
    /** Read the current published flow phase. */
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    EArcadeGameFlowState GetFlowState() const { return SessionState.FlowState; }

private:
    friend class UGameFlowComponent;
    friend class APinballGameModeBase;

    /** Publish only internally valid session snapshots after a rule update. */
    void PublishSession();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPinballScoringComponent> Scoring;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    FSessionState SessionState;
};
