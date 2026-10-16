// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ToonRayTracerEditorSettings.generated.h"

// 表示モード（r.ToonRayTracer.ShadingMode の値と一致させる）
UENUM()
enum class EToonRayTracerShadingMode : uint8
{
	Toon = 3			UMETA(DisplayName = "トゥーン"),
	Normal = 0			UMETA(DisplayName = "確認用：法線"),
	Materials = 1		UMETA(DisplayName = "確認用：本の第9〜11章（マテリアル）"),
	DirectLight = 2		UMETA(DisplayName = "確認用：ライトの N・L"),
};

// 交差判定の対象（r.ToonRayTracer.TraceMode の値と一致させる）
UENUM()
enum class EToonRayTracerTraceMode : uint8
{
	Scene = 1			UMETA(DisplayName = "シーンのメッシュ"),
	AnalyticSpheres = 0	UMETA(DisplayName = "解析的な球（本の第11章のシーン）"),
};

// 解析的な物体のシーン（r.ToonRayTracer.AnalyticScene の値と一致させる）
UENUM()
enum class EToonRayTracerAnalyticScene : uint8
{
	OneWeekend = 0		UMETA(DisplayName = "本の第11章のシーン（第4〜5章のテクスチャ付き）"),
	SimpleLight = 1		UMETA(DisplayName = "The Next Week 第7章：simple_light"),
	CornellBox = 2		UMETA(DisplayName = "The Next Week 第7章：コーネルボックス"),
	CornellBoxBoxes = 3	UMETA(DisplayName = "The Next Week 第8章：箱を置いたコーネルボックス"),
	CornellSmoke = 4	UMETA(DisplayName = "The Next Week 第9章：煙の箱（cornell_smoke）"),
	FinalScene = 5		UMETA(DisplayName = "The Next Week 第10章：final_scene"),
	RestOfYourLife = 6	UMETA(DisplayName = "The Rest of Your Life 第12章：アルミの箱とガラスの球のコーネルボックス"),
};

// 拡散反射の方向の選び方（r.ToonRayTracer.ScatterSampling の値と一致させる。『The Rest of Your Life』）
UENUM()
enum class EToonRayTracerScatterSampling : uint8
{
	UnitVector = 0			UMETA(DisplayName = "法線＋単位球上の点（One Weekend）"),
	UniformHemisphere = 1	UMETA(DisplayName = "半球上で一様（第6章の比較用）"),
	Cosine = 2				UMETA(DisplayName = "コサイン分布（第7〜8章）"),
	Light = 3				UMETA(DisplayName = "ライトを直接狙う（第9章）"),
	Mixture = 4				UMETA(DisplayName = "混合：ライトとコサイン分布（第10〜12章）"),
};

/**
 * ToonRayTracer のエディタパネルに表示する設定。
 * 値を変えると対応するコンソール変数（r.ToonRayTracer.*）にすぐ反映され、プロジェクトのユーザー設定に保存される。
 */
