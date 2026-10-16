// UE5.5 で TSR の前に描く（r.ToonRayTracer.Toon.BeforeTSR）
// 5.6 以降は、ポストプロセスの DOF の前（EPostProcessingPass::BeforeDOF）にパスを追加する
// 5.5 にはその口がないため、ポストプロセスの直前に呼ばれる PrePostProcessPass_RenderThread で、シーンの色を読んで描き、シーンの色に書き戻す
// （DOF・TSR・トーンマップはこの後にかかるので、5.6 以降と同じ順番になる）

#include "ToonRayTracerViewExtension.h"

#if !TOON_RAY_TRACER_BEFORE_DOF_PASS

#include "PostProcess/PostProcessInputs.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphUtils.h"
// ビューの描画範囲（FViewInfo::ViewRect。画面の解像度の割合を反映した描画解像度の範囲）を読むため、Renderer の非公開のヘッダーを使う
#include "SceneRendering.h"

void ToonRayTracerViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	if (!ShouldDrawBeforeTSR() || !Inputs.SceneTextures)
	{
		return;
	}

	FRDGTextureRef SceneColorTexture = Inputs.SceneTextures->GetParameters()->SceneColorTexture;
	if (!SceneColorTexture)
	{
		return;
	}

	const FIntRect ViewRect = static_cast<const FViewInfo&>(View).ViewRect;

	// ポストプロセスのパスと同じ形の入力にして、共通の描画処理に渡す
	FPostProcessMaterialInputs PassInputs;
	PassInputs.SetInput(GraphBuilder, EPostProcessMaterialInput::SceneColor, FScreenPassTexture(SceneColorTexture, ViewRect));
	PassInputs.SceneTextures.SceneTextures = Inputs.SceneTextures;

	const FScreenPassTexture Result = RenderToonRayTracing(GraphBuilder, View, PassInputs, true);

	// 描かなかった場合（TLAS がないなど）は、元のシーンの色がそのまま返る
	if (Result.Texture && Result.Texture != SceneColorTexture)
	{
		AddCopyTexturePass(GraphBuilder, Result.Texture, SceneColorTexture, ViewRect.Min, ViewRect.Min, ViewRect.Size());
	}
}

#endif
