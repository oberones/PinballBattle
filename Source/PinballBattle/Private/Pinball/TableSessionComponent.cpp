#include "Pinball/TableSessionComponent.h"
#include "Components/PrimitiveComponent.h"

UTableSessionComponent::UTableSessionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTableSessionComponent::RegisterBody(UPrimitiveComponent* Body)
{
    Bodies.RemoveAll([](const auto& Item) { return !Item.IsValid(); });
    if (IsValid(Body)) Bodies.AddUnique(Body);
}

void UTableSessionComponent::UnregisterBody(UPrimitiveComponent* Body)
{
    Bodies.RemoveAll([Body](const auto& Item) { return !Item.IsValid() || Item.Get() == Body; });
}
