// Fill out your copyright notice in the Description page of Project Settings.

#include "SToonRayTracerPanel.h"
#include "ToonRayTracerEditorSettings.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h"
#include "ToonCharacterComponent.h"
#include "ToonCharacterProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/DataAssetFactory.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "SToonRayTracerPanel"

namespace
{
	UToonCharacterProfile* GetActorProfile(const AActor* Actor)
	{
		const UToonCharacterComponent* Character = Actor ? Actor->FindComponentByClass<UToonCharacterComponent>() : nullptr;
		return Character ? Character->Profile.Get() : nullptr;
	}

	FName GetConsoleBoneName(const TCHAR* Name)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		return Variable ? FName(*Variable->GetString()) : FName();
	}

	// 一覧に出す状態：両目の間の距離（cm）か、顔のボーンが見つからないこと
	FText GetCharacterStatus(const USkeletalMeshComponent* Mesh, const UToonCharacterProfile* Profile)
	{
		const FName Head = Profile && !Profile->HeadBone.IsNone() ? Profile->HeadBone : GetConsoleBoneName(TEXT("r.ToonRayTracer.Toon.FaceHeadBone"));
		const FName LeftEye = Profile && !Profile->LeftEyeBone.IsNone() ? Profile->LeftEyeBone : GetConsoleBoneName(TEXT("r.ToonRayTracer.Toon.FaceLeftEyeBone"));
		const FName RightEye = Profile && !Profile->RightEyeBone.IsNone() ? Profile->RightEyeBone : GetConsoleBoneName(TEXT("r.ToonRayTracer.Toon.FaceRightEyeBone"));
		if (Mesh->GetBoneIndex(Head) == INDEX_NONE || Mesh->GetBoneIndex(LeftEye) == INDEX_NONE || Mesh->GetBoneIndex(RightEye) == INDEX_NONE)
		{
			return LOCTEXT("StatusNoFace", "顔のボーンなし");
		}
		const double EyeDistance = FVector::Distance(Mesh->GetBoneLocation(LeftEye), Mesh->GetBoneLocation(RightEye));
		FNumberFormattingOptions Format;
		Format.SetMaximumFractionalDigits(1);
		return FText::Format(LOCTEXT("StatusEyeDistance", "両目 {0} cm"), FText::AsNumber(EyeDistance, &Format));
	}
}

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

	// プロファイルを編集する詳細パネル（一覧の「編集」で選んだもの）
	ProfileDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

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

		// 鳴潮風ルックの確認用：時間帯ごとの環境（レベルの Directional Light と Skylight を切り替える）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 4.0f)
		[
			SNew(SWrapBox)
			.UseAllottedSize(true)

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 4.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EnvironmentLabel", "環境："))
				.ToolTipText(LOCTEXT("EnvironmentLabelTooltip", "レベルの Directional Light と Skylight の向き・色・強さを、時間帯に合わせて切り替えます（Ctrl+Z で元に戻せます）"))
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 4.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Morning", "朝"))
				.OnClicked(this, &SToonRayTracerPanel::OnEnvironmentPresetClicked, EToonEnvironmentPreset::Morning)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 4.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Noon", "昼"))
				.OnClicked(this, &SToonRayTracerPanel::OnEnvironmentPresetClicked, EToonEnvironmentPreset::Noon)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 4.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Evening", "夕方"))
				.OnClicked(this, &SToonRayTracerPanel::OnEnvironmentPresetClicked, EToonEnvironmentPreset::Evening)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Night", "夜"))
				.OnClicked(this, &SToonRayTracerPanel::OnEnvironmentPresetClicked, EToonEnvironmentPreset::Night)
			]

			+ SWrapBox::Slot()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return EnvironmentMessage; })
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]

		// キャラクター（上）と全体の設定（下）。境目はドラッグで動かせる
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)

			+ SSplitter::Slot()
			.Value(0.4f)
			[
				SNew(SVerticalBox)

				// 見出しと更新ボタン
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CharactersLabel", "キャラクター"))
						.ToolTipText(LOCTEXT("CharactersLabelTooltip", "レベルにいるスケルタルメッシュのアクターと、そのプロファイル（Toon Character Profile）。プロファイルがないキャラクターは全体の設定を使います"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RefreshCharacters", "一覧を更新"))
						.OnClicked(this, &SToonRayTracerPanel::OnRefreshCharactersClicked)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return CharacterMessage; })
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.AutoWrapText(true)
					]
				]

				// 一覧
				+ SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(220.0f)
				.Padding(8.0f, 0.0f, 8.0f, 4.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(CharacterListBox, SVerticalBox)
					]
				]

				// 選んだプロファイルの編集
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 4.0f, 8.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return ShownProfile.IsValid()
							? FText::Format(LOCTEXT("ProfileTitle", "プロファイル：{0}"), FText::FromString(ShownProfile->GetName()))
							: LOCTEXT("ProfileTitleNone", "プロファイル：一覧の「編集」で選ぶと、ここで編集できます");
					})
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					ProfileDetailsView.ToSharedRef()
				]
			]

			+ SSplitter::Slot()
			.Value(0.6f)
			[
				DetailsView.ToSharedRef()
			]
		]
	];

	RebuildCharacterList();

	// レベルのアクターが増えたり消えたり、レベルを開き直したりしたら、一覧を作り直す
	if (GEngine)
	{
		ActorAddedHandle = GEngine->OnLevelActorAdded().AddSP(this, &SToonRayTracerPanel::OnLevelActorChanged);
		ActorDeletedHandle = GEngine->OnLevelActorDeleted().AddSP(this, &SToonRayTracerPanel::OnLevelActorChanged);
	}
	MapOpenedHandle = FEditorDelegates::OnMapOpened.AddSP(this, &SToonRayTracerPanel::OnMapOpened);
}

