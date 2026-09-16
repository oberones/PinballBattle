#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "DrainComponent.generated.h"

class APinballTable;

UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UDrainComponent : public UBoxComponent
{
    GENERATED_BODY()
public:
    UDrainComponent();
    void Initialize(APinballTable* InTable) { Table = InTable; }
private:
    UFUNCTION() void OnEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
};

UCLASS()
class PINBALLBATTLE_API APinballDrain : public AActor
{
    GENERATED_BODY()
public:
    APinballDrain();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UDrainComponent> Drain;
};
