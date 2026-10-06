// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "RayTracingPayloadType.h"
#include "ShaderParameterStruct.h"
#include "SceneUniformBuffer.h"

class FToonRayGenShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonRayGenShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonRayGenShader, FGlobalShader);

	// シェーダーに渡せる球の最大数
	static constexpr int32 MaxSpheres = 8;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		// フレームをまたいだ平均値（線形色、ViewRect 内のローカル座標）
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, AccumulationTexture)
		// AccumulationTexture に蓄積済みのフレーム数（0 ならリセット直後）
		SHADER_PARAMETER(uint32, AccumulatedFrames)
		// 蓄積するフレーム数の上限
		SHADER_PARAMETER(uint32, MaxAccumulatedFrames)
		// 乱数の種に混ぜる値（蓄積中は毎フレーム変わる）
		SHADER_PARAMETER(uint32, RandomSeed)
		// シーンの TLAS（Translated World 空間で構築されている）
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		// 0: 解析的な球、1: シーンの TLAS に TraceRay
		SHADER_PARAMETER(uint32, TraceMode)
		// 1ピクセルあたりのサンプル数（アンチエイリアス）
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		// 0: 法線の可視化、1: 拡散反射
		SHADER_PARAMETER(uint32, ShadingMode)
		// レイが反射する最大回数
		SHADER_PARAMETER(uint32, MaxDepth)
		// クリップ空間 → Translated World 空間（カメラ位置が原点）への変換行列
		SHADER_PARAMETER(FMatrix44f, ClipToTranslatedWorld)
		// 出力テクスチャ内の描画範囲（ViewRect）
		SHADER_PARAMETER(FIntPoint, ViewRectMin)
		SHADER_PARAMETER(FIntPoint, ViewRectSize)
		// 球のリスト（xyz: Translated World 空間の中心、w: 半径）
		SHADER_PARAMETER_ARRAY(FVector4f, Spheres, [MaxSpheres])
		// 球のマテリアル（x: 種類, y: 金属のぼけ具合, z: 屈折率, w: トゥーンのハイライトとリムライトの強さ）と反射率（rgb）
		SHADER_PARAMETER_ARRAY(FVector4f, SphereMaterialParams, [MaxSpheres])
		SHADER_PARAMETER_ARRAY(FVector4f, SphereAlbedo, [MaxSpheres])
		SHADER_PARAMETER(uint32, NumSpheres)
		// カメラから最初に当たった面の法線を、通常描画の GBuffer から読むためのテクスチャ
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferATexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferCTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		// GBuffer 内の描画範囲（描画解像度。アップスケール後の ViewRect とは異なることがある）
		SHADER_PARAMETER(FIntPoint, GBufferViewRectMin)
		SHADER_PARAMETER(FIntPoint, GBufferViewRectSize)
		// 1 なら GBuffer の法線を使う
		SHADER_PARAMETER(uint32, bUseGBufferNormal)
		// 1 なら、マテリアル未指定のメッシュの色に GBuffer のベースカラーを使う
		SHADER_PARAMETER(uint32, bUseGBufferBaseColor)
		// Directional Light（光源へ向かう方向、色）。bHasDirectionalLight が 0 ならライトなし
		SHADER_PARAMETER(FVector3f, ToLightDirection)
		SHADER_PARAMETER(FVector3f, LightColor)
		SHADER_PARAMETER(uint32, bHasDirectionalLight)
		// トゥーン：段階数（2 or 3）、NdotL のしきい値、境目のぼかし幅、影色、明るい面のライト色
		SHADER_PARAMETER(uint32, ToonBands)
		SHADER_PARAMETER(float, ToonShadowThreshold)
		SHADER_PARAMETER(float, ToonLitThreshold)
		SHADER_PARAMETER(float, ToonEdgeSoftness)
		SHADER_PARAMETER(FVector3f, ToonShadowColor)
		SHADER_PARAMETER(FVector3f, ToonLightColor)
		// トゥーン T3：シャドウレイによる影を付けるか、シャドウレイの始点を面から浮かせる距離（cm）
		SHADER_PARAMETER(uint32, bToonCastShadows)
		SHADER_PARAMETER(float, ToonShadowBias)
		// トゥーン T5：ハイライト（NdotH のしきい値、強さ）とリムライト（1 - NdotV のしきい値、強さ、光が当たる側だけか）
		SHADER_PARAMETER(float, ToonHighlightThreshold)
		SHADER_PARAMETER(float, ToonHighlightStrength)
		SHADER_PARAMETER(float, ToonRimThreshold)
		SHADER_PARAMETER(float, ToonRimStrength)
		SHADER_PARAMETER(uint32, bToonRimLitSideOnly)
		// トゥーン T6：アウトラインを引くか、線の太さ（ピクセル）、線を引く距離の差のしきい値（補助レイをずらした量の何倍か）、線の色
		SHADER_PARAMETER(uint32, bToonOutline)
		SHADER_PARAMETER(float, ToonOutlineWidth)
		SHADER_PARAMETER(float, ToonOutlineThreshold)
		SHADER_PARAMETER(FVector3f, ToonOutlineColor)
		// トゥーン T7：金属・ガラスで反射・屈折を追いかける回数の上限
		SHADER_PARAMETER(uint32, ToonReflectionDepth)
		// GPUScene（各メッシュの Custom Primitive Data）を読むためのシーンのユニフォームバッファ
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TOON_MAX_SPHERES"), MaxSpheres);
		// GetPrimitiveData() を Primitive ユニフォームバッファではなく GPUScene のバッファから読むようにする
		// （未定義だとメッシュ描画用の Primitive ユニフォームバッファを参照してしまい、グローバルシェーダーではバインドできない）
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), 1);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};

// パイプライン作成に必須のヒットシェーダー（フェーズ1では未使用）
class FToonClosestHitShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonClosestHitShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonClosestHitShader, FGlobalShader);

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};

// SBT作成に必須のMissシェーダー
class FToonMissShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonMissShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonMissShader, FGlobalShader);

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};
