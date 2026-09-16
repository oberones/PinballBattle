#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PinballTable.generated.h"

class APinballBall;
class APinballFlipper;
class APinballPlunger;
class APinballBumper;
class APinballDrain;
class UCameraComponent;
class UTableSessionComponent;
class UPinballTuningData;
class UUserWidget;

UCLASS()
class PINBALLBATTLE_API APinballTable : public AActor
{
    GENERATED_BODY()
public:
    APinballTable();
    bool InitializeTable(FString& OutError);
    bool SpawnReadyBall();
    void RemoveBall();
    void CancelActions();
    bool IsCurrentBall(const APinballBall* Ball) const;
    APinballBall* GetBall() const { return CurrentBall; }
    UTableSessionComponent* GetSession() const { return Session; }
    FVector GetTableNormal() const { return GetActorUpVector(); }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UPinballTuningData> Tuning;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") TSubclassOf<APinballBall> BallClass;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballFlipper> LeftFlipper;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballFlipper> RightFlipper;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballPlunger> Plunger;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TArray<TObjectPtr<APinballBumper>> Bumpers;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Pinball") TObjectPtr<APinballDrain> Drain;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSubclassOf<UUserWidget> ControlsWidgetClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<USceneComponent> BallSpawn;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UCameraComponent> TableCamera;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTableSessionComponent> Session;
    UPROPERTY(Transient) TObjectPtr<APinballBall> CurrentBall;
};