UCLASS(config = EditorPerProjectUserSettings)
class UToonRayTracerEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UToonRayTracerEditorSettings();

	// ---- 表示 ----

	/** 表示モード */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "表示モード"))
	EToonRayTracerShadingMode ShadingMode;

	/** 交差判定の対象 */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "交差判定の対象"))
	EToonRayTracerTraceMode TraceMode;

	/** 交差判定の対象が「解析的な球」のときに表示するシーン */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "解析的な物体のシーン", EditCondition = "TraceMode == EToonRayTracerTraceMode::AnalyticSpheres"))
	EToonRayTracerAnalyticScene AnalyticScene;

	/** 1 ピクセルあたりのサンプル数（アンチエイリアス） */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "サンプル数", ClampMin = "1", ClampMax = "64", UIMin = "1", UIMax = "16"))
	int32 SamplesPerPixel;

	/** ピクセル内のサンプル位置を層化する（格子状のマスに 1 本ずつ散らす。『The Rest of Your Life』第3章） */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "サンプル位置を層化する"))
	bool bStratifiedSampling;

	/** カメラや物体が動いている間のサンプル数。少ないほど軽いが、動いている間は輪郭がギザつく（止まると蓄積でなめらかになる）。0 ならサンプル数と同じ */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "動いている間のサンプル数", ClampMin = "0", ClampMax = "64", UIMin = "0", UIMax = "16"))
	int32 MotionSamplesPerPixel;

	/** カメラから見えている面に GBuffer の滑らかな法線を使う */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "滑らかな法線を使う"))
	bool bUseGBufferNormal;

	/** マテリアルを Custom Primitive Data で指定していないメッシュに、UE のマテリアルの色（GBuffer のベースカラー）を使う */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "マテリアルの色を使う"))
	bool bUseGBufferBaseColor;

	/** 反射・屈折した先も、カメラから見えている点なら UE のマテリアルの色や滑らかな法線を使う */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "映り込みにもマテリアルの色を使う"))
	bool bUseGBufferForReflections;

	/** 映り込んだ物体の法線をなめらかにする補助レイの間隔（cm）。ポリゴン1枚分程度にすると明暗の境目の階段が目立たなくなる。0 なら面ごとの法線 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "映り込みの法線のなめらかさ（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0"))
	float ReflectionNormalSmoothing;

	/** カメラのレイが、通常描画に描かれていない面（マテリアルの透過で抜けた部分など）を通り抜ける */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "透過で抜けた面を通り抜ける"))
	bool bSkipMaskedSurfaces;

	/** マテリアルを Custom Primitive Data で指定していないメッシュを、UE のマテリアルのメタリックとラフネスから金属と判定する */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "金属を自動判定する"))
	bool bUseGBufferMetal;

	/** 金属として鏡面反射させるラフネスの上限。これより粗い金属は普通の塗りにする */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "金属とみなすラフネスの上限", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bUseGBufferMetal"))
	float MetalRoughnessThreshold;

	/** カメラが静止している間、フレームをまたいで結果を平均する */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "フレーム間で蓄積する"))
	bool bAccumulate;

	/** 蓄積するフレーム数の上限 */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "蓄積フレーム数の上限", ClampMin = "1", UIMin = "1", UIMax = "4096", EditCondition = "bAccumulate"))
	int32 MaxAccumulatedFrames;

	/** 動いている物体（と、動いて見えるようになった後ろ側）のピクセルだけ蓄積をやり直す */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "動く物体を検出する", EditCondition = "bAccumulate"))
	bool bDetectMotion;

	/** 物体が動いている間の蓄積フレーム数の上限。小さいほど動く影や映り込みの残像が短くなる */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "物体が動いている間の蓄積フレーム数の上限", ClampMin = "1", UIMin = "1", UIMax = "64", EditCondition = "bAccumulate && bDetectMotion"))
	int32 MotionMaxAccumulatedFrames;

	/** 確認用の「本の第9〜11章」モードでの反射回数の上限 */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "反射回数の上限（第9〜11章モード）", ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "50"))
	int32 MaxDepth;

	/** 確認用の「本の第9〜11章」モードで、拡散反射の方向をどう選ぶか（『The Rest of Your Life』）。平均の明るさは同じで、ノイズの量が変わる */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "拡散反射の方向の選び方（第9〜11章モード）"))
	EToonRayTracerScatterSampling ScatterSampling;

	// ---- トゥーン：対象 ----

	/** トゥーンで塗るのをキャラクター（スケルタルメッシュ）だけにし、背景は通常描画のままにする（Original のルック）。Custom Primitive Data の [12] でメッシュごとに上書きできる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|対象", meta = (DisplayName = "キャラクターだけトゥーンにする"))
	bool bCharacterOnly;

	/** キャラクターだけトゥーンにするとき、TSR の前（描画解像度、トーンマップ前）に描く。TSR がアンチエイリアスとアップスケールを行い、動いている間の輪郭のギザつきが抑えられる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|対象", meta = (DisplayName = "TSR の前に描く", EditCondition = "bCharacterOnly"))
	bool bBeforeTSR;

	/** キャラクターが自分自身の影を受ける。オフなら、服のまだらな影や顔に落ちる前髪の影が消え、ほかの物体の影だけ受ける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "自分の影を受ける"))
	bool bCharacterSelfShadow;

	/** シェーディングモデルが Subsurface / Preintegrated Skin / Subsurface Profile のマテリアルの面を肌とし、肌用の境目と影色で塗る */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "肌を別の塗りにする"))
	bool bSkin;

	/** 肌の影との境目の N・L。全体の値より下げると、顔に細かい影が入りにくくなる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "肌の影のしきい値", ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0", EditCondition = "bSkin"))
	float SkinShadowThreshold;

	/** 肌の影色（肌の色に掛ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "肌の影色", HideAlphaChannel, EditCondition = "bSkin"))
	FLinearColor SkinShadowColor;

	/** 光源の側の輪郭に、一定の幅で入れる明るい縁（リムライト）の強さ。0 なら入れない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "リムライトの強さ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float CharacterRimStrength;

	/** リムライトの幅（ピクセル） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "リムライトの幅（ピクセル）", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
	float CharacterRimWidth;

	/** 線を、黒ではなく素材の色にこの値を掛けた色にする。0 なら全体の線の色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "線の色（素材の色の倍率）", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterOutlineColorScale;

	/** 前髪の奥にある眉毛などのパーツを、髪の上に見せる割合。見せるパーツは「眉毛の透け：スロット番号」で指定する。0 なら透かさない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：濃さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SeeThroughOpacity;

	/** 眉毛が透けている部分で、髪の色を何倍に暗くするか（0 で黒、1 で変えない） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：明るさ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SeeThroughBrightness;

	/** 眉毛が透けている部分を、髪の色を暗くする代わりに、この色（眉毛の色）で塗る割合（0 なら髪を暗くするだけ、1 ならこの色で塗る） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：色で塗る割合", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SeeThroughColorBlend;

	/** 眉毛の透けで塗る色。光の色と、髪の明るさ（影なら暗く）を掛ける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：色", HideAlphaChannel))
	FLinearColor SeeThroughColor;

	/** 髪の奥を何 cm まで探すか。大きすぎると、頭の反対側のパーツまで透ける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：探す距離（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0"))
	float SeeThroughDistance;

	/** 髪越しに見せるパーツのマテリアルスロット番号（スケルタルメッシュのエディタのエレメント番号）。すべてのキャラクターに使う。メッシュの Custom Primitive Data の [13] を設定すると、そのメッシュだけ上書きできる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "眉毛の透け：スロット番号"))
	TArray<int32> SeeThroughSlots;

	/** キャラクターの目として扱うパーツのマテリアルスロット番号（スケルタルメッシュのエディタのエレメント番号） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：スロット番号"))
	TArray<int32> EyeSlots;

	/** 目のスロットの中で、目として扱う最初の三角形の番号。眉毛が目と同じスロットにある場合に、目の部分だけを選ぶ（r.ToonRayTracer.Debug.Geometry 3 で、目が緑・それ以外が赤になるように調整する） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：三角形の範囲（開始）", ClampMin = "0", UIMin = "0"))
	int32 EyeTriangleStart;

	/** 目として扱う三角形の範囲の終わり（この番号は含まない）。0 なら最後まで */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：三角形の範囲（終了）", ClampMin = "0", UIMin = "0"))
	int32 EyeTriangleEnd;

	/** 目には陰影を付けず、光の向きや影に関係なくいつも明るい面の色で塗る（アニメ調の目） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：陰影を付けない"))
	bool bEyeUnlit;

	/** 上まぶたの影：目の見えている部分の上の縁から、何 cm の幅を影にするか。大きいほど影の帯が太くなる。0 なら影なし */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：上まぶたの影の太さ（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
	float EyeShadowDistance;

	/** 上まぶたの影の部分の明るさの倍率（1 なら影なし） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：上まぶたの影の明るさ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float EyeShadowBrightness;

	/** 光に合わせた瞳のハイライトの強さ。目を球とみなした仮の向きと光の向きから、くっきりした光の点を加える。0 ならなし */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：ハイライトの強さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float EyeHighlightStrength;

	/** 瞳のハイライトのしきい値（N・H）。1 に近いほど点が小さい（r.ToonRayTracer.Debug.Geometry 5 で確認できる） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：ハイライトのしきい値", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.9", UIMax = "1.0"))
	float EyeHighlightThreshold;

	/** 瞳のハイライト：目を球とみなすために、見えている目の範囲の端を探す最大の距離（cm）。目の半径より大きくする */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "目：ハイライトの探す距離（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0"))
	float EyeHighlightSearchRadius;

	/** 顔の明暗を、面の向きではなく、頭の向きと光の向きから計算した境目で決める（SDF の顔影と同じ動き方）。横や後ろから光が当たったときの顔の不規則な明暗をなくす */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：影の境目を計算で作る"))
	bool bFaceShadow;

	/** 顔として扱うパーツのマテリアルスロット番号（スケルタルメッシュのエディタのエレメント番号） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：スロット番号"))
	TArray<int32> FaceSlots;

	/** 顔の影の境目の曲がり具合。目の高さでいちばん影の側へふくらみ、額や顎に向かうほど光の側へ戻る（頬に沿ったカーブ）。0 ならまっすぐ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：境目の曲がり", ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float FaceShadowCurve;

	/** 顔の影の境目のぼかし幅（顔の半分の幅を 1 としたとき）。大きいほど柔らかい影になる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：境目のぼかし", ClampMin = "0.001", UIMin = "0.01", UIMax = "1.0"))
	float FaceShadowSoftness;

	/** 顔の影：前からの光の補正。光の前後の成分（1：正面、0：真横）からこの値を引いて使う。横寄りの光で、横から見たときに鼻筋だけが細く明るく見えるのを防ぐ。0 なら補正なし */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：前からの光の補正", ClampMin = "0.0", ClampMax = "0.99", UIMin = "0.0", UIMax = "0.9"))
	float FaceShadowFrontBias;

	/** 顔の半分の幅（両目の間の距離の何倍か）。影の境目が顔の端に届く位置が変わる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：幅（両目の距離の倍率）", ClampMin = "0.1", UIMin = "0.5", UIMax = "2.0"))
	float FaceWidthScale;

	/** 大きさの基準。全体の設定の幅や距離（cm）を合わせたキャラクターの両目の間の距離（cm）。ほかのキャラクターでは両目の間の距離に比例して拡大縮小する。0 なら拡大縮小しない（両目の間の距離は Toon Character コンポーネントで確認できる） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "大きさの基準（両目の間の距離 cm）", ClampMin = "0.0", UIMax = "20.0"))
	float ReferenceEyeDistance;

	/** 前髪の影。キャラクターが自分の影を受けない設定でも、顔には前髪の影が落ちる
	 *  0：なし
	 *  1：見え方で決める。顔の点を上（と光の側）へずらした位置に髪が見えれば影にする。前髪の縁の下に帯状の影
	 *  2：光の方向へシャドウレイを飛ばし、髪に当たったときだけ影にする（横の髪の束の影も落ちる） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪の影（0：なし、1：縁の下、2：光の方向）", ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2"))
	int32 HairShadowMode;

	/** 髪として扱うパーツのマテリアルスロット番号（スケルタルメッシュのエディタのエレメント番号） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪：スロット番号"))
	TArray<int32> HairSlots;

	/** 前髪の影（1）：前髪の縁の下の影の幅（cm） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪の影：幅（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
	float HairShadowWidth;

	/** 前髪の影（1）：影の幅に対して、光の側へずらす割合。光の向きに合わせて影が左右に動く */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪の影：光の側へのずれ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float HairShadowLightShift;

	/** 前髪の影（2）：顔から光の方向へ、髪を探す距離（cm） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪の影：探す距離（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "100.0"))
	float HairShadowDistance;

	/** 前髪の影の濃さ（0 ～ 1） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪の影：濃さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HairShadowStrength;

	/** 顔の向きを求めるための、頭のボーンの名前 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：頭のボーン"))
	FString FaceHeadBone;

	/** 顔の向きと幅を求めるための、左目のボーンの名前 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：左目のボーン"))
	FString FaceLeftEyeBone;

	/** 顔の向きと幅を求めるための、右目のボーンの名前 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：右目のボーン"))
	FString FaceRightEyeBone;

	// ---- トゥーン：陰影 ----

	/** 陰影の段階数（2: 影 / 明るい面、3: 影 / 中間 / 明るい面） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "段階数", ClampMin = "2", ClampMax = "3", UIMin = "2", UIMax = "3"))
	int32 Bands;

	/** 影との境目の N・L。上げると影が広がる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "影のしきい値", ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float ShadowThreshold;

	/** 中間と明るい面の境目の N・L（段階数が 3 のとき） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "明るい面のしきい値", ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float LitThreshold;

	/** 段階の境目のぼかし幅（N・L） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "境目のぼかし幅", ClampMin = "0.0", ClampMax = "0.5", UIMin = "0.0", UIMax = "0.2"))
	float EdgeSoftness;

	/** 影色（素材の色に掛ける）。メッシュごとの設定がなければこれを使う */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "影色", HideAlphaChannel))
	FLinearColor ShadowColor;

	/** 明るい面の明るさの倍率 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "明るさの倍率", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float LightScale;

	/** 影の面を、Skylight（空と周りの光）の平均の色にどれだけ染めるか。夕方なら赤み、曇りなら灰色がかった影になり、背景になじむ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "影を空の色に染める強さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SkyTint;

	/** 影の色の鮮やかさ。影の色（素材の色 × 影色）の明るさは保ったまま、彩度を上げる。影がくすんだ灰色にならず、髪や服の色に合った鮮やかな影になる。0 なら変えない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "影の色の鮮やかさ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float ShadowSaturation;

	/** マテリアルの発光の強さ。UE のマテリアルの発光（エミッシブ）の色を、トゥーンの塗りに足す。カメラから見えている面だけ。0 なら足さない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "発光の強さ", ClampMin = "0.0", UIMax = "4.0"))
	float EmissiveStrength;

	/** Original モード。シェーディングモデルが Two Sided Foliage のマテリアルは、空いている GBuffer の欄に入れたゲームのテクスチャ（Subsurface Color：影色 LD、Metallic：髪のハイライト HM の G、Specular：キラッとしたハイライト HM の R、Roughness：影の出やすさ VC の G）で、影とハイライトを決める */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "Original モード"))
	bool bOriginal;

	/** Original モード：髪のハイライト（HM の G）の強さ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "髪のハイライトの強さ", ClampMin = "0.0", UIMax = "2.0"))
	float OriginalHighlightStrength;

	/** Original モード：キラッと光る小さなハイライト（HM の R）の強さ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "キラッとしたハイライトの強さ", ClampMin = "0.0", ClampMax = "1.0"))
	float OriginalSparkleStrength;

	/** Original モード：影の出やすさ（VC の G）で影の境目をずらす量。大きいほど毛束の奥などが影になりやすい */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "影の出やすさの効き", UIMin = "0.0", UIMax = "2.0"))
	float OriginalShadowBias;

	/** Original モード：光と反対の側の輪郭に入れる、空の色の縁（照り返しのリム）の強さ。キャラクターのリムライトの強さに掛ける。0 なら入れない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "照り返しのリムの強さ", ClampMin = "0.0", UIMax = "2.0"))
	float OriginalReflectionRimStrength;

	/** Original モード：Diffusion Filter の強さ（0 ～ 1）。キャラクターの色どうしを乗算した色に、ぼかした「乗算した色 × 光の色」をスクリーンで重ね、元の色と比べて明るい方を残す。明るい部分が光の色でふんわりにじむ。0 なら掛けない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "Diffusion Filter の強さ", ClampMin = "0.0", ClampMax = "1.0"))
	float OriginalDiffusionStrength;

	/** Original モード：Diffusion Filter のぼかしの半径（ピクセル） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "Diffusion Filter のぼかしの半径（ピクセル）", ClampMin = "0.0", UIMax = "64.0"))
	float OriginalDiffusionRadius;

	/** Original モード：Soft Focus の強さ（0 ～ 1）。キャラクターをぼかした画像を重ねる割合。輪郭や細い線が柔らかくなる。0 なら掛けない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "Soft Focus の強さ", ClampMin = "0.0", ClampMax = "1.0"))
	float OriginalSoftFocusStrength;

	/** Original モード：Soft Focus のぼかしの半径（ピクセル） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "Soft Focus のぼかしの半径（ピクセル）", ClampMin = "0.0", UIMax = "32.0"))
	float OriginalSoftFocusRadius;

	/** Original モード：光の側のリムをぼかして加える強さ（キャラクターの外側にもにじむ）。0 なら加えない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "リムの Bloom の強さ", ClampMin = "0.0", UIMax = "4.0"))
	float OriginalBloomStrength;

	/** Original モード：リムの Bloom のぼかしの半径（ピクセル） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "リムの Bloom のぼかしの半径（ピクセル）", ClampMin = "0.0", UIMax = "64.0"))
	float OriginalBloomRadius;

	/** Original モード：画面全体（背景も含む）に掛ける、上から下へのグラデーションの強さ。0 なら掛けない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "グラデーションの強さ", ClampMin = "0.0", UIMax = "2.0"))
	float OriginalGradationStrength;

	/** Original モード：グラデーションで画面の上端に掛ける色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "グラデーションの上の色", HideAlphaChannel))
	FLinearColor OriginalGradationTopColor;

	/** Original モード：グラデーションで画面の下端に掛ける色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|Original モード", meta = (DisplayName = "グラデーションの下の色", HideAlphaChannel))
	FLinearColor OriginalGradationBottomColor;

	// ---- トゥーン：影 ----

	/** シャドウレイによるくっきりした影を付ける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影を付ける"))
	bool bCastShadows;

	/** シャドウレイの始点を面から浮かせる距離（cm）。明暗の境目に斑点が出たら上げる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影のバイアス（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0", EditCondition = "bCastShadows"))
	float ShadowBias;

	/** 光源の見かけの大きさ（度）。0 より大きいと、影を落とす物体から離れるほど影の境目がぼける（面光源のやわらかい影）。0 ならくっきりした影 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "やわらかい影：光源の大きさ（度）", ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "20.0", EditCondition = "bCastShadows"))
	float SoftShadowAngle;

	/** やわらかい影のシャドウレイの本数。多いほどなめらかだが重い */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "やわらかい影：シャドウレイの本数", ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16", EditCondition = "bCastShadows"))
	int32 SoftShadowSamples;

	/** やわらかい影を何段階に区切るか。0 か 1 ならなめらかなグラデーション、2 以上ならトゥーンらしい段階 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "やわらかい影：段階の数", ClampMin = "0", ClampMax = "16", UIMin = "0", UIMax = "8", EditCondition = "bCastShadows"))
	int32 SoftShadowSteps;

	/** 照り返し（間接光）の強さ。周りの面の色を足し、赤い壁の近くが赤みを帯びるなどの照り返しを出す。0 なら照り返しなし */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "照り返しの強さ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float IndirectStrength;

	/** 照り返しを調べるレイの本数。多いほどなめらかだが重い */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|陰影", meta = (DisplayName = "照り返しのレイの本数", ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16"))
	int32 IndirectSamples;

	/** 影の境目を Perlin ノイズで手描き風に揺らす強さ。0 なら揺らさない（0.1 で控えめ、0.3 で強め） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影の境目の揺れの強さ", ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5"))
	float ShadowNoiseStrength;

	/** 揺れの大きさ（cm）。大きいほどゆったりした揺れになる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影の境目の揺れの大きさ（cm）", ClampMin = "0.1", UIMin = "1.0", UIMax = "100.0"))
	float ShadowNoiseSize;

	// ---- トゥーン：ハイライトとリムライト ----

	/** ハイライトのしきい値（N・H）。1 に近いほど小さい */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|ハイライトとリムライト", meta = (DisplayName = "ハイライトのしきい値", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.8", UIMax = "1.0"))
	float HighlightThreshold;

	/** ハイライトの強さの全体倍率（メッシュごとの強さに掛ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|ハイライトとリムライト", meta = (DisplayName = "ハイライトの強さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HighlightStrength;

	/** リムライトのしきい値（1 - N・V）。1 に近いほど細い */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|ハイライトとリムライト", meta = (DisplayName = "リムライトのしきい値", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RimThreshold;

	/** リムライトの強さの全体倍率（メッシュごとの強さに掛ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|ハイライトとリムライト", meta = (DisplayName = "リムライトの強さ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float RimStrength;

	/** リムライトを光が当たる側だけに出す */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|ハイライトとリムライト", meta = (DisplayName = "リムライトは光の当たる側だけ"))
	bool bRimLitSideOnly;

	// ---- トゥーン：アウトライン ----

	/** 物体の輪郭・奥行きの段差・面の折れ目に線を引く */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "アウトラインを引く"))
	bool bOutline;

	/** スケルタルメッシュ以外（背景や小物など）の線の太さ（ピクセル）。0 なら線を引かない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線の太さ：背景など（ピクセル）", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0", EditCondition = "bOutline"))
	float OutlineWidth;

	/** スケルタルメッシュ（キャラクターなど）の線の太さ（ピクセル）。0 なら線を引かない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線の太さ：キャラクター（ピクセル）", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0", EditCondition = "bOutline"))
	float OutlineWidthSkinned;

	/** 線を引く段差のしきい値。小さいほど細かい段差にも線が出る */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線のしきい値", ClampMin = "0.0", UIMin = "0.1", UIMax = "5.0", EditCondition = "bOutline"))
	float OutlineThreshold;

	/** 顔の中の線：顔のパーツどうしの間では、面の折れ目の線を引かず、段差の許容量をこの倍率で広げる。唇や鼻の小さな凹凸に線が出ない（見下ろす角度で口が「ω」のような線になるのを防ぐ）。顔と首・髪の境目（輪郭）には影響しない。1 なら他の部分と同じ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "顔の中の線の許容量（倍率）", ClampMin = "1.0", UIMin = "1.0", UIMax = "200.0", EditCondition = "bOutline"))
	float FaceOutlineTolerance;

	/** 線の色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線の色", HideAlphaChannel, EditCondition = "bOutline"))
	FLinearColor OutlineColor;

	// ---- トゥーン：反射 ----

	/** 金属・ガラスで反射・屈折を追いかける回数。0 なら普通の素材として塗る */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "反射・屈折の回数", ClampMin = "0", ClampMax = "16", UIMin = "0", UIMax = "8"))
	int32 ReflectionDepth;

	/** すべての値を初期値に戻す */
	void ResetToDefaults();

	/** すべての値を対応するコンソール変数に反映する */
	void ApplyToConsoleVariables() const;

	/** コンソール変数の現在の値を読み込む（コンソールで変更した値をパネルに反映する） */
	void ImportFromConsoleVariables();

	/** フレーム間の蓄積をリセットする */
	static void ResetAccumulation();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
