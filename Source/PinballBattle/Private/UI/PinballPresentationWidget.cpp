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

UButton* UPinballPresentationWidget::AddButton(UVerticalBox* Column, const FText& Text, FName Name)
{
    auto* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
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

// Build content-selected menus and passive round presentation; confirmation is consumed by controller.
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
    if (Screen >= EPinballScreen::Instructions) Status->SetWrapTextAt(StatusWrapWidth);
    if (Screen < EPinballScreen::Instructions) AddLabel(Column, FText::FromString(TEXT("LEFT / RIGHT  Flippers\nDOWN  Hold / release to launch\nESCAPE  Pause / resume")), 16);
    if (Screen == EPinballScreen::Recovery)
        AddButton(Column, FText::FromString(TEXT("Retry")), TEXT("RetryButton"))->OnClicked.AddDynamic(this, &ThisClass::RetryIntent);
    if (Screen == EPinballScreen::Start || Screen == EPinballScreen::Pause || Screen == EPinballScreen::GameOver || Screen == EPinballScreen::Recovery)
    {
        const TCHAR* Label = Screen == EPinballScreen::Start ? TEXT("Start") : Screen == EPinballScreen::Pause ? TEXT("Resume") : TEXT("Restart");
        AddButton(Column, FText::FromString(Label), TEXT("PrimaryButton"))->OnClicked.AddDynamic(this, &ThisClass::PrimaryIntent);
        AddButton(Column, FText::FromString(TEXT("Quit")), TEXT("QuitButton"))->OnClicked.AddDynamic(this, &ThisClass::QuitIntent);
        AddLabel(Column, FText::FromString(TEXT("Enter to confirm")), 14);
        Notice = AddLabel(Column, FText::GetEmpty(), 18);
        Notice->SetWrapTextAt(360);
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

// Round screens receive their summary directly from flow; preserve ordinary session HUD behavior.
void UPinballPresentationWidget::OnSession(const FSessionState& State)
{
    if (Screen >= EPinballScreen::Instructions) return;
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

// Instructions leave Enter/Space to the controller's release-gated confirmation boundary.
FReply UPinballPresentationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Screen < EPinballScreen::Instructions && !Event.IsRepeat() && Event.GetKey() == EKeys::Enter) { PrimaryIntent(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}

// Update only passive round screens so ordinary session/pause labels cannot be overwritten.
void UPinballPresentationWidget::ShowMiniGameStatus(const FText& Text)
{ if (Status && Screen >= EPinballScreen::Instructions) Status->SetText(Text); }
// Retry is an explicit flow intent, never a widget-owned timer or cleanup operation.
void UPinballPresentationWidget::RetryIntent()
{ if (auto* Controller = Cast<APinballPlayerController>(GetOwningPlayer())) Controller->RequestRetryIntent(); }

FString UPinballPresentationWidget::GetStatusText() const
{
    return (Status ? Status->GetText().ToString() : FString()) +
        (Notice ? TEXT("\n") + Notice->GetText().ToString() : FString());
}

void UPinballPresentationWidget::ShowStartFailure(const FText& Failure)
{
    if (Notice) Notice->SetText(Failure);
}
