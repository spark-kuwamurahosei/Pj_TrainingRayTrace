#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

// ToonRayTracer のコンソール変数（r.ToonRayTracer.*）と、色の初期値などの定数
// 定義は ToonRayTracerCVars.cpp。エディタの設定パネル（ToonRayTracerEditorSettings）は名前でこれらを読み書きする
namespace ToonRayTracerCVars
{
	// 0: シェーダー内の解析的な球（本の第6章）、1: UEシーンの TLAS に TraceRay
	extern TAutoConsoleVariable<int32> CVarToonRayTracerTraceMode;

	// 解析的な物体のシーン（TraceMode 0 のときに使う）
	extern TAutoConsoleVariable<int32> CVarToonRayTracerAnalyticScene;

	// 本の第8章（アンチエイリアス）の samples_per_pixel に相当
	// カメラや物体が動いている間のサンプル数（0 なら SamplesPerPixel と同じ）
	extern TAutoConsoleVariable<int32> CVarToonRayTracerMotionSamplesPerPixel;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerSamplesPerPixel;

	// 『The Rest of Your Life』：確認用の「本の第9〜11章」モードで、拡散反射の方向をどう選ぶか
	extern TAutoConsoleVariable<int32> CVarToonRayTracerScatterSampling;

	// 『The Rest of Your Life』第3章：ピクセル内のサンプル位置の層化
	extern TAutoConsoleVariable<int32> CVarToonRayTracerStratifiedSampling;

	// 3（トゥーン）が本来の表示。0 ～ 2 は実装の確認用
	extern TAutoConsoleVariable<int32> CVarToonRayTracerShadingMode;

	// 本の第9章の max_depth に相当
	extern TAutoConsoleVariable<int32> CVarToonRayTracerMaxDepth;

	// Original ルック フェーズ1：トゥーンで塗るのをキャラクター（スケルタルメッシュ）だけにし、背景は通常描画のままにする
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonCharacterOnly;

	// Original ルック フェーズ3：キャラクターだけトゥーンにするとき、TSR の前（描画解像度、トーンマップ前）に描く
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonBeforeTSR;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonInverseTonemap;

	// トゥーン T2：段階的な陰影の設定
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonBands;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonShadowThreshold;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonLitThreshold;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEdgeSoftness;

	// 文字列型のコンソール変数は ECVF_RenderThreadSafe を付けられない（付けるとモジュールの読み込み時にアサートする）ため、
	// ゲームスレッドの BeginRenderViewFamily で読み、レンダーコマンドでレンダースレッドへ渡す
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonShadowColor;

	inline const FLinearColor DefaultToonShadowColor(0.35f, 0.4f, 0.6f);

	// Original ルック フェーズ2：肌（UE のマテリアルのシェーディングモデルが Subsurface / Preintegrated Skin / Subsurface Profile の面）の塗り
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonSkin;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSkinShadowThreshold;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonSkinShadowColor;

	inline const FLinearColor DefaultToonSkinShadowColor(0.62f, 0.42f, 0.45f);

	// Original ルック フェーズ2：キャラクターのリムライトと線の色
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterRimStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterRimWidth;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonCharacterOutlineColorScale;

	// Original ルック フェーズ3：影の面を Skylight（空と周りの光）の色にどれだけ染めるか
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSkyTint;

	// 影の色の鮮やかさ：影の色の明るさは保ったまま、彩度を上げる
	// マテリアルの発光：UE のマテリアルの発光（エミッシブ）を、トゥーンの塗りに足す強さ
	// Original モード：ゲームのテクスチャ（影色 LD、ハイライト HM、影の出やすさ VC）を、Two Sided Foliage のマテリアルの GBuffer から読む
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonOriginal;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalHighlightStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSparkleStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalShadowBias;

	// Original モード：光と反対の側の輪郭に入れる、空の色の縁（照り返しのリム）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalReflectionRimStrength;

	// Original モード：仕上げに、キャラクターをぼかして重ねる（Diffusion Filter・Soft Focus・Bloom）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalDiffusionStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalDiffusionRadius;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSoftFocusStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalSoftFocusRadius;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalBloomStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalBloomRadius;

