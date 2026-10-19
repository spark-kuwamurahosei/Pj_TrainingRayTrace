#include "ToonRayTracerViewExtension.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#include "ToonRayGenShader.h"
#include "ToonRayTracerCVars.h"
#include "ToonRayTracerCompat.h"
#include "ToonRayTracerAnalyticScene.h"
#include "ToonPostEffectsPass.h"
#include "PipelineStateCache.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "FXRenderingUtils.h"
#include "SystemTextures.h"
#include "ToonCharacterProfile.h"
#include "RenderingThread.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RHIStaticStates.h"
#include "RenderGraphUtils.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"

// stat gpu に「ToonRayTracer」として GPU 時間を表示する
DECLARE_GPU_STAT(ToonRayTracer);
// 負荷の確認用：上のうちレイトレーシング本体（RayGen）だけの時間
DECLARE_GPU_STAT(ToonRayTracerRayGen);

using namespace ToonRayTracerCVars;
using namespace ToonRayTracerAnalytic;

namespace
{
	// この数のフレームで使われなかったビューの蓄積状態は破棄する
	constexpr uint32 AccumulationStateTimeoutFrames = 300;
}

ToonRayTracerViewExtension::ToonRayTracerViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

ToonRayTracerViewExtension::~ToonRayTracerViewExtension()
{
}

void ToonRayTracerViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	// 前のフレームのテクスチャ（別のグラフのもの）を使わないよう、フレームの始めに捨てる
	EmissiveTextures_RenderThread.Reset();
}

void ToonRayTracerViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	// トゥーンの表示でシーンのレイを使い、発光を足すときだけ写し取る
	if (CVarToonRayTracerShadingMode.GetValueOnRenderThread() != 3 || CVarToonRayTracerTraceMode.GetValueOnRenderThread() == 0
		|| CVarToonRayTracerToonEmissiveStrength.GetValueOnRenderThread() <= 0.0f)
	{
		return;
	}

	// 1 つ目のレンダーターゲットがシーンの色。ベースパスの直後はライティング前なので、マテリアルの発光（と Unlit の色）だけが入っている
	FRDGTextureRef SceneColor = RenderTargets[0].GetTexture();
	if (!SceneColor)
	{
		return;
	}
	const FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(SceneColor->Desc.Extent, SceneColor->Desc.Format, FClearValueBinding::Black, TexCreate_ShaderResource);
	FRDGTextureRef Emissive = GraphBuilder.CreateTexture(Desc, TEXT("ToonRayTracer.Emissive"));
	AddCopyTexturePass(GraphBuilder, SceneColor, Emissive);
	EmissiveTextures_RenderThread.Add(&InView, Emissive);
}

void ToonRayTracerViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// ライトの情報はゲームスレッド側にしかないため、ここで取得してレンダースレッドへ渡す
	// レンダーコマンドは順番に実行されるので、このビューファミリーの描画より前に反映される
	FToonDirectionalLight Light = FindDirectionalLight(InViewFamily);
	// 顔の影：ボーンの位置もゲームスレッド側にしかないため、ここで集めて渡す
	TArray<FToonFace> Faces;
	FindFaces(InViewFamily, Faces);
	// 文字列型のコンソール変数もゲームスレッドでしか読めないため、ここで色に変換して一緒に渡す
	const FLinearColor ShadowColor = ParseLinearColor(CVarToonRayTracerToonShadowColor.GetValueOnGameThread(), DefaultToonShadowColor);
	const FLinearColor OutlineColor = ParseLinearColor(CVarToonRayTracerToonOutlineColor.GetValueOnGameThread(), DefaultToonOutlineColor);
	const FLinearColor GlassEdgeColor = ParseLinearColor(CVarToonRayTracerToonGlassEdgeColor.GetValueOnGameThread(), DefaultToonGlassEdgeColor);
	const FLinearColor SkinShadowColor = ParseLinearColor(CVarToonRayTracerToonSkinShadowColor.GetValueOnGameThread(), DefaultToonSkinShadowColor);
	const FLinearColor SeeThroughColor = ParseLinearColor(CVarToonRayTracerToonSeeThroughColor.GetValueOnGameThread(), DefaultToonSeeThroughColor);
	const FLinearColor GradationTopColor = ParseLinearColor(CVarToonRayTracerToonOriginalGradationTopColor.GetValueOnGameThread(), DefaultToonGradationTopColor);
	const FLinearColor GradationBottomColor = ParseLinearColor(CVarToonRayTracerToonOriginalGradationBottomColor.GetValueOnGameThread(), DefaultToonGradationBottomColor);

	// 『The Next Week』第4章：球に貼る画像のアセットは、パスが変わったときだけ読み込む（見つからなければ何度も試さない）
	const FString Path = CVarToonRayTracerImageTexture.GetValueOnGameThread();
	if (Path != ImageTexturePath)
	{
		ImageTexturePath = Path;
		UTexture2D* Loaded = Path.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		ImageTexture.Reset(Loaded);
		if (!Loaded && !Path.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[ToonRayTracer] Image texture '%s' was not found. A UV checker is used instead"), *Path);
		}
	}
	if (ImageTexture.IsValid())
	{
		// マテリアルから使われていないテクスチャは、テクスチャストリーミングで低い解像度（ミップ）だけが残され、ぼやけてしまう
		// シェーダーから直接読むので、すべてのミップを常にメモリに置くよう指示し続ける（指示は指定した秒数で切れる）
		ImageTexture->SetForceMipLevelsToBeResident(ImageTextureResidentSeconds);
	}
	FTextureResource* ImageResource = ImageTexture.IsValid() ? ImageTexture->GetResource() : nullptr;

	// 顔の影の SDF 版：しきい値マップも、パスが変わったときだけ読み込む
	const FString FaceSDFPath = CVarToonRayTracerToonFaceShadowSDFTexture.GetValueOnGameThread();
	if (FaceSDFPath != FaceSDFTexturePath)
	{
		FaceSDFTexturePath = FaceSDFPath;
		UTexture2D* Loaded = FaceSDFPath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *FaceSDFPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		FaceSDFTexture.Reset(Loaded);
		if (!Loaded && !FaceSDFPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ToonRayTracer] Face shadow SDF texture '%s' was not found. The computed face shadow is used instead"), *FaceSDFPath);
		}
	}
	if (FaceSDFTexture.IsValid())
	{
		FaceSDFTexture->SetForceMipLevelsToBeResident(ImageTextureResidentSeconds);
	}
	FTextureResource* FaceSDFResource = FaceSDFTexture.IsValid() ? FaceSDFTexture->GetResource() : nullptr;

	ENQUEUE_RENDER_COMMAND(ToonRayTracerUpdateGameThreadState)(
		[this, Light, Faces = MoveTemp(Faces), ShadowColor, OutlineColor, SkinShadowColor, GradationTopColor, GradationBottomColor, SeeThroughColor, GlassEdgeColor, ImageResource, FaceSDFResource](FRHICommandListImmediate&)
		{
			ToonSeeThroughColor_RenderThread = SeeThroughColor;
			ToonGradationTopColor_RenderThread = GradationTopColor;
			ToonGradationBottomColor_RenderThread = GradationBottomColor;
			DirectionalLight_RenderThread = Light;
			Faces_RenderThread = Faces;
			ToonShadowColor_RenderThread = ShadowColor;
			ToonSkinShadowColor_RenderThread = SkinShadowColor;
			ToonOutlineColor_RenderThread = OutlineColor;
			ToonGlassEdgeColor_RenderThread = GlassEdgeColor;
			ImageTextureResource_RenderThread = ImageResource;
			FaceSDFTextureResource_RenderThread = FaceSDFResource;
		});
}

bool ToonRayTracerViewExtension::ShouldDrawBeforeTSR()
{
	return CVarToonRayTracerToonBeforeTSR.GetValueOnRenderThread() != 0
		&& CVarToonRayTracerToonCharacterOnly.GetValueOnRenderThread() != 0
		&& CVarToonRayTracerShadingMode.GetValueOnRenderThread() == 3
		&& CVarToonRayTracerTraceMode.GetValueOnRenderThread() == 1;
}

// パスの登録
void ToonRayTracerViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	// Original ルック フェーズ3：キャラクターだけトゥーンにするとき（トゥーン表示・シーンのレイ）は、TSR の前に描く
	// TSR がジッターを使ってアンチエイリアスとアップスケールを行い、動いている間の輪郭のギザつきが抑えられる
	// トーンマップや DOF、ブルームも、背景と同じようにキャラクターにかかる
	// UE5.5 では、TSR の前に描く場合は PrePostProcessPass_RenderThread で描くので、ここではトーンマップの後にも登録しない
	const bool bBeforeTSR = ShouldDrawBeforeTSR();

	if (bBeforeTSR)
	{
#if TOON_RAY_TRACER_BEFORE_DOF_PASS
		// DOF の前（TSR より前）のタイミングで登録
		if (Pass == EPostProcessingPass::BeforeDOF)
		{
			InOutPassCallbacks.Add(
				FAfterPassCallbackDelegate::CreateRaw(this, &ToonRayTracerViewExtension::RenderToonRayTracingPassBeforeTSR));
		}
#endif
	}
	// Tonemapのタイミングで、かつパスが有効な場合に登録
	else if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled)
	{
		InOutPassCallbacks.Add(
			FAfterPassCallbackDelegate::CreateRaw(this, &ToonRayTracerViewExtension::RenderToonRayTracingPass));
	}
}

FScreenPassTexture ToonRayTracerViewExtension::RenderToonRayTracingPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	return RenderToonRayTracing(GraphBuilder, View, Inputs, false);
}

