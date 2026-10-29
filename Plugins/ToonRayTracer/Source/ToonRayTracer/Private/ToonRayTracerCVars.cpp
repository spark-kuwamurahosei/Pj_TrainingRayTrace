#include "ToonRayTracerCVars.h"

namespace ToonRayTracerCVars
{
	// 0: シェーダー内の解析的な球（本の第6章）、1: UEシーンの TLAS に TraceRay
	TAutoConsoleVariable<int32> CVarToonRayTracerTraceMode(
		TEXT("r.ToonRayTracer.TraceMode"),
		1,
		TEXT("0: Analytic spheres in shader, 1: TraceRay against the scene TLAS"),
		ECVF_RenderThreadSafe);

	// 解析的な物体のシーン（TraceMode 0 のときに使う）
	TAutoConsoleVariable<int32> CVarToonRayTracerAnalyticScene(
		TEXT("r.ToonRayTracer.AnalyticScene"),
		0,
		TEXT("Scene used when TraceMode is 0. 0: One Weekend chapter 11 (with Next Week textures), 1: Next Week chapter 7 simple light, 2: Next Week chapter 7 Cornell box, 3: Next Week chapter 8 Cornell box with boxes, 4: Next Week chapter 9 Cornell smoke, 5: Next Week chapter 10 final scene (simplified), 6: Rest of Your Life chapter 12 Cornell box with an aluminum box and a glass sphere"),
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

	// 『The Rest of Your Life』：確認用の「本の第9〜11章」モードで、拡散反射の方向をどう選ぶか
	TAutoConsoleVariable<int32> CVarToonRayTracerScatterSampling(
		TEXT("r.ToonRayTracer.ScatterSampling"),
		4,
		TEXT("How diffuse scatter directions are sampled in ShadingMode 1 (0: normal + random unit vector, 1: uniform hemisphere, 2: cosine-weighted with an orthonormal basis, 3: toward the light quad, 4: mixture of light and cosine)"),
		ECVF_RenderThreadSafe);

	// 『The Rest of Your Life』第3章：ピクセル内のサンプル位置の層化
	TAutoConsoleVariable<int32> CVarToonRayTracerStratifiedSampling(
		TEXT("r.ToonRayTracer.StratifiedSampling"),
		1,
		TEXT("Stratify the sample positions within a pixel (one sample per cell of a sqrt(N) x sqrt(N) grid)"),
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

	// Original ルック フェーズ1：トゥーンで塗るのをキャラクター（スケルタルメッシュ）だけにし、背景は通常描画のままにする
	TAutoConsoleVariable<int32> CVarToonRayTracerToonCharacterOnly(
		TEXT("r.ToonRayTracer.Toon.CharacterOnly"),
		0,
		TEXT("Apply toon shading only to characters (skeletal meshes, or meshes with Custom Primitive Data [12] > 0). Other pixels keep the regular rendering"),
		ECVF_RenderThreadSafe);

	// Original ルック フェーズ3：キャラクターだけトゥーンにするとき、TSR の前（描画解像度、トーンマップ前）に描く
	TAutoConsoleVariable<int32> CVarToonRayTracerToonBeforeTSR(
		TEXT("r.ToonRayTracer.Toon.BeforeTSR"),
		1,
		TEXT("When CharacterOnly is on, render the toon characters before TSR (render resolution, HDR before tonemapping) so TSR anti-aliases and upscales them"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonInverseTonemap(
		TEXT("r.ToonRayTracer.Toon.InverseTonemap"),
		1,
		TEXT("When rendering before TSR, write colors with the inverse of the Filmic tonemapper applied so the result matches rendering after tonemapping"),
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

	// Original ルック フェーズ2：肌（UE のマテリアルのシェーディングモデルが Subsurface / Preintegrated Skin / Subsurface Profile の面）の塗り
	TAutoConsoleVariable<int32> CVarToonRayTracerToonSkin(
		TEXT("r.ToonRayTracer.Toon.Skin"),
		1,
		TEXT("Treat surfaces whose shading model is Subsurface, Preintegrated Skin or Subsurface Profile as skin, using SkinShadowThreshold and SkinShadowColor"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonSkinShadowThreshold(
		TEXT("r.ToonRayTracer.Toon.SkinShadowThreshold"),
		-0.3f,
		TEXT("N dot L at the shadow boundary for skin. Lower than ShadowThreshold keeps faces lit with fewer small shadows"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonSkinShadowColor(
		TEXT("r.ToonRayTracer.Toon.SkinShadowColor"),
		TEXT("0.62,0.42,0.45"),
		TEXT("Linear RGB tint multiplied with the skin color in shadow bands, as \"R,G,B\""),
		ECVF_Default);

	// 耳や指の光の透け：逆光のとき、肌の薄い部分（耳たぶや指先）だけを、赤みのある明るい色でくっきり塗る
	TAutoConsoleVariable<int32> CVarToonRayTracerToonTranslucency(
		TEXT("r.ToonRayTracer.Toon.Translucency"),
		1,
		TEXT("Backlit skin translucency: thin parts of skin (ears, fingers) facing away from the light glow with TranslucencyColor in toon steps (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonTranslucencyStrength(
		TEXT("r.ToonRayTracer.Toon.TranslucencyStrength"),
		0.8f,
		TEXT("Strength of the backlit skin translucency"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonTranslucencyThickness(
		TEXT("r.ToonRayTracer.Toon.TranslucencyThickness"),
		2.0f,
		TEXT("Thickness (cm) toward the light below which skin glows. Half of it or less is the brighter step"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonTranslucencyAngle(
		TEXT("r.ToonRayTracer.Toon.TranslucencyAngle"),
		60.0f,
		TEXT("Translucency appears when the angle between the view direction and the direction to the light is within this (degrees)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonTranslucencyColor(
		TEXT("r.ToonRayTracer.Toon.TranslucencyColor"),
		TEXT("1.0,0.45,0.3"),
		TEXT("Linear RGB color of the light passing through thin skin, multiplied with the skin color and the light color, as \"R,G,B\""),
		ECVF_Default);

	// Original ルック フェーズ2：キャラクターのリムライトと線の色
	TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterRimStrength(
		TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"),
		1.5f,
		TEXT("Strength of the screen-space rim light on characters (lit-side silhouette with a constant width). 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterRimWidth(
		TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"),
		2.5f,
		TEXT("Width in pixels of the character rim light"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonShadowSimplify(
		TEXT("r.ToonRayTracer.Toon.ShadowSimplify"),
		1,
		TEXT("Simplify the shape of cast shadows: also casts shadow rays from points shifted by ShadowSimplifyRadius perpendicular to the light, and treats the point as lit if any of them reaches the light. Shadows of thin objects (hair tips etc.) disappear and the edges of large shadows become simple. 0: off, 1: shadows on characters only, 2: all surfaces"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowSimplifyRadius(
		TEXT("r.ToonRayTracer.Toon.ShadowSimplifyRadius"),
		8.0f,
		TEXT("Simplify the shape of cast shadows: shadow ray thickness (cm). Objects thinner than about twice this value cast no shadow"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonCrease(
		TEXT("r.ToonRayTracer.Toon.Crease"),
		1,
		TEXT("Crease lines: ink lines in concave corners (cloth folds, collar folds, between fingers), found with short rays along the surface. 0: off, 1: characters only (not the face or eyes), 2: all surfaces"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCreaseDistance(
		TEXT("r.ToonRayTracer.Toon.CreaseDistance"),
		20.0f,
		TEXT("Crease lines: length (cm) of the rays along the surface. Roughly the width of the line"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCreaseLift(
		TEXT("r.ToonRayTracer.Toon.CreaseLift"),
		0.25f,
		TEXT("Crease lines: how much the rays are lifted from the surface (sine of the angle). Larger values only draw lines in sharper corners"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonCreaseSamples(
		TEXT("r.ToonRayTracer.Toon.CreaseSamples"),
		6,
		TEXT("Crease lines: number of rays per pixel (1-16). The directions change every frame and are averaged by the accumulation or TSR"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCreaseStrength(
		TEXT("r.ToonRayTracer.Toon.CreaseStrength"),
		1.0f,
		TEXT("Crease lines: strength of the line (0-1). Drawn with the outline color"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonBeforeTSRMaxBrightness(
		TEXT("r.ToonRayTracer.Toon.BeforeTSRMaxBrightness"),
		0.9f,
		TEXT("When drawing before TSR with the inverse tonemap, limits the brightest channel of the toon color (display value, 0-1) to this value. The inverse tonemap makes colors near white extremely bright, so bloom makes them glow; 1 means no limit"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonReflectionOutline(
		TEXT("r.ToonRayTracer.Toon.ReflectionOutline"),
		1,
		TEXT("Outlines inside mirror reflections: for objects seen in a mirror-like metal surface, compares the reflected hit with the reflections of neighbor pixels and draws the outline (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonReflectionCharacters(
		TEXT("r.ToonRayTracer.Toon.ReflectionCharacters"),
		1,
		TEXT("With CharacterOnly, also paints characters reflected in mirror-like metal surfaces with toon shading and outlines (pixels whose reflection hits a character are drawn by the toon pass instead of the normal rendering) (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	// つやのある床への映り込み：上を向いたつやのある面（GBuffer のラフネスが小さい面）に、映った物体をトゥーンのまま薄く重ねる
	TAutoConsoleVariable<int32> CVarToonRayTracerToonFloorReflection(
		TEXT("r.ToonRayTracer.Toon.FloorReflection"),
		0,
		TEXT("Floor reflection: upward-facing glossy surfaces (GBuffer roughness at or below FloorReflectionRoughness) reflect the scene with toon shading and outlines. With CharacterOnly, only reflected characters are blended over the normal rendering (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFloorReflectionStrength(
		TEXT("r.ToonRayTracer.Toon.FloorReflectionStrength"),
		0.5f,
		TEXT("Strength of the floor reflection on a perfectly smooth floor. Weaker for rougher floors, and stronger at grazing angles"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFloorReflectionRoughness(
		TEXT("r.ToonRayTracer.Toon.FloorReflectionRoughness"),
		0.2f,
		TEXT("Floors whose GBuffer roughness is at or below this value reflect"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonCaustics(
		TEXT("r.ToonRayTracer.Toon.Caustics"),
		1,
		TEXT("Toon caustics: bright light patterns that glass objects focus onto surfaces behind them, drawn in crisp steps. Found by tracing the light path backward from the surface through the glass (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCausticStrength(
		TEXT("r.ToonRayTracer.Toon.CausticStrength"),
		0.6f,
		TEXT("Toon caustics: brightness added per step (multiplied by the lit color)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCausticThreshold(
		TEXT("r.ToonRayTracer.Toon.CausticThreshold"),
		1.5f,
		TEXT("Toon caustics: how much the glass must concentrate the light (ratio to direct light) to start the pattern. The second step starts at three times this value"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeSeeThrough(
		TEXT("r.ToonRayTracer.Toon.EyeSeeThrough"),
		1,
		TEXT("Eyes seen through the bangs: where the bangs cover the eyes, overlay the eyes semi-transparently in three tones (lines, iris, whites). The tones come from copies of the eye parts drawn into the Custom Depth stencil (r.CustomDepth is set to 3 while in use), and their colors are recorded while the eyes are visible on screen (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeSeeThroughOpacity(
		TEXT("r.ToonRayTracer.Toon.EyeSeeThroughOpacity"),
		0.5f,
		TEXT("Eyes seen through the bangs: opacity of the overlaid eyes (0-1)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeSeeThroughDistance(
		TEXT("r.ToonRayTracer.Toon.EyeSeeThroughDistance"),
		20.0f,
		TEXT("Eyes seen through the bangs: maximum distance (cm) from the hair to the eyes behind it. Scaled to the character size in the same ratio as SeeThroughDistance. The side hair is often closer to the eyes than the bangs, so profile views are excluded by EyeSeeThroughMaxAngle, not by this distance"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeSeeThroughMaxAngle(
		TEXT("r.ToonRayTracer.Toon.EyeSeeThroughMaxAngle"),
		60.0f,
		TEXT("Eyes seen through the bangs: largest horizontal angle (degrees) between the face front and the camera at which the eyes are shown. They fade out over the last 15 degrees, so profile views do not show them through the side hair"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonCharacterRimShadow(
		TEXT("r.ToonRayTracer.Toon.CharacterRimShadow"),
		1,
		TEXT("Character rim light only where the light actually reaches: casts a shadow ray (passing through the character itself) from the rim point toward the light, and removes the rim if another object blocks it (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonDeepShadow(
		TEXT("r.ToonRayTracer.Toon.DeepShadow"),
		1,
		TEXT("Second shadow (a darker shadow in recessed places such as under the chin or inside the collar), decided by how much short rays from the surface are blocked. 0: off, 1: characters only (not the face or eyes), 2: all surfaces"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonDeepShadowDistance(
		TEXT("r.ToonRayTracer.Toon.DeepShadowDistance"),
		10.0f,
		TEXT("Second shadow: length (cm) of the short rays that check how recessed the surface is"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonDeepShadowSamples(
		TEXT("r.ToonRayTracer.Toon.DeepShadowSamples"),
		6,
		TEXT("Second shadow: number of short rays per pixel (1-16). The direction changes every frame and is averaged by the accumulation or TSR"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonDeepShadowThreshold(
		TEXT("r.ToonRayTracer.Toon.DeepShadowThreshold"),
		0.5f,
		TEXT("Second shadow: ratio of blocked rays above which the surface becomes the second shadow (0-1). Lower values make more of the surface the second shadow"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonDeepShadowSoftness(
		TEXT("r.ToonRayTracer.Toon.DeepShadowSoftness"),
		0.1f,
		TEXT("Second shadow: blur of the edge, as a ratio of blocked rays"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonDeepShadowBrightness(
		TEXT("r.ToonRayTracer.Toon.DeepShadowBrightness"),
		0.7f,
		TEXT("Second shadow: brightness of the second shadow relative to the normal shadow color (1: same as the shadow)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonDeepShadowLit(
		TEXT("r.ToonRayTracer.Toon.DeepShadowLit"),
		0.0f,
		TEXT("Second shadow: how much recessed places on the lit side become the normal shadow (0: the lit side is unchanged, 1: recessed places on the lit side are shadowed)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterOutlineColorScale(
		TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"),
		0.25f,
		TEXT("Character outlines use the material color multiplied by this value instead of OutlineColor. 0: use OutlineColor"),
		ECVF_RenderThreadSafe);

	// Original ルック フェーズ3：影の面を Skylight（空と周りの光）の色にどれだけ染めるか
	TAutoConsoleVariable<float> CVarToonRayTracerToonSkyTint(
		TEXT("r.ToonRayTracer.Toon.SkyTint"),
		0.3f,
		TEXT("How much the shadow bands are tinted with the average Skylight color (0-1), so toon surfaces blend with the regularly lit background"),
		ECVF_RenderThreadSafe);

	// 影の色の鮮やかさ：影の色の明るさは保ったまま、彩度を上げる
	// マテリアルの発光：UE のマテリアルの発光（エミッシブ）を、トゥーンの塗りに足す強さ
	// Original モード：ゲームのテクスチャ（影色 LD、ハイライト HM、影の出やすさ VC）を、Two Sided Foliage のマテリアルの GBuffer から読む
	TAutoConsoleVariable<int32> CVarToonRayTracerToonOriginal(
		TEXT("r.ToonRayTracer.Toon.Original"),
		1,
		TEXT("Original mode: materials with the Two Sided Foliage shading model pass the game textures through the GBuffer (Subsurface Color: shadow color, Metallic: hair highlight, Specular: sparkle, Roughness: shadow bias) (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalHighlightStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalHighlightStrength"),
		0.5f,
		TEXT("Strength of the hair highlight from the game texture in the Original mode"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSparkleStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalSparkleStrength"),
		1.0f,
		TEXT("Strength of the small sparkle highlight from the game texture in the Original mode"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalShadowBias(
		TEXT("r.ToonRayTracer.Toon.OriginalShadowBias"),
		0.6f,
		TEXT("How much the shadow bias from the game texture shifts the shadow boundary (N dot L) in the Original mode"),
		ECVF_RenderThreadSafe);

	// Original モード：光と反対の側の輪郭に入れる、空の色の縁（照り返しのリム）
	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalReflectionRimStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalReflectionRimStrength"),
		0.5f,
		TEXT("Strength of the sky-colored rim on the shadow-side silhouette in the Original mode (multiplied with the character rim strength). 0: off"),
		ECVF_RenderThreadSafe);

	// Original モード：仕上げに、キャラクターをぼかして重ねる（Diffusion Filter・Soft Focus・Bloom）
	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalDiffusionStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalDiffusionStrength"),
		0.6f,
		TEXT("Diffusion filter in the Original mode (0-1): the blurred \"color x color x light color\" of the character is screened over the multiplied color, keeping the lighter of it and the original. 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalDiffusionRadius(
		TEXT("r.ToonRayTracer.Toon.OriginalDiffusionRadius"),
		24.0f,
		TEXT("Blur radius in pixels of the diffusion filter"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSoftFocusStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusStrength"),
		0.2f,
		TEXT("Soft focus in the Original mode (0-1): how much a blurred copy of the character is blended over it. 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSoftFocusRadius(
		TEXT("r.ToonRayTracer.Toon.OriginalSoftFocusRadius"),
		6.0f,
		TEXT("Blur radius in pixels of the soft focus"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalBloomStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalBloomStrength"),
		2.0f,
		TEXT("Bloom of the lit-side character rim in the Original mode: the blurred rim is added (also around the character). 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalBloomRadius(
		TEXT("r.ToonRayTracer.Toon.OriginalBloomRadius"),
		16.0f,
		TEXT("Blur radius in pixels of the rim bloom"),
		ECVF_RenderThreadSafe);

	// Original モード：画面全体に、上から下へのグラデーションを掛ける（仕上げの色味）
	TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalGradationStrength(
		TEXT("r.ToonRayTracer.Toon.OriginalGradationStrength"),
		1.0f,
		TEXT("Strength of the top-to-bottom color gradation multiplied over the whole screen in the Original mode. 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonOriginalGradationTopColor(
		TEXT("r.ToonRayTracer.Toon.OriginalGradationTopColor"),
		TEXT("1.06,1.03,0.96"),
		TEXT("Linear RGB multiplied at the top of the screen by the gradation, as \"R,G,B\""),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonOriginalGradationBottomColor(
		TEXT("r.ToonRayTracer.Toon.OriginalGradationBottomColor"),
		TEXT("0.9,0.92,1.0"),
		TEXT("Linear RGB multiplied at the bottom of the screen by the gradation, as \"R,G,B\""),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEmissiveStrength(
		TEXT("r.ToonRayTracer.Toon.EmissiveStrength"),
		1.0f,
		TEXT("Strength of the material emissive color added on top of the toon shading, for surfaces seen from the camera (0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowSaturation(
		TEXT("r.ToonRayTracer.Toon.ShadowSaturation"),
		0.6f,
		TEXT("How much the saturation of the shadow color is increased while keeping its luminance, so shadows are vivid instead of dull gray (0: unchanged)"),
		ECVF_RenderThreadSafe);

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

	// トゥーンの面光源のやわらかい影（『The Rest of Your Life』の応用）
	TAutoConsoleVariable<float> CVarToonRayTracerToonSoftShadowAngle(
		TEXT("r.ToonRayTracer.Toon.SoftShadowAngle"),
		5.0f,
		TEXT("Apparent size of the light in degrees (cone angle) for soft toon shadows. 0: hard shadows with a single shadow ray"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonSoftShadowSamples(
		TEXT("r.ToonRayTracer.Toon.SoftShadowSamples"),
		4,
		TEXT("Number of shadow rays per shading point for soft toon shadows (1-16)"),
		ECVF_RenderThreadSafe);

	// トゥーンの照り返し（間接光、『The Rest of Your Life』の応用）
	TAutoConsoleVariable<float> CVarToonRayTracerToonIndirectStrength(
		TEXT("r.ToonRayTracer.Toon.IndirectStrength"),
		0.5f,
		TEXT("Strength of the toon bounce light (color bleeding from nearby surfaces). 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonIndirectSamples(
		TEXT("r.ToonRayTracer.Toon.IndirectSamples"),
		2,
		TEXT("Number of cosine-weighted rays per shading point for the toon bounce light (1-16)"),
		ECVF_RenderThreadSafe);

	// 眉毛の透け：前髪の奥にある眉毛などのパーツ（Custom Primitive Data の [13] で指定）を、髪の上に見せる
	TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughOpacity(
		TEXT("r.ToonRayTracer.Toon.SeeThroughOpacity"),
		0.25f,
		TEXT("Opacity of the parts (e.g. eyebrows) shown through the character's hair. The parts are selected by SeeThroughSlotMask, or per mesh by Custom Primitive Data [13]. 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonSeeThroughSlotMask(
		TEXT("r.ToonRayTracer.Toon.SeeThroughSlotMask"),
		16,
		TEXT("Bit mask of the material slots shown through the character's hair (slot n: 2^n, e.g. 16 for slot 4). Custom Primitive Data [13] overrides it per mesh. 0: none"),
		ECVF_RenderThreadSafe);

	// 目の表現：キャラクターの目のパーツ（マテリアルスロットで指定）
	TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeSlotMask(
		TEXT("r.ToonRayTracer.Toon.EyeSlotMask"),
		16,
		TEXT("Bit mask of the material slots treated as the character's eyes (slot n: 2^n, e.g. 16 for slot 4). 0: none"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeTriangleStart(
		TEXT("r.ToonRayTracer.Toon.EyeTriangleStart"),
		1850,
		TEXT("First triangle index (within the eye material slot) treated as the eyes. Use it when eyebrows share the slot. See r.ToonRayTracer.Debug.Geometry 3"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeTriangleEnd(
		TEXT("r.ToonRayTracer.Toon.EyeTriangleEnd"),
		0,
		TEXT("End (exclusive) of the triangle indices treated as the eyes. 0: up to the last triangle"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeUnlit(
		TEXT("r.ToonRayTracer.Toon.EyeUnlit"),
		1,
		TEXT("Draw the character's eyes with the lit color regardless of the light direction and shadows (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	// 目の表現：上まぶたの影
	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeShadowDistance(
		TEXT("r.ToonRayTracer.Toon.EyeShadowDistance"),
		2.0f,
		TEXT("Width (cm) of the upper eyelid shadow band, measured upward (world Z) from each eye point: the point is shadowed when the eye is no longer visible from the camera at that offset. 0: off"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeShadowBrightness(
		TEXT("r.ToonRayTracer.Toon.EyeShadowBrightness"),
		0.7f,
		TEXT("Brightness multiplier of the eye under the upper eyelid (1: no shadow)"),
		ECVF_RenderThreadSafe);

	// 目の表現：光に合わせた瞳のハイライト
	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightStrength(
		TEXT("r.ToonRayTracer.Toon.EyeHighlightStrength"),
		1.0f,
		TEXT("Strength of the light-dependent highlight on the eyes (0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightThreshold(
		TEXT("r.ToonRayTracer.Toon.EyeHighlightThreshold"),
		0.99f,
		TEXT("N dot H threshold of the eye highlight. Closer to 1 makes the highlight smaller. See r.ToonRayTracer.Debug.Geometry 5"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightSearchRadius(
		TEXT("r.ToonRayTracer.Toon.EyeHighlightSearchRadius"),
		10.0f,
		TEXT("Maximum distance (cm) to search for the edges of the visible eye, used to treat the eye as a sphere for the highlight. Should be larger than the eye radius"),
		ECVF_RenderThreadSafe);

	// 顔の影：顔の明暗を、頭の向きと光の向きから計算した境目で決める
	TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceShadow(
		TEXT("r.ToonRayTracer.Toon.FaceShadow"),
		1,
		TEXT("Shade the character's face with a shadow boundary computed from the head direction and the light direction instead of the surface normals, like SDF face shadows (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	// 顔の影の SDF 版：境目の形を、顔の正面に投影したしきい値マップ（SDF テクスチャ）で決める
	TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceShadowSDF(
		TEXT("r.ToonRayTracer.Toon.FaceShadowSDF"),
		0,
		TEXT("Use the shadow threshold map (FaceShadowSDFTexture) projected onto the front of the face for the face shadow boundary instead of the computed one (0: computed, 1: SDF texture). Needs FaceShadow 1"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceShadowSDFTexture(
		TEXT("r.ToonRayTracer.Toon.FaceShadowSDFTexture"),
		TEXT(""),
		TEXT("Texture asset path of the face shadow threshold map, drawn as seen from the front of the face with the light coming from the right of the image. Made with sdf_shadow_threshold_map from masks ordered from the light behind to the light in front: darker texels become lit earlier as the light moves from behind to the front"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSDFScale(
		TEXT("r.ToonRayTracer.Toon.FaceShadowSDFScale"),
		1.6f,
		TEXT("Half size of the face shadow threshold map, in half face widths (the distance between the eyes is 1)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSDFOffsetY(
		TEXT("r.ToonRayTracer.Toon.FaceShadowSDFOffsetY"),
		-0.4f,
		TEXT("Height of the center of the face shadow threshold map from the eyes, in half face widths (negative is below the eyes)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceSlotMask(
		TEXT("r.ToonRayTracer.Toon.FaceSlotMask"),
		8,
		TEXT("Bit mask of the material slots treated as the character's face (slot n: 2^n, e.g. 8 for slot 3)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowCurve(
		TEXT("r.ToonRayTracer.Toon.FaceShadowCurve"),
		0.15f,
		TEXT("How much the face shadow boundary curves along the cheeks: farther above or below the eyes, the boundary moves back toward the lit side (0: straight)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSoftness(
		TEXT("r.ToonRayTracer.Toon.FaceShadowSoftness"),
		0.03f,
		TEXT("Half width of the soft face shadow boundary, relative to the half width of the face. Larger values make the face shadow softer"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowFrontBias(
		TEXT("r.ToonRayTracer.Toon.FaceShadowFrontBias"),
		0.4f,
		TEXT("Subtracted from the front component of the light (-1 to 1) for the face shadow, so light slightly in front of the side does not leave only a thin lit strip along the nose when viewed from the side (0: off)"),
		ECVF_RenderThreadSafe);

	// 前髪の影：顔に髪の影だけを落とす
	TAutoConsoleVariable<int32> CVarToonRayTracerToonHairShadow(
		TEXT("r.ToonRayTracer.Toon.HairShadow"),
		1,
		TEXT("Bangs shadow on the character's face, even when the character does not receive its own shadow. 0: off, 1: band under the edge of the bangs (decided by what the camera sees above each face point), 2: shadow rays toward the light that only hit the hair"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonHairSlotMask(
		TEXT("r.ToonRayTracer.Toon.HairSlotMask"),
		64,
		TEXT("Bit mask of the material slots treated as the character's hair (slot n: 2^n, e.g. 64 for slot 6)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowDistance(
		TEXT("r.ToonRayTracer.Toon.HairShadowDistance"),
		30.0f,
		TEXT("Mode 2: maximum distance (cm) from the face toward the light to look for the hair casting the bangs shadow"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowWidth(
		TEXT("r.ToonRayTracer.Toon.HairShadowWidth"),
		4.0f,
		TEXT("Mode 1: width (cm) of the bangs shadow band under the edge of the bangs"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowLightShift(
		TEXT("r.ToonRayTracer.Toon.HairShadowLightShift"),
		0.5f,
		TEXT("Mode 1: horizontal shift toward the light, relative to the band width, so the bangs shadow moves with the light"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowStrength(
		TEXT("r.ToonRayTracer.Toon.HairShadowStrength"),
		1.0f,
		TEXT("Strength of the bangs shadow on the face (0-1)"),
		ECVF_RenderThreadSafe);

	// 髪の帯状のハイライト（天使の輪）
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightStrength(
		TEXT("r.ToonRayTracer.Toon.HairHighlightStrength"),
		0.9f,
		TEXT("Strength of the band highlight on the hair (angel ring) that moves with the light and the view (0: off, 1: the band is fully the highlight color). Not used for hair in the Original mode"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightWidth(
		TEXT("r.ToonRayTracer.Toon.HairHighlightWidth"),
		0.1f,
		TEXT("Width of the hair band highlight, as |dot(hair strand direction, half vector)| (larger is wider)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightShift(
		TEXT("r.ToonRayTracer.Toon.HairHighlightShift"),
		0.0f,
		TEXT("Moves the hair band highlight along the strands (positive: toward the top of the head, negative: toward the ends)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJagged(
		TEXT("r.ToonRayTracer.Toon.HairHighlightJagged"),
		1.5f,
		TEXT("How jagged the lower edge of the hair band highlight is, relative to its width (0: straight band)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<int32> CVarToonRayTracerToonHairHighlightMode(
		TEXT("r.ToonRayTracer.Toon.HairHighlightMode"),
		0,
		TEXT("Hair highlight computed from the light and the view (not used for characters that read the highlight from the texture).\n")
		TEXT(" 0: none (only the texture highlight)\n")
		TEXT(" 1: one ring around the head (hair direction from the head sphere)\n")
		TEXT(" 2: short pieces per hair bundle (hair direction from the mesh normal, limited to the area around the ring)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightTextureStrength(
		TEXT("r.ToonRayTracer.Toon.HairHighlightTextureStrength"),
		1.0f,
		TEXT("Strength of the hair highlight drawn from the texture mask (the Specular of the hair material, for characters whose profile reads the highlight from the texture). Adds HairHighlightTextureColor times the light color, times the mask and this value"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<FString> CVarToonRayTracerToonHairHighlightTextureColor(
		TEXT("r.ToonRayTracer.Toon.HairHighlightTextureColor"),
		TEXT("0.46,0.4,0.25"),
		TEXT("Linear RGB color added where the hair highlight mask is 1 (multiplied by the light color), as \"R,G,B\""),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightFalloff(
		TEXT("r.ToonRayTracer.Toon.HairHighlightFalloff"),
		0.5f,
		TEXT("Falloff of the texture hair highlight (also the Original mode hair highlight): the highlight fades out toward the outline where N.V is below this value (0: no falloff)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightArea(
		TEXT("r.ToonRayTracer.Toon.HairHighlightArea"),
		3.0f,
		TEXT("Mode 1: width of the area around the ring where the pieces can appear, relative to the band width"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightSoftness(
		TEXT("r.ToonRayTracer.Toon.HairHighlightSoftness"),
		0.05f,
		TEXT("Blur of the edges of the hair band highlight, relative to its width (0: hard edge)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightTaper(
		TEXT("r.ToonRayTracer.Toon.HairHighlightTaper"),
		0.0f,
		TEXT("How much the hair band highlight narrows toward the sides of the head seen from the camera (0: same width everywhere, 1: vanishes at the outline)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJaggedSharpness(
		TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedSharpness"),
		4.0f,
		TEXT("Shape of the jags of the hair band highlight (1: triangles, larger: thin pointed spikes like hair strands)"),
		ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJaggedCount(
		TEXT("r.ToonRayTracer.Toon.HairHighlightJaggedCount"),
		40.0f,
		TEXT("Number of jags of the hair band highlight around the head"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceWidthScale(
		TEXT("r.ToonRayTracer.Toon.FaceWidthScale"),
		1.0f,
		TEXT("Half width of the face as a multiple of the distance between the eyes"),
		ECVF_Default);

	// 頭と両目のボーンの名前（ゲームスレッドで読む）
	TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceHeadBone(
		TEXT("r.ToonRayTracer.Toon.FaceHeadBone"),
		TEXT("jnt_head"),
		TEXT("Name of the head bone used to find the face direction"),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceLeftEyeBone(
		TEXT("r.ToonRayTracer.Toon.FaceLeftEyeBone"),
		TEXT("jnt_eyeL"),
		TEXT("Name of the left eye bone used to find the face direction and width"),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceRightEyeBone(
		TEXT("r.ToonRayTracer.Toon.FaceRightEyeBone"),
		TEXT("jnt_eyeR"),
		TEXT("Name of the right eye bone used to find the face direction and width"),
		ECVF_Default);

	// キャラクターの大きさの基準：全体の設定の幅や距離（cm）を、両目の間の距離がこの値のキャラクター向けの値として扱い、
	// ほかのキャラクターでは両目の間の距離に比例して拡大縮小する（0 なら拡大縮小しない）
	TAutoConsoleVariable<float> CVarToonRayTracerToonReferenceEyeDistance(
		TEXT("r.ToonRayTracer.Toon.ReferenceEyeDistance"),
		39.6f,
		TEXT("Distance between the eyes (cm) of the character that the global widths and distances are tuned for. Other characters scale them by their own eye distance (0: no scaling)"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughBrightness(
		TEXT("r.ToonRayTracer.Toon.SeeThroughBrightness"),
		0.5f,
		TEXT("Brightness multiplier of the hair color where the parts are shown through it (0: black, 1: unchanged)"),
		ECVF_RenderThreadSafe);

	// 眉毛の透け：髪の色を暗くする代わりに、指定した色（眉毛の色）で塗る
	TAutoConsoleVariable<FString> CVarToonRayTracerToonSeeThroughColor(
		TEXT("r.ToonRayTracer.Toon.SeeThroughColor"),
		TEXT("0.3,0.08,0.09"),
		TEXT("Linear RGB of the parts (e.g. eyebrows) painted over the hair when SeeThroughColorBlend is above 0, as \"R,G,B\". The light color and the hair brightness are applied"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughColorBlend(
		TEXT("r.ToonRayTracer.Toon.SeeThroughColorBlend"),
		0.0f,
		TEXT("0: show the parts through the hair by darkening the hair color (SeeThroughBrightness), 1: paint them with SeeThroughColor"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughDistance(
		TEXT("r.ToonRayTracer.Toon.SeeThroughDistance"),
		20.0f,
		TEXT("Maximum distance (cm) behind the hair to look for the parts shown through it"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonSoftShadowSteps(
		TEXT("r.ToonRayTracer.Toon.SoftShadowSteps"),
		1,
		TEXT("Quantize the soft shadow visibility into this many steps (0 or 1: smooth gradient, 2 or more: toon steps)"),
		ECVF_RenderThreadSafe);

	// ガラスの影の濃さ
	TAutoConsoleVariable<float> CVarToonRayTracerToonGlassShadowOpacity(
		TEXT("r.ToonRayTracer.Toon.GlassShadowOpacity"),
		0.5f,
		TEXT("How much light glass blocks in the toon shadows (0: no shadow, 1: as dark as opaque objects). Each glass object the shadow ray passes through multiplies the light by (1 - this value)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarToonRayTracerToonLocalLights(
		TEXT("r.ToonRayTracer.Toon.LocalLights"),
		1,
		TEXT("Colored light: light the toon surfaces with the point and spot lights (e.g. lights placed by neon signs or glowing objects) in their color, in crisp toon steps. A shadow ray to each light decides where the light reaches; full color within half the attenuation radius, half color beyond. Up to 8 lights nearest to the camera (1: on, 0: off)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonLocalLightStrength(
		TEXT("r.ToonRayTracer.Toon.LocalLightStrength"),
		0.6f,
		TEXT("Colored light: brightness of the point and spot lights added to the toon surfaces (multiplied by the surface color and the light color)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonGlassShadowColor(
		TEXT("r.ToonRayTracer.Toon.GlassShadowColor"),
		1.0f,
		TEXT("Colored shadows: how much a colored glass (Custom Primitive Data [1][2][3] of a glass mesh) tints its shadow and the light passing through it (0: gray shadow, 1: the glass color). Requires GlassShadowOpacity below 1"),
		ECVF_RenderThreadSafe);

	// Perlin ノイズの応用：影の境目を手描き風に揺らす
	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowNoiseStrength(
		TEXT("r.ToonRayTracer.Toon.ShadowNoiseStrength"),
		0.0f,
		TEXT("Strength of the Perlin noise that wobbles the toon shadow boundaries for a hand-drawn look (0: off, around 0.1-0.3 is subtle to strong)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonShadowNoiseSize(
		TEXT("r.ToonRayTracer.Toon.ShadowNoiseSize"),
		40.0f,
		TEXT("Size (cm) of the shadow boundary wobble. Computed in mesh local space with the mesh scale applied, so the pattern sticks to the mesh"),
		ECVF_RenderThreadSafe);

	// Original ルック フェーズ2：キャラクターが自分自身の影を受けるか（0 なら、服のまだらな影や顔に落ちる前髪の影が消える）
	TAutoConsoleVariable<int32> CVarToonRayTracerToonCharacterSelfShadow(
		TEXT("r.ToonRayTracer.Toon.CharacterSelfShadow"),
		0,
		TEXT("Characters receive their own shadows. When 0, shadow rays from characters pass through the character itself and only other objects cast shadows on them"),
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
		0.5f,
		TEXT("Outline width in pixels for skeletal meshes (characters)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineThreshold(
		TEXT("r.ToonRayTracer.Toon.OutlineThreshold"),
		1.0f,
		TEXT("Outline is drawn where the neighbor ray hits farther than this multiple of the ray offset from the extended center plane. Smaller draws lines at gentler creases"),
		ECVF_RenderThreadSafe);

	// 顔の中の線：顔のパーツどうしの間では、面の折れ目の線を引かず、段差の許容量を広げる
	// 唇や鼻の小さな凹凸に線が出ないようにする（見下ろす角度で、口が「ω」のような線になるのを防ぐ）。顔と首・髪の境目（輪郭）には線が残る
	TAutoConsoleVariable<float> CVarToonRayTracerToonFaceOutlineTolerance(
		TEXT("r.ToonRayTracer.Toon.FaceOutlineTolerance"),
		100.0f,
		TEXT("Between points that are both on the face slots, creases are not outlined and the depth step tolerance is multiplied by this value, so small bumps such as the lips get no lines. 1: same as other parts"),
		ECVF_RenderThreadSafe);

	// 奥行きで太さが変わる線：接している部分や面の折れ目は細く、背景から大きく離れた輪郭は太くする（手描きの線のような強弱）
	TAutoConsoleVariable<int32> CVarToonRayTracerToonOutlineDepthWidth(
		TEXT("r.ToonRayTracer.Toon.OutlineDepthWidth"),
		1,
		TEXT("Vary the outline width by the depth gap: thin where parts touch or at creases, thick at silhouettes far from the background (0: off, 1: on)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineThinScale(
		TEXT("r.ToonRayTracer.Toon.OutlineThinScale"),
		0.6f,
		TEXT("Outline width multiplier where the depth gap is small (touching parts and creases)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineThickScale(
		TEXT("r.ToonRayTracer.Toon.OutlineThickScale"),
		1.0f,
		TEXT("Outline width multiplier at silhouettes whose depth gap is OutlineDepthRange or more"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineDepthRange(
		TEXT("r.ToonRayTracer.Toon.OutlineDepthRange"),
		30.0f,
		TEXT("Depth gap (cm) at which the outline reaches OutlineThickScale"),
		ECVF_RenderThreadSafe);

	// 文字列型のため ECVF_RenderThreadSafe は付けられない。ゲームスレッドで読んでレンダースレッドへ渡す
	// 『The Next Week』第4章：第10章のシーンで球に貼る画像（本の earthmap.jpg）のアセットのパス
	// 見つからなければ UV のチェッカーで代用する。文字列型のためゲームスレッドで読む
	TAutoConsoleVariable<FString> CVarToonRayTracerImageTexture(
		TEXT("r.ToonRayTracer.ImageTexture"),
		TEXT("/ToonRayTracer/Textures/T_EarthMap.T_EarthMap"),
		TEXT("Texture asset path of the image texture used by the Next Week final scene (the book's earthmap.jpg). A UV checker is used when it is not found"),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonOutlineColor(
		TEXT("r.ToonRayTracer.Toon.OutlineColor"),
		TEXT("0.02,0.02,0.04"),
		TEXT("Linear RGB color of the toon outline, as \"R,G,B\""),
		ECVF_Default);

	// トゥーン T7：様式化された反射
	TAutoConsoleVariable<int32> CVarToonRayTracerToonReflectionDepth(
		TEXT("r.ToonRayTracer.Toon.ReflectionDepth"),
		4,
		TEXT("Maximum number of reflections / refractions followed on metal and glass in toon shading (0 shades them as plain materials)"),
		ECVF_RenderThreadSafe);

	// トゥーンのガラスの縁：輪郭に近い部分に色の帯を乗せて、背景が単色でもガラスに見えるようにする
	TAutoConsoleVariable<float> CVarToonRayTracerToonGlassEdgeStrength(
		TEXT("r.ToonRayTracer.Toon.GlassEdgeStrength"),
		0.6f,
		TEXT("Strength of the colored band painted near the silhouette of glass in toon shading (0: off, 1: the band is fully the edge color)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarToonRayTracerToonGlassEdgeWidth(
		TEXT("r.ToonRayTracer.Toon.GlassEdgeWidth"),
		0.2f,
		TEXT("Width of the glass edge band: the band covers where sin(angle between the view ray and the surface normal) is above 1 - this value. On a sphere, it is this fraction of the radius from the silhouette (0 to 1)"),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<FString> CVarToonRayTracerToonGlassEdgeColor(
		TEXT("r.ToonRayTracer.Toon.GlassEdgeColor"),
		TEXT("0.55,0.8,1.0"),
		TEXT("Linear RGB color of the glass edge band, as \"R,G,B\""),
		ECVF_Default);

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

	// 負荷の確認用：パスが呼ばれるたびに、どのビューで何回描いているかをログに出す
	TAutoConsoleVariable<int32> CVarToonRayTracerDebugLogPasses(
		TEXT("r.ToonRayTracer.Debug.LogPasses"),
		0,
		TEXT("Log every toon ray tracing pass with its view (frame, view rect, capture flags) to find extra views rendered per frame"),
		ECVF_RenderThreadSafe);

	// 確認用：カメラのレイが当たった面のジオメトリ番号（スケルタルメッシュではマテリアルスロット）を色で表示する
	TAutoConsoleVariable<int32> CVarToonRayTracerDebugGeometry(
		TEXT("r.ToonRayTracer.Debug.Geometry"),
		0,
		TEXT("1: debug view of the geometry index (material slot of skeletal meshes) hit by the camera ray. 0 red, 1 green, 2 blue, 3 yellow, 4 magenta, 5 cyan, 6 white, 7 orange, 8+ gray. 2: debug view of the see-through result. 3: triangles treated as the eyes (green) and the rest of the eye slot (red). 4: upper eyelid shadow result on the eyes. 5: N dot H of the eye treated as a sphere (brightness) and the highlight (red). 6: face shadow (yellow: lit, blue: shadow, magenta: no face nearby). 7 / 8: coordinates on the face shadow threshold map (R, G: coarse / fine, B: 1 inside the map), used to make the drawing template. 11: backlit skin translucency (R: skin, G: backlit side facing away from the light, B: thin enough for the light to pass, 0.5 for the first step)"),
		ECVF_RenderThreadSafe);
}
