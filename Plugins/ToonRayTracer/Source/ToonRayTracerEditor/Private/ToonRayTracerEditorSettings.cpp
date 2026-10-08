// Fill out your copyright notice in the Description page of Project Settings.

#include "ToonRayTracerEditorSettings.h"
#include "HAL/IConsoleManager.h"

namespace
{
	IConsoleVariable* FindToonConsoleVariable(const TCHAR* Name)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		ensureMsgf(Variable, TEXT("Console variable %s was not found. Is the ToonRayTracer runtime module loaded?"), Name);
		return Variable;
	}

	void SetInt(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByConsole);
		}
	}

	void SetFloat(const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByConsole);
		}
	}

	void SetBool(const TCHAR* Name, bool bValue)
	{
		SetInt(Name, bValue ? 1 : 0);
	}

	// 色のコンソール変数は "R,G,B" 形式の文字列
	void SetColor(const TCHAR* Name, const FLinearColor& Color)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			Variable->Set(*FString::Printf(TEXT("%g,%g,%g"), Color.R, Color.G, Color.B), ECVF_SetByConsole);
		}
	}

	void GetInt(const TCHAR* Name, int32& OutValue)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			OutValue = Variable->GetInt();
		}
	}

	void GetFloat(const TCHAR* Name, float& OutValue)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			OutValue = Variable->GetFloat();
		}
	}

	void GetBool(const TCHAR* Name, bool& bOutValue)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			bOutValue = Variable->GetInt() != 0;
		}
	}

	void GetColor(const TCHAR* Name, FLinearColor& OutColor)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			TArray<FString> Parts;
			Variable->GetString().ParseIntoArray(Parts, TEXT(","), true);
			if (Parts.Num() == 3)
			{
				OutColor = FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
			}
		}
	}

	template <typename EnumType>
	void GetEnum(const TCHAR* Name, EnumType& OutValue)
	{
		int32 Value = static_cast<int32>(OutValue);
		GetInt(Name, Value);
		OutValue = static_cast<EnumType>(Value);
	}
}

UToonRayTracerEditorSettings::UToonRayTracerEditorSettings()
{
	ResetToDefaults();
}

// 初期値はランタイムモジュールのコンソール変数の既定値と一致させる
void UToonRayTracerEditorSettings::ResetToDefaults()
{
	ShadingMode = EToonRayTracerShadingMode::Toon;
	TraceMode = EToonRayTracerTraceMode::Scene;
	AnalyticScene = EToonRayTracerAnalyticScene::OneWeekend;
	SamplesPerPixel = 4;
	MotionSamplesPerPixel = 2;
	bUseGBufferNormal = true;
	bUseGBufferBaseColor = true;
	bUseGBufferForReflections = true;
	ReflectionNormalSmoothing = 15.0f;
	bUseGBufferMetal = true;
	bSkipMaskedSurfaces = true;
	MetalRoughnessThreshold = 0.4f;
	bAccumulate = true;
	MaxAccumulatedFrames = 64;
	bDetectMotion = true;
	MotionMaxAccumulatedFrames = 8;
	MaxDepth = 10;

	Bands = 2;
	bCharacterOnly = false;
	bBeforeTSR = true;
	bCharacterSelfShadow = false;
	bSkin = true;
	SkinShadowThreshold = -0.3f;
	SkinShadowColor = FLinearColor(0.75f, 0.5f, 0.5f);
	CharacterRimStrength = 1.5f;
	CharacterRimWidth = 2.5f;
	CharacterOutlineColorScale = 0.25f;
	ShadowThreshold = 0.0f;
	LitThreshold = 0.5f;
	EdgeSoftness = 0.02f;
	ShadowColor = FLinearColor(0.35f, 0.4f, 0.6f);
	LightScale = 1.0f;
	SkyTint = 0.3f;

	bCastShadows = true;
	ShadowBias = 2.0f;
	ShadowNoiseStrength = 0.0f;
	ShadowNoiseSize = 40.0f;

	HighlightThreshold = 0.97f;
	HighlightStrength = 1.0f;
	RimThreshold = 0.7f;
	RimStrength = 0.3f;
	bRimLitSideOnly = true;

	bOutline = true;
	OutlineWidth = 1.5f;
	OutlineWidthSkinned = 1.0f;
	OutlineThreshold = 1.0f;
	OutlineColor = FLinearColor(0.02f, 0.02f, 0.04f);

	ReflectionDepth = 4;
}