	// Original モード：画面全体に、上から下へのグラデーションを掛ける（仕上げの色味）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOriginalGradationStrength;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonOriginalGradationTopColor;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonOriginalGradationBottomColor;

	inline const FLinearColor DefaultToonGradationTopColor(1.06f, 1.03f, 0.96f);

	inline const FLinearColor DefaultToonGradationBottomColor(0.9f, 0.92f, 1.0f);

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEmissiveStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonShadowSaturation;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonLightScale;

	// トゥーン T3：シャドウレイによる影
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonCastShadows;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonShadowBias;

	// トゥーンの面光源のやわらかい影（『The Rest of Your Life』の応用）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSoftShadowAngle;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonSoftShadowSamples;

	// トゥーンの照り返し（間接光、『The Rest of Your Life』の応用）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonIndirectStrength;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonIndirectSamples;

	// 眉毛の透け：前髪の奥にある眉毛などのパーツ（Custom Primitive Data の [13] で指定）を、髪の上に見せる
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughOpacity;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonSeeThroughSlotMask;

	// 目の表現：キャラクターの目のパーツ（マテリアルスロットで指定）
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeSlotMask;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeTriangleStart;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeTriangleEnd;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonEyeUnlit;

	// 目の表現：上まぶたの影
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEyeShadowDistance;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEyeShadowBrightness;

	// 目の表現：光に合わせた瞳のハイライト
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightThreshold;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonEyeHighlightSearchRadius;

	// 顔の影：顔の明暗を、頭の向きと光の向きから計算した境目で決める
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceShadow;

	// 顔の影の SDF 版
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceShadowSDF;
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceShadowSDFTexture;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSDFScale;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSDFOffsetY;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonFaceSlotMask;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowCurve;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowSoftness;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceShadowFrontBias;

	// 前髪の影：顔に髪の影だけを落とす
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonHairShadow;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonHairSlotMask;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowDistance;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowWidth;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowLightShift;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairShadowStrength;

	// 髪の帯状のハイライト（天使の輪）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightStrength;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightWidth;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightShift;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJagged;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJaggedCount;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightJaggedSharpness;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightSoftness;
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonHairHighlightMode;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightArea;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightTextureStrength;
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonHairHighlightTextureColor;
	inline const FLinearColor DefaultToonHairHighlightTextureColor(0.46f, 0.4f, 0.25f);
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightFalloff;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHairHighlightTaper;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceWidthScale;

	// 頭と両目のボーンの名前（ゲームスレッドで読む）
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceHeadBone;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceLeftEyeBone;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonFaceRightEyeBone;

	// 顔の影：この顔として扱う範囲の半径（両目の間の距離の何倍か）
	inline constexpr double FaceRadiusScale = 4.0;

	// キャラクターの大きさの基準：全体の設定の幅や距離（cm）を、両目の間の距離がこの値のキャラクター向けの値として扱い、
	// ほかのキャラクターでは両目の間の距離に比例して拡大縮小する（0 なら拡大縮小しない）
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonReferenceEyeDistance;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughBrightness;

	// 眉毛の透け：髪の色を暗くする代わりに、指定した色（眉毛の色）で塗る
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonSeeThroughColor;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughColorBlend;

	inline const FLinearColor DefaultToonSeeThroughColor(0.3f, 0.08f, 0.09f);

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonSeeThroughDistance;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonSoftShadowSteps;

	// ガラスの影の濃さ
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonGlassShadowOpacity;

	// Perlin ノイズの応用：影の境目を手描き風に揺らす
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonShadowNoiseStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonShadowNoiseSize;

	// Original ルック フェーズ2：キャラクターが自分自身の影を受けるか（0 なら、服のまだらな影や顔に落ちる前髪の影が消える）
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonCharacterSelfShadow;

	// トゥーン T5：ハイライトとリムライト
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHighlightThreshold;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonHighlightStrength;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonRimThreshold;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonRimStrength;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonRimLitSideOnly;

	// トゥーン T6：アウトライン
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonOutline;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineWidth;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineWidthSkinned;

	extern TAutoConsoleVariable<float> CVarToonRayTracerToonOutlineThreshold;

