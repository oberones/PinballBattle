#include "UI/PinballPresentationWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballScoringComponent.h"
#include "Framework/PinballPlayerController.h"
#include "Engine/World.h"

UTextBlock* UPinballPresentationWidget::AddLabel(UVerticalBox* Column, const FText& Text, int32 Size)
{
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(Text);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(.8f, .93f, 1.f)));
    Label->SetAutoWrapText(true);
    Column->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, 16));
    return Label;
}

UButton* UPinballPresentationWidget::AddButton(UVerticalBox* Column, const FText& Text)
{
    auto* Button = WidgetTree->ConstructWidget<UButton>();
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(Text);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = 22;
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(.01f, .04f, .08f)));
    Button->AddChild(Label);
    Column->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 8));
    return Button;
}

void UPinballPresentationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(Screen != EPinballScreen::HUD);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    auto* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(.015f, .03f, .055f, .96f));
    Panel->SetPadding(FMargin(24));
    auto* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAutoSize(true);
    PanelSlot->SetPosition(FVector2D(24, 36));
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Column);
    AddLabel(Column, Heading, 28);
    Status = AddLabel(Column, FText::GetEmpty(), 23);
    AddLabel(Column, FText::FromString(TEXT("LEFT / RIGHT  Flippers\nDOWN  Hold / release to launch\nESCAPE  Pause / resume")), 16);
    if (Screen != EPinballScreen::HUD)
    {
        const TCHAR* Label = Screen == EPinballScreen::Start ? TEXT("Start") : Screen == EPinballScreen::Pause ? TEXT("Resume") : TEXT("Restart");
        AddButton(Column, FText::FromString(Label))->OnClicked.AddDynamic(this, &ThisClass::PrimaryIntent);
        AddButton(Column, FText::FromString(TEXT("Quit")))->OnClicked.AddDynamic(this, &ThisClass::QuitIntent);
        AddLabel(Column, FText::FromString(TEXT("Enter to confirm")), 14);
    }
    else SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPinballPresentationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    StateSource = GetWorld()->GetGameState<APinballGameStateBase>();
    if (!StateSource) return;
    StateSource->OnSessionChanged.AddUniqueDynamic(this, &ThisClass::OnSession);
    StateSource->GetScoring()->OnScoreChanged.AddUniqueDynamic(this, &ThisClass::OnScore);
    OnSession(StateSource->GetSessionState());
}

void UPinballPresentationWidget::NativeDestruct()
{
    if (StateSource)
    {
        StateSource->OnSessionChanged.RemoveDynamic(this, &ThisClass::OnSession);
        StateSource->GetScoring()->OnScoreChanged.RemoveDynamic(this, &ThisClass::OnScore);
    }
    StateSource = nullptr;
    Super::NativeDestruct();
}

void UPinballPresentationWidget::OnSession(const FSessionState& State)
{
    if (!Status || !StateSource) return;
    const TCHAR* Hint = State.FlowState == EArcadeGameFlowState::PINBALL_READY ? TEXT("Ready to launch") :
        State.FlowState == EArcadeGameFlowState::PAUSED ? TEXT("PAUSED") :
        State.FlowState == EArcadeGameFlowState::GAME_OVER ? TEXT("Final score") :
        State.FlowState == EArcadeGameFlowState::ATTRACT ? TEXT("Three-ball game") : TEXT("Ball in play");
    Status->SetText(FText::FromString(FString::Printf(TEXT("%s\nScore  %lld\nBalls  %d     Multiplier  x%d"),
        Hint, StateSource->GetScoring()->GetTotalScore(), State.BallsRemaining, State.Multiplier)));
}

void UPinballPresentationWidget::OnScore(const FScoreAward& Award)
{
    if (StateSource) OnSession(StateSource->GetSessionState());
}

void UPinballPresentationWidget::PrimaryIntent()
{
    if (auto* Controller = Cast<APinballPlayerController>(GetOwningPlayer()))
    {
        if (Screen == EPinballScreen::Pause) Controller->RequestPauseIntent();
        else Controller->RequestStartIntent();
    }
}

void UPinballPresentationWidget::QuitIntent()
{
    if (auto* Controller = Cast<APinballPlayerController>(GetOwningPlayer())) Controller->RequestQuitIntent();
}

FReply UPinballPresentationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && Event.GetKey() == EKeys::Enter) { PrimaryIntent(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}

FString UPinballPresentationWidget::GetStatusText() const
{
    return Status ? Status->GetText().ToString() : FString();
}