void UToonRayTracerEditorSettings::ApplyToConsoleVariables() const
{
	SetInt(TEXT("r.ToonRayTracer.ShadingMode"), static_cast<int32>(ShadingMode));
	SetInt(TEXT("r.ToonRayTracer.TraceMode"), static_cast<int32>(TraceMode));
	SetInt(TEXT("r.ToonRayTracer.AnalyticScene"), static_cast<int32>(AnalyticScene));
	SetInt(TEXT("r.ToonRayTracer.SamplesPerPixel"), SamplesPerPixel);
	SetInt(TEXT("r.ToonRayTracer.MotionSamplesPerPixel"), MotionSamplesPerPixel);
	SetBool(TEXT("r.ToonRayTracer.UseGBufferNormal"), bUseGBufferNormal);
	SetBool(TEXT("r.ToonRayTracer.UseGBufferBaseColor"), bUseGBufferBaseColor);
	SetBool(TEXT("r.ToonRayTracer.UseGBufferForReflections"), bUseGBufferForReflections);
	SetFloat(TEXT("r.ToonRayTracer.ReflectionNormalSmoothing"), ReflectionNormalSmoothing);
	SetBool(TEXT("r.ToonRayTracer.UseGBufferMetal"), bUseGBufferMetal);
	SetBool(TEXT("r.ToonRayTracer.SkipMaskedSurfaces"), bSkipMaskedSurfaces);
	SetFloat(TEXT("r.ToonRayTracer.MetalRoughnessThreshold"), MetalRoughnessThreshold);
	SetBool(TEXT("r.ToonRayTracer.Accumulate"), bAccumulate);
	SetInt(TEXT("r.ToonRayTracer.MaxAccumulatedFrames"), MaxAccumulatedFrames);
	SetBool(TEXT("r.ToonRayTracer.DetectMotion"), bDetectMotion);
	SetInt(TEXT("r.ToonRayTracer.MotionMaxAccumulatedFrames"), MotionMaxAccumulatedFrames);
	SetInt(TEXT("r.ToonRayTracer.MaxDepth"), MaxDepth);

	SetBool(TEXT("r.ToonRayTracer.Toon.CharacterOnly"), bCharacterOnly);
	SetBool(TEXT("r.ToonRayTracer.Toon.BeforeTSR"), bBeforeTSR);
	SetBool(TEXT("r.ToonRayTracer.Toon.CharacterSelfShadow"), bCharacterSelfShadow);
	SetBool(TEXT("r.ToonRayTracer.Toon.Skin"), bSkin);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SkinShadowThreshold"), SkinShadowThreshold);
	SetColor(TEXT("r.ToonRayTracer.Toon.SkinShadowColor"), SkinShadowColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"), CharacterRimStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"), CharacterRimWidth);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"), CharacterOutlineColorScale);
	SetInt(TEXT("r.ToonRayTracer.Toon.Bands"), Bands);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowThreshold"), ShadowThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.LitThreshold"), LitThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EdgeSoftness"), EdgeSoftness);
	SetColor(TEXT("r.ToonRayTracer.Toon.ShadowColor"), ShadowColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.LightScale"), LightScale);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SkyTint"), SkyTint);

	SetBool(TEXT("r.ToonRayTracer.Toon.CastShadows"), bCastShadows);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowBias"), ShadowBias);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowNoiseStrength"), ShadowNoiseStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowNoiseSize"), ShadowNoiseSize);

	SetFloat(TEXT("r.ToonRayTracer.Toon.HighlightThreshold"), HighlightThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HighlightStrength"), HighlightStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.RimThreshold"), RimThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.RimStrength"), RimStrength);
	SetBool(TEXT("r.ToonRayTracer.Toon.RimLitSideOnly"), bRimLitSideOnly);

	SetBool(TEXT("r.ToonRayTracer.Toon.Outline"), bOutline);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OutlineWidth"), OutlineWidth);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OutlineWidthSkinned"), OutlineWidthSkinned);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OutlineThreshold"), OutlineThreshold);
	SetColor(TEXT("r.ToonRayTracer.Toon.OutlineColor"), OutlineColor);

	SetInt(TEXT("r.ToonRayTracer.Toon.ReflectionDepth"), ReflectionDepth);
}

