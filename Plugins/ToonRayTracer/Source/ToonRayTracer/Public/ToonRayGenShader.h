// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "RayTracingPayloadType.h"
#include "ShaderParameterStruct.h"
#include "SceneUniformBuffer.h"

// 汎用化 G4：物体ごとの代表色のハッシュ表（ToonObjectColorTable.ush と一致させる）
namespace ToonObjectColorTable
{
	// 表の要素数（2 のべき乗）
	constexpr uint32 Size = 4096;
	// 1 要素あたりの uint の数（このフレームの合計：キー、R・G・B の合計、サンプル数 / 代表色：キー、R・G・B）
	constexpr uint32 SumStride = 5;
	constexpr uint32 TableStride = 4;

	inline void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("TOON_OBJECT_COLOR_TABLE_SIZE"), Size);
	}
}

class FToonRayGenShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonRayGenShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonRayGenShader, FGlobalShader);

	// 交差判定の対象（解析的な物体 / シーンの TLAS）と、表示モード（トゥーン / 確認用）でシェーダーを分ける
	// 1 つのシェーダーに全部入れると TraceRay を呼ぶ箇所が増えすぎ、ドライバーの組み立てに数分かかり、
	// 実行時にも GPU が応答しなくなる（TDR）ため、使う組み合わせのコードだけを含むようにする
	class FAnalyticSceneDim : SHADER_PERMUTATION_BOOL("TOON_ANALYTIC_SCENE");
	class FToonShadingDim : SHADER_PERMUTATION_BOOL("TOON_SHADING");
	using FPermutationDomain = TShaderPermutationDomain<FAnalyticSceneDim, FToonShadingDim>;

	// シェーダーに渡せる球の最大数
	static constexpr int32 MaxSpheres = 8;
	// シェーダーに渡せる四角形の最大数（『The Next Week』第6章）
	static constexpr int32 MaxQuads = 8;
	// シェーダーに渡せる箱の最大数（『The Next Week』第8〜9章）
	static constexpr int32 MaxBoxes = 4;

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
		// 動く物体への対応：GBuffer の速度、前のフレームの深度、シーン内で動きを検出したピクセル数（前のフレーム / このフレーム）
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferVelocityTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, AccumulationDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, AccumulationSignatureTexture)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, PreviousSceneMotion)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, SceneMotion)
		// 1 なら動く物体を検出して蓄積をやり直す
		SHADER_PARAMETER(uint32, bDetectMotion)
		// 動いている物体があるフレームでの蓄積数の上限
		SHADER_PARAMETER(uint32, MotionMaxAccumulatedFrames)
		// 1 なら蓄積をやり直した理由を色で表示する（確認用）
		SHADER_PARAMETER(uint32, DebugMotion)
		// シーンの TLAS（Translated World 空間で構築されている）
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		// 0: 解析的な球、1: シーンの TLAS に TraceRay
		SHADER_PARAMETER(uint32, TraceMode)
		// 1ピクセルあたりのサンプル数（アンチエイリアス）
		SHADER_PARAMETER(uint32, SamplesPerPixel)
		// 蓄積をやり直したピクセルと、物体が動いている間のサンプル数
		SHADER_PARAMETER(uint32, MotionSamplesPerPixel)
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
		// 『The Next Week』第4章：球のテクスチャ（x: 種類, y: 大きさ）と、チェッカーのもう一方の色（rgb）
		SHADER_PARAMETER_ARRAY(FVector4f, SphereTextureParams, [MaxSpheres])
		SHADER_PARAMETER_ARRAY(FVector4f, SphereAlbedo2, [MaxSpheres])
		// 『The Next Week』第2章：シャッターが開いている間に球が動く量（xyz、cm）
		SHADER_PARAMETER_ARRAY(FVector4f, SphereMotion, [MaxSpheres])
		// 『The Next Week』第6章：四角形（Translated World 空間の角の位置と 2 辺）とマテリアル
		SHADER_PARAMETER_ARRAY(FVector4f, QuadQ, [MaxQuads])
		SHADER_PARAMETER_ARRAY(FVector4f, QuadU, [MaxQuads])
		SHADER_PARAMETER_ARRAY(FVector4f, QuadV, [MaxQuads])
		SHADER_PARAMETER_ARRAY(FVector4f, QuadMaterialParams, [MaxQuads])
		SHADER_PARAMETER_ARRAY(FVector4f, QuadAlbedo, [MaxQuads])
		SHADER_PARAMETER(uint32, NumQuads)
		// 『The Next Week』第8〜9章：箱（xyz: Translated World 空間の中心, w: 上下軸まわりの回転（ラジアン））、半分の大きさ、マテリアル
		SHADER_PARAMETER_ARRAY(FVector4f, BoxCenter, [MaxBoxes])
		SHADER_PARAMETER_ARRAY(FVector4f, BoxHalfExtent, [MaxBoxes])
		SHADER_PARAMETER_ARRAY(FVector4f, BoxMaterialParams, [MaxBoxes])
		SHADER_PARAMETER_ARRAY(FVector4f, BoxAlbedo, [MaxBoxes])
		SHADER_PARAMETER(uint32, NumBoxes)
		// 『The Next Week』第10章：地面に敷き詰めた箱の格子（角の位置と 1 マスの大きさ、マスの数と高さの範囲、色）
		SHADER_PARAMETER(FVector4f, GroundGridOrigin)
		SHADER_PARAMETER(FVector4f, GroundGridParams)
		SHADER_PARAMETER(FVector4f, GroundGridAlbedo)
		SHADER_PARAMETER(uint32, bGroundGrid)
		// 『The Next Week』第3章：BVH のノード（float4 2 つで 1 ノード）と小さな球、塊の原点（Translated World 空間）と色
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, BvhNodes)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, BvhSpheres)
		SHADER_PARAMETER(FVector4f, BvhOrigin)
		SHADER_PARAMETER(FVector4f, BvhAlbedo)
		SHADER_PARAMETER(uint32, NumBvhNodes)
		// 『The Next Week』第7章：1 なら背景を黒にする
		SHADER_PARAMETER(uint32, bBlackBackground)
		SHADER_PARAMETER(uint32, NumSpheres)
		// カメラから最初に当たった面の法線を、通常描画の GBuffer から読むためのテクスチャ
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferATexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferBTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferCTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		// GBuffer 内の描画範囲（描画解像度。アップスケール後の ViewRect とは異なることがある）
		SHADER_PARAMETER(FIntPoint, GBufferViewRectMin)
		SHADER_PARAMETER(FIntPoint, GBufferViewRectSize)
		// 1 なら GBuffer の法線を使う
		SHADER_PARAMETER(uint32, bUseGBufferNormal)
		// 1 なら、マテリアル未指定のメッシュの色に GBuffer のベースカラーを使う
		SHADER_PARAMETER(uint32, bUseGBufferBaseColor)
		// 1 なら、反射・屈折した先の点もカメラから見えていれば GBuffer の値を使う
		SHADER_PARAMETER(uint32, bUseGBufferForReflections)
		// 反射・屈折した先の法線をなめらかにするための補助レイの間隔（cm）。0 なら面ごとの法線
		SHADER_PARAMETER(float, ReflectionNormalSmoothing)
		// Translated World 空間 → クリップ空間への変換行列（反射した先の点を画面に投影する）
		SHADER_PARAMETER(FMatrix44f, TranslatedWorldToClip)
		// 1 なら、マテリアル未指定のメッシュを GBuffer のメタリックとラフネスから金属と判定する
		SHADER_PARAMETER(uint32, bUseGBufferMetal)
		// 金属として鏡面反射させるラフネスの上限
		SHADER_PARAMETER(float, MetalRoughnessThreshold)
		// 1 なら、カメラのレイが GBuffer に描かれていない面（透過で抜けた部分など）を通り抜ける
		SHADER_PARAMETER(uint32, bSkipMaskedSurfaces)
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
		// Perlin ノイズの応用：影の境目を揺らす強さと、揺れの大きさ（cm）
		SHADER_PARAMETER(float, ToonShadowNoiseStrength)
		SHADER_PARAMETER(float, ToonShadowNoiseSize)
		// Translated World 空間の原点のワールド座標（メッシュのローカル座標を求めるため、double を High と Low に分けたもの）
		SHADER_PARAMETER(FVector3f, PreViewTranslationHigh)
		SHADER_PARAMETER(FVector3f, PreViewTranslationLow)
		// トゥーン T5：ハイライト（NdotH のしきい値、強さ）とリムライト（1 - NdotV のしきい値、強さ、光が当たる側だけか）
		SHADER_PARAMETER(float, ToonHighlightThreshold)
		SHADER_PARAMETER(float, ToonHighlightStrength)
		SHADER_PARAMETER(float, ToonRimThreshold)
		SHADER_PARAMETER(float, ToonRimStrength)
		SHADER_PARAMETER(uint32, bToonRimLitSideOnly)
		// トゥーン T6：アウトラインを引くか、線の太さ（ピクセル）、線を引く距離の差のしきい値（補助レイをずらした量の何倍か）、線の色
		SHADER_PARAMETER(uint32, bToonOutline)
		SHADER_PARAMETER(float, ToonOutlineWidth)
		SHADER_PARAMETER(float, ToonOutlineWidthSkinned)
		SHADER_PARAMETER(float, ToonOutlineThreshold)
		SHADER_PARAMETER(FVector3f, ToonOutlineColor)
		// トゥーン T7：金属・ガラスで反射・屈折を追いかける回数の上限
		SHADER_PARAMETER(uint32, ToonReflectionDepth)
		// GPUScene（各メッシュの Custom Primitive Data）を読むためのシーンのユニフォームバッファ
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
		// 汎用化 G4：物体ごとの代表色（前のフレームまで）と、このフレームに見えた色の合計
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, ObjectColorTable)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, ObjectColorFrameSums)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TOON_MAX_SPHERES"), MaxSpheres);
		OutEnvironment.SetDefine(TEXT("TOON_MAX_QUADS"), MaxQuads);
		OutEnvironment.SetDefine(TEXT("TOON_MAX_BOXES"), MaxBoxes);
		// GetPrimitiveData() を Primitive ユニフォームバッファではなく GPUScene のバッファから読むようにする
		// （未定義だとメッシュ描画用の Primitive ユニフォームバッファを参照してしまい、グローバルシェーダーではバインドできない）
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), 1);
		ToonObjectColorTable::ModifyCompilationEnvironment(OutEnvironment);
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

// 汎用化 G4：このフレームに見えた色の合計から物体ごとの平均色を求め、フレームをまたいで保持する表に書き込む
class FToonObjectColorResolveCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonObjectColorResolveCS);
	SHADER_USE_PARAMETER_STRUCT(FToonObjectColorResolveCS, FGlobalShader);

	static constexpr int32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, ObjectColorFrameSums)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, ObjectColorTable)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		ToonObjectColorTable::ModifyCompilationEnvironment(OutEnvironment);
	}
};
