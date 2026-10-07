#include "ToonRayTracerViewExtension.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#include "ToonRayGenShader.h"
#include "PipelineStateCache.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "FXRenderingUtils.h"
#include "SystemTextures.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "RenderingThread.h"
#include "RenderGraphUtils.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"

// stat gpu に「ToonRayTracer」として GPU 時間を表示する
DECLARE_GPU_STAT(ToonRayTracer);

namespace
{
	// 0: シェーダー内の解析的な球（本の第6章）、1: UEシーンの TLAS に TraceRay
	TAutoConsoleVariable<int32> CVarToonRayTracerTraceMode(
		TEXT("r.ToonRayTracer.TraceMode"),
		1,
		TEXT("0: Analytic spheres in shader, 1: TraceRay against the scene TLAS"),
		ECVF_RenderThreadSafe);

	// 本の第8章（アンチエイリアス）の samples_per_pixel に相当
	// カメラや物体が動いている間のサンプル数（0 なら SamplesPerPixel と同じ）
	TAutoConsoleVariable<int32> CVarToonRayTracerMotionSamplesPerPixel(
		TEXT("r.ToonRayTracer.MotionSamplesPerPixel"),
		2,
		TEXT("Samples per pixel while the camera or objects are moving (0: same as SamplesPerPixel). Anti-aliasing is restored by the accumulation once they stop"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerSamplesPerPixel(
		TEXT("r.ToonRayTracer.SamplesPerPixel"),
		4,
		TEXT("Number of jittered camera rays per pixel for anti-aliasing (1-64)"),
		ECVF_RenderThreadSafe);

	// 3（トゥーン）が本来の表示。0 ～ 2 は実装の確認用
	TAutoConsoleVariable<int32> CVarToonRayTracerShadingMode(
		TEXT("r.ToonRayTracer.ShadingMode"),
		3,
		TEXT("3: Toon shading (default). Debug views: 0: Visualize normals, 1: Path traced materials (Ray Tracing in One Weekend chapters 9-11), 2: Directional light N dot L"),
		ECVF_RenderThreadSafe);

	// 本の第9章の max_depth に相当
	TAutoConsoleVariable<int32> CVarToonRayTracerMaxDepth(
		TEXT("r.ToonRayTracer.MaxDepth"),
		10,
		TEXT("Maximum number of ray bounces (1-50) for ShadingMode 1. Toon shading uses r.ToonRayTracer.Toon.ReflectionDepth instead"),
		ECVF_RenderThreadSafe);

	// トゥーン T2：段階的な陰影の設定
	TAutoConsoleVariable<int32> CVarToonRayTracerToonBands(
		TEXT("r.ToonRayTracer.Toon.Bands"),
		2,
		TEXT("Number of shading bands for toon shading (2: shadow / lit, 3: shadow / mid / lit)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowThreshold(
		TEXT("r.ToonRayTracer.Toon.ShadowThreshold"),
		0.0f,
		TEXT("N dot L threshold (-1 to 1) between the shadow band and the next band"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonLitThreshold(
		TEXT("r.ToonRayTracer.Toon.LitThreshold"),
		0.5f,
		TEXT("N dot L threshold (-1 to 1) between the mid band and the lit band (used when Bands is 3)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEdgeSoftness(
		TEXT("r.ToonRayTracer.Toon.EdgeSoftness"),
		0.02f,
		TEXT("Half width (in N dot L) of the smooth transition at band edges"),
		ECVF_RenderThreadSafe);

	// 文字列型のコンソール変数は ECVF_RenderThreadSafe を付けられない（付けるとモジュールの読み込み時にアサートする）ため、
	// ゲームスレッドの BeginRenderViewFamily で読み、レンダーコマンドでレンダースレッドへ渡す
	TAutoConsoleVariable<FString> CVarToonRayTracerToonShadowColor(
		TEXT("r.ToonRayTracer.Toon.ShadowColor"),
		TEXT("0.35,0.4,0.6"),
		TEXT("Linear RGB tint multiplied with the material color in shadow bands, as \"R,G,B\""),
		ECVF_Default);

	const FLinearColor DefaultToonShadowColor(0.35f, 0.4f, 0.6f);

	TAutoConsoleVariable<float> CVarToonRayTracerToonLightScale(
		TEXT("r.ToonRayTracer.Toon.LightScale"),
		1.0f,
		TEXT("Multiplier for the directional light color in the lit band (the light's lux intensity is not used)"),
		ECVF_RenderThreadSafe);

	// トゥーン T3：シャドウレイによる影
	TAutoConsoleVariable<int32> CVarToonRayTracerToonCastShadows(
		TEXT("r.ToonRayTracer.Toon.CastShadows"),
		1,
		TEXT("Trace shadow rays toward the directional light for hard toon shadows (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowBias(
		TEXT("r.ToonRayTracer.Toon.ShadowBias"),
		2.0f,
		TEXT("Offset (cm) of the shadow ray origin along the geometric normal, to avoid self-shadowing artifacts"),
		ECVF_RenderThreadSafe);

	// トゥーン T5：ハイライトとリムライト
	TAutoConsoleVariable<float> CVarToonRayTracerToonHighlightThreshold(
		TEXT("r.ToonRayTracer.Toon.HighlightThreshold"),
		0.97f,
		TEXT("N dot H threshold for the toon highlight. Closer to 1 makes the highlight smaller"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonHighlightStrength(
		TEXT("r.ToonRayTracer.Toon.HighlightStrength"),
		1.0f,
		TEXT("Strength (0-1) of the toon highlight. 0 disables it"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonRimThreshold(
		TEXT("r.ToonRayTracer.Toon.RimThreshold"),
		0.7f,
		TEXT("(1 - N dot V) threshold for the rim light. Closer to 1 makes the rim thinner"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonRimStrength(
		TEXT("r.ToonRayTracer.Toon.RimStrength"),
		0.3f,
		TEXT("Strength of the rim light (added light). 0 disables it"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonRimLitSideOnly(
		TEXT("r.ToonRayTracer.Toon.RimLitSideOnly"),
		1,
		TEXT("Show the rim light only on the lit side (0: everywhere, 1: lit side only)"),
		ECVF_RenderThreadSafe);

	// トゥーン T6：アウトライン
	TAutoConsoleVariable<int32> CVarToonRayTracerToonOutline(
		TEXT("r.ToonRayTracer.Toon.Outline"),
		1,
		TEXT("Draw toon outlines at silhouettes, depth gaps and creases (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineWidth(
		TEXT("r.ToonRayTracer.Toon.OutlineWidth"),
		1.5f,
		TEXT("Outline width in pixels (offset of the neighbor rays) for meshes other than skeletal meshes (backgrounds, props)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineWidthSkinned(
		TEXT("r.ToonRayTracer.Toon.OutlineWidthSkinned"),
		1.0f,
		TEXT("Outline width in pixels for skeletal meshes (characters)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineThreshold(
		TEXT("r.ToonRayTracer.Toon.OutlineThreshold"),
		1.0f,
		TEXT("Outline is drawn where the neighbor ray hits farther than this multiple of the ray offset from the extended center plane. Smaller draws lines at gentler creases"),
		ECVF_RenderThreadSafe);

	// 文字列型のため ECVF_RenderThreadSafe は付けられない。ゲームスレッドで読んでレンダースレッドへ渡す
	TAutoConsoleVariable<FString> CVarToonRayTracerToonOutlineColor(
		TEXT("r.ToonRayTracer.Toon.OutlineColor"),
		TEXT("0.02,0.02,0.04"),
		TEXT("Linear RGB color of the toon outline, as \"R,G,B\""),
		ECVF_Default);

	const FLinearColor DefaultToonOutlineColor(0.02f, 0.02f, 0.04f);

	// トゥーン T7：様式化された反射
	TAutoConsoleVariable<int32> CVarToonRayTracerToonReflectionDepth(
		TEXT("r.ToonRayTracer.Toon.ReflectionDepth"),
		4,
		TEXT("Maximum number of reflections / refractions followed on metal and glass in toon shading (0 shades them as plain materials)"),
		ECVF_RenderThreadSafe);

	// "R,G,B" 形式の文字列を色に変換する（読めなければ Default を返す）
	FLinearColor ParseLinearColor(const FString& Text, const FLinearColor& Default)
	{
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT(","), true);
		if (Parts.Num() != 3)
		{
			return Default;
		}
		return FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
	}

	TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferNormal(
		TEXT("r.ToonRayTracer.UseGBufferNormal"),
		1,
		TEXT("Use smooth normals from the GBuffer for the first hit of camera rays (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	// 汎用化 G1：UE のマテリアルの色（GBuffer のベースカラー）を素材の色として使う
	TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferBaseColor(
		TEXT("r.ToonRayTracer.UseGBufferBaseColor"),
		1,
		TEXT("Use the material base color from the GBuffer for the first hit of camera rays on meshes without a material set in Custom Primitive Data (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	// 汎用化 G3：反射・屈折した先も、カメラから見えている点なら GBuffer の値を使う
	TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferForReflections(
		TEXT("r.ToonRayTracer.UseGBufferForReflections"),
		1,
		TEXT("For reflected / refracted hits that are visible on screen, use the GBuffer base color, normal and metal detection (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	// 反射・屈折した先の法線をなめらかにする補助レイの間隔（ポリゴン1枚分程度にすると明暗の境目の階段が目立たなくなる）
	TAutoConsoleVariable<float> CVarToonRayTracerReflectionNormalSmoothing(
		TEXT("r.ToonRayTracer.ReflectionNormalSmoothing"),
		15.0f,
		TEXT("Spacing (cm) of the probe rays used to smooth normals on reflected / refracted hits. 0 uses flat per-triangle normals"),
		ECVF_RenderThreadSafe);

	// 汎用化 G2：UE のマテリアルのメタリックとラフネスから金属を自動判定する
	TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferMetal(
		TEXT("r.ToonRayTracer.UseGBufferMetal"),
		1,
		TEXT("Treat glossy metallic materials (from the GBuffer) as mirror metals on meshes without a material set in Custom Primitive Data (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerMetalRoughnessThreshold(
		TEXT("r.ToonRayTracer.MetalRoughnessThreshold"),
		0.4f,
		TEXT("Maximum roughness (0-1) of a metallic material to be treated as a mirror metal. Rougher metals are shaded as plain materials"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerAccumulate(
		TEXT("r.ToonRayTracer.Accumulate"),
		1,
		TEXT("Accumulate samples across frames while the camera is still (0: off, 1: on). Toggle to reset."),
		ECVF_RenderThreadSafe);

	// 値が変わると蓄積をリセットする（エディタパネルの「蓄積をリセット」ボタンが 1 ずつ増やす）
	// レベル上のオブジェクトを動かしただけでは自動でリセットされないため、その場合に使う
	TAutoConsoleVariable<int32> CVarToonRayTracerResetAccumulation(
		TEXT("r.ToonRayTracer.ResetAccumulation"),
		0,
		TEXT("Changing this value resets the accumulation (e.g. after moving objects in the level)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerMaxAccumulatedFrames(
		TEXT("r.ToonRayTracer.MaxAccumulatedFrames"),
		64,
		TEXT("Upper limit of accumulated frames. After reaching it, the accumulated result is shown without tracing new rays"),
		ECVF_RenderThreadSafe);

	// カメラのレイが、GBuffer に描かれていない面（マテリアルの透過で抜けた部分など）を通り抜けるか
	TAutoConsoleVariable<int32> CVarToonRayTracerSkipMaskedSurfaces(
		TEXT("r.ToonRayTracer.SkipMaskedSurfaces"),
		1,
		TEXT("Camera rays pass through surfaces that are not in the GBuffer (e.g. cut out by masked materials)"),
		ECVF_RenderThreadSafe);

	// 動く物体への対応
	TAutoConsoleVariable<int32> CVarToonRayTracerDetectMotion(
		TEXT("r.ToonRayTracer.DetectMotion"),
		1,
		TEXT("Detect moving objects (GBuffer velocity and depth changes) and restart the accumulation of the affected pixels"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerMotionMaxAccumulatedFrames(
		TEXT("r.ToonRayTracer.MotionMaxAccumulatedFrames"),
		8,
		TEXT("Upper limit of accumulated frames while something is moving in the scene. Smaller values shorten the trails of moving shadows and reflections"),
		ECVF_RenderThreadSafe);

	// 確認用：蓄積をやり直した理由を色で表示し、カメラや設定の変更による全体のリセットをログに出す
	TAutoConsoleVariable<int32> CVarToonRayTracerDebugMotion(
		TEXT("r.ToonRayTracer.Debug.Motion"),
		0,
		TEXT("Debug view of accumulation resets. White: camera/settings, red: moving object, green: depth change, blue: signature change. Also logs the reason of full resets"),
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

	// テクスチャの種類（シェーダー側の TOON_TEXTURE_* と一致させる）
	enum class EToonTextureType : uint32
	{
		Solid = 0,				// 単色（本の solid_color）
		Checker = 1,			// 空間のチェッカー（本の checker_texture）
		SphereUVChecker = 2,	// 球のテクスチャ座標（UV）上のチェッカー
	};

	struct FToonSphere
	{
		FVector Center;				// ワールド座標（cm）
		double Radius;				// 半径（cm）
		EToonMaterialType Material;
		FLinearColor Albedo;		// 反射率（本の albedo）
		float Fuzz;					// 金属の反射のぼけ具合（0 ～ 1）
		float RefractionIndex;		// 誘電体の屈折率（本の refraction_index）
		// 『The Next Week』第4章：テクスチャ
		EToonTextureType Texture = EToonTextureType::Solid;
		float TextureScale = 1.0f;	// チェッカー：1 マスの辺の長さ（cm）、UV のチェッカー：経度方向のマスの数
		FLinearColor Albedo2 = FLinearColor::Black;	// チェッカーのもう一方の色
	};

	// 『Ray Tracing in One Weekend』第11章のシーンをUEの単位系（cm, Z-up）に置き換えたもの
	// 本の座標（x: 右, y: 上, -z: 奥）を UE（+Y: 右, +Z: 上, +X: 奥）に対応させ、100倍して cm にする
	// 本では球の中心が y=0、地面の上面が y=-0.5 のため、全体を 50cm 持ち上げて地面の上面を z=0 にしている
	const FToonSphere GToonSpheres[] =
	{
		// 地面：『The Next Week』第4章のチェッカー（本の checker_texture(0.32, color(.2, .3, .1), color(.9, .9, .9))）
		{ FVector(0.0, 0.0, -10000.0), 10000.0, EToonMaterialType::Lambertian, FLinearColor(0.2f, 0.3f, 0.1f), 0.0f, 1.0f,
			EToonTextureType::Checker, 32.0f, FLinearColor(0.9f, 0.9f, 0.9f) },
		// 中央：青いランバート。第4章の球のテクスチャ座標の確認用に、UV 上のチェッカーにしている
		{ FVector(120.0, 0.0, 50.0), 50.0, EToonMaterialType::Lambertian, FLinearColor(0.1f, 0.2f, 0.5f), 0.0f, 1.0f,
			EToonTextureType::SphereUVChecker, 16.0f, FLinearColor(0.8f, 0.8f, 0.8f) },
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

// ゲームスレッドで、ビューファミリーのワールドにある Directional Light を探す
// 大気の太陽として使われているライトを優先し、なければ最も強いライトを使う
ToonRayTracerViewExtension::FToonDirectionalLight ToonRayTracerViewExtension::FindDirectionalLight(const FSceneViewFamily& ViewFamily)
{
	FToonDirectionalLight Result;

	const UWorld* World = ViewFamily.Scene ? ViewFamily.Scene->GetWorld() : nullptr;
	if (!World)
	{
		return Result;
	}

	const UDirectionalLightComponent* BestLight = nullptr;
	for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
	{
		const UDirectionalLightComponent* Light = *It;
		if (Light->GetWorld() != World || !Light->IsRegistered() || !Light->IsVisible() || !Light->bAffectsWorld || Light->Intensity <= 0.0f)
		{
			continue;
		}

		const bool bIsSun = Light->IsUsedAsAtmosphereSunLight() && Light->GetAtmosphereSunLightIndex() == 0;
		const bool bBestIsSun = BestLight && BestLight->IsUsedAsAtmosphereSunLight() && BestLight->GetAtmosphereSunLightIndex() == 0;
		if (!BestLight
			|| (bIsSun && !bBestIsSun)
			|| (bIsSun == bBestIsSun && Light->Intensity > BestLight->Intensity))
		{
			BestLight = Light;
		}
	}

	if (BestLight)
	{
		// GetDirection() は光が進む向きなので、光源へ向かう方向はその逆
		Result.ToLightDirection = FVector3f(-BestLight->GetDirection().GetSafeNormal());
		Result.Color = BestLight->GetLightColor();
		Result.Intensity = BestLight->Intensity;
		Result.bValid = true;
	}
	return Result;
}

void ToonRayTracerViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// ライトの情報はゲームスレッド側にしかないため、ここで取得してレンダースレッドへ渡す
	// レンダーコマンドは順番に実行されるので、このビューファミリーの描画より前に反映される
	FToonDirectionalLight Light = FindDirectionalLight(InViewFamily);
	// 文字列型のコンソール変数もゲームスレッドでしか読めないため、ここで色に変換して一緒に渡す
	const FLinearColor ShadowColor = ParseLinearColor(CVarToonRayTracerToonShadowColor.GetValueOnGameThread(), DefaultToonShadowColor);
	const FLinearColor OutlineColor = ParseLinearColor(CVarToonRayTracerToonOutlineColor.GetValueOnGameThread(), DefaultToonOutlineColor);
	ENQUEUE_RENDER_COMMAND(ToonRayTracerUpdateGameThreadState)(
		[this, Light, ShadowColor, OutlineColor](FRHICommandListImmediate&)
		{
			DirectionalLight_RenderThread = Light;
			ToonShadowColor_RenderThread = ShadowColor;
			ToonOutlineColor_RenderThread = OutlineColor;
		});
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

	// このパスで追加する処理（色の表のクリア・解決、蓄積、レイトレーシング）の GPU 時間をまとめて計測する
	RDG_EVENT_SCOPE_STAT(GraphBuilder, ToonRayTracer, "ToonRayTracer");

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
	const bool bDetectMotion = bHasGBuffer && CVarToonRayTracerDetectMotion.GetValueOnRenderThread() != 0;
	const bool bSkipMaskedSurfaces = bHasGBuffer && CVarToonRayTracerSkipMaskedSurfaces.GetValueOnRenderThread() != 0;
	const uint32 MotionMaxAccumulatedFrames = static_cast<uint32>(FMath::Max(CVarToonRayTracerMotionMaxAccumulatedFrames.GetValueOnRenderThread(), 1));
	const FIntPoint ViewRectSize = Output.ViewRect.Size();

	// TAA 用のジッターを含まない行列でレイを作る
	// （ジッターは毎フレーム変わるため、含めると蓄積が毎フレームリセットされてしまう。AA は自前のサンプリングで行う）
	const FMatrix& WorldToView = View.ViewMatrices.GetWorldToView();
	const FMatrix& ViewToClipNoAA = View.ViewMatrices.GetViewToClipNoAA();
	const FMatrix ClipToTranslatedWorldNoAA = (View.ViewMatrices.GetTranslatedViewMatrix() * ViewToClipNoAA).Inverse();

	// フレームをまたいだ蓄積（カメラか設定が変わったらリセット）
	// ビューの状態を持たないビュー（サムネイル描画など）では蓄積しない
	const uint32 FrameNumber = View.Family->FrameNumber;
	// トゥーンの設定
	const uint32 ToonBands = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonBands.GetValueOnRenderThread(), 2, 3));
	const float ToonShadowThreshold = CVarToonRayTracerToonShadowThreshold.GetValueOnRenderThread();
	const float ToonLitThreshold = CVarToonRayTracerToonLitThreshold.GetValueOnRenderThread();
	const float ToonEdgeSoftness = FMath::Max(CVarToonRayTracerToonEdgeSoftness.GetValueOnRenderThread(), 0.0f);
	const FLinearColor ToonShadowColor = ToonShadowColor_RenderThread;
	const float ToonLightScale = FMath::Max(CVarToonRayTracerToonLightScale.GetValueOnRenderThread(), 0.0f);
	const bool bToonCastShadows = CVarToonRayTracerToonCastShadows.GetValueOnRenderThread() != 0;
	const float ToonShadowBias = FMath::Max(CVarToonRayTracerToonShadowBias.GetValueOnRenderThread(), 0.0f);
	const float ToonHighlightThreshold = CVarToonRayTracerToonHighlightThreshold.GetValueOnRenderThread();
	const float ToonHighlightStrength = FMath::Clamp(CVarToonRayTracerToonHighlightStrength.GetValueOnRenderThread(), 0.0f, 1.0f);
	const float ToonRimThreshold = CVarToonRayTracerToonRimThreshold.GetValueOnRenderThread();
	const float ToonRimStrength = FMath::Max(CVarToonRayTracerToonRimStrength.GetValueOnRenderThread(), 0.0f);
	const bool bToonRimLitSideOnly = CVarToonRayTracerToonRimLitSideOnly.GetValueOnRenderThread() != 0;
	const bool bToonOutline = CVarToonRayTracerToonOutline.GetValueOnRenderThread() != 0;
	const float ToonOutlineWidth = FMath::Max(CVarToonRayTracerToonOutlineWidth.GetValueOnRenderThread(), 0.0f);
	const float ToonOutlineWidthSkinned = FMath::Max(CVarToonRayTracerToonOutlineWidthSkinned.GetValueOnRenderThread(), 0.0f);
	const float ToonOutlineThreshold = FMath::Max(CVarToonRayTracerToonOutlineThreshold.GetValueOnRenderThread(), 0.0f);
	const FLinearColor ToonOutlineColor = ToonOutlineColor_RenderThread;
	const uint32 ToonReflectionDepth = static_cast<uint32>(FMath::Clamp(CVarToonRayTracerToonReflectionDepth.GetValueOnRenderThread(), 0, 16));

	// ライトの向きや色、トゥーンの設定が変わったときも蓄積をリセットする
	const FToonDirectionalLight& DirectionalLight = DirectionalLight_RenderThread;
	const uint32 LightHash = HashCombine(HashCombine(GetTypeHash(DirectionalLight.ToLightDirection), GetTypeHash(DirectionalLight.Color)),
		HashCombine(GetTypeHash(DirectionalLight.Intensity), GetTypeHash(DirectionalLight.bValid)));
	const uint32 ToonHash = HashCombine(
		HashCombine(HashCombine(GetTypeHash(ToonBands), GetTypeHash(ToonShadowThreshold)), HashCombine(GetTypeHash(ToonLitThreshold), GetTypeHash(ToonEdgeSoftness))),
		HashCombine(HashCombine(GetTypeHash(ToonShadowColor), GetTypeHash(ToonLightScale)), HashCombine(GetTypeHash(bToonCastShadows), GetTypeHash(ToonShadowBias))));
	const uint32 ToonHighlightHash = HashCombine(
		HashCombine(GetTypeHash(ToonHighlightThreshold), GetTypeHash(ToonHighlightStrength)),
		HashCombine(HashCombine(GetTypeHash(ToonRimThreshold), GetTypeHash(ToonRimStrength)), GetTypeHash(bToonRimLitSideOnly)));
	const uint32 ToonOutlineHash = HashCombine(
		HashCombine(HashCombine(GetTypeHash(bToonOutline), GetTypeHash(ToonOutlineWidth)), GetTypeHash(ToonOutlineWidthSkinned)),
		HashCombine(HashCombine(GetTypeHash(ToonOutlineThreshold), GetTypeHash(ToonOutlineColor)), GetTypeHash(ToonReflectionDepth)));
	const uint32 SettingsHash = HashCombine(HashCombine(HashCombine(
		HashCombine(HashCombine(GetTypeHash(TraceMode), GetTypeHash(SamplesPerPixel)), HashCombine(GetTypeHash(ShadingMode), GetTypeHash(MaxDepth))),
		HashCombine(HashCombine(GetTypeHash(bUseGBufferNormal), GetTypeHash(bUseGBufferBaseColor)), HashCombine(HashCombine(GetTypeHash(bUseGBufferMetal), GetTypeHash(MetalRoughnessThreshold)), HashCombine(HashCombine(GetTypeHash(bUseGBufferForReflections), GetTypeHash(ReflectionNormalSmoothing)), GetTypeHash(bSkipMaskedSurfaces))))), LightHash), HashCombine(HashCombine(ToonHash, ToonHighlightHash), ToonOutlineHash))
		^ GetTypeHash(CVarToonRayTracerResetAccumulation.GetValueOnRenderThread());
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
			|| !AccumulationState->SignatureTexture.IsValid();
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
	PassParameters->AccumulationTexture = GraphBuilder.CreateUAV(AccumulationTexture);
	PassParameters->AccumulatedFrames = AccumulatedFrames;
	PassParameters->MaxAccumulatedFrames = MaxAccumulatedFrames;
	PassParameters->RandomSeed = AccumulationState ? AccumulationState->RandomSeed : 0;
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
	if (bUseGBufferNormal || bUseGBufferBaseColor || bUseGBufferMetal || bUseGBufferForReflections || bDetectMotion || bSkipMaskedSurfaces)
	{
		// GBuffer は描画解像度で作られるため、アップスケール前の描画範囲を渡す
		const FIntRect GBufferViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
		PassParameters->GBufferATexture = SceneTextureParameters->GBufferATexture;
		PassParameters->GBufferBTexture = SceneTextureParameters->GBufferBTexture;
		PassParameters->GBufferCTexture = SceneTextureParameters->GBufferCTexture;
		PassParameters->SceneDepthTexture = SceneTextureParameters->SceneDepthTexture;
		PassParameters->GBufferViewRectMin = GBufferViewRect.Min;
		PassParameters->GBufferViewRectSize = GBufferViewRect.Size();
	}
	else
	{
		PassParameters->GBufferATexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferBTexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->GBufferCTexture = GSystemTextures.GetBlackDummy(GraphBuilder);
		PassParameters->SceneDepthTexture = GSystemTextures.GetDepthDummy(GraphBuilder);
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
	PassParameters->bToonCastShadows = bToonCastShadows ? 1u : 0u;
	PassParameters->ToonShadowBias = ToonShadowBias;
	PassParameters->ToonHighlightThreshold = ToonHighlightThreshold;
	PassParameters->ToonHighlightStrength = ToonHighlightStrength;
	PassParameters->ToonRimThreshold = ToonRimThreshold;
	PassParameters->ToonRimStrength = ToonRimStrength;
	PassParameters->bToonRimLitSideOnly = bToonRimLitSideOnly ? 1u : 0u;
	PassParameters->bToonOutline = bToonOutline ? 1u : 0u;
	PassParameters->ToonOutlineWidth = ToonOutlineWidth;
	PassParameters->ToonOutlineWidthSkinned = ToonOutlineWidthSkinned;
	PassParameters->ToonOutlineThreshold = ToonOutlineThreshold;
	PassParameters->ToonReflectionDepth = ToonReflectionDepth;
	PassParameters->ToonOutlineColor = FVector3f(ToonOutlineColor.R, ToonOutlineColor.G, ToonOutlineColor.B);
	// GPUScene（各メッシュの Custom Primitive Data など）を読むためのシーンのユニフォームバッファ
	FSceneUniformBuffer& SceneUniformBuffer = UE::FXRenderingUtils::CreateSceneUniformBuffer(GraphBuilder, View.Family->Scene);
	PassParameters->Scene = UE::FXRenderingUtils::GetSceneUniformBuffer(GraphBuilder, SceneUniformBuffer);
	PassParameters->ObjectColorTable = GraphBuilder.CreateSRV(ObjectColorTableBuffer);
	PassParameters->ObjectColorFrameSums = GraphBuilder.CreateUAV(ObjectColorFrameSums);
	PassParameters->TraceMode = TraceMode;
	PassParameters->SamplesPerPixel = SamplesPerPixel;
	// 蓄積しない場合は毎フレームやり直しになるため、常に通常のサンプル数を使う
	const int32 MotionSamplesPerPixelSetting = CVarToonRayTracerMotionSamplesPerPixel.GetValueOnRenderThread();
	PassParameters->MotionSamplesPerPixel = AccumulationState && MotionSamplesPerPixelSetting > 0
		? static_cast<uint32>(FMath::Clamp(MotionSamplesPerPixelSetting, 1, static_cast<int32>(SamplesPerPixel)))
		: SamplesPerPixel;
	PassParameters->ShadingMode = ShadingMode;
	PassParameters->MaxDepth = MaxDepth;
	PassParameters->ClipToTranslatedWorld = FMatrix44f(ClipToTranslatedWorldNoAA);
	PassParameters->TranslatedWorldToClip = FMatrix44f(View.ViewMatrices.GetTranslatedViewMatrix() * ViewToClipNoAA);
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
		// w：トゥーンのハイライトとリムライトの強さ。平らに近い地面（巨大な球）には出さない
		const float ToonAccentStrength = Index == 0 ? 0.0f : 1.0f;
		PassParameters->SphereMaterialParams[Index] = FVector4f(static_cast<float>(Sphere.Material), Sphere.Fuzz, Sphere.RefractionIndex, ToonAccentStrength);
		PassParameters->SphereAlbedo[Index] = FVector4f(Sphere.Albedo.R, Sphere.Albedo.G, Sphere.Albedo.B, 0.0f);
		PassParameters->SphereTextureParams[Index] = FVector4f(static_cast<float>(Sphere.Texture), Sphere.TextureScale, 0.0f, 0.0f);
		PassParameters->SphereAlbedo2[Index] = FVector4f(Sphere.Albedo2.R, Sphere.Albedo2.G, Sphere.Albedo2.B, 0.0f);
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

	// 蓄積テクスチャを次のフレームまで保持する
	// （RDG では、書き込むパスを登録した後でないと抽出を登録できない）
	if (AccumulationState)
	{
		GraphBuilder.QueueTextureExtraction(AccumulationTexture, &AccumulationState->Texture);
		GraphBuilder.QueueTextureExtraction(AccumulationDepthTexture, &AccumulationState->DepthTexture);
		GraphBuilder.QueueTextureExtraction(AccumulationSignatureTexture, &AccumulationState->SignatureTexture);
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