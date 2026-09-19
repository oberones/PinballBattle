#include "Tests/ReturnBallProbe.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballFlipper.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "PinballBattle.h"

void UReturnBallProbe::Begin(APinballTable* InTable, APlayerController* InController)
{
    Table = InTable; Controller = InController; Ball = InTable->GetBall();
    ReleasePosition = InTable->GetActorTransform().InverseTransformPosition(Ball->GetActorLocation());
    bLeft = FVector::DistSquared2D(Ball->GetActorLocation(), InTable->LeftFlipper->GetActorLocation()) <
        FVector::DistSquared2D(Ball->GetActorLocation(), InTable->RightFlipper->GetActorLocation());
    Ball->GetBody()->OnComponentHit.AddDynamic(this, &ThisClass::ObserveHit);
    Key(EKeys::Left, false); Key(EKeys::Right, false);
}

void UReturnBallProbe::Key(FKey Input, bool Pressed)
{
    if (Controller.IsValid()) Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void UReturnBallProbe::ObserveHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult&)
{
    if (Table.IsValid() && bPressed && OtherActor == (bLeft ? Table->LeftFlipper.Get() : Table->RightFlipper.Get())) bContact = true;
}

UReturnBallProbe::EResult UReturnBallProbe::Tick(float DeltaSeconds, FString& Failure)
{
    if (!Table.IsValid() || !Ball.IsValid() || !Controller.IsValid() || Table->GetBall() != Ball.Get())
    { Failure = TEXT("Returned ball lost before a playable flipper feed"); return EResult::Failed; }
    Age += DeltaSeconds;
    const FTransform Frame = Table->GetActorTransform();
    const FVector Position = Frame.InverseTransformPosition(Ball->GetActorLocation());
    const FVector Velocity = Frame.InverseTransformVectorNoScale(Ball->GetBody()->GetPhysicsLinearVelocity());
    const auto* Flipper = bLeft ? Table->LeftFlipper.Get() : Table->RightFlipper.Get();
    const float PivotY = Frame.InverseTransformPosition(Flipper->GetActorLocation()).Y;
    if (!bPressed && Position.Y < PivotY + 50 && Velocity.Y < 0)
    { Key(bLeft ? EKeys::Left : EKeys::Right, true); bPressed = true; PressAt = Age; }
    if (bContact && Velocity.Y > 150 && Position.Y > PivotY + 100 && PressAt >= .6f)
    {
        Key(bLeft ? EKeys::Left : EKeys::Right, false);
        Ball->GetBody()->OnComponentHit.RemoveDynamic(this, &ThisClass::ObserveHit);
        UE_LOG(LogPinballBattle, Display, TEXT("RETURN_BALL_PASS release=%s paddle=%s reaction=%.2fs upfieldSpeed=%.1f contact=1"),
            *ReleasePosition.ToCompactString(), bLeft ? TEXT("Left") : TEXT("Right"), PressAt, Velocity.Y);
        return EResult::Passed;
    }
    if (Age > 4 || Position.Y < PivotY - 85)
    {
        Key(bLeft ? EKeys::Left : EKeys::Right, false);
        Failure = FString::Printf(TEXT("Unplayable return: release=%s ball=%s velocity=%s contact=%d reaction=%.2f"),
            *ReleasePosition.ToCompactString(), *Position.ToCompactString(), *Velocity.ToCompactString(), bContact, PressAt);
        return EResult::Failed;
    }
    return EResult::Running;
}
