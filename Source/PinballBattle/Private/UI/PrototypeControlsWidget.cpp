#include "UI/PrototypeControlsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Framework/PinballGameModeBase.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballPlunger.h"
#include "Engine/World.h"

/** Build the temporary practice controls without authoritative score or gameplay state. */
void UPrototypeControlsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Column);
    CanvasSlot->SetPosition(FVector2D(32, 48));
    CanvasSlot->SetSize(FVector2D(310, 500));
    auto AddText = [&](const FText& Text, int32 Size, FLinearColor Color)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(Text);
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = Size;
        Label->SetFont(Font);
        Label->SetColorAndOpacity(FSlateColor(Color));
        Label->SetAutoWrapText(true);
        Column->AddChildToVerticalBox(Label);
        return Label;
    };
    AddText(Heading, 28, FLinearColor(.25f, .85f, 1.f));
    AddText(Instructions, 18, FLinearColor(.85f, .9f, .95f));
    Status = AddText(FText::GetEmpty(), 21, FLinearColor(1.f, .8f, .3f));
}

/** Display launch power and observed event counts; no feedback path mutates score. */
void UPrototypeControlsWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (!Status || !Mode || !Mode->GetTable()) return;
    const APinballPlunger* Plunger = Mode->GetTable()->Plunger;
    FString Text = TEXT("Ball in play");
    if (Mode->CanLaunch()) Text = Plunger->IsCharging()
        ? FString::Printf(TEXT("Launch power: %d%%"), FMath::RoundToInt(100.f * Plunger->GetChargeAlpha()))
        : TEXT("Ready to launch");
    else if (!Mode->CanPlay()) Text = TEXT("Preparing next ball...");
    const APinballTable* Table = Mode->GetTable();
    if (Table->bRequireCompleteInventory)
    {
        if (Table->GetBallHandle().Disposition == EBallDisposition::Recovering) Text = TEXT("Return path blocked - ball secured");
        Text += FString::Printf(TEXT("\n\nHITS / TRAVERSALS\nTargets: %d\nBumpers: %d\nLanes: %d\nSafe returns: %d"),
            Table->GetInteractionCount(EScoringCategory::Target), Table->GetInteractionCount(EScoringCategory::Bumper),
            Table->GetInteractionCount(EScoringCategory::Lane), Table->GetRecoveryCount());
    }
    Status->SetText(FText::FromString(Text));
}
