#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/MiniGameTypes.h"
#include "MiniGameDefinition.generated.h"
class AMiniGameRuntimeBase;
class APawn;
class UInputMappingContext;
class UInputAction;
class UUserWidget;

/** Content-owned isolated arena and local rules, resolved during boot rather than activation. */
UCLASS(BlueprintType)
class PINBALLBATTLE_API UMiniGameDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName MiniGameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Instructions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> Map;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<AMiniGameRuntimeBase> RuntimeClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<APawn> PawnClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UInputMappingContext> InputContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UInputAction> ActionInput;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<UUserWidget> HUDClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform ArenaTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector ArenaExtent = FVector(500);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double DurationSeconds = 30;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LocalLives = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double MinimumResultMultiplier = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double MaximumResultMultiplier = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumObjectives = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, double> MetricBounds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ScoringProfileKey;
    /** Reject incomplete assets and nonfinite or out-of-contract local rules at boot. */
    bool Validate(FString& Error) const;
};
