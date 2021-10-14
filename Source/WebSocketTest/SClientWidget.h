// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "WebSocketClient.h"
/**
 *
 */
class SClientWidget final : public SCompoundWidget
{

	TSharedPtr<SImage> ConnectionIndicator;

private:
	SLATE_BEGIN_ARGS(SClientWidget) {}

	SLATE_ARGUMENT(TWeakObjectPtr<class AClientHUD>, OwningHUD)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FReply OnConnectClicked() const;
	void HandleError(const FMgsError& Error) const;
	FReply OnDebugLoginClicked() const;
	FReply OnEchoClicked() const;
	FReply OnDisconnectClicked() const;
	FReply OnQuitClicked() const;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** The HUD that created this widget*/
	TWeakObjectPtr<class AClientHUD> OwningHUD;

	virtual bool SupportsKeyboardFocus() const override { return true;};

	TSharedPtr<FWebSocketClient> Client;
};
