#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CabinetDefinition.generated.h"

class APinballTable;
class APinballControlPawn;
class UScoringProfile;
class UPinballTuningData;
class UPinballPresentationWidget;

/** Content-owned cabinet configuration; Phase 4 explicitly runs without minigames. */
UCLASS(BlueprintType)
class PINBALLBATTLE_API UCabinetDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    /** Validate the Phase 4 development cabinet and its registered table/pawn/camera contract. */
    bool Validate(const APinballTable* Table, const APawn* Pawn, FString& Error) const;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CabinetId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayTitle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> PersistentMap;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<APinballTable> TableClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<APinballControlPawn> ControlPawnClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UScoringProfile> ScoringProfile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UPinballTuningData> Tuning;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InitialBalls = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double ReturnTriggerProtectionSeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double ResultsPresentationSeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDevelopmentWithoutMinigames = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> StartWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> HUDWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> PauseWidget;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UPinballPresentationWidget> GameOverWidget;
};
