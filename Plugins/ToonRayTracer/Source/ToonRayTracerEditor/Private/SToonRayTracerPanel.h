// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ToonRayTracerEnvironmentPresets.h"

class IDetailsView;

/**
 * ToonRayTracer の設定を切り替えるパネル。
 * 上部に操作ボタン、その下に設定の詳細パネル（UToonRayTracerEditorSettings）を表示する。
 */
class SToonRayTracerPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SToonRayTracerPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnResetAccumulationClicked();
	FReply OnImportFromConsoleClicked();
	FReply OnResetToDefaultsClicked();
	FReply OnEnvironmentPresetClicked(EToonEnvironmentPreset Preset);

	// 環境プリセットを適用した結果のメッセージ
	FText EnvironmentMessage;

	TSharedPtr<IDetailsView> DetailsView;
};
