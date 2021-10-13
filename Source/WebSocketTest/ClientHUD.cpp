// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientHUD.h"
#include "SClientWidget.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"


void AClientHUD::BeginPlay()
{
	Super::BeginPlay();

	ShowMenu();
}

void AClientHUD::ShowMenu()
{
	if (GEngine && GEngine->GameViewport)
	{
		/** our widget now knows about the HUD class*/
		ClientWidget = SNew(SClientWidget).OwningHUD(this);
		GEngine->GameViewport->AddViewportWidgetContent(SAssignNew(ClientWidgetContainer, SWeakWidget).PossiblyNullContent(ClientWidget.ToSharedRef()));
        
		if (PlayerOwner)
		{
			// we can only click on the ui so we don't move the player around
			PlayerOwner->bShowMouseCursor = true;
			PlayerOwner->SetInputMode(FInputModeUIOnly());
		}
	}
}

void AClientHUD::RemoveMenu()
{
	if (GEngine && GEngine->GameViewport && ClientWidgetContainer.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ClientWidgetContainer.ToSharedRef());
        
		if (PlayerOwner)
		{
			PlayerOwner->bShowMouseCursor = false;
			PlayerOwner->SetInputMode(FInputModeGameOnly());
		}
	}
}
