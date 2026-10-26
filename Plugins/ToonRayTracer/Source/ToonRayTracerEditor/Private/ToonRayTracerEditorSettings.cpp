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

	// マテリアルスロット番号の一覧と、シェーダーで使うビットの集まり（0 ～ 30 番まで）を相互に変換する
	int32 SlotsToMask(const TArray<int32>& Slots)
	{
		int32 SlotMask = 0;
		for (const int32 Slot : Slots)
		{
			if (Slot >= 0 && Slot <= 30)
			{
				SlotMask |= 1 << Slot;
			}
		}
		return SlotMask;
	}

	TArray<int32> MaskToSlots(int32 SlotMask)
	{
		TArray<int32> Slots;
		for (int32 Slot = 0; Slot <= 30; ++Slot)
		{
			if ((SlotMask & (1 << Slot)) != 0)
			{
				Slots.Add(Slot);
			}
		}
		return Slots;
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

	void SetString(const TCHAR* Name, const FString& Value)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			Variable->Set(*Value, ECVF_SetByConsole);
		}
	}

	void GetString(const TCHAR* Name, FString& OutValue)
	{
		if (IConsoleVariable* Variable = FindToonConsoleVariable(Name))
		{
			OutValue = Variable->GetString();
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
	bStratifiedSampling = true;
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
	ScatterSampling = EToonRayTracerScatterSampling::Mixture;

	Bands = 2;
	bCharacterOnly = false;
	bBeforeTSR = true;
	bCharacterSelfShadow = false;
	bSkin = true;
	SkinShadowThreshold = -0.3f;
	SkinShadowColor = FLinearColor(0.62f, 0.42f, 0.45f);
	CharacterRimStrength = 1.5f;
	CharacterRimWidth = 2.5f;
	CharacterRimShadow = 1;
	Caustics = 1;
	CausticStrength = 0.6f;
	CausticThreshold = 1.5f;
	ReflectionOutline = 1;
	ReflectionCharacters = 1;
	ShadowSimplify = 1;
	ShadowSimplifyRadius = 8.0f;
	Crease = 0;
	CreaseDistance = 60.0f;
	CreaseLift = 0.25f;
	CreaseSamples = 6;
	CreaseStrength = 1.0f;
	BeforeTSRMaxBrightness = 0.9f;
	DeepShadow = 1;
	DeepShadowDistance = 40.0f;
	DeepShadowSamples = 6;
	DeepShadowThreshold = 0.5f;
	DeepShadowSoftness = 0.1f;
	DeepShadowBrightness = 0.7f;
	DeepShadowLit = 1.0f;
	CharacterOutlineColorScale = 0.25f;
	SeeThroughOpacity = 0.25f;
	SeeThroughBrightness = 0.5f;
	SeeThroughColorBlend = 0.0f;
	SeeThroughColor = FLinearColor(0.3f, 0.08f, 0.09f);
	SeeThroughDistance = 20.0f;
	// 確認に使っているキャラクター（Kanau）では、眉毛・目・まつ毛がエレメント 4 にある
	SeeThroughSlots = { 4 };
	// 眉毛と同じく、Kanau では目もエレメント 4 にある
	EyeSlots = { 4 };
	// Kanau では、目のスロットの三角形が「眉毛 → まぶた（まつ毛）→ 目」の順に並んでいて、1850 番から目になる
	EyeTriangleStart = 1850;
	EyeTriangleEnd = 0;
	bEyeUnlit = true;
	EyeShadowDistance = 2.0f;
	EyeShadowBrightness = 0.7f;
	EyeHighlightStrength = 1.0f;
	EyeHighlightThreshold = 0.99f;
	EyeHighlightSearchRadius = 10.0f;
	bFaceShadow = true;
	bFaceShadowSDF = false;
	FaceShadowSDFTexture.Reset();
	FaceShadowSDFScale = 1.6f;
	FaceShadowSDFOffsetY = -0.4f;
	// Kanau では、顔（fa）がエレメント 3。ボーンの名前も Kanau のもの
	FaceSlots = { 3 };
	FaceWidthScale = 1.0f;
	ReferenceEyeDistance = 39.6f;
	FaceShadowCurve = 0.15f;
	FaceShadowSoftness = 0.03f;
	FaceShadowFrontBias = 0.4f;
	HairShadowMode = 1;
	HairShadowWidth = 4.0f;
	HairShadowLightShift = 0.5f;
	// Kanau では、髪（h1）がエレメント 6
	HairSlots = { 6 };
	HairShadowDistance = 30.0f;
	HairShadowStrength = 1.0f;
	HairHighlightTextureStrength = 1.0f;
	HairHighlightTextureColor = FLinearColor(0.46f, 0.4f, 0.25f);
	HairHighlightFalloff = 0.5f;
	HairHighlightMode = 0;
	HairHighlightStrength = 0.9f;
	HairHighlightWidth = 0.1f;
	HairHighlightShift = 0.0f;
	HairHighlightJagged = 1.5f;
	HairHighlightJaggedCount = 40.0f;
	HairHighlightJaggedSharpness = 4.0f;
	HairHighlightSoftness = 0.05f;
	HairHighlightTaper = 0.0f;
	HairHighlightArea = 3.0f;
	FaceHeadBone = TEXT("jnt_head");
	FaceLeftEyeBone = TEXT("jnt_eyeL");
	FaceRightEyeBone = TEXT("jnt_eyeR");
	ShadowThreshold = 0.0f;
	LitThreshold = 0.5f;
	EdgeSoftness = 0.02f;
	ShadowColor = FLinearColor(0.35f, 0.4f, 0.6f);
	LightScale = 1.0f;
	SkyTint = 0.3f;
	ShadowSaturation = 0.6f;
	EmissiveStrength = 1.0f;
	bOriginal = true;
	OriginalHighlightStrength = 0.5f;
	OriginalSparkleStrength = 1.0f;
	OriginalShadowBias = 0.6f;
	OriginalReflectionRimStrength = 0.5f;
	OriginalDiffusionStrength = 0.8f;
	OriginalDiffusionRadius = 24.0f;
	OriginalSoftFocusStrength = 0.2f;
	OriginalSoftFocusRadius = 6.0f;
	OriginalBloomStrength = 2.0f;
	OriginalBloomRadius = 16.0f;
	OriginalGradationStrength = 1.0f;
	OriginalGradationTopColor = FLinearColor(1.06f, 1.03f, 0.96f);
	OriginalGradationBottomColor = FLinearColor(0.9f, 0.92f, 1.0f);

	bCastShadows = true;
	ShadowBias = 2.0f;
	SoftShadowAngle = 5.0f;
	SoftShadowSamples = 4;
	SoftShadowSteps = 1;
	GlassShadowOpacity = 0.5f;
	IndirectStrength = 0.5f;
	IndirectSamples = 2;
	ShadowNoiseStrength = 0.0f;
	ShadowNoiseSize = 40.0f;

	HighlightThreshold = 0.97f;
	HighlightStrength = 1.0f;
	RimThreshold = 0.7f;
	RimStrength = 0.3f;
	bRimLitSideOnly = true;

	bOutline = true;
	OutlineWidth = 1.5f;
	OutlineWidthSkinned = 0.5f;
	OutlineThreshold = 1.0f;
	FaceOutlineTolerance = 100.0f;
	OutlineColor = FLinearColor(0.02f, 0.02f, 0.04f);

	ReflectionDepth = 4;
	GlassEdgeStrength = 0.6f;
	GlassEdgeWidth = 0.2f;
	GlassEdgeColor = FLinearColor(0.55f, 0.8f, 1.0f);
}

void UToonRayTracerEditorSettings::ApplyToConsoleVariables() const
{
	SetInt(TEXT("r.ToonRayTracer.ShadingMode"), static_cast<int32>(ShadingMode));
	SetInt(TEXT("r.ToonRayTracer.TraceMode"), static_cast<int32>(TraceMode));
	SetInt(TEXT("r.ToonRayTracer.AnalyticScene"), static_cast<int32>(AnalyticScene));
	SetInt(TEXT("r.ToonRayTracer.SamplesPerPixel"), SamplesPerPixel);
	SetBool(TEXT("r.ToonRayTracer.StratifiedSampling"), bStratifiedSampling);
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
	SetInt(TEXT("r.ToonRayTracer.ScatterSampling"), static_cast<int32>(ScatterSampling));

	SetBool(TEXT("r.ToonRayTracer.Toon.CharacterOnly"), bCharacterOnly);
	SetBool(TEXT("r.ToonRayTracer.Toon.BeforeTSR"), bBeforeTSR);
	SetBool(TEXT("r.ToonRayTracer.Toon.CharacterSelfShadow"), bCharacterSelfShadow);
	SetBool(TEXT("r.ToonRayTracer.Toon.Skin"), bSkin);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SkinShadowThreshold"), SkinShadowThreshold);
	SetColor(TEXT("r.ToonRayTracer.Toon.SkinShadowColor"), SkinShadowColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"), CharacterRimStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"), CharacterRimWidth);
	SetInt(TEXT("r.ToonRayTracer.Toon.CharacterRimShadow"), CharacterRimShadow);
	SetInt(TEXT("r.ToonRayTracer.Toon.Caustics"), Caustics);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CausticStrength"), CausticStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CausticThreshold"), CausticThreshold);
	SetInt(TEXT("r.ToonRayTracer.Toon.ReflectionOutline"), ReflectionOutline);
	SetInt(TEXT("r.ToonRayTracer.Toon.ReflectionCharacters"), ReflectionCharacters);
	SetInt(TEXT("r.ToonRayTracer.Toon.ShadowSimplify"), ShadowSimplify);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowSimplifyRadius"), ShadowSimplifyRadius);
	SetInt(TEXT("r.ToonRayTracer.Toon.Crease"), Crease);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CreaseDistance"), CreaseDistance);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CreaseLift"), CreaseLift);
	SetInt(TEXT("r.ToonRayTracer.Toon.CreaseSamples"), CreaseSamples);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CreaseStrength"), CreaseStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.BeforeTSRMaxBrightness"), BeforeTSRMaxBrightness);
	SetInt(TEXT("r.ToonRayTracer.Toon.DeepShadow"), DeepShadow);
	SetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowDistance"), DeepShadowDistance);
	SetInt(TEXT("r.ToonRayTracer.Toon.DeepShadowSamples"), DeepShadowSamples);
	SetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowThreshold"), DeepShadowThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowSoftness"), DeepShadowSoftness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowBrightness"), DeepShadowBrightness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowLit"), DeepShadowLit);
	SetFloat(TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"), CharacterOutlineColorScale);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughOpacity"), SeeThroughOpacity);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughBrightness"), SeeThroughBrightness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughColorBlend"), SeeThroughColorBlend);
	SetColor(TEXT("r.ToonRayTracer.Toon.SeeThroughColor"), SeeThroughColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughDistance"), SeeThroughDistance);
	{
		// スロット番号の一覧を、シェーダーで使うビットの集まりにする（0 ～ 30 番まで）
		int32 SlotMask = 0;
		for (const int32 Slot : SeeThroughSlots)
		{
			if (Slot >= 0 && Slot <= 30)
			{
				SlotMask |= 1 << Slot;
			}
		}
		SetInt(TEXT("r.ToonRayTracer.Toon.SeeThroughSlotMask"), SlotMask);
	}
	SetInt(TEXT("r.ToonRayTracer.Toon.EyeSlotMask"), SlotsToMask(EyeSlots));
	SetInt(TEXT("r.ToonRayTracer.Toon.EyeUnlit"), bEyeUnlit ? 1 : 0);
	SetInt(TEXT("r.ToonRayTracer.Toon.EyeTriangleStart"), EyeTriangleStart);
	SetInt(TEXT("r.ToonRayTracer.Toon.EyeTriangleEnd"), EyeTriangleEnd);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EyeShadowDistance"), EyeShadowDistance);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EyeShadowBrightness"), EyeShadowBrightness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightStrength"), EyeHighlightStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightThreshold"), EyeHighlightThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightSearchRadius"), EyeHighlightSearchRadius);
	SetBool(TEXT("r.ToonRayTracer.Toon.FaceShadow"), bFaceShadow);
	SetBool(TEXT("r.ToonRayTracer.Toon.FaceShadowSDF"), bFaceShadowSDF);
	SetString(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFTexture"), FaceShadowSDFTexture.ToString());
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFScale"), FaceShadowSDFScale);
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFOffsetY"), FaceShadowSDFOffsetY);
	SetInt(TEXT("r.ToonRayTracer.Toon.FaceSlotMask"), SlotsToMask(FaceSlots));
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceWidthScale"), FaceWidthScale);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ReferenceEyeDistance"), ReferenceEyeDistance);
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowCurve"), FaceShadowCurve);
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSoftness"), FaceShadowSoftness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowFrontBias"), FaceShadowFrontBias);
	SetInt(TEXT("r.ToonRayTracer.Toon.HairShadow"), HairShadowMode);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowWidth"), HairShadowWidth);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowLightShift"), HairShadowLightShift);
	SetInt(TEXT("r.ToonRayTracer.Toon.HairSlotMask"), SlotsToMask(HairSlots));
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowDistance"), HairShadowDistance);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowStrength"), HairShadowStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightStrength"), HairHighlightStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightWidth"), HairHighlightWidth);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightShift"), HairHighlightShift);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJagged"), HairHighlightJagged);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedCount"), HairHighlightJaggedCount);
	SetInt(TEXT("r.ToonRayTracer.Toon.HairHighlightMode"), HairHighlightMode);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightTextureStrength"), HairHighlightTextureStrength);
	SetColor(TEXT("r.ToonRayTracer.Toon.HairHighlightTextureColor"), HairHighlightTextureColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightFalloff"), HairHighlightFalloff);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedSharpness"), HairHighlightJaggedSharpness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightSoftness"), HairHighlightSoftness);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightTaper"), HairHighlightTaper);
	SetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightArea"), HairHighlightArea);
	SetString(TEXT("r.ToonRayTracer.Toon.FaceHeadBone"), FaceHeadBone);
	SetString(TEXT("r.ToonRayTracer.Toon.FaceLeftEyeBone"), FaceLeftEyeBone);
	SetString(TEXT("r.ToonRayTracer.Toon.FaceRightEyeBone"), FaceRightEyeBone);
	SetInt(TEXT("r.ToonRayTracer.Toon.Bands"), Bands);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowThreshold"), ShadowThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.LitThreshold"), LitThreshold);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EdgeSoftness"), EdgeSoftness);
	SetColor(TEXT("r.ToonRayTracer.Toon.ShadowColor"), ShadowColor);
	SetFloat(TEXT("r.ToonRayTracer.Toon.LightScale"), LightScale);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SkyTint"), SkyTint);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowSaturation"), ShadowSaturation);
	SetFloat(TEXT("r.ToonRayTracer.Toon.EmissiveStrength"), EmissiveStrength);
	SetBool(TEXT("r.ToonRayTracer.Toon.Original"), bOriginal);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalHighlightStrength"), OriginalHighlightStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSparkleStrength"), OriginalSparkleStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalShadowBias"), OriginalShadowBias);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalReflectionRimStrength"), OriginalReflectionRimStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalDiffusionStrength"), OriginalDiffusionStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalDiffusionRadius"), OriginalDiffusionRadius);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusStrength"), OriginalSoftFocusStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusRadius"), OriginalSoftFocusRadius);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalBloomStrength"), OriginalBloomStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalBloomRadius"), OriginalBloomRadius);
	SetFloat(TEXT("r.ToonRayTracer.Toon.OriginalGradationStrength"), OriginalGradationStrength);
	SetColor(TEXT("r.ToonRayTracer.Toon.OriginalGradationTopColor"), OriginalGradationTopColor);
	SetColor(TEXT("r.ToonRayTracer.Toon.OriginalGradationBottomColor"), OriginalGradationBottomColor);

	SetBool(TEXT("r.ToonRayTracer.Toon.CastShadows"), bCastShadows);
	SetFloat(TEXT("r.ToonRayTracer.Toon.ShadowBias"), ShadowBias);
	SetFloat(TEXT("r.ToonRayTracer.Toon.SoftShadowAngle"), SoftShadowAngle);
	SetInt(TEXT("r.ToonRayTracer.Toon.SoftShadowSamples"), SoftShadowSamples);
	SetInt(TEXT("r.ToonRayTracer.Toon.SoftShadowSteps"), SoftShadowSteps);
	SetFloat(TEXT("r.ToonRayTracer.Toon.GlassShadowOpacity"), GlassShadowOpacity);
	SetFloat(TEXT("r.ToonRayTracer.Toon.IndirectStrength"), IndirectStrength);
	SetInt(TEXT("r.ToonRayTracer.Toon.IndirectSamples"), IndirectSamples);
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
	SetFloat(TEXT("r.ToonRayTracer.Toon.FaceOutlineTolerance"), FaceOutlineTolerance);
	SetColor(TEXT("r.ToonRayTracer.Toon.OutlineColor"), OutlineColor);

	SetInt(TEXT("r.ToonRayTracer.Toon.ReflectionDepth"), ReflectionDepth);
	SetFloat(TEXT("r.ToonRayTracer.Toon.GlassEdgeStrength"), GlassEdgeStrength);
	SetFloat(TEXT("r.ToonRayTracer.Toon.GlassEdgeWidth"), GlassEdgeWidth);
	SetColor(TEXT("r.ToonRayTracer.Toon.GlassEdgeColor"), GlassEdgeColor);
}

