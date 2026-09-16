#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PinballBall.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPinballTuningData;

UCLASS()
class PINBALLBATTLE_API APinballBall : public AActor
{
    GENERATED_BODY()
public:
    APinballBall();
    void Configure(UPinballTuningData* InTuning);
    bool Launch(const FVector& Direction, float Impulse);
    void AddBoundedImpulse(const FVector& Impulse);
    bool MarkDrained();
    bool IsLaunched() const { return bLaunched && !bDrained; }
    USphereComponent* GetBody() const { return Body; }
    int32 GetContactCount() const { return ContactCount; }
    virtual void Tick(float DeltaSeconds) override;
private:
    UFUNCTION() void OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(Transient) TObjectPtr<UPinballTuningData> Tuning;
    bool bLaunched = false;
    bool bDrained = false;
    int32 ContactCount = 0;
};
