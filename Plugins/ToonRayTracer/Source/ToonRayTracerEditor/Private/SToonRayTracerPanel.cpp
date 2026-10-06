// Fill out your copyright notice in the Description page of Project Settings.

#include "SToonRayTracerPanel.h"
#include "ToonRayTracerEditorSettings.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SToonRayTracerPanel"

void SToonRayTracerPanel::Construct(const FArguments& InArgs)
{
	UToonRayTracerEditorSettings* Settings = GetMutableDefault<UToonRayTracerEditorSettings>();

	// パネルを開いた時点のコンソール変数の値を表示する（コンソールで変更した値も反映される）
	Settings->ImportFromConsoleVariables();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bAllowSearch = true;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Settings);

	ChildSlot
	[
		SNew(SVerticalBox)

		// 操作ボタン
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SWrapBox)
			.UseAllottedSize(true)

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ResetAccumulation", "蓄積をリセット"))
				.ToolTipText(LOCTEXT("ResetAccumulationTooltip", "フレーム間の蓄積をやり直します。レベル上のオブジェクトを動かしたあとに使います"))
				.OnClicked(this, &SToonRayTracerPanel::OnResetAccumulationClicked)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportFromConsole", "コンソールの値を読み込む"))
				.ToolTipText(LOCTEXT("ImportFromConsoleTooltip", "コンソールで変更した r.ToonRayTracer.* の値をパネルに反映します"))
				.OnClicked(this, &SToonRayTracerPanel::OnImportFromConsoleClicked)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ResetToDefaults", "初期値に戻す"))
				.ToolTipText(LOCTEXT("ResetToDefaultsTooltip", "すべての設定を初期値に戻します"))
				.OnClicked(this, &SToonRayTracerPanel::OnResetToDefaultsClicked)
			]
		]

		// 設定
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			DetailsView.ToSharedRef()
		]
	];
}

FReply SToonRayTracerPanel::OnResetAccumulationClicked()
{
	UToonRayTracerEditorSettings::ResetAccumulation();
	return FReply::Handled();
}

FReply SToonRayTracerPanel::OnImportFromConsoleClicked()
{
	UToonRayTracerEditorSettings* Settings = GetMutableDefault<UToonRayTracerEditorSettings>();
	Settings->ImportFromConsoleVariables();
	Settings->SaveConfig();
	DetailsView->ForceRefresh();
	return FReply::Handled();
}

FReply SToonRayTracerPanel::OnResetToDefaultsClicked()
{
	UToonRayTracerEditorSettings* Settings = GetMutableDefault<UToonRayTracerEditorSettings>();
	Settings->ResetToDefaults();
	Settings->ApplyToConsoleVariables();
	Settings->SaveConfig();
	DetailsView->ForceRefresh();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
