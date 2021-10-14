// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClientPlayerController.generated.h"

/**
 *
 */
UCLASS()
class WEBSOCKETTEST_API AClientPlayerController : public APlayerController
{
	GENERATED_BODY()

	protected:

	AClientPlayerController();

	virtual void SetupInputComponent() override;

	void OpenMenu();
};
