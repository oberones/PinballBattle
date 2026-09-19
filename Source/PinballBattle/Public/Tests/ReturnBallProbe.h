#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReturnBallProbe.generated.h"
class APinballTable;
class APinballBall;
class APlayerController;
class UPrimitiveComponent;

/** Observe a real minigame release without repositioning the ball or applying an impulse. */
UCLASS()
class UReturnBallProbe : public UObject
{
    GENERATED_BODY()
public:
    enum class EResult { Running, Passed, Failed };
    /** Observe contacts on the returned ball and release stale arrows before a fresh strike. */
    void Begin(APinballTable* InTable, APlayerController* InController);
    /** Time one mapped flipper press and require a real contact followed by upfield travel. */
    EResult Tick(float DeltaSeconds, FString& Failure);
private:
    /** Record only physical contacts with the selected flipper. */
    UFUNCTION() void ObserveHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* Other, FVector NormalImpulse, const FHitResult& Hit);
    /** Use the same controller boundary as a player's arrow key. */
    void Key(FKey Input, bool Pressed);
    TWeakObjectPtr<APinballTable> Table;
    TWeakObjectPtr<APinballBall> Ball;
    TWeakObjectPtr<APlayerController> Controller;
    FVector ReleasePosition;
    float Age = 0;
    float PressAt = 0;
    bool bLeft = false;
    bool bPressed = false;
    bool bContact = false;
};
