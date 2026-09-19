#include "Tests/FlipperReturnCheck.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballFlipper.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "InputKeyEventArgs.h"
#include "PinballBattle.h"

// IsHeld alone does not prove the restored Chaos hinge can move its body.
FFlipperReturnCheck::EResult FFlipperReturnCheck::Tick(APinballTable* Table, APlayerController* Controller, float DeltaSeconds, FString& Failure)
{
    const auto Key = [Controller](FKey Input, bool Pressed)
    {
        Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
            Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
    };
    const auto Angle = [](const APinballFlipper* Flipper)
    {
        const FVector Local = Flipper->GetActorTransform().InverseTransformVectorNoScale(Flipper->GetBody()->GetForwardVector());
        return FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
    };
    auto* Left = Table->LeftFlipper.Get(); auto* Right = Table->RightFlipper.Get();
    if (!Controller || !Left || !Right) { Failure = TEXT("Missing return control targets"); return EResult::Failed; }
    Age += DeltaSeconds;
    if (Step == 0)
    {
        Key(EKeys::Left, false); Key(EKeys::Right, false); Step = 1; Age = 0;
    }
    else if (Step == 1 && Age >= .25f)
    {
        LeftRest = Angle(Left); RightRest = Angle(Right);
        Key(EKeys::Left, true); Step = 2; Age = 0;
    }
    else if (Step == 2 && Age >= .25f)
    {
        LeftTravel = FMath::Abs(FMath::FindDeltaAngleDegrees(LeftRest, Angle(Left)));
        if (!Left->IsHeld() || Right->IsHeld() || LeftTravel < 35 || FMath::Abs(FMath::FindDeltaAngleDegrees(RightRest, Angle(Right))) > 5)
        {
            const auto* Hinge = Left->FindComponentByClass<UPhysicsConstraintComponent>();
            Failure = FString::Printf(TEXT("Left paddle after return: held=%d otherHeld=%d travel=%.2f simulated=%d hingeValid=%d"),
                Left->IsHeld(), Right->IsHeld(), LeftTravel, Left->GetBody()->IsSimulatingPhysics(), Hinge && Hinge->ConstraintInstance.IsValidConstraintInstance());
            return EResult::Failed;
        }
        Key(EKeys::Left, false); Key(EKeys::Right, true); Step = 3; Age = 0;
    }
    else if (Step == 3 && Age >= .25f)
    {
        RightTravel = FMath::Abs(FMath::FindDeltaAngleDegrees(RightRest, Angle(Right)));
        if (!Right->IsHeld() || Left->IsHeld() || RightTravel < 35 || FMath::Abs(FMath::FindDeltaAngleDegrees(LeftRest, Angle(Left))) > 5)
        {
            Failure = FString::Printf(TEXT("Right paddle after return: held=%d otherHeld=%d travel=%.2f"), Right->IsHeld(), Left->IsHeld(), RightTravel);
            return EResult::Failed;
        }
        Key(EKeys::Right, false); Step = 4; Age = 0;
    }
    else if (Step == 4 && Age >= .25f)
    {
        if (Left->IsHeld() || Right->IsHeld() || FMath::Abs(FMath::FindDeltaAngleDegrees(LeftRest, Angle(Left))) > 5 ||
            FMath::Abs(FMath::FindDeltaAngleDegrees(RightRest, Angle(Right))) > 5)
        { Failure = TEXT("Paddles did not return to neutral after key release"); return EResult::Failed; }
        UE_LOG(LogPinballBattle, Display, TEXT("FLIPPER_RETURN_PASS leftTravel=%.2f rightTravel=%.2f independent input and neutral release"), LeftTravel, RightTravel);
        return EResult::Passed;
    }
    return EResult::Running;
}
