// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientPlayerController.h"
#include "ClientHUD.h"


AClientPlayerController::AClientPlayerController()
{
	
}

void AClientPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (InputComponent)
	{
		InputComponent->BindAction("OpenMenu", IE_Pressed, this, &AClientPlayerController::OpenMenu);
	}
}

void AClientPlayerController::OpenMenu()
{
	if (AClientHUD* MenuHUD = Cast<AClientHUD>(GetHUD()))
	{
		MenuHUD->ShowMenu();
	}
}
