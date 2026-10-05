#include "ToonRayTracerViewExtension.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#include "ToonRayGenShader.h"
#include "PipelineStateCache.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "FXRenderingUtils.h"

namespace
{
	// 0: シェーダー内の解析的な球（本の第6章）、1: UEシーンの TLAS に TraceRay
	TAutoConsoleVariable<int32> CVarToonRayTracerTraceMode(
		TEXT("r.ToonRayTracer.TraceMode"),
		1,
		TEXT("0: Analytic spheres in shader, 1: TraceRay against the scene TLAS"),
		ECVF_RenderThreadSafe);

	// 本の第8章（アンチエイリアス）の samples_per_pixel に相当
	TAutoConsoleVariable<int32> CVarToonRayTracerSamplesPerPixel(
		TEXT("r.ToonRayTracer.SamplesPerPixel"),
		4,
		TEXT("Number of jittered camera rays per pixel for anti-aliasing (1-64)"),
		ECVF_RenderThreadSafe);

	struct FToonSphere
	{
		FVector Center;	// ワールド座標（cm）
		double Radius;	// 半径（cm）
	};

	// 『Ray Tracing in One Weekend』第6章のシーンをUEの単位系（cm, Z-up）に置き換えたもの
	// ワールド原点に半径50cmの球を置き、その下に地面代わりの巨大な球を置く
	const FToonSphere GToonSpheres[] =
	{
		{ FVector(0.0, 0.0, 50.0), 50.0 },
		{ FVector(0.0, 0.0, -10000.0), 10000.0 },
	};
}

ToonRayTracerViewExtension::ToonRayTracerViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

ToonRayTracerViewExtension::~ToonRayTracerViewExtension()
{
}

// パスの登録
void ToonRayTracerViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	// Tonemapのタイミングで、かつパスが有効な場合に登録
	if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled)
	{
		InOutPassCallbacks.Add(
			FPostProcessingPassDelegate::CreateRaw(this, &ToonRayTracerViewExtension::RenderToonRayTracingPass));
	}
}

