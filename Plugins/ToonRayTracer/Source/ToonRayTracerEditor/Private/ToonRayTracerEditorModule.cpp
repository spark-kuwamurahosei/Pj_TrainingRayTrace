// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "SToonRayTracerPanel.h"
#include "ToonRayTracerEditorSettings.h"

#define LOCTEXT_NAMESPACE "FToonRayTracerEditorModule"

/**
 * ToonRayTracer のエディタ用モジュール。
 * メニューの「ウィンドウ」から開ける設定パネルを登録する。
 */
class FToonRayTracerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// 前回保存した設定をコンソール変数に反映する（ランタイムモジュールは PostConfigInit で先に読み込まれている）
		GetDefault<UToonRayTracerEditorSettings>()->ApplyToConsoleVariables();

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PanelTabName, FOnSpawnTab::CreateRaw(this, &FToonRayTracerEditorModule::SpawnPanelTab))
			.SetDisplayName(LOCTEXT("PanelTabTitle", "ToonRayTracer"))
			.SetTooltipText(LOCTEXT("PanelTabTooltip", "ToonRayTracer の表示モードやトゥーンの設定を切り替えます"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
	}

	virtual void ShutdownModule() override
	{
		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PanelTabName);
		}
	}

private:
	TSharedRef<SDockTab> SpawnPanelTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SToonRayTracerPanel)
			];
	}

	static const FName PanelTabName;
};

const FName FToonRayTracerEditorModule::PanelTabName(TEXT("ToonRayTracerPanel"));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FToonRayTracerEditorModule, ToonRayTracerEditor)