FScreenPassTexture ToonRayTracerViewExtension::RenderToonRayTracingPassBeforeTSR(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	return RenderToonRayTracing(GraphBuilder, View, Inputs, true);
}

// レイトレーシングのメイン描画処理
// bBeforeTSR が true なら TSR の前（描画解像度、トーンマップ前の HDR）、false ならトーンマップの後（出力解像度）に描く
FScreenPassTexture ToonRayTracerViewExtension::RenderToonRayTracing(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs, bool bBeforeTSR)
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
	// 版によって受け取り方が違う（5.8 は RDG のバッファ、5.7 はビューごとの RHI のビュー、5.6 以前はシーンごとの RHI のビュー）
#if TOON_RAY_TRACER_SCENE_REFERENCE_API
	if (!UE::FXRenderingUtils::RayTracing::HasRayTracingScene(*View.Family->Scene))
#else
	if (!UE::FXRenderingUtils::RayTracing::HasRayTracingScene(View.Family->Scene))
#endif
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}
#if TOON_RAY_TRACER_RDG_TLAS
	FRDGBufferSRVRef TLAS = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneViewRDG(*View.Family->Scene, View);
#elif TOON_RAY_TRACER_SCENE_REFERENCE_API
	FRHIShaderResourceView* TLAS = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneView(GraphBuilder.RHICmdList, *View.Family->Scene, View);
#else
	FRHIShaderResourceView* TLAS = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneView(GraphBuilder.RHICmdList, View.Family->Scene);