SToonRayTracerPanel::~SToonRayTracerPanel()
{
	if (GEngine)
	{
		GEngine->OnLevelActorAdded().Remove(ActorAddedHandle);
		GEngine->OnLevelActorDeleted().Remove(ActorDeletedHandle);
	}
	FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);
}

void SToonRayTracerPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// アクターの追加や削除の通知の最中に作り直すと、消えかけのアクターが残るため、次の更新で作り直す
	if (bCharacterListDirty)
	{
		bCharacterListDirty = false;
		RebuildCharacterList();
	}
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

FReply SToonRayTracerPanel::OnEnvironmentPresetClicked(EToonEnvironmentPreset Preset)
{
	ToonRayTracerEnvironment::ApplyPreset(Preset, EnvironmentMessage);
	return FReply::Handled();
}

void SToonRayTracerPanel::OnLevelActorChanged(AActor* Actor)
{
	RequestRebuildCharacterList();
}

void SToonRayTracerPanel::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	ShownProfile.Reset();
	ProfileDetailsView->SetObject(nullptr);
	RequestRebuildCharacterList();
}

void SToonRayTracerPanel::RebuildCharacterList()
{
	CharacterListBox->ClearChildren();
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}

	int32 NumCharacters = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		const USkeletalMeshComponent* Mesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
		if (!Mesh || !Mesh->GetSkeletalMeshAsset())
		{
			continue;
		}
		++NumCharacters;
		const TWeakObjectPtr<AActor> WeakActor(Actor);

		CharacterListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)

			// アクターの名前（押すとレベルで選ぶ）
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("SelectActorTooltip", "レベルでこのアクターを選びます"))
				.OnClicked(this, &SToonRayTracerPanel::OnSelectActorClicked, WeakActor)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Actor->GetActorLabel()))
				]
			]

			// プロファイルの選択
			+ SHorizontalBox::Slot()
			.FillWidth(0.45f)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UToonCharacterProfile::StaticClass())
				.DisplayThumbnail(false)
				.AllowClear(true)
				.ObjectPath_Lambda([WeakActor]()
				{
					const UToonCharacterProfile* CurrentProfile = GetActorProfile(WeakActor.Get());
					return CurrentProfile ? CurrentProfile->GetPathName() : FString();
				})
				.OnObjectChanged_Lambda([this, WeakActor](const FAssetData& AssetData)
				{
					if (AActor* Target = WeakActor.Get())
					{
						SetProfile(Target, Cast<UToonCharacterProfile>(AssetData.GetAsset()));
					}
				})
			]

			// プロファイルを作る（候補を自動で入れて、このアクターに選ぶ）
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateProfile", "作成"))
				.ToolTipText(LOCTEXT("CreateProfileTooltip", "モデルと同じフォルダにプロファイルを作り、ボーン・パーツ・大きさ・見た目の候補を自動で入れて、このアクターに選びます"))
				.IsEnabled_Lambda([WeakActor]() { return WeakActor.IsValid() && GetActorProfile(WeakActor.Get()) == nullptr; })
				.OnClicked(this, &SToonRayTracerPanel::OnCreateProfileClicked, WeakActor)
			]

			// プロファイルをパネルの中で編集する
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("EditProfile", "編集"))
				.ToolTipText(LOCTEXT("EditProfileTooltip", "このアクターのプロファイルを、下の欄で編集します（値を変えるとすぐに描画に反映されます）"))
				.IsEnabled_Lambda([WeakActor]() { return GetActorProfile(WeakActor.Get()) != nullptr; })
				.OnClicked(this, &SToonRayTracerPanel::OnEditProfileClicked, WeakActor)
			]

			// 状態（両目の間の距離。プロファイルのボーンを変えたときにも追従する）
			+ SHorizontalBox::Slot()
			.FillWidth(0.25f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([WeakActor]()
				{
					const AActor* Target = WeakActor.Get();
					const USkeletalMeshComponent* TargetMesh = Target ? Target->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
					return TargetMesh ? GetCharacterStatus(TargetMesh, GetActorProfile(Target)) : FText::GetEmpty();
				})
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	}

	if (NumCharacters == 0)
	{
		CharacterListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoCharacters", "レベルにスケルタルメッシュのアクターがありません"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	}
}

