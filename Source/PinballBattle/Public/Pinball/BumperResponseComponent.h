#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "BumperResponseComponent.generated.h"

class APinballTable;
class UStaticMeshComponent;

UCLASS(ClassGroup=Pinball, meta=(BlueprintSpawnableComponent))
class PINBALLBATTLE_API UBumperResponseComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void Initialize(APinballTable* InTable, UPrimitiveComponent* InSurface);
private:
    UFUNCTION() void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<UPrimitiveComponent> Surface;
    double LastImpulseTime = -100.;
};

/** Simple reusable host; the response also works on other explicitly registered surfaces. */
UCLASS()
class PINBALLBATTLE_API APinballBumper : public AActor
{
    GENERATED_BODY()
public:
    APinballBumper();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UStaticMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pinball") TObjectPtr<UBumperResponseComponent> Response;
};
