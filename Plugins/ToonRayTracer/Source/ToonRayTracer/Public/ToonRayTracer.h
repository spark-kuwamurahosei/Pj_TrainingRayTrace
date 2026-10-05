// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FToonRayTracerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	// 遅延登録用
	void OnPostEngineInit();
	
	// SVEのインスタンスを保持するスマートポインタ
	TSharedPtr<class ToonRayTracerViewExtension, ESPMode::ThreadSafe> ToonViewExtension;
};