#endif
	if (!TLAS)
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}

	// このパスで追加する処理（色の表のクリア・解決、蓄積、レイトレーシング）の GPU 時間をまとめて計測する
	RDG_EVENT_SCOPE_STAT(GraphBuilder, ToonRayTracer, "ToonRayTracer");

	// 現在の画面の情報を取得
	FScreenPassTexture SceneColor = (FScreenPassTexture)Inputs.GetInput(EPostProcessMaterialInput::SceneColor);

	if (CVarToonRayTracerDebugLogPasses.GetValueOnRenderThread() != 0)
	{
		const FIntRect& Rect = SceneColor.ViewRect;
		UE_LOG(LogTemp, Log, TEXT("[ToonRayTracer] Pass frame=%u view=%p state=%p rect=%dx%d beforeTSR=%d sceneCapture=%d reflectionCapture=%d planarReflection=%d gameView=%d realtime=%d"),
			View.Family->FrameNumber, &View, View.State, Rect.Width(), Rect.Height(), bBeforeTSR ? 1 : 0,
			View.bIsSceneCapture ? 1 : 0, View.bIsReflectionCapture ? 1 : 0, View.bIsPlanarReflection ? 1 : 0,
			View.bIsGameView ? 1 : 0, View.Family->bRealtimeUpdate ? 1 : 0);
	}

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= ETextureCreateFlags::UAV;

	// 設定をもとにRDG上に新しいテクスチャを確保する
	FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("ToonRayTracerOutput"));
	// 入力と同じ描画範囲（ViewRect）を持たせる。内部バッファが画面より大きい場合に対応するため
	FScreenPassRenderTarget Output(OutputTexture, SceneColor.ViewRect, View.GetOverwriteLoadAction());

	const uint32 TraceMode = static_cast<uint32>(CVarToonRayTracerTraceMode.GetValueOnRenderThread());
	const int32 AnalyticSceneIndex = CVarToonRayTracerAnalyticScene.GetValueOnRenderThread();
	const uint32 SamplesPerPixel = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerSamplesPerPixel.GetValueOnRenderThread(), 1, 64));
	const bool bStratifiedSampling = CVarToonRayTracerStratifiedSampling.GetValueOnRenderThread() != 0;
	const uint32 ScatterSampling = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerScatterSampling.GetValueOnRenderThread(), 0, 4));
	const uint32 ShadingMode = static_cast<uint32>(CVarToonRayTracerShadingMode.GetValueOnRenderThread());
	const uint32 MaxDepth = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerMaxDepth.GetValueOnRenderThread(), 1, 50));
	// TSR の前に描く場合は、フレームをまたいだ平均を TSR に任せるため、自前の蓄積と動きの判定はしない
	const bool bAccumulate = !bBeforeTSR && CVarToonRayTracerAccumulate.GetValueOnRenderThread() != 0;

	// 通常描画の GBuffer（法線、ベースカラー、深度）。取得できない場合（モバイルなど）は使わない
	const FSceneTextureUniformParameters* SceneTextureParameters =
		Inputs.SceneTextures.SceneTextures ? Inputs.SceneTextures.SceneTextures->GetParameters().GetContents() : nullptr;
	const bool bHasGBuffer = SceneTextureParameters && SceneTextureParameters->GBufferATexture && SceneTextureParameters->GBufferBTexture
		&& SceneTextureParameters->GBufferCTexture && SceneTextureParameters->SceneDepthTexture;
	const bool bUseGBufferNormal = bHasGBuffer && CVarToonRayTracerUseGBufferNormal.GetValueOnRenderThread() != 0;
	const bool bUseGBufferBaseColor = bHasGBuffer && CVarToonRayTracerUseGBufferBaseColor.GetValueOnRenderThread() != 0;
	const bool bUseGBufferMetal = bHasGBuffer && CVarToonRayTracerUseGBufferMetal.GetValueOnRenderThread() != 0;
	const bool bUseGBufferForReflections = bHasGBuffer && CVarToonRayTracerUseGBufferForReflections.GetValueOnRenderThread() != 0;
	const float ReflectionNormalSmoothing = FMath::Max(CVarToonRayTracerReflectionNormalSmoothing.GetValueOnRenderThread(), 0.0f);
	const float MetalRoughnessThreshold = FMath::Clamp(CVarToonRayTracerMetalRoughnessThreshold.GetValueOnRenderThread(), 0.0f, 1.0f);
	const uint32 MaxAccumulatedFrames = static_cast<uint32>(FMath::Max(CVarToonRayTracerMaxAccumulatedFrames.GetValueOnRenderThread(), 1));
	const bool bDetectMotion = !bBeforeTSR && bHasGBuffer && CVarToonRayTracerDetectMotion.GetValueOnRenderThread() != 0;
	const bool bSkipMaskedSurfaces = bHasGBuffer && CVarToonRayTracerSkipMaskedSurfaces.GetValueOnRenderThread() != 0;
	const uint32 MotionMaxAccumulatedFrames = static_cast<uint32>(FMath::Max(CVarToonRayTracerMotionMaxAccumulatedFrames.GetValueOnRenderThread(), 1));
	const FIntPoint ViewRectSize = Output.ViewRect.Size();

	// TAA 用のジッターを含まない行列でレイを作る
	// （ジッターは毎フレーム変わるため、含めると蓄積が毎フレームリセットされてしまう。AA は自前のサンプリングで行う）
	const FMatrix& WorldToView = ToonRayTracerCompat::GetWorldToView(View.ViewMatrices);
	const FMatrix& ViewToClipNoAA = ToonRayTracerCompat::GetViewToClipNoAA(View.ViewMatrices);
	// TSR の前に描く場合は、GBuffer と同じジッターを含む行列でレイを作る（TSR はジッターしたピクセル中心の値を前提にしている）
	const FMatrix& ViewToClipForRays = bBeforeTSR ? ToonRayTracerCompat::GetViewToClip(View.ViewMatrices) : ViewToClipNoAA;
	const FMatrix TranslatedWorldToClipForRays = View.ViewMatrices.GetTranslatedViewMatrix() * ViewToClipForRays;

	// フレームをまたいだ蓄積（カメラか設定が変わったらリセット）
	// ビューの状態を持たないビュー（サムネイル描画など）では蓄積しない
	const uint32 FrameNumber = View.Family->FrameNumber;
	// トゥーンの設定
	const uint32 ToonBands = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonBands.GetValueOnRenderThread(), 2, 3));
	const bool bToonCharacterOnly = CVarToonRayTracerToonCharacterOnly.GetValueOnRenderThread() != 0;
	const float ToonShadowThreshold = CVarToonRayTracerToonShadowThreshold.GetValueOnRenderThread();
	const float ToonLitThreshold = CVarToonRayTracerToonLitThreshold.GetValueOnRenderThread();
	const float ToonEdgeSoftness = FMath::Max(CVarToonRayTracerToonEdgeSoftness.GetValueOnRenderThread(), 0.0f);
	const FLinearColor ToonShadowColor = ToonShadowColor_RenderThread;
	const bool bToonSkin = CVarToonRayTracerToonSkin.GetValueOnRenderThread() != 0;
	const float ToonSkinShadowThreshold = CVarToonRayTracerToonSkinShadowThreshold.GetValueOnRenderThread();
	const FLinearColor ToonSkinShadowColor = ToonSkinShadowColor_RenderThread;
	const float ToonCharacterRimStrength = FMath::Max(CVarToonRayTracerToonCharacterRimStrength.GetValueOnRenderThread(), 0.0f);
	const float ToonCharacterRimWidth = FMath::Max(CVarToonRayTracerToonCharacterRimWidth.GetValueOnRenderThread(), 0.0f);
	const float ToonCharacterOutlineColorScale = FMath::Max(CVarToonRayTracerToonCharacterOutlineColorScale.GetValueOnRenderThread(), 0.0f);
	const float ToonSeeThroughOpacity = FMath::Clamp(CVarToonRayTracerToonSeeThroughOpacity.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonSeeThroughBrightness = FMath::Clamp(CVarToonRayTracerToonSeeThroughBrightness.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonSeeThroughDistance = FMath::Max(CVarToonRayTracerToonSeeThroughDistance.GetValueOnRenderThread(), 0.0f);
	const uint32 ToonSeeThroughSlotMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonSeeThroughSlotMask.GetValueOnRenderThread(), 0));
	const uint32 ToonEyeSlotMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonEyeSlotMask.GetValueOnRenderThread(), 0));
	const bool bToonEyeUnlit = CVarToonRayTracerToonEyeUnlit.GetValueOnRenderThread() != 0;
	const bool bToonFaceShadow = CVarToonRayTracerToonFaceShadow.GetValueOnRenderThread() != 0;
	FRHITexture* FaceSDFTextureRHI = FaceSDFTextureResource_RenderThread ? FaceSDFTextureResource_RenderThread->GetTexture2DRHI() : nullptr;
	const bool bToonFaceShadowSDF = CVarToonRayTracerToonFaceShadowSDF.GetValueOnRenderThread() != 0 && FaceSDFTextureRHI != nullptr;
	const float ToonFaceSDFScale = FMath::Max(CVarToonRayTracerToonFaceShadowSDFScale.GetValueOnRenderThread(), 0.01f);
	const float ToonFaceSDFOffsetY = CVarToonRayTracerToonFaceShadowSDFOffsetY.GetValueOnRenderThread();
	const uint32 ToonFaceSlotMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonFaceSlotMask.GetValueOnRenderThread(), 0));
	const float ToonFaceShadowCurve = FMath::Max(CVarToonRayTracerToonFaceShadowCurve.GetValueOnRenderThread(), 0.0f);
	const float ToonFaceShadowSoftness = FMath::Max(CVarToonRayTracerToonFaceShadowSoftness.GetValueOnRenderThread(), 0.001f);
	const float ToonFaceShadowFrontBias = FMath::Clamp(CVarToonRayTracerToonFaceShadowFrontBias.GetValueOnRenderThread(), 0.0f, 0.99f);
	const uint32 ToonHairShadowMode = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonHairShadow.GetValueOnRenderThread(), 0, 2));
	const float ToonHairShadowWidth = FMath::Max(CVarToonRayTracerToonHairShadowWidth.GetValueOnRenderThread(), 0.0f);
	const float ToonHairShadowLightShift = FMath::Max(CVarToonRayTracerToonHairShadowLightShift.GetValueOnRenderThread(), 0.0f);
	const uint32 ToonHairSlotMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonHairSlotMask.GetValueOnRenderThread(), 0));
	const float ToonHairShadowDistance = FMath::Max(CVarToonRayTracerToonHairShadowDistance.GetValueOnRenderThread(), 0.0f);
	const float ToonHairShadowStrength = FMath::Clamp(CVarToonRayTracerToonHairShadowStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const uint32 ToonEyeTriangleStart = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonEyeTriangleStart.GetValueOnRenderThread(), 0));
	const uint32 ToonEyeTriangleEnd = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonEyeTriangleEnd.GetValueOnRenderThread(), 0));
	const float ToonEyeShadowDistance = FMath::Max(CVarToonRayTracerToonEyeShadowDistance.GetValueOnRenderThread(), 0.0f);
	const float ToonEyeShadowBrightness = FMath::Clamp(CVarToonRayTracerToonEyeShadowBrightness.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonEyeHighlightStrength = FMath::Clamp(CVarToonRayTracerToonEyeHighlightStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonEyeHighlightThreshold = FMath::Clamp(CVarToonRayTracerToonEyeHighlightThreshold.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonEyeHighlightSearchRadius = FMath::Max(CVarToonRayTracerToonEyeHighlightSearchRadius.GetValueOnRenderThread(), 0.0f);
	const float ToonLightScale = FMath::Max(CVarToonRayTracerToonLightScale.GetValueOnRenderThread(), 0.0f);
	const float ToonSkyTint = FMath::Clamp(CVarToonRayTracerToonSkyTint.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonShadowSaturation = FMath::Max(CVarToonRayTracerToonShadowSaturation.GetValueOnRenderThread(), 0.0f);
	const bool bToonCastShadows = CVarToonRayTracerToonCastShadows.GetValueOnRenderThread() != 0;
	const float ToonShadowBias = FMath::Max(CVarToonRayTracerToonShadowBias.GetValueOnRenderThread(), 0.0f);
	const float ToonSoftShadowAngle = FMath::Clamp(CVarToonRayTracerToonSoftShadowAngle.GetValueOnRenderThread(), 0.0f, 90.0f);
	const uint32 ToonSoftShadowSamples = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonSoftShadowSamples.GetValueOnRenderThread(), 1, 16));
	const uint32 ToonSoftShadowSteps = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonSoftShadowSteps.GetValueOnRenderThread(), 0, 16));
	const float ToonGlassShadowOpacity = FMath::Clamp(CVarToonRayTracerToonGlassShadowOpacity.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonIndirectStrength = FMath::Max(CVarToonRayTracerToonIndirectStrength.GetValueOnRenderThread(), 0.0f);
	const uint32 ToonIndirectSamples = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonIndirectSamples.GetValueOnRenderThread(), 1, 16));
	const float ToonShadowNoiseStrength = FMath::Max(CVarToonRayTracerToonShadowNoiseStrength.GetValueOnRenderThread(), 0.0f);
	const float ToonShadowNoiseSize = FMath::Max(CVarToonRayTracerToonShadowNoiseSize.GetValueOnRenderThread(), 0.1f);
	const bool bToonCharacterSelfShadow = CVarToonRayTracerToonCharacterSelfShadow.GetValueOnRenderThread() != 0;
	const float ToonHighlightThreshold = CVarToonRayTracerToonHighlightThreshold.GetValueOnRenderThread();
	const float ToonHighlightStrength = FMath::Clamp(CVarToonRayTracerToonHighlightStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonRimThreshold = CVarToonRayTracerToonRimThreshold.GetValueOnRenderThread();
	const float ToonRimStrength = FMath::Max(CVarToonRayTracerToonRimStrength.GetValueOnRenderThread(), 0.0f);
	const bool bToonRimLitSideOnly = CVarToonRayTracerToonRimLitSideOnly.GetValueOnRenderThread() != 0;
	const bool bToonOutline = CVarToonRayTracerToonOutline.GetValueOnRenderThread() != 0;
	const float ToonOutlineWidth = FMath::Max(CVarToonRayTracerToonOutlineWidth.GetValueOnRenderThread(), 0.0f);
	const float ToonOutlineWidthSkinned = FMath::Max(CVarToonRayTracerToonOutlineWidthSkinned.GetValueOnRenderThread(), 0.0f);
	const float ToonOutlineThreshold = FMath::Max(CVarToonRayTracerToonOutlineThreshold.GetValueOnRenderThread(), 0.0f);
	const float ToonFaceOutlineTolerance = FMath::Max(CVarToonRayTracerToonFaceOutlineTolerance.GetValueOnRenderThread(), 1.0f);
	const FLinearColor ToonOutlineColor = ToonOutlineColor_RenderThread;
	const uint32 ToonReflectionDepth = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonReflectionDepth.GetValueOnRenderThread(), 0, 16));
	const float ToonGlassEdgeStrength = FMath::Clamp(CVarToonRayTracerToonGlassEdgeStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonGlassEdgeWidth = FMath::Clamp(CVarToonRayTracerToonGlassEdgeWidth.GetValueOnRenderThread(), 0.0f, 1.0f);
	const FLinearColor ToonGlassEdgeColor = ToonGlassEdgeColor_RenderThread;

	// ライトの向きや色、トゥーンの設定が変わったときも蓄積をリセットする
	const FToonDirectionalLight& DirectionalLight = DirectionalLight_RenderThread;
	const uint32 LightHash = HashCombine(HashCombine(GetTypeHash(DirectionalLight.ToLightDirection), GetTypeHash(DirectionalLight.Color)),
		HashCombine(GetTypeHash(DirectionalLight.Intensity), GetTypeHash(DirectionalLight.bValid)));
	const uint32 ToonHash = HashCombine(
		HashCombine(HashCombine(GetTypeHash(ToonBands), GetTypeHash(ToonShadowThreshold)), HashCombine(GetTypeHash(ToonLitThreshold), GetTypeHash(ToonEdgeSoftness))),
		HashCombine(HashCombine(GetTypeHash(ToonShadowColor), HashCombine(GetTypeHash(ToonLightScale), HashCombine(GetTypeHash(ToonSkyTint), GetTypeHash(ToonShadowSaturation)))), HashCombine(HashCombine(HashCombine(GetTypeHash(bToonCastShadows), GetTypeHash(ToonShadowBias)), HashCombine(HashCombine(GetTypeHash(ToonSoftShadowAngle), GetTypeHash(ToonSoftShadowSamples)), HashCombine(HashCombine(GetTypeHash(ToonSoftShadowSteps), GetTypeHash(ToonGlassShadowOpacity)), HashCombine(GetTypeHash(ToonIndirectStrength), GetTypeHash(ToonIndirectSamples))))), HashCombine(HashCombine(GetTypeHash(ToonShadowNoiseStrength), GetTypeHash(ToonShadowNoiseSize)), HashCombine(GetTypeHash(bToonCharacterSelfShadow), HashCombine(HashCombine(GetTypeHash(bToonSkin), GetTypeHash(ToonSkinShadowThreshold)), GetTypeHash(ToonSkinShadowColor)))))));
	const uint32 ToonHighlightHash = HashCombine(
		HashCombine(GetTypeHash(ToonHighlightThreshold), GetTypeHash(ToonHighlightStrength)),
		HashCombine(HashCombine(GetTypeHash(ToonRimThreshold), GetTypeHash(ToonRimStrength)), GetTypeHash(bToonRimLitSideOnly)));
	const uint32 ToonOutlineHash = HashCombine(
		HashCombine(HashCombine(GetTypeHash(bToonOutline), GetTypeHash(ToonOutlineWidth)), GetTypeHash(ToonOutlineWidthSkinned)),
		HashCombine(HashCombine(HashCombine(GetTypeHash(ToonOutlineThreshold), GetTypeHash(ToonFaceOutlineTolerance)), GetTypeHash(ToonOutlineColor)), HashCombine(HashCombine(GetTypeHash(ToonReflectionDepth), HashCombine(HashCombine(GetTypeHash(ToonGlassEdgeStrength), GetTypeHash(ToonGlassEdgeWidth)), GetTypeHash(ToonGlassEdgeColor))),
			HashCombine(HashCombine(GetTypeHash(ToonCharacterRimStrength), GetTypeHash(ToonCharacterRimWidth)), HashCombine(GetTypeHash(ToonCharacterOutlineColorScale),
				HashCombine(HashCombine(GetTypeHash(ToonSeeThroughOpacity), GetTypeHash(ToonSeeThroughBrightness)), HashCombine(HashCombine(GetTypeHash(ToonSeeThroughDistance), GetTypeHash(ToonSeeThroughSlotMask)), HashCombine(HashCombine(HashCombine(GetTypeHash(ToonEyeSlotMask), GetTypeHash(bToonEyeUnlit)), HashCombine(HashCombine(GetTypeHash(bToonFaceShadow), GetTypeHash(ToonFaceSlotMask)), HashCombine(GetTypeHash(ToonFaceShadowCurve), HashCombine(HashCombine(GetTypeHash(ToonFaceShadowSoftness), GetTypeHash(ToonFaceShadowFrontBias)),
					HashCombine(HashCombine(HashCombine(GetTypeHash(ToonHairShadowMode), GetTypeHash(ToonHairSlotMask)), HashCombine(GetTypeHash(ToonHairShadowWidth), GetTypeHash(ToonHairShadowLightShift))), HashCombine(GetTypeHash(ToonHairShadowDistance), GetTypeHash(ToonHairShadowStrength))))))), HashCombine(HashCombine(GetTypeHash(ToonEyeTriangleStart), GetTypeHash(ToonEyeTriangleEnd)), HashCombine(HashCombine(GetTypeHash(ToonEyeShadowDistance), GetTypeHash(ToonEyeShadowBrightness)), HashCombine(HashCombine(GetTypeHash(ToonEyeHighlightStrength), GetTypeHash(ToonEyeHighlightThreshold)), GetTypeHash(ToonEyeHighlightSearchRadius))))))))))));
	// キャラクターごとの設定が変わったら蓄積をやり直す（顔の位置は含めない）
	uint32 CharacterHash = HashCombine(HashCombine(GetTypeHash(bToonFaceShadowSDF), GetTypeHash(FaceSDFTextureRHI)), HashCombine(GetTypeHash(ToonFaceSDFScale), GetTypeHash(ToonFaceSDFOffsetY)));
	for (const FToonFace& Face : Faces_RenderThread)
	{
		CharacterHash = HashCombine(CharacterHash, HashCombine(
			HashCombine(HashCombine(GetTypeHash(Face.Slots.X), GetTypeHash(Face.Slots.Y)), HashCombine(GetTypeHash(Face.Slots.Z), GetTypeHash(Face.Slots.W))),
			HashCombine(HashCombine(HashCombine(GetTypeHash(Face.Slots2.X), GetTypeHash(Face.Slots2.Y)), HashCombine(GetTypeHash(Face.Slots2.Z), GetTypeHash(Face.Slots2.W))),
				HashCombine(HashCombine(GetTypeHash(Face.Slots3.X), GetTypeHash(Face.Slots3.Y)), GetTypeHash(Face.Slots3.Z)))));
		CharacterHash = HashCombine(CharacterHash, HashCombine(
			HashCombine(HashCombine(GetTypeHash(Face.Params.X), GetTypeHash(Face.Params.Y)), HashCombine(GetTypeHash(Face.Params.Z), GetTypeHash(Face.Params.W))),
			HashCombine(HashCombine(GetTypeHash(Face.Params2.X), GetTypeHash(Face.Params2.Y)), HashCombine(GetTypeHash(Face.Params2.Z), GetTypeHash(Face.Params2.W)))));
		CharacterHash = HashCombine(CharacterHash, HashCombine(GetTypeHash(Face.CharacterId), GetTypeHash(Face.LookOverrideMask)));
		CharacterHash = HashCombine(CharacterHash, HashCombine(HashCombine(GetTypeHash(Face.FaceShadowMethod), GetTypeHash(Face.FaceSDFResource)),
			HashCombine(GetTypeHash(Face.FaceSDFScale), HashCombine(GetTypeHash(Face.FaceSDFOffsetY), GetTypeHash(Face.bFaceSDFOffsetY)))));
		for (const FVector4f& Look : Face.Look)
		{
			CharacterHash = HashCombine(CharacterHash, HashCombine(HashCombine(GetTypeHash(Look.X), GetTypeHash(Look.Y)), HashCombine(GetTypeHash(Look.Z), GetTypeHash(Look.W))));
		}
	}
	const uint32 SettingsHash = HashCombine(HashCombine(HashCombine(
		HashCombine(HashCombine(HashCombine(GetTypeHash(TraceMode), GetTypeHash(AnalyticSceneIndex)), HashCombine(GetTypeHash(SamplesPerPixel), GetTypeHash(bStratifiedSampling))), HashCombine(HashCombine(GetTypeHash(ShadingMode), GetTypeHash(MaxDepth)), GetTypeHash(ScatterSampling))),
		HashCombine(HashCombine(GetTypeHash(bUseGBufferNormal), GetTypeHash(bUseGBufferBaseColor)), HashCombine(HashCombine(GetTypeHash(bUseGBufferMetal), GetTypeHash(MetalRoughnessThreshold)), HashCombine(HashCombine(GetTypeHash(bUseGBufferForReflections), GetTypeHash(ReflectionNormalSmoothing)), GetTypeHash(bSkipMaskedSurfaces))))), LightHash), HashCombine(HashCombine(ToonHash, ToonHighlightHash), HashCombine(ToonOutlineHash, GetTypeHash(bToonCharacterOnly))))
		^ GetTypeHash(CVarToonRayTracerResetAccumulation.GetValueOnRenderThread()) ^ CharacterHash
		^ HashCombine(HashCombine(GetTypeHash(CVarToonRayTracerToonOriginal.GetValueOnRenderThread()), GetTypeHash(CVarToonRayTracerToonOriginalHighlightStrength.GetValueOnRenderThread())),
			HashCombine(HashCombine(GetTypeHash(CVarToonRayTracerToonOriginalSparkleStrength.GetValueOnRenderThread()), GetTypeHash(CVarToonRayTracerToonOriginalShadowBias.GetValueOnRenderThread())),
				HashCombine(GetTypeHash(CVarToonRayTracerToonOriginalReflectionRimStrength.GetValueOnRenderThread()),
					HashCombine(GetTypeHash(ToonSeeThroughColor_RenderThread), GetTypeHash(CVarToonRayTracerToonSeeThroughColorBlend.GetValueOnRenderThread())))));
	FAccumulationState* AccumulationState = nullptr;
	if (bAccumulate && View.State)
	{
		TSharedPtr<FAccumulationState>& StatePtr = AccumulationStates.FindOrAdd(View.State);
		if (!StatePtr.IsValid())
		{
			StatePtr = MakeShared<FAccumulationState>();
		}
		AccumulationState = StatePtr.Get();

		const bool bCameraChanged = !AccumulationState->WorldToView.Equals(WorldToView, 0.0);
		const bool bProjectionChanged = !AccumulationState->ViewToClipNoAA.Equals(ViewToClipNoAA, 0.0);
		const bool bViewRectChanged = AccumulationState->ViewRectSize != ViewRectSize;
		const bool bSettingsChanged = AccumulationState->SettingsHash != SettingsHash;
		const bool bHistoryMissing = !AccumulationState->Texture.IsValid()
			|| !AccumulationState->DepthTexture.IsValid()
			|| !AccumulationState->SignatureTexture.IsValid()
			|| !AccumulationState->GlowTexture.IsValid();
		const bool bCameraOrSettingsChanged = bCameraChanged || bProjectionChanged || bViewRectChanged || bSettingsChanged || bHistoryMissing;
		if (bCameraOrSettingsChanged)
		{
			if (CVarToonRayTracerDebugMotion.GetValueOnRenderThread() != 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[ToonRayTracer] Accumulation reset (frame %u): camera=%d projection=%d viewrect=%d settings=%d history=%d"),
					FrameNumber, bCameraChanged, bProjectionChanged, bViewRectChanged, bSettingsChanged, bHistoryMissing);
			}

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

	// 汎用化 G4：物体ごとの代表色の表（前のフレームまでにカメラから見えた UE のマテリアルの色の平均）
	// 反射・屈折の先が画面外や隠れた点のとき、灰色の代わりにこの色を使う
	// 「蓄積をリセット」でも空にする（物体を消したり入れ替えたりしたあとに、古い色が残らないように）
	// しばらく描画されていないシーンの表を破棄する
	for (auto It = ObjectColorTables.CreateIterator(); It; ++It)
	{
		if (FrameNumber - It.Value()->LastUsedFrameNumber > AccumulationStateTimeoutFrames)
		{
			It.RemoveCurrent();
		}
	}

	const int32 ResetAccumulationCounter = CVarToonRayTracerResetAccumulation.GetValueOnRenderThread();
	TSharedPtr<FObjectColorTableState>& ObjectColorTableStatePtr = ObjectColorTables.FindOrAdd(View.Family->Scene);
	if (!ObjectColorTableStatePtr.IsValid())
	{
		ObjectColorTableStatePtr = MakeShared<FObjectColorTableState>();
	}
	// 抽出先のポインタはグラフの実行まで有効である必要があるため、マップの要素ではなく共有ポインタの中身を使う
	FObjectColorTableState& ObjectColorTableState = *ObjectColorTableStatePtr;
	ObjectColorTableState.LastUsedFrameNumber = FrameNumber;
	FRDGBufferRef ObjectColorTableBuffer = nullptr;
	if (ObjectColorTableState.Buffer.IsValid() && ObjectColorTableState.ResetCounter == ResetAccumulationCounter)
	{
		ObjectColorTableBuffer = GraphBuilder.RegisterExternalBuffer(ObjectColorTableState.Buffer, TEXT("ToonRayTracerObjectColorTable"));
	}
	else
	{
		ObjectColorTableBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), ToonObjectColorTable::Size * ToonObjectColorTable::TableStride),
			TEXT("ToonRayTracerObjectColorTable"));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(ObjectColorTableBuffer), 0u);
		ObjectColorTableState.ResetCounter = ResetAccumulationCounter;
	}

	// このフレームに見えた色の合計（毎フレーム空にしてから RayGen で足し込む）
	FRDGBufferRef ObjectColorFrameSums = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), ToonObjectColorTable::Size * ToonObjectColorTable::SumStride),
		TEXT("ToonRayTracerObjectColorFrameSums"));
	AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(ObjectColorFrameSums), 0u);

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

	// Original モードの仕上げのぼかしの元（蓄積用テクスチャと同じく、フレームをまたいで平均する）
	FRDGTextureRef GlowTexture = nullptr;
	if (AccumulationState && AccumulationState->GlowTexture.IsValid() && AccumulationState->AccumulatedFrames > 0)
	{
		GlowTexture = GraphBuilder.RegisterExternalTexture(AccumulationState->GlowTexture, TEXT("ToonRayTracerGlow"));
	}
	else
	{
		const FRDGTextureDesc GlowDesc = FRDGTextureDesc::Create2D(
			ViewRectSize, PF_FloatRGBA, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		GlowTexture = GraphBuilder.CreateTexture(GlowDesc, TEXT("ToonRayTracerGlow"));
	}

	// 動く物体への対応：前のフレームの深度（蓄積用テクスチャと同じく、リセット時は作り直す）
	FRDGTextureRef AccumulationDepthTexture = nullptr;
	if (AccumulationState && AccumulationState->DepthTexture.IsValid() && AccumulationState->AccumulatedFrames > 0)
	{
		AccumulationDepthTexture = GraphBuilder.RegisterExternalTexture(AccumulationState->DepthTexture, TEXT("ToonRayTracerAccumulationDepth"));
	}
	else
	{
		const FRDGTextureDesc DepthDesc = FRDGTextureDesc::Create2D(
			ViewRectSize, PF_R32_FLOAT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		AccumulationDepthTexture = GraphBuilder.CreateTexture(DepthDesc, TEXT("ToonRayTracerAccumulationDepth"));
	}

	// 動く物体への対応：ピクセルの中心で何が見えていたかの要約
	FRDGTextureRef AccumulationSignatureTexture = nullptr;
	if (AccumulationState && AccumulationState->SignatureTexture.IsValid() && AccumulationState->AccumulatedFrames > 0)
	{
		AccumulationSignatureTexture = GraphBuilder.RegisterExternalTexture(AccumulationState->SignatureTexture, TEXT("ToonRayTracerAccumulationSignature"));
	}
	else
	{
		const FRDGTextureDesc SignatureDesc = FRDGTextureDesc::Create2D(
			ViewRectSize, PF_R32_UINT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		AccumulationSignatureTexture = GraphBuilder.CreateTexture(SignatureDesc, TEXT("ToonRayTracerAccumulationSignature"));
	}

	// 動く物体への対応：シーン内で物体が動いていたかのフラグ
	// このフレームの結果は次のフレームで使う（前のフレームのものがなければ「動いていない」として 0 のバッファを使う）
	// [0]：動いている物体が見えていたピクセル数、[1]：深度が変わったピクセル数
	const FRDGBufferDesc SceneMotionDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 2);
	FRDGBufferRef PreviousSceneMotion = nullptr;
	if (AccumulationState && AccumulationState->SceneMotionBuffer.IsValid())
	{
		PreviousSceneMotion = GraphBuilder.RegisterExternalBuffer(AccumulationState->SceneMotionBuffer, TEXT("ToonRayTracerPreviousSceneMotion"));
	}
	else
	{
		PreviousSceneMotion = GraphBuilder.CreateBuffer(SceneMotionDesc, TEXT("ToonRayTracerPreviousSceneMotion"));
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(PreviousSceneMotion), 0u);
	}
	FRDGBufferRef SceneMotion = GraphBuilder.CreateBuffer(SceneMotionDesc, TEXT("ToonRayTracerSceneMotion"));
	AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(SceneMotion), 0u);
	const uint32 AccumulatedFrames = AccumulationState ? AccumulationState->AccumulatedFrames : 0;

	FToonRayGenShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FToonRayGenShader::FParameters>();
	PassParameters->OutputTexture = GraphBuilder.CreateUAV(Output.Texture);
	PassParameters->SceneColorTexture = SceneColor.Texture;
	PassParameters->bToonCharacterOnly = bToonCharacterOnly ? 1u : 0u;
	PassParameters->bBeforeTSR = bBeforeTSR ? 1u : 0u;
	PassParameters->bToonInverseTonemap = CVarToonRayTracerToonInverseTonemap.GetValueOnRenderThread() != 0 ? 1u : 0u;
	PassParameters->ToonEyeAdaptationExposure = View.GetLastEyeAdaptationExposure();
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->AccumulationTexture = GraphBuilder.CreateUAV(AccumulationTexture);
	PassParameters->ToonGlowTexture = GraphBuilder.CreateUAV(GlowTexture);
	PassParameters->AccumulatedFrames = AccumulatedFrames;
	PassParameters->MaxAccumulatedFrames = MaxAccumulatedFrames;
	// TSR の前に描く場合は自前の蓄積がないため、フレーム番号で乱数を変え、TSR に平均させる（面光源のやわらかい影など）
	PassParameters->RandomSeed = AccumulationState ? AccumulationState->RandomSeed : (bBeforeTSR ? View.Family->FrameNumber : 0u);
	PassParameters->AccumulationDepthTexture = GraphBuilder.CreateUAV(AccumulationDepthTexture);
	PassParameters->AccumulationSignatureTexture = GraphBuilder.CreateUAV(AccumulationSignatureTexture);
	PassParameters->PreviousSceneMotion = GraphBuilder.CreateSRV(PreviousSceneMotion);
	PassParameters->SceneMotion = GraphBuilder.CreateUAV(SceneMotion);
	PassParameters->bDetectMotion = bDetectMotion ? 1u : 0u;
	PassParameters->MotionMaxAccumulatedFrames = MotionMaxAccumulatedFrames;
	PassParameters->DebugMotion = CVarToonRayTracerDebugMotion.GetValueOnRenderThread() != 0 ? 1u : 0u;
	PassParameters->GBufferVelocityTexture = bDetectMotion && SceneTextureParameters->GBufferVelocityTexture
		? SceneTextureParameters->GBufferVelocityTexture
		: GSystemTextures.GetBlackDummy(GraphBuilder);
	PassParameters->TLAS = TLAS;
	if (bUseGBufferNormal || bUseGBufferBaseColor || bUseGBufferMetal || bUseGBufferForReflections || bDetectMotion || bSkipMaskedSurfaces || bToonCharacterOnly)
	{
		// GBuffer は描画解像度で作られるため、アップスケール前の描画範囲を渡す
		const FIntRect GBufferViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
		PassParameters->GBufferATexture = SceneTextureParameters->GBufferATexture;
		PassParameters->GBufferBTexture = SceneTextureParameters->GBufferBTexture;
		PassParameters->GBufferDTexture = SceneTextureParameters->GBufferDTexture ? SceneTextureParameters->GBufferDTexture : GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferCTexture = SceneTextureParameters->GBufferCTexture;
		PassParameters->SceneDepthTexture = SceneTextureParameters->SceneDepthTexture;
		PassParameters->CustomDepthTexture = SceneTextureParameters->CustomDepthTexture ? SceneTextureParameters->CustomDepthTexture : GSystemTextures.GetDepthDummy(GraphBuilder);
		PassParameters->GBufferViewRectMin = GBufferViewRect.Min;
		PassParameters->GBufferViewRectSize = GBufferViewRect.Size();
	}
	else
	{
		PassParameters->GBufferATexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferBTexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferDTexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferCTexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->SceneDepthTexture = GSystemTextures.GetDepthDummy(GraphBuilder);
		PassParameters->CustomDepthTexture = GSystemTextures.GetDepthDummy(GraphBuilder);
		PassParameters->GBufferViewRectMin = FIntPoint::ZeroValue;
		PassParameters->GBufferViewRectSize = FIntPoint(1, 1);
	}
	PassParameters->bUseGBufferNormal = bUseGBufferNormal ? 1u : 0u;
	PassParameters->bUseGBufferBaseColor = bUseGBufferBaseColor ? 1u : 0u;
	PassParameters->bUseGBufferMetal = bUseGBufferMetal ? 1u : 0u;
	PassParameters->bUseGBufferForReflections = bUseGBufferForReflections ? 1u : 0u;
	PassParameters->ReflectionNormalSmoothing = ReflectionNormalSmoothing;
	PassParameters->MetalRoughnessThreshold = MetalRoughnessThreshold;
	PassParameters->bSkipMaskedSurfaces = bSkipMaskedSurfaces ? 1u : 0u;
	PassParameters->ToLightDirection = DirectionalLight.ToLightDirection;
	PassParameters->LightColor = FVector3f(DirectionalLight.Color.R, DirectionalLight.Color.G, DirectionalLight.Color.B);
	PassParameters->bHasDirectionalLight = DirectionalLight.bValid ? 1u : 0u;
	PassParameters->ToonBands = ToonBands;
	PassParameters->ToonShadowThreshold = ToonShadowThreshold;
	PassParameters->ToonLitThreshold = ToonLitThreshold;
	PassParameters->ToonEdgeSoftness = ToonEdgeSoftness;
	PassParameters->ToonShadowColor = FVector3f(ToonShadowColor.R, ToonShadowColor.G, ToonShadowColor.B);
	PassParameters->ToonLightColor = PassParameters->LightColor * ToonLightScale;
	PassParameters->ToonSkyTint = ToonSkyTint;
	PassParameters->ToonShadowSaturation = ToonShadowSaturation;
	// マテリアルの発光：このビューのベースパスの直後に写し取ったテクスチャ（なければ黒）
	{
		const FRDGTextureRef* Emissive = EmissiveTextures_RenderThread.Find(&View);
		const bool bHasEmissive = Emissive && *Emissive && bHasGBuffer;
		PassParameters->ToonEmissiveTexture = bHasEmissive ? *Emissive : GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->ToonEmissiveStrength = bHasEmissive ? FMath::Max(CVarToonRayTracerToonEmissiveStrength.GetValueOnRenderThread(), 0.0f) : 0.0f;
	}
	// Original モード
	PassParameters->bToonOriginalMode = CVarToonRayTracerToonOriginal.GetValueOnRenderThread() != 0 ? 1u : 0u;
	PassParameters->ToonOriginalHighlightStrength = FMath::Max(CVarToonRayTracerToonOriginalHighlightStrength.GetValueOnRenderThread(), 0.0f);
	PassParameters->ToonOriginalSparkleStrength = FMath::Max(CVarToonRayTracerToonOriginalSparkleStrength.GetValueOnRenderThread(), 0.0f);
	PassParameters->ToonOriginalShadowBias = CVarToonRayTracerToonOriginalShadowBias.GetValueOnRenderThread();
	PassParameters->ToonOriginalReflectionRimStrength = FMath::Max(CVarToonRayTracerToonOriginalReflectionRimStrength.GetValueOnRenderThread(), 0.0f);
	PassParameters->bToonCastShadows = bToonCastShadows ? 1u : 0u;
	PassParameters->ToonShadowBias = ToonShadowBias;
	// 円錐の全体の角度（度）から、半角の cos にする
	PassParameters->ToonSoftShadowCosAngle = FMath::Cos(FMath::DegreesToRadians(ToonSoftShadowAngle * 0.5f));
	PassParameters->ToonSoftShadowSamples = ToonSoftShadowSamples;
	PassParameters->ToonSoftShadowSteps = ToonSoftShadowSteps;
	PassParameters->ToonGlassShadowOpacity = ToonGlassShadowOpacity;
	PassParameters->ToonIndirectStrength = ToonIndirectStrength;
	PassParameters->ToonIndirectSamples = ToonIndirectSamples;
	PassParameters->ToonShadowNoiseStrength = ToonShadowNoiseStrength;
	PassParameters->ToonShadowNoiseSize = ToonShadowNoiseSize;
	PassParameters->bToonCharacterSelfShadow = bToonCharacterSelfShadow ? 1u : 0u;
	PassParameters->bToonSkin = bToonSkin ? 1u : 0u;
	PassParameters->ToonSkinShadowThreshold = ToonSkinShadowThreshold;
	PassParameters->ToonSkinShadowColor = FVector3f(ToonSkinShadowColor.R, ToonSkinShadowColor.G, ToonSkinShadowColor.B);
	PassParameters->ToonCharacterRimStrength = ToonCharacterRimStrength;
	PassParameters->ToonCharacterRimWidth = ToonCharacterRimWidth;
	PassParameters->ToonCharacterOutlineColorScale = ToonCharacterOutlineColorScale;
	PassParameters->ToonSeeThroughOpacity = ToonSeeThroughOpacity;
	PassParameters->ToonSeeThroughBrightness = ToonSeeThroughBrightness;
	PassParameters->ToonSeeThroughColor = FVector3f(ToonSeeThroughColor_RenderThread.R, ToonSeeThroughColor_RenderThread.G, ToonSeeThroughColor_RenderThread.B);
	PassParameters->ToonSeeThroughColorBlend = FMath::Clamp(CVarToonRayTracerToonSeeThroughColorBlend.GetValueOnRenderThread(), 0.0f, 1.0f);
	PassParameters->ToonSeeThroughDistance = ToonSeeThroughDistance;
	PassParameters->ToonSeeThroughSlotMask = ToonSeeThroughSlotMask;
	PassParameters->ToonEyeSlotMask = ToonEyeSlotMask;
	PassParameters->bToonEyeUnlit = bToonEyeUnlit ? 1u : 0u;
	// 顔の影：顔の位置は Translated World 空間に変換して渡す（double で足してから float にする）
	PassParameters->bToonFaceShadow = bToonFaceShadow ? 1u : 0u;
	// 顔の影の SDF 版：顔ごとに、使うしきい値マップと投影の大きさ・上下の位置を決める
	// プロファイルが「全体の設定に従う」なら全体の設定、「SDF テクスチャで作る」ならプロファイルのテクスチャ（なければ計算で作る）
	// シェーダーのパラメータではテクスチャを配列にできないので、顔の番号ごとのテクスチャのスロットで渡す（使わない顔には黒のダミー）
	{
		static_assert(FToonRayGenShader::MaxFaces == 8, "ToonFaceSDFTexture0 ～ 7 の数と合わせる");
		FRDGTextureRef FaceSDFTextures[FToonRayGenShader::MaxFaces];
		for (int32 Index = 0; Index < FToonRayGenShader::MaxFaces; ++Index)
		{
			const FToonFace* Face = Faces_RenderThread.IsValidIndex(Index) ? &Faces_RenderThread[Index] : nullptr;
			FRHITexture* TextureRHI = nullptr;
			float Scale = ToonFaceSDFScale;
			float OffsetY = ToonFaceSDFOffsetY;
			if (Face && Face->FaceShadowMethod == static_cast<uint32>(EToonFaceShadowMethod::SDF))
			{
				TextureRHI = Face->FaceSDFResource ? Face->FaceSDFResource->GetTexture2DRHI() : nullptr;
				Scale = Face->FaceSDFScale > 0.0f ? Face->FaceSDFScale : Scale;
				OffsetY = Face->bFaceSDFOffsetY ? Face->FaceSDFOffsetY : OffsetY;
			}
			else if (!Face || Face->FaceShadowMethod == static_cast<uint32>(EToonFaceShadowMethod::Global))
			{
				TextureRHI = bToonFaceShadowSDF ? FaceSDFTextureRHI : nullptr;
			}
			PassParameters->ToonFaceSDFParams[Index] = FVector4f(TextureRHI ? 1.0f : 0.0f, FMath::Max(Scale, 0.01f), OffsetY, 0.0f);
			FaceSDFTextures[Index] = TextureRHI
				? RegisterExternalTexture(GraphBuilder, TextureRHI, TEXT("ToonRayTracerFaceSDFTexture"))
				: GSystemTextures.GetBlackDummy(GraphBuilder);
		}
		PassParameters->ToonFaceSDFTexture0 = FaceSDFTextures[0];
		PassParameters->ToonFaceSDFTexture1 = FaceSDFTextures[1];
		PassParameters->ToonFaceSDFTexture2 = FaceSDFTextures[2];
		PassParameters->ToonFaceSDFTexture3 = FaceSDFTextures[3];
		PassParameters->ToonFaceSDFTexture4 = FaceSDFTextures[4];
		PassParameters->ToonFaceSDFTexture5 = FaceSDFTextures[5];
		PassParameters->ToonFaceSDFTexture6 = FaceSDFTextures[6];
		PassParameters->ToonFaceSDFTexture7 = FaceSDFTextures[7];
		PassParameters->ToonFaceSDFTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	}
	PassParameters->ToonFaceSlotMask = ToonFaceSlotMask;
	PassParameters->NumToonFaces = static_cast<uint32>(Faces_RenderThread.Num());
	PassParameters->ToonFaceShadowCurve = ToonFaceShadowCurve;
	PassParameters->ToonFaceShadowSoftness = ToonFaceShadowSoftness;
	PassParameters->ToonFaceShadowFrontBias = ToonFaceShadowFrontBias;
	PassParameters->bToonHairShadow = ToonHairShadowMode;
	PassParameters->ToonHairShadowWidth = ToonHairShadowWidth;
	PassParameters->ToonHairShadowLightShift = ToonHairShadowLightShift;
	PassParameters->ToonHairSlotMask = ToonHairSlotMask;
	PassParameters->ToonHairShadowDistance = ToonHairShadowDistance;
	PassParameters->ToonHairShadowStrength = ToonHairShadowStrength;
	{
		const FVector FacePreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
		// キャラクターごとの見た目：プロファイルで上書きしていない成分は、全体の設定を使う
		const FVector4f GlobalLook[UToonCharacterProfile::NumLookVectors] = {
			FVector4f(ToonShadowColor.R, ToonShadowColor.G, ToonShadowColor.B, ToonShadowSaturation),
			FVector4f(ToonSkinShadowColor.R, ToonSkinShadowColor.G, ToonSkinShadowColor.B, ToonOutlineWidthSkinned),
			FVector4f(ToonCharacterOutlineColorScale, ToonCharacterRimStrength, ToonCharacterRimWidth, ToonFaceShadowCurve),
			FVector4f(ToonFaceShadowFrontBias, ToonEyeShadowBrightness, ToonEyeHighlightStrength, ToonEyeHighlightThreshold),
			FVector4f(ToonHairShadowStrength, ToonHairShadowLightShift, ToonSeeThroughOpacity, ToonSeeThroughBrightness),
			FVector4f(ToonSeeThroughColor_RenderThread.R, ToonSeeThroughColor_RenderThread.G, ToonSeeThroughColor_RenderThread.B,
				FMath::Clamp(CVarToonRayTracerToonSeeThroughColorBlend.GetValueOnRenderThread(), 0.0f, 1.0f)) };
		for (int32 Index = 0; Index < FToonRayGenShader::MaxFaces; ++Index)
		{
			const FToonFace Face = Faces_RenderThread.IsValidIndex(Index) ? Faces_RenderThread[Index] : FToonFace();
			PassParameters->ToonFaceCenter[Index] = FVector4f(FVector3f(Face.Center + FacePreViewTranslation), Face.HalfWidth);
			PassParameters->ToonFaceRight[Index] = FVector4f(Face.Right, static_cast<float>(Face.CharacterId));
			PassParameters->ToonFaceForward[Index] = FVector4f(Face.Forward, Face.Radius);
			PassParameters->ToonCharacterSlots[Index] = Face.Slots;
			PassParameters->ToonCharacterSlots2[Index] = Face.Slots2;
			PassParameters->ToonCharacterSlots3[Index] = Face.Slots3;
			PassParameters->ToonCharacterParams[Index] = Face.Params;
			PassParameters->ToonCharacterParams2[Index] = Face.Params2;
			FVector4f Look[UToonCharacterProfile::NumLookVectors];
			for (int32 Vector = 0; Vector < UToonCharacterProfile::NumLookVectors; ++Vector)
			{
				for (int32 Component = 0; Component < 4; ++Component)
				{
					const bool bOverride = (Face.LookOverrideMask & (1u << (Vector * 4 + Component))) != 0;
					Look[Vector][Component] = bOverride ? Face.Look[Vector][Component] : GlobalLook[Vector][Component];
				}
			}
			PassParameters->ToonCharacterLook0[Index] = Look[0];
			PassParameters->ToonCharacterLook1[Index] = Look[1];
			PassParameters->ToonCharacterLook2[Index] = Look[2];
			PassParameters->ToonCharacterLook3[Index] = Look[3];
			PassParameters->ToonCharacterLook4[Index] = Look[4];
			PassParameters->ToonCharacterLook5[Index] = Look[5];
		}
	}
	PassParameters->ToonEyeTriangleStart = ToonEyeTriangleStart;
	PassParameters->ToonEyeTriangleEnd = ToonEyeTriangleEnd;
	PassParameters->ToonEyeShadowDistance = ToonEyeShadowDistance;
	PassParameters->ToonEyeShadowBrightness = ToonEyeShadowBrightness;
	PassParameters->ToonEyeHighlightStrength = ToonEyeHighlightStrength;
	PassParameters->ToonEyeHighlightThreshold = ToonEyeHighlightThreshold;
	PassParameters->ToonEyeHighlightSearchRadius = ToonEyeHighlightSearchRadius;
	PassParameters->bToonDebugGeometry = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerDebugGeometry.GetValueOnRenderThread(), 0, 8));
	{
		// double のワールド座標を float 2 つ（上位と下位）に分けて渡す（シェーダー側で MakeDFVector3 に戻す）
		const FVector PreViewTranslationForNoise = View.ViewMatrices.GetPreViewTranslation();
		const FVector3f High = FVector3f(PreViewTranslationForNoise);
		PassParameters->PreViewTranslationHigh = High;
		PassParameters->PreViewTranslationLow = FVector3f(PreViewTranslationForNoise - FVector(High));
	}
	PassParameters->ToonHighlightThreshold = ToonHighlightThreshold;
	PassParameters->ToonHighlightStrength = ToonHighlightStrength;
	PassParameters->ToonRimThreshold = ToonRimThreshold;
	PassParameters->ToonRimStrength = ToonRimStrength;
	PassParameters->bToonRimLitSideOnly = bToonRimLitSideOnly ? 1u : 0u;
	PassParameters->bToonOutline = bToonOutline ? 1u : 0u;
	PassParameters->ToonOutlineWidth = ToonOutlineWidth;
	PassParameters->ToonOutlineWidthSkinned = ToonOutlineWidthSkinned;
	PassParameters->ToonOutlineThreshold = ToonOutlineThreshold;
	PassParameters->ToonFaceOutlineTolerance = ToonFaceOutlineTolerance;
	PassParameters->ToonReflectionDepth = ToonReflectionDepth;
	PassParameters->ToonGlassEdgeStrength = ToonGlassEdgeStrength;
	PassParameters->ToonGlassEdgeWidth = ToonGlassEdgeWidth;
	PassParameters->ToonGlassEdgeColor = FVector3f(ToonGlassEdgeColor.R, ToonGlassEdgeColor.G, ToonGlassEdgeColor.B);
	PassParameters->ToonOutlineColor = FVector3f(ToonOutlineColor.R, ToonOutlineColor.G, ToonOutlineColor.B);
	// GPUScene（各メッシュの Custom Primitive Data など）を読むためのシーンのユニフォームバッファ
	FSceneUniformBuffer& SceneUniformBuffer = UE::FXRenderingUtils::CreateSceneUniformBuffer(GraphBuilder, View.Family->Scene);
	PassParameters->Scene = UE::FXRenderingUtils::GetSceneUniformBuffer(GraphBuilder, SceneUniformBuffer);
	PassParameters->ObjectColorTable = GraphBuilder.CreateSRV(ObjectColorTableBuffer);
	PassParameters->ObjectColorFrameSums = GraphBuilder.CreateUAV(ObjectColorFrameSums);
	PassParameters->TraceMode = TraceMode;
	PassParameters->SamplesPerPixel = SamplesPerPixel;
	PassParameters->bStratifiedSampling = bStratifiedSampling ? 1u : 0u;
	PassParameters->ScatterSampling = ScatterSampling;
	// 蓄積しない場合は毎フレームやり直しになるため、常に通常のサンプル数を使う
	const int32 MotionSamplesPerPixelSetting = CVarToonRayTracerMotionSamplesPerPixel.GetValueOnRenderThread();
	PassParameters->MotionSamplesPerPixel = AccumulationState && MotionSamplesPerPixelSetting > 0
		? static_cast<uint32>(FMath::Clamp(MotionSamplesPerPixelSetting, 1, static_cast<int32>(SamplesPerPixel)))
		: SamplesPerPixel;
	PassParameters->ShadingMode = ShadingMode;
	PassParameters->MaxDepth = MaxDepth;
	PassParameters->ClipToTranslatedWorld = FMatrix44f(TranslatedWorldToClipForRays.Inverse());
	PassParameters->TranslatedWorldToClip = FMatrix44f(TranslatedWorldToClipForRays);
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
	const FToonAnalyticScene AnalyticScene = GetAnalyticScene(AnalyticSceneIndex);
	check(AnalyticScene.Spheres.Num() <= FToonRayGenShader::MaxSpheres && AnalyticScene.Quads.Num() <= FToonRayGenShader::MaxQuads
		&& AnalyticScene.Boxes.Num() <= FToonRayGenShader::MaxBoxes);
	const FVector PreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
	for (int32 Index = 0; Index < AnalyticScene.Spheres.Num(); ++Index)
	{
		const FToonSphere& Sphere = AnalyticScene.Spheres[Index];
		const FVector TranslatedCenter = Sphere.Center + PreViewTranslation;
		PassParameters->Spheres[Index] = FVector4f(FVector3f(TranslatedCenter), static_cast<float>(Sphere.Radius));
		// w：トゥーンのハイライトとリムライトの強さ。平らに近い地面（巨大な球）には出さない
		const float ToonAccentStrength = Sphere.Radius > 1000.0 ? 0.0f : 1.0f;
		PassParameters->SphereMaterialParams[Index] = FVector4f(static_cast<float>(Sphere.Material), Sphere.Fuzz, Sphere.RefractionIndex, ToonAccentStrength);
		PassParameters->SphereAlbedo[Index] = FVector4f(Sphere.Albedo.R, Sphere.Albedo.G, Sphere.Albedo.B, 0.0f);
		PassParameters->SphereTextureParams[Index] = FVector4f(static_cast<float>(Sphere.Texture), Sphere.TextureScale, Sphere.TextureUnitLength, Sphere.Density);
		PassParameters->SphereAlbedo2[Index] = FVector4f(Sphere.Albedo2.R, Sphere.Albedo2.G, Sphere.Albedo2.B, 0.0f);
		PassParameters->SphereMotion[Index] = FVector4f(FVector3f(Sphere.Motion), 0.0f);
	}

	// 『The Next Week』第4章：球に貼る画像（読み込めていなければ黒のダミーを渡し、シェーダー側で UV のチェッカーにする）
	FRHITexture* ImageTextureRHI = ImageTextureResource_RenderThread ? ImageTextureResource_RenderThread->GetTexture2DRHI() : nullptr;
	PassParameters->bImageTexture = ImageTextureRHI ? 1u : 0u;
	PassParameters->ImageTexture = ImageTextureRHI
		? RegisterExternalTexture(GraphBuilder, ImageTextureRHI, TEXT("ToonRayTracerImageTexture"))
		: GSystemTextures.GetBlackDummy(GraphBuilder);
	// 横（経度）方向はつながっているので繰り返し、縦（緯度）方向は端で止める
	PassParameters->ImageTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->NumSpheres = AnalyticScene.Spheres.Num();

	// 『The Next Week』第6章：四角形も角の位置だけ Translated World 空間に変換する（2 辺は向きと長さなのでそのまま）
	for (int32 Index = 0; Index < AnalyticScene.Quads.Num(); ++Index)
	{
		const FToonQuad& Quad = AnalyticScene.Quads[Index];
		PassParameters->QuadQ[Index] = FVector4f(FVector3f(Quad.Q + PreViewTranslation), 0.0f);
		PassParameters->QuadU[Index] = FVector4f(FVector3f(Quad.U), 0.0f);
		PassParameters->QuadV[Index] = FVector4f(FVector3f(Quad.V), 0.0f);
		PassParameters->QuadMaterialParams[Index] = FVector4f(static_cast<float>(Quad.Material), 0.0f, 1.0f, 0.0f);
		PassParameters->QuadAlbedo[Index] = FVector4f(Quad.Albedo.R, Quad.Albedo.G, Quad.Albedo.B, 0.0f);
	}
	PassParameters->NumQuads = AnalyticScene.Quads.Num();

	// 『The Next Week』第8〜9章：箱も中心だけ Translated World 空間に変換する
	for (int32 Index = 0; Index < AnalyticScene.Boxes.Num(); ++Index)
	{
		const FToonBox& Box = AnalyticScene.Boxes[Index];
		PassParameters->BoxCenter[Index] = FVector4f(FVector3f(Box.Center + PreViewTranslation), static_cast<float>(FMath::DegreesToRadians(Box.YawDegrees)));
		PassParameters->BoxHalfExtent[Index] = FVector4f(FVector3f(Box.HalfExtent), 0.0f);
		PassParameters->BoxMaterialParams[Index] = FVector4f(static_cast<float>(Box.Material), Box.Density, 0.0f, 0.0f);
		PassParameters->BoxAlbedo[Index] = FVector4f(Box.Albedo.R, Box.Albedo.G, Box.Albedo.B, 0.0f);
	}
	PassParameters->NumBoxes = AnalyticScene.Boxes.Num();

	// 『The Next Week』第10章：地面の箱の格子
	const FToonGroundGrid& Grid = AnalyticScene.GroundGrid;
	PassParameters->bGroundGrid = Grid.bEnabled ? 1u : 0u;
	PassParameters->GroundGridOrigin = FVector4f(FVector3f(Grid.Origin + PreViewTranslation), static_cast<float>(Grid.CellSize));
	PassParameters->GroundGridParams = FVector4f(static_cast<float>(Grid.Count.X), static_cast<float>(Grid.Count.Y), Grid.MinHeight, Grid.MaxHeight);
	PassParameters->GroundGridAlbedo = FVector4f(Grid.Albedo.R, Grid.Albedo.G, Grid.Albedo.B, 0.0f);

	// 『The Next Week』第3章：BVH で探す小さな球の塊（使わないシーンでも、シェーダーにバッファを渡す必要があるため空の内容で作る）
	{
		static const TArray<FVector4f> EmptyData = { FVector4f(0.0f, 0.0f, 0.0f, 0.0f), FVector4f(0.0f, 0.0f, 0.0f, 0.0f) };
		const FToonSphereClusterBvh* Cluster = AnalyticScene.bSphereCluster ? &GetFinalSceneSphereCluster() : nullptr;
		const TArray<FVector4f>& Nodes = Cluster ? Cluster->Nodes : EmptyData;
		const TArray<FVector4f>& ClusterSpheres = Cluster ? Cluster->Spheres : EmptyData;
		FRDGBufferRef NodesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("ToonRayTracerBvhNodes"), sizeof(FVector4f), Nodes.Num(),
			Nodes.GetData(), Nodes.Num() * sizeof(FVector4f), ERDGInitialDataFlags::NoCopy);
		FRDGBufferRef SpheresBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("ToonRayTracerBvhSpheres"), sizeof(FVector4f), ClusterSpheres.Num(),
			ClusterSpheres.GetData(), ClusterSpheres.Num() * sizeof(FVector4f), ERDGInitialDataFlags::NoCopy);
		PassParameters->BvhNodes = GraphBuilder.CreateSRV(NodesBuffer);
		PassParameters->BvhSpheres = GraphBuilder.CreateSRV(SpheresBuffer);
		PassParameters->NumBvhNodes = Cluster ? Cluster->Nodes.Num() / 2 : 0;
		PassParameters->BvhOrigin = FVector4f(FVector3f(Cluster ? Cluster->Origin + PreViewTranslation : FVector::ZeroVector), 0.0f);
		const FLinearColor ClusterAlbedo = Cluster ? Cluster->Albedo : FLinearColor::Black;
		PassParameters->BvhAlbedo = FVector4f(ClusterAlbedo.R, ClusterAlbedo.G, ClusterAlbedo.B, 0.0f);
	}
	PassParameters->bBlackBackground = TraceMode == 0 && AnalyticScene.bBlackBackground ? 1u : 0u;

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	// 使う組み合わせのシェーダーだけを選ぶ（ShadingMode 3 がトゥーン）
	FToonRayGenShader::FPermutationDomain PermutationVector;
	PermutationVector.Set<FToonRayGenShader::FAnalyticSceneDim>(TraceMode == 0);
	PermutationVector.Set<FToonRayGenShader::FToonShadingDim>(ShadingMode == 3);
	TShaderMapRef<FToonRayGenShader> RayGenShader(ShaderMap, PermutationVector);
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
#if TOON_RAY_TRACER_SBT_LIFETIME
	SBTInitializer.Lifetime = ERayTracingShaderBindingTableLifetime::Transient;
