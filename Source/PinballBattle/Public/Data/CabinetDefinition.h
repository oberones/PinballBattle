#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CabinetDefinition.generated.h"

class APinballTable;
class APinballControlPawn;
class UScoringProfile;
class UPinballTuningData;
class UPinballPresentationWidget;
class UMiniGameDefinition;

/** Content-owned cabinet configuration with strict production and explicit development associations. */
UCLASS(BlueprintType)
class PINBALLBATTLE_API UCabinetDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    /** Validate registered table/pawn/camera and independent arena associations before boot. */
    bool Validate(const APinballTable* Table, const APawn* Pawn, FString& Error) const;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CabinetId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayTitle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> PersistentMap;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<APinballTable> TableClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<APinballControlPawn> ControlPawnClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UScoringProfile> ScoringProfile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UPinballTuningData> Tuning;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InitialBalls = 3;
    /** Phase 5 special-objective rearm delay after minigame return; pause freezes it and exit/re-entry is also required.
     * Ordinary trap recovery and score/drain gates do not use this delay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double ReturnTriggerProtectionSeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double ResultsPresentationSeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDevelopmentWithoutMinigames = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UMiniGameDefinition>> MiniGames;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> InstructionsWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> MiniGameHUDWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> ResultsWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> RecoveryWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> StartWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> HUDWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> PauseWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> GameOverWidget;
};
