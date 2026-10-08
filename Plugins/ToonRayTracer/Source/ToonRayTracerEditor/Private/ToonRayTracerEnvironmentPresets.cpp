// Fill out your copyright notice in the Description page of Project Settings.

#include "ToonRayTracerEnvironmentPresets.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ToonRayTracerEnvironment"

namespace
{
	// 時間帯ごとの設定
	struct FToonEnvironmentSettings
	{
		FRotator SunRotation;		// Directional Light の向き（Pitch が負なら上から照らす。太陽の高さ = -Pitch）
		FLinearColor SunColor;		// Directional Light の色（sRGB）
		float SunIntensity;			// Directional Light の強さ（lux）
		bool bAtmosphereSun;		// Sky Atmosphere の太陽として使うか（夜は月の光なので空を明るくしない）
		FLinearColor SkyColor;		// Skylight の色
		float SkyIntensity;			// Skylight の強さ
	};

	FToonEnvironmentSettings GetSettings(EToonEnvironmentPreset Preset)
	{
		switch (Preset)
		{
		case EToonEnvironmentPreset::Morning:
			// 低い位置からのやわらかい暖色の光と、澄んだ青空
			return { FRotator(-15.0f, -60.0f, 0.0f), FLinearColor(1.0f, 0.87f, 0.72f), 6.0f, true, FLinearColor(0.9f, 0.95f, 1.0f), 1.0f };
		case EToonEnvironmentPreset::Noon:
			// 高い位置からの白い強い光
			return { FRotator(-60.0f, -30.0f, 0.0f), FLinearColor(1.0f, 0.98f, 0.95f), 10.0f, true, FLinearColor::White, 1.0f };
		case EToonEnvironmentPreset::Evening:
			// 地平線近くからの橙色の光と、夕焼けの空（Sky Atmosphere が太陽の高さから空の色を作る）
			return { FRotator(-6.0f, 120.0f, 0.0f), FLinearColor(1.0f, 0.55f, 0.3f), 4.0f, true, FLinearColor(1.0f, 0.8f, 0.65f), 1.0f };
		case EToonEnvironmentPreset::Night:
		default:
			// 青白い月の光。空の太陽として使わないので空は暗くなり、Skylight も青みのある弱い光にする
			return { FRotator(-40.0f, 30.0f, 0.0f), FLinearColor(0.75f, 0.82f, 1.0f), 0.5f, false, FLinearColor(0.5f, 0.6f, 1.0f), 0.5f };
		}
	}

	FText GetPresetName(EToonEnvironmentPreset Preset)
	{
		switch (Preset)
		{
		case EToonEnvironmentPreset::Morning: return LOCTEXT("Morning", "朝");
		case EToonEnvironmentPreset::Noon: return LOCTEXT("Noon", "昼");
		case EToonEnvironmentPreset::Evening: return LOCTEXT("Evening", "夕方");
		case EToonEnvironmentPreset::Night:
		default: return LOCTEXT("Night", "夜");
		}
	}

	// ToonRayTracer が使うのと同じ選び方で Directional Light を探す（空の太陽として使っているものを優先し、次に強いもの）
	ADirectionalLight* FindSunLight(UWorld* World)
	{
		ADirectionalLight* Best = nullptr;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			const UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			if (!Light)
			{
				continue;
			}
			const UDirectionalLightComponent* BestLight = Best ? Cast<UDirectionalLightComponent>(Best->GetLightComponent()) : nullptr;
			const bool bIsSun = Light->IsUsedAsAtmosphereSunLight() && Light->GetAtmosphereSunLightIndex() == 0;
			const bool bBestIsSun = BestLight && BestLight->IsUsedAsAtmosphereSunLight() && BestLight->GetAtmosphereSunLightIndex() == 0;
			if (!BestLight || (bIsSun && !bBestIsSun) || (bIsSun == bBestIsSun && Light->Intensity > BestLight->Intensity))
			{
				Best = *It;
			}
		}
		return Best;
	}
}

namespace ToonRayTracerEnvironment
{
	bool ApplyPreset(EToonEnvironmentPreset Preset, FText& OutMessage)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			OutMessage = LOCTEXT("NoWorld", "エディタのレベルが見つかりません");
			return false;
		}

		ADirectionalLight* Sun = FindSunLight(World);
		ASkyLight* Sky = nullptr;
		for (TActorIterator<ASkyLight> It(World); It; ++It)
		{
			Sky = *It;
			break;
		}
		if (!Sun)
		{
			OutMessage = LOCTEXT("NoSun", "レベルに Directional Light がありません");
			return false;
		}

		const FToonEnvironmentSettings Settings = GetSettings(Preset);
		const FText PresetName = GetPresetName(Preset);
		const FScopedTransaction Transaction(FText::Format(LOCTEXT("ApplyPresetTransaction", "環境を{0}にする"), PresetName));

		// Directional Light：向き・色・強さ・空の太陽として使うか
		UDirectionalLightComponent* SunComponent = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		Sun->Modify();
		SunComponent->Modify();
		Sun->SetActorRotation(Settings.SunRotation);
		SunComponent->bUseTemperature = false;
		SunComponent->SetLightColor(Settings.SunColor);
		SunComponent->SetIntensity(Settings.SunIntensity);
		SunComponent->SetAtmosphereSunLight(Settings.bAtmosphereSun);

		// Skylight：色と強さ（リアルタイムで空を取り込む設定でなければ、取り込み直す）
		if (Sky)
		{
			USkyLightComponent* SkyComponent = Sky->GetLightComponent();
			Sky->Modify();
			SkyComponent->Modify();
			SkyComponent->SetLightColor(Settings.SkyColor);
			SkyComponent->SetIntensity(Settings.SkyIntensity);
			if (!SkyComponent->bRealTimeCapture)
			{
				SkyComponent->RecaptureSky();
			}
		}

		OutMessage = Sky
			? FText::Format(LOCTEXT("Applied", "環境を{0}にしました（{1} と {2}）"), PresetName, FText::FromString(Sun->GetActorLabel()), FText::FromString(Sky->GetActorLabel()))
			: FText::Format(LOCTEXT("AppliedNoSky", "環境を{0}にしました（{1}。Skylight がないため空の光は変えていません）"), PresetName, FText::FromString(Sun->GetActorLabel()));
		return true;
	}
}

#undef LOCTEXT_NAMESPACE
