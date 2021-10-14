// Fill out your copyright notice in the Description page of Project Settings.


#include "SClientWidget.h"

#include <iostream>

#include "SlateOptMacros.h"
#include "ClientHUD.h"
#include "WebSocketStructs.h"
#include "IWebSocket.h"
#include "WebSocketClient.h"
#include "Misc/MessageDialog.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "Client"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SClientWidget::Construct(const FArguments& InArgs)
{
    bCanSupportFocus = true;

    OwningHUD = InArgs._OwningHUD;

    const FMargin ContentPadding = FMargin(300.f, 100.f);
    const FMargin ButtonPadding = FMargin(25.f);

    const FText TitleText = LOCTEXT("WebSocket Client", "WebSocket Client");
    const FText ConnectText = LOCTEXT("Connect", "Connect");
    const FText DebugLoginText = LOCTEXT("Debug Login", "Debug Login");
    const FText EchoText = LOCTEXT("Echo", "Echo");
    const FText DisconnectText = LOCTEXT("Disconnect", "Disconnect");
    const FText QuitText = LOCTEXT("Quit", "Quit");

    FSlateFontInfo ButtonTextStyle = FCoreStyle::Get().GetFontStyle("EmbossedText");
    ButtonTextStyle.Size = 40.f;

    FSlateFontInfo TitleTextStyle = ButtonTextStyle;
    TitleTextStyle.Size = 60.f;

ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SImage)
            .ColorAndOpacity(FColor::Black)
        ]
        + SOverlay::Slot()
        .Padding(25.f, 25.f)
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        [
            //Connection Indicator
            SAssignNew(ConnectionIndicator, SImage)
            .ColorAndOpacity(FColor::Red)
        ]
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .Padding(ContentPadding)
        [
            SNew(SVerticalBox)
            //Title text
            + SVerticalBox::Slot()
            [
                SNew(STextBlock)
                .Font(TitleTextStyle)
                .Text(TitleText)
                .Justification(ETextJustify::Center)
            ]

            //Connect button
            + SVerticalBox::Slot()
            .Padding(ButtonPadding)
            [
                SNew(SButton)
                .OnClicked(this, &SClientWidget::OnConnectClicked)
                [
                    SNew(STextBlock)
                    .Font(ButtonTextStyle)
                    .Text(ConnectText)
                    .Justification(ETextJustify::Center)
                ]
            ]
             //DebugLogin button
             + SVerticalBox::Slot()
             .Padding(ButtonPadding)
             [
                 SNew(SButton)
                 .OnClicked(this, &SClientWidget::OnDebugLoginClicked)
                 [
                     SNew(STextBlock)
                     .Font(ButtonTextStyle)
                     .Text(DebugLoginText)
                     .Justification(ETextJustify::Center)
                 ]
             ]

             //Echo button
             + SVerticalBox::Slot()
             .Padding(ButtonPadding)
             [
                 SNew(SButton)
                .OnClicked(this, &SClientWidget::OnEchoClicked)
                 [
                     SNew(STextBlock)
                     .Font(ButtonTextStyle)
                     .Text(EchoText)
                     .Justification(ETextJustify::Center)
                 ]
             ]

             //Disconnect button
             + SVerticalBox::Slot()
             .Padding(ButtonPadding)
             [
                 SNew(SButton)
                .OnClicked(this, &SClientWidget::OnDisconnectClicked)
                 [
                     SNew(STextBlock)
                     .Font(ButtonTextStyle)
                     .Text(DisconnectText)
                     .Justification(ETextJustify::Center)
                 ]
             ]

             //Quit button
             + SVerticalBox::Slot()
             .Padding(ButtonPadding)
             [
                SNew(SButton)
                .OnClicked(this, &SClientWidget::OnQuitClicked)
                [
                    SNew(STextBlock)
                    .Font(ButtonTextStyle)
                    .Text(QuitText)
                    .Justification(ETextJustify::Center)
                ]
            ]
        ]
    ];

    struct FWebSocketConfiguration Config;
    Config.Num_Retries = 10;
    Config.Sleep_Length = 3;

    Client = MakeShareable(new FWebSocketClient(Config));


    Client->ConnectionDelegate.AddLambda([this](const bool IsSuccess)
    {
        if (IsSuccess)
        {
            ConnectionIndicator->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Green));
            Client->On<FChatMessage>("ChatMessage", [](const FChatMessage& Message)
            {
                FMessageDialog().Debugf(FText::FromString(Message.Message));
            });
        }
    });

    Client->OnClosed.AddLambda([this]() {
        ConnectionIndicator->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Red));
    });

    Client->OnReconnection.AddLambda([this]() {
        ConnectionIndicator->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor::Orange));
    });
}

FReply SClientWidget::OnConnectClicked() const
{
    Client->ConnectToServer();

    return FReply::Handled();
}

void SClientWidget::HandleError(const FMgsError& Error) const
{
    AsyncTask(ENamedThreads::GameThread, [Error]()
    {
        FMessageDialog().Debugf(FText::FromString(Error.Message));
    });
}

FReply SClientWidget::OnDebugLoginClicked() const
{
    if (Client->WebSocket && Client->IsConnected())
    {
        const FDebugLoginRequestData DebugLogin{"myToken"};
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, DebugLogin]()
        {
            try
            {
                const auto Login = Client-> SendAsync<FDebugLoginRequestData, FDebugLoginResponseData>(DebugLogin, true);
                UE_LOG(LogTemp, Log, TEXT("%s"), *Client-> SendAsync<FDebugLoginRequestData, FDebugLoginResponseData>(DebugLogin, true).Id);
                AsyncTask(ENamedThreads::GameThread, [Login]()
                {
                    FMessageDialog().Debugf(FText::FromString("Logged in: " + Login.Id));
                });
            } catch (FMgsError e)
            {
                HandleError(e);
            }
        });
    } else
    {
        FMessageDialog().Debugf(FText::FromString("Not connected to a server"));
    }

    return FReply::Handled();
}

FReply SClientWidget::OnEchoClicked() const
{
    if (Client->WebSocket && Client->IsConnected())
    {
        const FEchoRequestData Echo{"Testing...testing...1..2..3.."};
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Echo]()
        {
            try
            {
                const auto EchoResponse = Client->SendAsync<FEchoRequestData, FEchoResponseData>(Echo, true).Val;
                UE_LOG(LogTemp, Log, TEXT("%s"), *EchoResponse);
                AsyncTask(ENamedThreads::GameThread, [EchoResponse]()
                {
                    FMessageDialog().Debugf(FText::FromString(EchoResponse));
                });
            } catch (const FMgsError& e)
            {
                HandleError(e);
            }
        });

    } else
    {
        FMessageDialog().Debugf(FText::FromString("Not connected to a server"));
    }

    return FReply::Handled();
}

FReply SClientWidget::OnDisconnectClicked() const
{
    if (Client->WebSocket && Client->IsConnected())
    {
        Client->DisconnectFromServer();
    } else
    {
        FMessageDialog().Debugf(FText::FromString("Not connected to a server"));
    }

    return FReply::Handled();
}

FReply SClientWidget::OnQuitClicked() const
{
    if (OwningHUD.IsValid())
    {
        if (APlayerController* PC = OwningHUD->PlayerOwner)
        {
            Client->Quit();
            PC->ConsoleCommand("quit");
        }
    }

    return FReply::Handled();
}

void SClientWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (Client->IsConnected())
    {
        Client->ProcessPushMessages(1);
    }
}
#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