void UToonRayTracerEditorSettings::ImportFromConsoleVariables()
{
	GetEnum(TEXT("r.ToonRayTracer.ShadingMode"), ShadingMode);
	GetEnum(TEXT("r.ToonRayTracer.TraceMode"), TraceMode);
	GetEnum(TEXT("r.ToonRayTracer.AnalyticScene"), AnalyticScene);
	GetInt(TEXT("r.ToonRayTracer.SamplesPerPixel"), SamplesPerPixel);
	GetInt(TEXT("r.ToonRayTracer.MotionSamplesPerPixel"), MotionSamplesPerPixel);
	GetBool(TEXT("r.ToonRayTracer.UseGBufferNormal"), bUseGBufferNormal);
	GetBool(TEXT("r.ToonRayTracer.UseGBufferBaseColor"), bUseGBufferBaseColor);
	GetBool(TEXT("r.ToonRayTracer.UseGBufferForReflections"), bUseGBufferForReflections);
	GetFloat(TEXT("r.ToonRayTracer.ReflectionNormalSmoothing"), ReflectionNormalSmoothing);
	GetBool(TEXT("r.ToonRayTracer.UseGBufferMetal"), bUseGBufferMetal);
	GetBool(TEXT("r.ToonRayTracer.SkipMaskedSurfaces"), bSkipMaskedSurfaces);
	GetFloat(TEXT("r.ToonRayTracer.MetalRoughnessThreshold"), MetalRoughnessThreshold);
	GetBool(TEXT("r.ToonRayTracer.Accumulate"), bAccumulate);
	GetInt(TEXT("r.ToonRayTracer.MaxAccumulatedFrames"), MaxAccumulatedFrames);
	GetBool(TEXT("r.ToonRayTracer.DetectMotion"), bDetectMotion);
	GetInt(TEXT("r.ToonRayTracer.MotionMaxAccumulatedFrames"), MotionMaxAccumulatedFrames);
	GetInt(TEXT("r.ToonRayTracer.MaxDepth"), MaxDepth);

	GetBool(TEXT("r.ToonRayTracer.Toon.CharacterOnly"), bCharacterOnly);
	GetBool(TEXT("r.ToonRayTracer.Toon.BeforeTSR"), bBeforeTSR);
	GetBool(TEXT("r.ToonRayTracer.Toon.CharacterSelfShadow"), bCharacterSelfShadow);
	GetBool(TEXT("r.ToonRayTracer.Toon.Skin"), bSkin);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SkinShadowThreshold"), SkinShadowThreshold);
	GetColor(TEXT("r.ToonRayTracer.Toon.SkinShadowColor"), SkinShadowColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"), CharacterRimStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"), CharacterRimWidth);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"), CharacterOutlineColorScale);
	GetInt(TEXT("r.ToonRayTracer.Toon.Bands"), Bands);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowThreshold"), ShadowThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.LitThreshold"), LitThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EdgeSoftness"), EdgeSoftness);
	GetColor(TEXT("r.ToonRayTracer.Toon.ShadowColor"), ShadowColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.LightScale"), LightScale);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SkyTint"), SkyTint);

	GetBool(TEXT("r.ToonRayTracer.Toon.CastShadows"), bCastShadows);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowBias"), ShadowBias);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowNoiseStrength"), ShadowNoiseStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowNoiseSize"), ShadowNoiseSize);

	GetFloat(TEXT("r.ToonRayTracer.Toon.HighlightThreshold"), HighlightThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HighlightStrength"), HighlightStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.RimThreshold"), RimThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.RimStrength"), RimStrength);
	GetBool(TEXT("r.ToonRayTracer.Toon.RimLitSideOnly"), bRimLitSideOnly);

	GetBool(TEXT("r.ToonRayTracer.Toon.Outline"), bOutline);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OutlineWidth"), OutlineWidth);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OutlineWidthSkinned"), OutlineWidthSkinned);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OutlineThreshold"), OutlineThreshold);
	GetColor(TEXT("r.ToonRayTracer.Toon.OutlineColor"), OutlineColor);

	GetInt(TEXT("r.ToonRayTracer.Toon.ReflectionDepth"), ReflectionDepth);
}

void UToonRayTracerEditorSettings::ResetAccumulation()
{
	// ランタイム側はこの値が変わったことを検知して蓄積をリセットする
	if (IConsoleVariable* Variable = FindToonConsoleVariable(TEXT("r.ToonRayTracer.ResetAccumulation")))
	{
		Variable->Set(Variable->GetInt() + 1, ECVF_SetByConsole);
	}
}

#if WITH_EDITOR
void UToonRayTracerEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// スライダーを動かしている間もすぐ反映し、操作を終えたときだけ保存する
	ApplyToConsoleVariables();
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		SaveConfig();
	}
}
#endif
