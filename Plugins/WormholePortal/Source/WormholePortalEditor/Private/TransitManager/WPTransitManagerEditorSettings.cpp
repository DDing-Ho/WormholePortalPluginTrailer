// Copyright 2026 Team Beaver Studio. All Rights Reserved.

#include "WPTransitManagerEditorSettings.h"

#include "Engine/LevelScriptActor.h"
#include "GameFramework/WorldSettings.h"
#include "WormholePortalActor.h"

UWPTransitManagerEditorSettings::UWPTransitManagerEditorSettings()
{
	if (ExcludedClasses.IsEmpty())
	{
		ExcludedClasses.Add(AWormholePortalActor::StaticClass());
		ExcludedClasses.Add(AWorldSettings::StaticClass());
		ExcludedClasses.Add(ALevelScriptActor::StaticClass());
	}
}