FReply SToonRayTracerPanel::OnRefreshCharactersClicked()
{
	CharacterMessage = FText::GetEmpty();
	RebuildCharacterList();
	return FReply::Handled();
}

FReply SToonRayTracerPanel::OnSelectActorClicked(TWeakObjectPtr<AActor> Actor)
{
	if (GEditor && Actor.IsValid())
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(Actor.Get(), true, true);
	}
	return FReply::Handled();
}

void SToonRayTracerPanel::SetProfile(AActor* Actor, UToonCharacterProfile* Profile)
{
	UToonCharacterComponent* Character = Actor->FindComponentByClass<UToonCharacterComponent>();
	if (!Character && !Profile)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetProfileTransaction", "キャラクターのプロファイルを選ぶ"));
	if (!Character)
	{
		Actor->Modify();
		Character = NewObject<UToonCharacterComponent>(Actor, NAME_None, RF_Transactional);
		Actor->AddInstanceComponent(Character);
		Character->RegisterComponent();
	}

	Character->Modify();
	Character->Profile = Profile;
	FPropertyChangedEvent ChangedEvent(FindFProperty<FProperty>(UToonCharacterComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UToonCharacterComponent, Profile)));
	static_cast<UObject*>(Character)->PostEditChangeProperty(ChangedEvent);
	CharacterMessage = Profile
		? FText::Format(LOCTEXT("ProfileSet", "{0} に {1} を選びました"), FText::FromString(Actor->GetActorLabel()), FText::FromString(Profile->GetName()))
		: FText::Format(LOCTEXT("ProfileCleared", "{0} のプロファイルを外しました（全体の設定を使います）"), FText::FromString(Actor->GetActorLabel()));
}

void SToonRayTracerPanel::ShowProfile(UToonCharacterProfile* Profile)
{
	ShownProfile = Profile;
	ProfileDetailsView->SetObject(Profile);
}

FReply SToonRayTracerPanel::OnCreateProfileClicked(TWeakObjectPtr<AActor> Actor)
{
	AActor* Target = Actor.Get();
	const USkeletalMeshComponent* Mesh = Target ? Target->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
	USkeletalMesh* SkeletalMesh = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	if (!SkeletalMesh)
	{
		return FReply::Handled();
	}

	// モデルと同じフォルダに TCP_<モデルの名前> を作る（エンジンやプラグインのフォルダのモデルなら /Game に作る）
	FString PackagePath = FPackageName::GetLongPackagePath(SkeletalMesh->GetOutermost()->GetName());
	if (!PackagePath.StartsWith(TEXT("/Game")))
	{
		PackagePath = TEXT("/Game");
	}
	FString BaseName = SkeletalMesh->GetName();
	BaseName.RemoveFromStart(TEXT("SK_"));
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / (TEXT("TCP_") + BaseName), FString(), UniquePackageName, UniqueAssetName);

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UToonCharacterProfile::StaticClass();
	UToonCharacterProfile* Profile = Cast<UToonCharacterProfile>(AssetTools.CreateAsset(UniqueAssetName, PackagePath, UToonCharacterProfile::StaticClass(), Factory));
	if (!Profile)
	{
		CharacterMessage = LOCTEXT("CreateProfileFailed", "プロファイルを作れませんでした");
		return FReply::Handled();
	}

	Profile->SkeletalMesh = SkeletalMesh;
	Profile->FillCandidates();
	Profile->MarkPackageDirty();
	SetProfile(Target, Profile);
	ShowProfile(Profile);
	CharacterMessage = FText::Format(LOCTEXT("ProfileCreated", "{0} を作りました（アセットとレベルを保存してください）"), FText::FromString(UniqueAssetName));
	return FReply::Handled();
}

FReply SToonRayTracerPanel::OnEditProfileClicked(TWeakObjectPtr<AActor> Actor)
{
	if (UToonCharacterProfile* Profile = GetActorProfile(Actor.Get()))
	{
		ShowProfile(Profile);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
