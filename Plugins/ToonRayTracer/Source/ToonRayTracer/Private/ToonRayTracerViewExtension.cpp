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

	TAutoConsoleVariable<int32> CVarToonRayTracerShadingMode(
		TEXT("r.ToonRayTracer.ShadingMode"),
		1,
		TEXT("0: Visualize normals, 1: Materials (Ray Tracing in One Weekend chapters 9-10)"),
		ECVF_RenderThreadSafe);

	// 本の第9章の max_depth に相当
	TAutoConsoleVariable<int32> CVarToonRayTracerMaxDepth(
		TEXT("r.ToonRayTracer.MaxDepth"),
		10,
		TEXT("Maximum number of ray bounces (1-50)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerAccumulate(
		TEXT("r.ToonRayTracer.Accumulate"),
		1,
		TEXT("Accumulate samples across frames while the camera is still (0: off, 1: on). Toggle to reset."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerMaxAccumulatedFrames(
		TEXT("r.ToonRayTracer.MaxAccumulatedFrames"),
		1024,
		TEXT("Upper limit of accumulated frames. Further frames keep blending with this weight."),
		ECVF_RenderThreadSafe);

	// この数のフレームで使われなかったビューの蓄積状態は破棄する
	constexpr uint32 AccumulationStateTimeoutFrames = 300;

	// マテリアルの種類（シェーダー側の TOON_MATERIAL_* と一致させる）
	enum class EToonMaterialType : uint32
	{
		Default = 0,	// 未設定（灰色のランバート反射）
		Lambertian = 1,	// 拡散反射（本の lambertian）
		Metal = 2,		// 金属（本の metal）
		Dielectric = 3,	// ガラスなどの誘電体（本の dielectric）
	};

	struct FToonSphere
	{
		FVector Center;				// ワールド座標（cm）
		double Radius;				// 半径（cm）
		EToonMaterialType Material;
		FLinearColor Albedo;		// 反射率（本の albedo）
		float Fuzz;					// 金属の反射のぼけ具合（0 ～ 1）
		float RefractionIndex;		// 誘電体の屈折率（本の refraction_index）
	};

	// 『Ray Tracing in One Weekend』第11章のシーンをUEの単位系（cm, Z-up）に置き換えたもの
	// 本の座標（x: 右, y: 上, -z: 奥）を UE（+Y: 右, +Z: 上, +X: 奥）に対応させ、100倍して cm にする
	// 本では球の中心が y=0、地面の上面が y=-0.5 のため、全体を 50cm 持ち上げて地面の上面を z=0 にしている
	const FToonSphere GToonSpheres[] =
	{
		// 地面
		{ FVector(0.0, 0.0, -10000.0), 10000.0, EToonMaterialType::Lambertian, FLinearColor(0.8f, 0.8f, 0.0f), 0.0f, 1.0f },
		// 中央：青いランバート
		{ FVector(120.0, 0.0, 50.0), 50.0, EToonMaterialType::Lambertian, FLinearColor(0.1f, 0.2f, 0.5f), 0.0f, 1.0f },
		// 左：中空のガラス球（外側はガラス、内側の一回り小さい球は「ガラスの中の空気」）
		{ FVector(100.0, -100.0, 50.0), 50.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.5f },
		{ FVector(100.0, -100.0, 50.0), 40.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.0f / 1.5f },
		// 右：ぼけの強い金色の金属
		{ FVector(100.0, 100.0, 50.0), 50.0, EToonMaterialType::Metal, FLinearColor(0.8f, 0.6f, 0.2f), 1.0f, 1.0f },
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

	const uint32 TraceMode = static_cast<uint32>(CVarToonRayTracerTraceMode.GetValueOnRenderThread());
	const uint32 SamplesPerPixel = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerSamplesPerPixel.GetValueOnRenderThread(), 1, 64));
	const uint32 ShadingMode = static_cast<uint32>(CVarToonRayTracerShadingMode.GetValueOnRenderThread());
	const uint32 MaxDepth = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerMaxDepth.GetValueOnRenderThread(), 1, 50));
	const bool bAccumulate = CVarToonRayTracerAccumulate.GetValueOnRenderThread() != 0;
	const uint32 MaxAccumulatedFrames = static_cast<uint32>(FMath::Max(CVarToonRayTracerMaxAccumulatedFrames.GetValueOnRenderThread(), 1));
	const FIntPoint ViewRectSize = Output.ViewRect.Size();

	// TAA 用のジッターを含まない行列でレイを作る
	// （ジッターは毎フレーム変わるため、含めると蓄積が毎フレームリセットされてしまう。AA は自前のサンプリングで行う）
	const FMatrix& WorldToView = View.ViewMatrices.GetWorldToView();
	const FMatrix& ViewToClipNoAA = View.ViewMatrices.GetViewToClipNoAA();
	const FMatrix ClipToTranslatedWorldNoAA = (View.ViewMatrices.GetTranslatedViewMatrix() * ViewToClipNoAA).Inverse();

	// フレームをまたいだ蓄積（カメラか設定が変わったらリセット）
	// ビューの状態を持たないビュー（サムネイル描画など）では蓄積しない
	const uint32 FrameNumber = View.Family->FrameNumber;
	const uint32 SettingsHash = HashCombine(HashCombine(GetTypeHash(TraceMode), GetTypeHash(SamplesPerPixel)),
		HashCombine(GetTypeHash(ShadingMode), GetTypeHash(MaxDepth)));
	FAccumulationState* AccumulationState = nullptr;
	if (bAccumulate && View.State)
	{
		TSharedPtr<FAccumulationState>& StatePtr = AccumulationStates.FindOrAdd(View.State);
		if (!StatePtr.IsValid())
		{
			StatePtr = MakeShared<FAccumulationState>();
		}
		AccumulationState = StatePtr.Get();

		const bool bCameraOrSettingsChanged =
			!AccumulationState->WorldToView.Equals(WorldToView, 0.0)
			|| !AccumulationState->ViewToClipNoAA.Equals(ViewToClipNoAA, 0.0)
			|| AccumulationState->ViewRectSize != ViewRectSize
			|| AccumulationState->SettingsHash != SettingsHash
			|| !AccumulationState->Texture.IsValid();
		if (bCameraOrSettingsChanged)
		{
			AccumulationState->AccumulatedFrames = 0;
			AccumulationState->WorldToView = WorldToView;
			AccumulationState->ViewToClipNoAA = ViewToClipNoAA;
			AccumulationState->ViewRectSize = ViewRectSize;
			AccumulationState->SettingsHash = SettingsHash;
		}
		AccumulationState->LastUsedFrameNumber = FrameNumber;
	}
	else
	{
		// 蓄積を切ったらすべての状態を破棄し、次に有効にしたときは最初からやり直す
		AccumulationStates.Reset();
	}

	// 閉じたビューポートなど、しばらく使われていない状態を破棄する
	for (auto It = AccumulationStates.CreateIterator(); It; ++It)
	{
		if (FrameNumber - It.Value()->LastUsedFrameNumber > AccumulationStateTimeoutFrames)
		{
			It.RemoveCurrent();
		}
	}

	// 蓄積用テクスチャ（ViewRect サイズ、線形色を高精度で保持）
	FRDGTextureRef AccumulationTexture = nullptr;
	if (AccumulationState && AccumulationState->Texture.IsValid() && AccumulationState->AccumulatedFrames > 0)
	{
		AccumulationTexture = GraphBuilder.RegisterExternalTexture(AccumulationState->Texture, TEXT("ToonRayTracerAccumulation"));
	}
	else
	{
		const FRDGTextureDesc AccumulationDesc = FRDGTextureDesc::Create2D(
			ViewRectSize, PF_A32B32G32R32F, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		AccumulationTexture = GraphBuilder.CreateTexture(AccumulationDesc, TEXT("ToonRayTracerAccumulation"));
	}
	const uint32 AccumulatedFrames = AccumulationState ? AccumulationState->AccumulatedFrames : 0;

	FToonRayGenShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FToonRayGenShader::FParameters>();
	PassParameters->OutputTexture = GraphBuilder.CreateUAV(Output.Texture);
	PassParameters->AccumulationTexture = GraphBuilder.CreateUAV(AccumulationTexture);
	PassParameters->AccumulatedFrames = AccumulatedFrames;
	PassParameters->MaxAccumulatedFrames = MaxAccumulatedFrames;
	PassParameters->RandomSeed = AccumulationState ? AccumulationState->RandomSeed : 0;
	PassParameters->TLAS = TLAS;
	// GPUScene（各メッシュの Custom Primitive Data など）を読むためのシーンのユニフォームバッファ
	FSceneUniformBuffer& SceneUniformBuffer = UE::FXRenderingUtils::CreateSceneUniformBuffer(GraphBuilder, View.Family->Scene);
	PassParameters->Scene = UE::FXRenderingUtils::GetSceneUniformBuffer(GraphBuilder, SceneUniformBuffer);
	PassParameters->TraceMode = TraceMode;
	PassParameters->SamplesPerPixel = SamplesPerPixel;
	PassParameters->ShadingMode = ShadingMode;
	PassParameters->MaxDepth = MaxDepth;
	PassParameters->ClipToTranslatedWorld = FMatrix44f(ClipToTranslatedWorldNoAA);
	PassParameters->ViewRectMin = Output.ViewRect.Min;
	PassParameters->ViewRectSize = ViewRectSize;

	// 次のフレームのために蓄積数を進める（上限に達したら、シェーダー側で蓄積済みの結果を表示し続ける）
	// 乱数の種は上限と関係なく進める。上限で止めると毎フレーム同じサンプルになり、
	// 平均がその1枚のノイズ画像に引き寄せられてしまうため
	if (AccumulationState)
	{
		AccumulationState->AccumulatedFrames = FMath::Min(AccumulationState->AccumulatedFrames + 1, MaxAccumulatedFrames);
		++AccumulationState->RandomSeed;
	}

	// 球の中心をワールド空間から Translated World 空間に変換して渡す
	// （double で引き算してから float にすることで、カメラから遠い位置でも精度を保つ）
	static_assert(UE_ARRAY_COUNT(GToonSpheres) <= FToonRayGenShader::MaxSpheres, "Too many spheres");
	const FVector PreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(GToonSpheres); ++Index)
	{
		const FToonSphere& Sphere = GToonSpheres[Index];
		const FVector TranslatedCenter = Sphere.Center + PreViewTranslation;
		PassParameters->Spheres[Index] = FVector4f(FVector3f(TranslatedCenter), static_cast<float>(Sphere.Radius));
		PassParameters->SphereMaterialParams[Index] = FVector4f(static_cast<float>(Sphere.Material), Sphere.Fuzz, Sphere.RefractionIndex, 0.0f);
		PassParameters->SphereAlbedo[Index] = FVector4f(Sphere.Albedo.R, Sphere.Albedo.G, Sphere.Albedo.B, 0.0f);
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

	// 蓄積テクスチャを次のフレームまで保持する
	// （RDG では、書き込むパスを登録した後でないと抽出を登録できない）
	if (AccumulationState)
	{
		GraphBuilder.QueueTextureExtraction(AccumulationTexture, &AccumulationState->Texture);
	}

	// このパスがポストプロセスの最後の場合、エンジンが用意した出力先へ書き戻す必要がある
	if (Inputs.OverrideOutput.IsValid())
	{
		AddDrawTexturePass(GraphBuilder, FScreenPassViewInfo(View), Output, Inputs.OverrideOutput);
		return Inputs.OverrideOutput;
	}

	return MoveTemp(Output);
}