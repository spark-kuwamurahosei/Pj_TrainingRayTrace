#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/CoreDelegates.h"
#include "SceneView.h"

// UE5.5 ～ 5.8 の API の違いを吸収する
// ・5.8：TLAS を RDG のバッファとして受け取る（GetRayTracingSceneViewRDG）。ビューの行列の名前が GetWorldToView / GetViewToClip に変わった
// ・5.7：TLAS は RHI のビュー（ビューごと）。シーンを参照で渡す
// ・5.5 ～ 5.6：TLAS は RHI のビュー（シーンごと）。シーンをポインターで渡す

// 5.8 以降は TLAS を RDG のバッファとして扱う（シェーダーのパラメータの型も変わる）
#define TOON_RAY_TRACER_RDG_TLAS (!UE_VERSION_OLDER_THAN(5, 8, 0))
// 5.7 以降は、FXRenderingUtils の関数にシーンを参照で渡し、TLAS はビューごとになる
#define TOON_RAY_TRACER_SCENE_REFERENCE_API (!UE_VERSION_OLDER_THAN(5, 7, 0))
// 5.6 以降は、シェーダーバインディングテーブルに寿命（Lifetime）を指定する
#define TOON_RAY_TRACER_SBT_LIFETIME (!UE_VERSION_OLDER_THAN(5, 6, 0))
// 5.6 以降は、ポストプロセスの DOF の前（TSR より前）のタイミングにパスを追加できる（EPostProcessingPass::BeforeDOF）
// 5.5 にはないため、ポストプロセスの直前（PrePostProcessPass_RenderThread）で描いてシーンの色に書き戻す（ToonRayTracerViewExtensionPrePost.cpp）
#define TOON_RAY_TRACER_BEFORE_DOF_PASS (!UE_VERSION_OLDER_THAN(5, 6, 0))
// 5.7 以降は、UMaterialInterface::GetUsedTextures の品質とプラットフォームの引数が省略できる
#define TOON_RAY_TRACER_USED_TEXTURES_OPTIONAL (!UE_VERSION_OLDER_THAN(5, 7, 0))

namespace ToonRayTracerCompat
{
	// ワールド → ビュー
	inline const FMatrix& GetWorldToView(const FViewMatrices& Matrices)
	{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		return Matrices.GetViewMatrix();
#else
		return Matrices.GetWorldToView();
#endif
	}

	// ビュー → クリップ（TAA のジッターを含む）
	inline const FMatrix& GetViewToClip(const FViewMatrices& Matrices)
	{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		return Matrices.GetProjectionMatrix();
#else
		return Matrices.GetViewToClip();
#endif
	}

	// ビュー → クリップ（TAA のジッターを含まない）
	inline const FMatrix& GetViewToClipNoAA(const FViewMatrices& Matrices)
	{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		return Matrices.GetProjectionNoAAMatrix();
#else
		return Matrices.GetViewToClipNoAA();
#endif
	}

	// エンジンの初期化が終わったときのイベント
	inline FSimpleMulticastDelegate& OnPostEngineInit()
	{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		return FCoreDelegates::OnPostEngineInit;
#else
		return FCoreDelegates::GetOnPostEngineInit();
#endif
	}
}
