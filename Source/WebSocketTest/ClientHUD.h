// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ClientHUD.generated.h"


/**
 * 
 */
UCLASS()
class WEBSOCKETTEST_API AClientHUD : public AHUD
{
	GENERATED_BODY()

	
	protected:
        
	TSharedPtr<class SClientWidget> ClientWidget;
	TSharedPtr<class SWidget> ClientWidgetContainer;
        
	virtual void BeginPlay() override;
    	
	public:
        
	void ShowMenu();
	void RemoveMenu();
};
