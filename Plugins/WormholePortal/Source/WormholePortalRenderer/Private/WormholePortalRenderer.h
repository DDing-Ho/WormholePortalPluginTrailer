// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FWPRendererService;

class FWormholePortalRendererModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TUniquePtr<FWPRendererService> RendererService;
};
