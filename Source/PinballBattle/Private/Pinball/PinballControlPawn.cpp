#include "Pinball/PinballControlPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballFlipper.h"
#include "Pinball/PinballPlunger.h"
#include "Framework/PinballGameModeBase.h"
#include "Engine/World.h"

APinballControlPawn::APinballControlPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    FoundationCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FoundationCamera"));
    FoundationCamera->SetupAttachment(RootComponent);
    FoundationCamera->bUsePawnControlRotation = false;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void APinballControlPawn::SetLeftHeld(bool bHeld)
{
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (Table && Table->LeftFlipper)
        Table->LeftFlipper->SetHeld(bHeld && Mode && (Mode->CanPlay() || Mode->CanLaunch()));
}

void APinballControlPawn::SetRightHeld(bool bHeld)
{
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (Table && Table->RightFlipper)
        Table->RightFlipper->SetHeld(bHeld && Mode && (Mode->CanPlay() || Mode->CanLaunch()));
}

void APinballControlPawn::BeginPlunger()
{
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (Table && Mode && Mode->CanLaunch()) Table->Plunger->BeginCharge();
}

void APinballControlPawn::ReleasePlunger()
{
    if (!Table || !Table->Plunger) return;
    const float Impulse = Table->Plunger->ReleaseCharge();
    if (APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>())
        if (Impulse > 0.f) Mode->RequestLaunch(Impulse);
}

void APinballControlPawn::CancelActions()
{
    if (Table) Table->CancelActions();
}