// レイトレーシングのメイン描画処理
FScreenPassTexture ToonRayTracerViewExtension::RenderToonRayTracingPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	// 動作確認用のログ出力
	static bool bHasLogged = false;
	if (!bHasLogged)
	{
		if (!IsRayTracingEnabled())
		{
			UE_LOG(LogTemp, Error, TEXT("===== [ToonRayTracer] RayTracing is DISABLED! Pass Skipped. ====="));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("===== [ToonRayTracer] RayTracing is ENABLED! Executing Pass. ====="));
		}
		bHasLogged = true;
	}
	
	// プロジェクト設定やハードウェア要件でレイトレーシングが無効な場合は、何もせず元の画像を返す
	if (!IsRayTracingEnabled() || !View.Family || !View.Family->Scene)
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}

	// シーンの TLAS（レイトレーシング用の加速構造）を Public API 経由で取得する
	// まだ構築されていないフレームでは何もせず元の画像を返す
	if (!UE::FXRenderingUtils::RayTracing::HasRayTracingScene(*View.Family->Scene))
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}
	FRDGBufferSRVRef TLAS = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneViewRDG(*View.Family->Scene, View);
	if (!TLAS)
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}

	// 現在の画面の情報を取得
	FScreenPassTexture SceneColor = (FScreenPassTexture)Inputs.GetInput(EPostProcessMaterialInput::SceneColor);

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= ETextureCreateFlags::UAV;

	// 設定をもとにRDG上に新しいテクスチャを確保する
	FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("ToonRayTracerOutput"));
	// 入力と同じ描画範囲（ViewRect）を持たせる。内部バッファが画面より大きい場合に対応するため
	FScreenPassRenderTarget Output(OutputTexture, SceneColor.ViewRect, View.GetOverwriteLoadAction());

	FToonRayGenShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FToonRayGenShader::FParameters>();
	PassParameters->OutputTexture = GraphBuilder.CreateUAV(Output.Texture);
	PassParameters->TLAS = TLAS;
	PassParameters->TraceMode = static_cast<uint32>(CVarToonRayTracerTraceMode.GetValueOnRenderThread());
	PassParameters->SamplesPerPixel = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerSamplesPerPixel.GetValueOnRenderThread(), 1, 64));
	PassParameters->ClipToTranslatedWorld = FMatrix44f(View.ViewMatrices.GetClipToTranslatedWorld());
	PassParameters->ViewRectMin = Output.ViewRect.Min;
	PassParameters->ViewRectSize = Output.ViewRect.Size();

	// 球の中心をワールド空間から Translated World 空間に変換して渡す
	// （double で引き算してから float にすることで、カメラから遠い位置でも精度を保つ）
	static_assert(UE_ARRAY_COUNT(GToonSpheres) <= FToonRayGenShader::MaxSpheres, "Too many spheres");
	const FVector PreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(GToonSpheres); ++Index)
	{
		const FToonSphere& Sphere = GToonSpheres[Index];
		const FVector TranslatedCenter = Sphere.Center + PreViewTranslation;
		PassParameters->Spheres[Index] = FVector4f(FVector3f(TranslatedCenter), static_cast<float>(Sphere.Radius));
	}
	PassParameters->NumSpheres = UE_ARRAY_COUNT(GToonSpheres);

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	TShaderMapRef<FToonRayGenShader> RayGenShader(ShaderMap);
	TShaderMapRef<FToonClosestHitShader> ClosestHitShader(ShaderMap);
	TShaderMapRef<FToonMissShader> MissShader(ShaderMap);
	
	// ディスパッチは描画範囲（ViewRect）のサイズで行う
	FIntPoint Resolution = Output.ViewRect.Size();

	// レイトレーシングパイプライン（RTPSO）の構築
	// RayGen / HitGroup / Miss をそれぞれ最低1つずつ登録する必要がある
	FRayTracingPipelineStateInitializer Initializer;

	FRHIRayTracingShader* RayGenShaderTable[] = { RayGenShader.GetRayTracingShader() };
	Initializer.SetRayGenShaderTable(RayGenShaderTable);

	FRHIRayTracingShader* HitGroupTable[] = { ClosestHitShader.GetRayTracingShader() };
	Initializer.SetHitGroupTable(HitGroupTable);

	FRHIRayTracingShader* MissShaderTable[] = { MissShader.GetRayTracingShader() };
	Initializer.SetMissShaderTable(MissShaderTable);

	FRayTracingPipelineState* PipelineState = PipelineStateCache::GetAndOrCreateRayTracingPipelineState(GraphBuilder.RHICmdList, Initializer);

	// シェーダーバインディングテーブル（SBT）を確保する（RayTraceDispatch に必須）
	// HitGroupIndexingMode::Disallow にすることで、シーンのジオメトリ数に関係なく
	// デフォルトのヒットグループ用レコードが1つだけ確保される
	FRayTracingShaderBindingTableInitializer SBTInitializer;
	SBTInitializer.Lifetime = ERayTracingShaderBindingTableLifetime::Transient;
	SBTInitializer.ShaderBindingMode = ERayTracingShaderBindingMode::RTPSO;
	SBTInitializer.HitGroupIndexingMode = ERayTracingHitGroupIndexingMode::Disallow;
	SBTInitializer.LocalBindingDataSize = Initializer.GetMaxLocalBindingDataSize();
	SBTInitializer.NumMissShaderSlots = 1;

	FShaderBindingTableRHIRef SBT = GraphBuilder.RHICmdList.CreateRayTracingShaderBindingTable(SBTInitializer);

	// RDGにレイトレーシング用のパスを登録してディスパッチ
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("ToonRayTracing_Phase1"),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, RayGenShader, PipelineState, SBT, Resolution](FRDGAsyncTask, FRHICommandList& RHICmdList)
		{
			FRHIBatchedShaderParameters& GlobalResources = RHICmdList.GetScratchShaderParameters();
			SetShaderParameters(GlobalResources, RayGenShader, *PassParameters);

			// デフォルトのヒットグループとMissシェーダーをSBTに設定してコミット
			RHICmdList.SetDefaultRayTracingHitGroup(SBT, PipelineState, 0);
			RHICmdList.SetRayTracingMissShader(SBT, 0, PipelineState, 0 /* ShaderIndexInPipeline */, 0, nullptr, 0);
			RHICmdList.CommitShaderBindingTable(SBT);

			RHICmdList.RayTraceDispatch(
				PipelineState,
				RayGenShader.GetRayTracingShader(),
				SBT,
				GlobalResources,
				Resolution.X,
				Resolution.Y
			);
		}
	);

	// このパスがポストプロセスの最後の場合、エンジンが用意した出力先へ書き戻す必要がある
	if (Inputs.OverrideOutput.IsValid())
	{
		AddDrawTexturePass(GraphBuilder, FScreenPassViewInfo(View), Output, Inputs.OverrideOutput);
		return Inputs.OverrideOutput;
	}

	return MoveTemp(Output);
}