#include "ToonPostEffectsPass.h"
#include "ToonRayGenShader.h"
#include "ToonRayTracerCVars.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "SceneView.h"

using namespace ToonRayTracerCVars;

void AddToonPostEffectsPasses(FRDGBuilder& GraphBuilder, const FSceneView& View, const FToonPostEffectsInputs& Inputs)
{
	// Original モード：仕上げに、キャラクターをぼかして重ね（Diffusion Filter・Soft Focus・Bloom）、画面全体へ上から下へのグラデーションを掛ける
	// 背景も含めた画面に掛ける。蓄積の結果には含めない
	const float GradationStrength = FMath::Max(CVarToonRayTracerToonOriginalGradationStrength.GetValueOnRenderThread(), 0.0f);
	const float DiffusionStrength = FMath::Clamp(CVarToonRayTracerToonOriginalDiffusionStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float SoftFocusStrength = FMath::Clamp(CVarToonRayTracerToonOriginalSoftFocusStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float BloomStrength = FMath::Max(CVarToonRayTracerToonOriginalBloomStrength.GetValueOnRenderThread(), 0.0f);
	const bool bGlow = DiffusionStrength > 0.0f || SoftFocusStrength > 0.0f || BloomStrength > 0.0f;
	if (CVarToonRayTracerToonOriginal.GetValueOnRenderThread() == 0 || (GradationStrength <= 0.0f && !bGlow))
	{
		return;
	}

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	const FIntPoint Resolution = Inputs.Output.ViewRect.Size();
	RDG_EVENT_SCOPE(GraphBuilder, "ToonRayTracing_PostEffects");
	const FIntPoint HalfSize((Resolution.X + 1) / 2, (Resolution.Y + 1) / 2);
	const FRDGTextureDesc HalfDesc = FRDGTextureDesc::Create2D(
		HalfSize, PF_FloatRGBA, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
	FRDGTextureRef DiffusionTexture = GraphBuilder.CreateTexture(HalfDesc, TEXT("ToonRayTracerDiffusion"));
	FRDGTextureRef SoftFocusTexture = GraphBuilder.CreateTexture(HalfDesc, TEXT("ToonRayTracerSoftFocus"));
	FRDGTextureRef BloomTexture = GraphBuilder.CreateTexture(HalfDesc, TEXT("ToonRayTracerBloom"));

	if (bGlow)
	{
		// 1. 半分の解像度で、ぼかす元の画像を作る
		// Diffusion Filter で掛ける光の色は、明るさをそろえる（一番強い成分を 1 にする）
		const FVector3f LightColor = Inputs.LightColor;
		const float LightMax = FMath::Max3(LightColor.X, LightColor.Y, LightColor.Z);
		FToonPostEffectsPrepareCS::FParameters* PrepareParameters = GraphBuilder.AllocParameters<FToonPostEffectsPrepareCS::FParameters>();
		PrepareParameters->ViewRectSize = Resolution;
		PrepareParameters->HalfSize = HalfSize;
		PrepareParameters->AccumulationTexture = Inputs.AccumulationTexture;
		PrepareParameters->ToonGlowTexture = Inputs.GlowTexture;
		PrepareParameters->DiffusionLightColor = LightMax > 1.0e-4f ? LightColor / LightMax : FVector3f(1.0f);
		PrepareParameters->DiffusionSourceTexture = GraphBuilder.CreateUAV(DiffusionTexture);
		PrepareParameters->SoftFocusSourceTexture = GraphBuilder.CreateUAV(SoftFocusTexture);
		PrepareParameters->BloomSourceTexture = GraphBuilder.CreateUAV(BloomTexture);
		TShaderMapRef<FToonPostEffectsPrepareCS> PrepareShader(ShaderMap);
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("Prepare %dx%d", HalfSize.X, HalfSize.Y),
			PrepareShader,
			PrepareParameters,
			FComputeShaderUtils::GetGroupCount(HalfSize, FToonPostEffectsShaderBase::ThreadGroupSize));

		// 2. 横、縦の順にぼかす（半径は元の解像度のピクセルで指定し、半分の解像度に直す）
		TShaderMapRef<FToonPostEffectsBlurCS> BlurShader(ShaderMap);
		FRDGTextureRef BlurTemp = GraphBuilder.CreateTexture(HalfDesc, TEXT("ToonRayTracerBlurTemp"));
		auto AddBlur = [&](FRDGTextureRef Texture, float RadiusPixels)
		{
			const float Radius = FMath::Max(RadiusPixels, 0.0f) * 0.5f;
			for (int32 Axis = 0; Axis < 2; ++Axis)
			{
				FToonPostEffectsBlurCS::FParameters* BlurParameters = GraphBuilder.AllocParameters<FToonPostEffectsBlurCS::FParameters>();
				BlurParameters->HalfSize = HalfSize;
				BlurParameters->BlurInputTexture = Axis == 0 ? Texture : BlurTemp;
				BlurParameters->BlurOutputTexture = GraphBuilder.CreateUAV(Axis == 0 ? BlurTemp : Texture);
				BlurParameters->BlurDirection = Axis == 0 ? FIntPoint(1, 0) : FIntPoint(0, 1);
				BlurParameters->BlurRadius = Radius;
				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("Blur %s", Axis == 0 ? TEXT("X") : TEXT("Y")),
					BlurShader,
					BlurParameters,
					FComputeShaderUtils::GetGroupCount(HalfSize, FToonPostEffectsShaderBase::ThreadGroupSize));
			}
		};
		AddBlur(DiffusionTexture, CVarToonRayTracerToonOriginalDiffusionRadius.GetValueOnRenderThread());
		AddBlur(SoftFocusTexture, CVarToonRayTracerToonOriginalSoftFocusRadius.GetValueOnRenderThread());
		AddBlur(BloomTexture, CVarToonRayTracerToonOriginalBloomRadius.GetValueOnRenderThread());
	}
	else
	{
		// 重ねない場合も、シェーダーに渡すテクスチャは用意する
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(DiffusionTexture), FVector4f(0.0f));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(SoftFocusTexture), FVector4f(0.0f));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(BloomTexture), FVector4f(0.0f));
	}

	// 3. 元の画面に重ね、グラデーションを掛ける
	FToonPostEffectsCS::FParameters* PostParameters = GraphBuilder.AllocParameters<FToonPostEffectsCS::FParameters>();
	PostParameters->OutputTexture = GraphBuilder.CreateUAV(Inputs.Output.Texture);
	PostParameters->ViewRectMin = Inputs.Output.ViewRect.Min;
	PostParameters->ViewRectSize = Resolution;
	PostParameters->HalfSize = HalfSize;
	PostParameters->AccumulationTexture = Inputs.AccumulationTexture;
	PostParameters->ToonGlowTexture = Inputs.GlowTexture;
	PostParameters->DiffusionBlurTexture = DiffusionTexture;
	PostParameters->SoftFocusBlurTexture = SoftFocusTexture;
	PostParameters->BloomBlurTexture = BloomTexture;
	PostParameters->BlurSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PostParameters->DiffusionStrength = DiffusionStrength;
	PostParameters->SoftFocusStrength = SoftFocusStrength;
	PostParameters->BloomStrength = BloomStrength;
	PostParameters->OutputEncoding = Inputs.OutputEncoding;
	PostParameters->ToonEyeAdaptationExposure = Inputs.EyeAdaptationExposure;
	PostParameters->View = View.ViewUniformBuffer;
	PostParameters->GradationStrength = GradationStrength;
	PostParameters->GradationTopColor = FVector3f(Inputs.GradationTopColor.R, Inputs.GradationTopColor.G, Inputs.GradationTopColor.B);
	PostParameters->GradationBottomColor = FVector3f(Inputs.GradationBottomColor.R, Inputs.GradationBottomColor.G, Inputs.GradationBottomColor.B);

	TShaderMapRef<FToonPostEffectsCS> PostShader(ShaderMap);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("Composite %dx%d", Resolution.X, Resolution.Y),
		PostShader,
		PostParameters,
		FComputeShaderUtils::GetGroupCount(Resolution, FToonPostEffectsShaderBase::ThreadGroupSize));
}
