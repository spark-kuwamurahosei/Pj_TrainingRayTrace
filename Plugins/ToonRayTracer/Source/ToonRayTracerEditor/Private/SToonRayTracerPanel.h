// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ToonRayTracerEnvironmentPresets.h"

class AActor;
class IDetailsView;
class SVerticalBox;
class UToonCharacterProfile;

/**
 * ToonRayTracer の設定を切り替えるパネル。
 * 上部に操作ボタン、その下にキャラクターの一覧とプロファイルの編集、いちばん下に設定の詳細パネル（UToonRayTracerEditorSettings）を表示する。
 */
class SToonRayTracerPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SToonRayTracerPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SToonRayTracerPanel() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FReply OnResetAccumulationClicked();
	FReply OnImportFromConsoleClicked();
	FReply OnResetToDefaultsClicked();
	FReply OnEnvironmentPresetClicked(EToonEnvironmentPreset Preset);

	// キャラクターの一覧：レベルにいるスケルタルメッシュのアクターと、そのプロファイル
	void RebuildCharacterList();
	void RequestRebuildCharacterList() { bCharacterListDirty = true; }
	void OnLevelActorChanged(AActor* Actor);
	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	FReply OnRefreshCharactersClicked();
	FReply OnSelectActorClicked(TWeakObjectPtr<AActor> Actor);
	FReply OnCreateProfileClicked(TWeakObjectPtr<AActor> Actor);
	FReply OnEditProfileClicked(TWeakObjectPtr<AActor> Actor);
	// アクターに Toon Character コンポーネントがなければ追加して、プロファイルを選ぶ（Ctrl+Z で元に戻せる）
	void SetProfile(AActor* Actor, UToonCharacterProfile* Profile);
	// プロファイルをパネルの中で編集できるようにする
	void ShowProfile(UToonCharacterProfile* Profile);

	// 環境プリセットを適用した結果のメッセージ
	FText EnvironmentMessage;
	// キャラクターの一覧の操作の結果のメッセージ
	FText CharacterMessage;

	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<IDetailsView> ProfileDetailsView;
	TSharedPtr<SVerticalBox> CharacterListBox;
	TWeakObjectPtr<UToonCharacterProfile> ShownProfile;
	bool bCharacterListDirty = false;

	FDelegateHandle ActorAddedHandle;
	FDelegateHandle ActorDeletedHandle;
	FDelegateHandle MapOpenedHandle;
};
