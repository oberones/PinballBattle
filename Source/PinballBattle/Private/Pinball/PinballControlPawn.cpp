#include "Pinball/PinballControlPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

APinballControlPawn::APinballControlPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    FoundationCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FoundationCamera"));
    FoundationCamera->SetupAttachment(RootComponent);
    FoundationCamera->bUsePawnControlRotation = false;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}
