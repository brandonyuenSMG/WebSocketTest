// Copyright Epic Games, Inc. All Rights Reserved.

#include "WebSocketTestGameMode.h"

#include "ClientHUD.h"
#include "ClientPlayerController.h"

AWebSocketTestGameMode::AWebSocketTestGameMode()
{
	PlayerControllerClass = AClientPlayerController::StaticClass();

	HUDClass = AClientHUD::StaticClass();
}