	// 顔の中の線：顔のパーツどうしの間では、面の折れ目の線を引かず、段差の許容量を広げる
	// 唇や鼻の小さな凹凸に線が出ないようにする（見下ろす角度で、口が「ω」のような線になるのを防ぐ）。顔と首・髪の境目（輪郭）には線が残る
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonFaceOutlineTolerance;

	// 文字列型のため ECVF_RenderThreadSafe は付けられない。ゲームスレッドで読んでレンダースレッドへ渡す
	// 『The Next Week』第4章：第10章のシーンで球に貼る画像（本の earthmap.jpg）のアセットのパス
	// 見つからなければ UV のチェッカーで代用する。文字列型のためゲームスレッドで読む
	extern TAutoConsoleVariable<FString> CVarToonRayTracerImageTexture;

	// 球に貼る画像のすべてのミップをメモリに置く指示の有効時間（秒）。毎フレーム延長する
	inline constexpr float ImageTextureResidentSeconds = 30.0f;

	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonOutlineColor;

	inline const FLinearColor DefaultToonOutlineColor(0.02f, 0.02f, 0.04f);

	// トゥーン T7：様式化された反射
	extern TAutoConsoleVariable<int32> CVarToonRayTracerToonReflectionDepth;

	// トゥーンのガラスの縁
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonGlassEdgeStrength;
	extern TAutoConsoleVariable<float> CVarToonRayTracerToonGlassEdgeWidth;
	extern TAutoConsoleVariable<FString> CVarToonRayTracerToonGlassEdgeColor;
	inline const FLinearColor DefaultToonGlassEdgeColor(0.55f, 0.8f, 1.0f);

	// "R,G,B" 形式の文字列を色に変換する（読めなければ Default を返す）
	FLinearColor ParseLinearColor(const FString& Text, const FLinearColor& Default);

	extern TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferNormal;

	// 汎用化 G1：UE のマテリアルの色（GBuffer のベースカラー）を素材の色として使う
	extern TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferBaseColor;

	// 汎用化 G3：反射・屈折した先も、カメラから見えている点なら GBuffer の値を使う
	extern TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferForReflections;

	// 反射・屈折した先の法線をなめらかにする補助レイの間隔（ポリゴン1枚分程度にすると明暗の境目の階段が目立たなくなる）
	extern TAutoConsoleVariable<float> CVarToonRayTracerReflectionNormalSmoothing;

	// 汎用化 G2：UE のマテリアルのメタリックとラフネスから金属を自動判定する
	extern TAutoConsoleVariable<int32> CVarToonRayTracerUseGBufferMetal;

	extern TAutoConsoleVariable<float> CVarToonRayTracerMetalRoughnessThreshold;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerAccumulate;

	// 値が変わると蓄積をリセットする（エディタパネルの「蓄積をリセット」ボタンが 1 ずつ増やす）
	// レベル上のオブジェクトを動かしただけでは自動でリセットされないため、その場合に使う
	extern TAutoConsoleVariable<int32> CVarToonRayTracerResetAccumulation;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerMaxAccumulatedFrames;

	// カメラのレイが、GBuffer に描かれていない面（マテリアルの透過で抜けた部分など）を通り抜けるか
	extern TAutoConsoleVariable<int32> CVarToonRayTracerSkipMaskedSurfaces;

	// 動く物体への対応
	extern TAutoConsoleVariable<int32> CVarToonRayTracerDetectMotion;

	extern TAutoConsoleVariable<int32> CVarToonRayTracerMotionMaxAccumulatedFrames;

	// 確認用：蓄積をやり直した理由を色で表示し、カメラや設定の変更による全体のリセットをログに出す
	extern TAutoConsoleVariable<int32> CVarToonRayTracerDebugMotion;

	// 負荷の確認用：パスが呼ばれるたびに、どのビューで何回描いているかをログに出す
	extern TAutoConsoleVariable<int32> CVarToonRayTracerDebugLogPasses;

	// 確認用：カメラのレイが当たった面のジオメトリ番号（スケルタルメッシュではマテリアルスロット）を色で表示する
	extern TAutoConsoleVariable<int32> CVarToonRayTracerDebugGeometry;
}