#endif
	SBTInitializer.ShaderBindingMode = ERayTracingShaderBindingMode::RTPSO;
	SBTInitializer.HitGroupIndexingMode = ERayTracingHitGroupIndexingMode::Disallow;
	SBTInitializer.LocalBindingDataSize = Initializer.GetMaxLocalBindingDataSize();
	SBTInitializer.NumMissShaderSlots = 1;

	FShaderBindingTableRHIRef SBT = GraphBuilder.RHICmdList.CreateRayTracingShaderBindingTable(SBTInitializer);

	// RDGにレイトレーシング用のパスを登録してディスパッチ
	{
	RDG_EVENT_SCOPE_STAT(GraphBuilder, ToonRayTracerRayGen, "ToonRayTracerRayGen");
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("ToonRayTracing_RayGen %dx%d", Resolution.X, Resolution.Y),
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
	}

	// 汎用化 G4：このフレームに見えた色の合計から物体ごとの平均色を求め、代表色の表を更新する
	{
		FToonObjectColorResolveCS::FParameters* ResolveParameters = GraphBuilder.AllocParameters<FToonObjectColorResolveCS::FParameters>();
		ResolveParameters->ObjectColorFrameSums = GraphBuilder.CreateSRV(ObjectColorFrameSums);
		ResolveParameters->ObjectColorTable = GraphBuilder.CreateUAV(ObjectColorTableBuffer);

		TShaderMapRef<FToonObjectColorResolveCS> ResolveShader(ShaderMap);
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("ToonRayTracing_ObjectColorResolve"),
			ResolveShader,
			ResolveParameters,
			FComputeShaderUtils::GetGroupCount(static_cast<int32>(ToonObjectColorTable::Size), FToonObjectColorResolveCS::ThreadGroupSize));
	}
	GraphBuilder.QueueBufferExtraction(ObjectColorTableBuffer, &ObjectColorTableState.Buffer);

	// Original モード：仕上げに、キャラクターをぼかして重ね（Diffusion Filter・Soft Focus・Bloom）、画面全体へ上から下へのグラデーションを掛ける
	// 背景も含めた画面に掛ける。蓄積の結果には含めない
	if (ShadingMode == 3)
	{
		FToonPostEffectsInputs PostInputs;
		PostInputs.Output = Output;
		PostInputs.AccumulationTexture = AccumulationTexture;
		PostInputs.GlowTexture = GlowTexture;
		PostInputs.LightColor = PassParameters->LightColor;
		// 出力の色の形式は、RayGen の最後の変換に合わせる
		PostInputs.OutputEncoding = bBeforeTSR ? (PassParameters->bToonInverseTonemap != 0 ? 1u : 2u) : 0u;
		PostInputs.EyeAdaptationExposure = PassParameters->ToonEyeAdaptationExposure;
		PostInputs.GradationTopColor = ToonGradationTopColor_RenderThread;
		PostInputs.GradationBottomColor = ToonGradationBottomColor_RenderThread;
		AddToonPostEffectsPasses(GraphBuilder, View, PostInputs);
	}

	// 蓄積テクスチャを次のフレームまで保持する
	// （RDG では、書き込むパスを登録した後でないと抽出を登録できない）
	if (AccumulationState)
	{
		GraphBuilder.QueueTextureExtraction(AccumulationTexture, &AccumulationState->Texture);
		GraphBuilder.QueueTextureExtraction(AccumulationDepthTexture, &AccumulationState->DepthTexture);
		GraphBuilder.QueueTextureExtraction(AccumulationSignatureTexture, &AccumulationState->SignatureTexture);
		GraphBuilder.QueueTextureExtraction(GlowTexture, &AccumulationState->GlowTexture);
		GraphBuilder.QueueBufferExtraction(SceneMotion, &AccumulationState->SceneMotionBuffer);
	}

	// このパスがポストプロセスの最後の場合、エンジンが用意した出力先へ書き戻す必要がある
	if (Inputs.OverrideOutput.IsValid())
	{
		AddDrawTexturePass(GraphBuilder, FScreenPassViewInfo(View), Output, Inputs.OverrideOutput);
		return Inputs.OverrideOutput;
	}

	return MoveTemp(Output);
}