// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WebSocketTest : ModuleRules
{
	public WebSocketTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "WebSockets", "Json", "JsonUtilities"});
		
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore"});
	}
}