void UToonRayTracerEditorSettings::ImportFromConsoleVariables()
{
	GetEnum(TEXT("r.ToonRayTracer.ShadingMode"), ShadingMode);
	GetEnum(TEXT("r.ToonRayTracer.TraceMode"), TraceMode);
	GetEnum(TEXT("r.ToonRayTracer.AnalyticScene"), AnalyticScene);
	GetInt(TEXT("r.ToonRayTracer.SamplesPerPixel"), SamplesPerPixel);
	GetBool(TEXT("r.ToonRayTracer.StratifiedSampling"), bStratifiedSampling);
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
	GetEnum(TEXT("r.ToonRayTracer.ScatterSampling"), ScatterSampling);

	GetBool(TEXT("r.ToonRayTracer.Toon.CharacterOnly"), bCharacterOnly);
	GetBool(TEXT("r.ToonRayTracer.Toon.BeforeTSR"), bBeforeTSR);
	GetBool(TEXT("r.ToonRayTracer.Toon.CharacterSelfShadow"), bCharacterSelfShadow);
	GetBool(TEXT("r.ToonRayTracer.Toon.Skin"), bSkin);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SkinShadowThreshold"), SkinShadowThreshold);
	GetColor(TEXT("r.ToonRayTracer.Toon.SkinShadowColor"), SkinShadowColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"), CharacterRimStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"), CharacterRimWidth);
	GetInt(TEXT("r.ToonRayTracer.Toon.CharacterRimShadow"), CharacterRimShadow);
	GetInt(TEXT("r.ToonRayTracer.Toon.Caustics"), Caustics);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CausticStrength"), CausticStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CausticThreshold"), CausticThreshold);
	GetInt(TEXT("r.ToonRayTracer.Toon.ReflectionOutline"), ReflectionOutline);
	GetInt(TEXT("r.ToonRayTracer.Toon.ReflectionCharacters"), ReflectionCharacters);
	GetInt(TEXT("r.ToonRayTracer.Toon.ShadowSimplify"), ShadowSimplify);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowSimplifyRadius"), ShadowSimplifyRadius);
	GetInt(TEXT("r.ToonRayTracer.Toon.Crease"), Crease);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CreaseDistance"), CreaseDistance);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CreaseLift"), CreaseLift);
	GetInt(TEXT("r.ToonRayTracer.Toon.CreaseSamples"), CreaseSamples);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CreaseStrength"), CreaseStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.BeforeTSRMaxBrightness"), BeforeTSRMaxBrightness);
	GetInt(TEXT("r.ToonRayTracer.Toon.DeepShadow"), DeepShadow);
	GetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowDistance"), DeepShadowDistance);
	GetInt(TEXT("r.ToonRayTracer.Toon.DeepShadowSamples"), DeepShadowSamples);
	GetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowThreshold"), DeepShadowThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowSoftness"), DeepShadowSoftness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowBrightness"), DeepShadowBrightness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.DeepShadowLit"), DeepShadowLit);
	GetFloat(TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"), CharacterOutlineColorScale);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughOpacity"), SeeThroughOpacity);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughBrightness"), SeeThroughBrightness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughColorBlend"), SeeThroughColorBlend);
	GetColor(TEXT("r.ToonRayTracer.Toon.SeeThroughColor"), SeeThroughColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughDistance"), SeeThroughDistance);
	{
		int32 SlotMask = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.SeeThroughSlotMask"), SlotMask);
		SeeThroughSlots.Reset();
		for (int32 Slot = 0; Slot <= 30; ++Slot)
		{
			if ((SlotMask & (1 << Slot)) != 0)
			{
				SeeThroughSlots.Add(Slot);
			}
		}
	}
	{
		int32 SlotMask = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.EyeSlotMask"), SlotMask);
		EyeSlots = MaskToSlots(SlotMask);
	}
	{
		int32 Value = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.EyeUnlit"), Value);
		bEyeUnlit = Value != 0;
	}
	GetInt(TEXT("r.ToonRayTracer.Toon.EyeTriangleStart"), EyeTriangleStart);
	GetInt(TEXT("r.ToonRayTracer.Toon.EyeTriangleEnd"), EyeTriangleEnd);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EyeShadowDistance"), EyeShadowDistance);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EyeShadowBrightness"), EyeShadowBrightness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightStrength"), EyeHighlightStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightThreshold"), EyeHighlightThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightSearchRadius"), EyeHighlightSearchRadius);
	{
		int32 Value = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.FaceShadow"), Value);
		bFaceShadow = Value != 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.FaceShadowSDF"), Value);
		bFaceShadowSDF = Value != 0;
		FString Path;
		GetString(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFTexture"), Path);
		FaceShadowSDFTexture.SetPath(Path);
		GetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFScale"), FaceShadowSDFScale);
		GetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSDFOffsetY"), FaceShadowSDFOffsetY);
		int32 SlotMask = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.FaceSlotMask"), SlotMask);
		FaceSlots = MaskToSlots(SlotMask);
	}
	GetFloat(TEXT("r.ToonRayTracer.Toon.FaceWidthScale"), FaceWidthScale);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ReferenceEyeDistance"), ReferenceEyeDistance);
	GetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowCurve"), FaceShadowCurve);
	GetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSoftness"), FaceShadowSoftness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowFrontBias"), FaceShadowFrontBias);
	{
		GetInt(TEXT("r.ToonRayTracer.Toon.HairShadow"), HairShadowMode);
		int32 SlotMask = 0;
		GetInt(TEXT("r.ToonRayTracer.Toon.HairSlotMask"), SlotMask);
		HairSlots = MaskToSlots(SlotMask);
	}
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowDistance"), HairShadowDistance);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowStrength"), HairShadowStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightStrength"), HairHighlightStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightWidth"), HairHighlightWidth);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightShift"), HairHighlightShift);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJagged"), HairHighlightJagged);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedCount"), HairHighlightJaggedCount);
	GetInt(TEXT("r.ToonRayTracer.Toon.HairHighlightMode"), HairHighlightMode);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightTextureStrength"), HairHighlightTextureStrength);
	GetColor(TEXT("r.ToonRayTracer.Toon.HairHighlightTextureColor"), HairHighlightTextureColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightFalloff"), HairHighlightFalloff);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedSharpness"), HairHighlightJaggedSharpness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightSoftness"), HairHighlightSoftness);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightTaper"), HairHighlightTaper);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightArea"), HairHighlightArea);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowWidth"), HairShadowWidth);
	GetFloat(TEXT("r.ToonRayTracer.Toon.HairShadowLightShift"), HairShadowLightShift);
	GetString(TEXT("r.ToonRayTracer.Toon.FaceHeadBone"), FaceHeadBone);
	GetString(TEXT("r.ToonRayTracer.Toon.FaceLeftEyeBone"), FaceLeftEyeBone);
	GetString(TEXT("r.ToonRayTracer.Toon.FaceRightEyeBone"), FaceRightEyeBone);
	GetInt(TEXT("r.ToonRayTracer.Toon.Bands"), Bands);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowThreshold"), ShadowThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.LitThreshold"), LitThreshold);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EdgeSoftness"), EdgeSoftness);
	GetColor(TEXT("r.ToonRayTracer.Toon.ShadowColor"), ShadowColor);
	GetFloat(TEXT("r.ToonRayTracer.Toon.LightScale"), LightScale);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SkyTint"), SkyTint);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowSaturation"), ShadowSaturation);
	GetFloat(TEXT("r.ToonRayTracer.Toon.EmissiveStrength"), EmissiveStrength);
	GetBool(TEXT("r.ToonRayTracer.Toon.Original"), bOriginal);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalHighlightStrength"), OriginalHighlightStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSparkleStrength"), OriginalSparkleStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalShadowBias"), OriginalShadowBias);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalReflectionRimStrength"), OriginalReflectionRimStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalDiffusionStrength"), OriginalDiffusionStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalDiffusionRadius"), OriginalDiffusionRadius);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusStrength"), OriginalSoftFocusStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusRadius"), OriginalSoftFocusRadius);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalBloomStrength"), OriginalBloomStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalBloomRadius"), OriginalBloomRadius);
	GetFloat(TEXT("r.ToonRayTracer.Toon.OriginalGradationStrength"), OriginalGradationStrength);
	GetColor(TEXT("r.ToonRayTracer.Toon.OriginalGradationTopColor"), OriginalGradationTopColor);
	GetColor(TEXT("r.ToonRayTracer.Toon.OriginalGradationBottomColor"), OriginalGradationBottomColor);

	GetBool(TEXT("r.ToonRayTracer.Toon.CastShadows"), bCastShadows);
	GetFloat(TEXT("r.ToonRayTracer.Toon.ShadowBias"), ShadowBias);
	GetFloat(TEXT("r.ToonRayTracer.Toon.SoftShadowAngle"), SoftShadowAngle);
	GetInt(TEXT("r.ToonRayTracer.Toon.SoftShadowSamples"), SoftShadowSamples);
	GetInt(TEXT("r.ToonRayTracer.Toon.SoftShadowSteps"), SoftShadowSteps);
	GetFloat(TEXT("r.ToonRayTracer.Toon.GlassShadowOpacity"), GlassShadowOpacity);
	GetFloat(TEXT("r.ToonRayTracer.Toon.IndirectStrength"), IndirectStrength);
	GetInt(TEXT("r.ToonRayTracer.Toon.IndirectSamples"), IndirectSamples);
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
	GetFloat(TEXT("r.ToonRayTracer.Toon.FaceOutlineTolerance"), FaceOutlineTolerance);
	GetColor(TEXT("r.ToonRayTracer.Toon.OutlineColor"), OutlineColor);

	GetInt(TEXT("r.ToonRayTracer.Toon.ReflectionDepth"), ReflectionDepth);
	GetFloat(TEXT("r.ToonRayTracer.Toon.GlassEdgeStrength"), GlassEdgeStrength);
	GetFloat(TEXT("r.ToonRayTracer.Toon.GlassEdgeWidth"), GlassEdgeWidth);
	GetColor(TEXT("r.ToonRayTracer.Toon.GlassEdgeColor"), GlassEdgeColor);
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
