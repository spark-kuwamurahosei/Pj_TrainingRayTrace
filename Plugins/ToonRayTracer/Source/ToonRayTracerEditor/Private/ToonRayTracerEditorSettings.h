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

	/** リムを入れる点から光の方向へレイを飛ばし、壁やほかのキャラクターなどに遮られていればリムを消す（キャラクター自身は通り抜ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "リムライト：光が届く場所だけ", ClampMin = "0", ClampMax = "1"))
	int32 CharacterRimShadow;

	/** 前髪が目にかかっている場所に、目を線・瞳・白目の 3 色で半透明に重ねる。色は、目が画面に見えている間に段階ごとに記録した色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪越しの目", ClampMin = "0", ClampMax = "1"))
	int32 EyeSeeThrough;

	/** 前髪の上に重ねる目の濃さ（0 ～ 1） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪越しの目：濃さ", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float EyeSeeThroughOpacity;

	/** 髪から奥の目までの最大の距離（cm）。眉毛の透けの距離と同じ割合でキャラクターの大きさに合わせる。横の髪は前髪より目に近いことが多いため、横顔は角度で除く */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪越しの目：距離", ClampMin = "0.0", UIMax = "20.0"))
	float EyeSeeThroughDistance;

	/** 顔の正面とカメラの水平の角度（度）がこの値までなら重ねる。最後の 15° で薄くする（横顔で横の髪に目が出ないように） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "前髪越しの目：角度", ClampMin = "0.0", ClampMax = "180.0"))
	float EyeSeeThroughMaxAngle;

	/** ガラスの物体が後ろの面に集めた光の模様を、くっきりした段階で描く。面からガラスを通る光の道すじを逆向きにたどって求める */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "コースティクス", ClampMin = "0", ClampMax = "1"))
	int32 Caustics;

	/** 1 段階ごとに足す明るさ（明るい面の色に掛ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "コースティクス：明るさ", ClampMin = "0.0", UIMax = "2.0"))
	float CausticStrength;

	/** ガラスが光を集める割合（直接の光に対する倍率）がこれを超えた場所から模様にする。2 段階目は、この 3 倍から */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "コースティクス：しきい値", ClampMin = "0.1", UIMax = "5.0"))
	float CausticThreshold;

	/** 鏡のような金属に映った物体にも、アウトラインを引く。となりのピクセルの映り込みの先と比べて、輪郭や段差を見つける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "映り込みの中の線", ClampMin = "0", ClampMax = "1"))
	int32 ReflectionOutline;

	/** キャラクターだけトゥーンにするとき、鏡のような金属に映ったキャラクターも、トゥーンの塗りと線で描く（映り込みの先がキャラクターのピクセルだけ、通常の描画の代わりにトゥーンで描く） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "鏡に映ったキャラクターもトゥーン", ClampMin = "0", ClampMax = "1"))
	int32 ReflectionCharacters;

	/** 光に垂直な方向へ「太さ」の分だけずらした点からもシャドウレイを飛ばし、どれか 1 本でも光が届けば明るい面にする。細い物（髪の毛先など）の影は消え、大きな影の輪郭は単純になる。0：なし、1：キャラクターに落ちる影だけ、2：すべての面 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "影の形を整える", ClampMin = "0", ClampMax = "2"))
	int32 ShadowSimplify;

	/** シャドウレイの太さ（cm）。この 2 倍ほどより細い物は影を落とさない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "影の形を整える：太さ（cm）", ClampMin = "0.0", UIMax = "30.0"))
	float ShadowSimplifyRadius;

	/** へこんだ角（服のしわ、襟の折り目、指の間など）に線を入れる。面に沿った短いレイで、すぐ近くの面を見つけて判定する。0：なし、1：キャラクターだけ（顔と目は除く）、2：すべての面 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "くぼみの線", ClampMin = "0", ClampMax = "2"))
	int32 Crease;

	/** 面に沿って飛ばす短いレイの長さ（cm）。おおよそ線の太さになる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "くぼみの線：調べる距離（cm）", ClampMin = "0.01", UIMax = "60.0"))
	float CreaseDistance;

	/** 面に沿ったレイを、面から持ち上げる角度（sin）。大きいほど深い角だけに線が入る */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "くぼみの線：レイの持ち上げ", ClampMin = "0.0", ClampMax = "0.9", UIMax = "0.9"))
	float CreaseLift;

	/** 1 ピクセルあたりのレイの本数（1 ～ 16）。向きはフレームごとに変わり、蓄積や TSR で平均される */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "くぼみの線：レイの本数", ClampMin = "1", ClampMax = "16", UIMax = "16.0"))
	int32 CreaseSamples;

	/** 線の濃さ（0 ～ 1）。色はアウトラインと同じ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "くぼみの線：濃さ", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float CreaseStrength;

	/** TSR の前に描くとき、トゥーンの色（画面の値）のいちばん明るいチャンネルをこの値までに抑える。白に近い色はトーンマップの逆で極端に明るくなり、ブルームで光って見えるため。1 なら抑えない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "TSR の前：明るさの上限", ClampMin = "0.1", ClampMax = "1.0", UIMax = "1.0"))
	float BeforeTSRMaxBrightness;

	/** 奥まった場所（あごの下、襟の中など）に、もう一段暗い影を入れる。面から短いレイを飛ばし、近くで遮られる割合で決める。0：なし、1：キャラクターだけ（顔と目は除く）、2：すべての面 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影", ClampMin = "0", ClampMax = "2"))
	int32 DeepShadow;

	/** 遮られ具合を調べる短いレイの長さ（cm）。大きいほど広い範囲のくぼみが 2 影になる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：調べる距離（cm）", ClampMin = "0.01", UIMax = "100.0"))
	float DeepShadowDistance;

	/** 1 ピクセルあたりの短いレイの本数（1 ～ 16）。向きはフレームごとに変わり、蓄積や TSR で平均される */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：レイの本数", ClampMin = "1", ClampMax = "16", UIMax = "16.0"))
	int32 DeepShadowSamples;

	/** 遮られたレイの割合がこれを超えた場所を 2 影にする（0 ～ 1）。下げるほど広がる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：しきい値", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DeepShadowThreshold;

	/** 2 影の境目のぼかし幅（遮られたレイの割合） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：境目のぼかし", ClampMin = "0.0", ClampMax = "1.0", UIMax = "0.5"))
	float DeepShadowSoftness;

	/** 2 影の明るさ（影の色に掛ける倍率。1 なら普通の影と同じ） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：明るさ", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DeepShadowBrightness;

	/** 明るい面のくぼみ（しわの奥など）を、普通の影にする割合（0 なら明るい面は変えない） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "2 影：明るい面のくぼみを影にする", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DeepShadowLit;

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

	/** 顔の影の境目の形を、計算ではなく、顔の正面に投影したしきい値マップ（SDF テクスチャ）で決める。「顔：影の境目を計算で作る」が有効なときに使う */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：影の境目を SDF テクスチャで作る", EditCondition = "bFaceShadow"))
	bool bFaceShadowSDF;

	/** 顔の影のしきい値マップ。顔を正面から見た形で、光が画像の右から来るときに描く（sdf_shadow_threshold_map で、光が後ろから前の順に並べたマスクから作る。値の小さい場所ほど、光が後ろから前へ回るときに先に明るくなる） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：SDF テクスチャ", AllowedClasses = "/Script/Engine.Texture2D", EditCondition = "bFaceShadow && bFaceShadowSDF"))
	FSoftObjectPath FaceShadowSDFTexture;

	/** しきい値マップの半分の大きさ（顔の半分の幅を 1 としたとき） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：SDF テクスチャの大きさ", ClampMin = "0.1", UIMin = "0.5", UIMax = "3.0", EditCondition = "bFaceShadow && bFaceShadowSDF"))
	float FaceShadowSDFScale;

	/** しきい値マップの中心の、目の高さからの上下の位置（顔の半分の幅を 1 としたとき。下が負） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "顔：SDF テクスチャの上下の位置", UIMin = "-2.0", UIMax = "2.0", EditCondition = "bFaceShadow && bFaceShadowSDF"))
	float FaceShadowSDFOffsetY;

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

	/** テクスチャ（髪のマテリアルの Specular）から描く髪のハイライトの強さ。明るい面の色に掛けて足す。プロファイルで「髪のハイライトをテクスチャから描く」にしたキャラクターだけ */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：テクスチャの強さ", ClampMin = "0.0", UIMax = "1.0"))
	float HairHighlightTextureStrength;

	/** テクスチャの髪のハイライトの色（マスクが 1 の場所で、光の色を掛けて足す） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：テクスチャの色", HideAlphaChannel))
	FLinearColor HairHighlightTextureColor;

	/** テクスチャの髪のハイライト（と Original モードの髪のハイライト）を、輪郭の側で消す範囲（N・V がこれより小さいところほど薄くする。0 なら消さない） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：Falloff", ClampMin = "0.0", ClampMax = "1.0"))
	float HairHighlightFalloff;

	/** 光と視線から計算する髪のハイライト。0：なし、1：頭を一周する 1 本の帯、2：毛の束ごとの短い切れ端（髪のメッシュの向きから求める。帯の周りだけ）。テクスチャから描くキャラクターには入れない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：計算で作る形", ClampMin = "0", ClampMax = "2"))
	int32 HairHighlightMode;

	/** 光と視線に合わせて髪の上を動く、帯状のハイライト（天使の輪）の強さ（0 ～ 1）。0 ならなし。Original モードの髪には入れない */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：強さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HairHighlightStrength;

	/** 髪のハイライトの帯の幅 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：幅", ClampMin = "0.001", ClampMax = "1.0", UIMin = "0.01", UIMax = "0.5"))
	float HairHighlightWidth;

	/** 髪のハイライトの帯を毛の流れに沿ってずらす量（正：頭のてっぺんの側、負：毛先の側） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：上下のずらし", UIMin = "-0.5", UIMax = "0.5"))
	float HairHighlightShift;

	/** 髪のハイライトの下の縁のギザギザの大きさ（帯の幅に対する割合。0 ならまっすぐな帯） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：ギザギザの大きさ", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float HairHighlightJagged;

	/** 髪のハイライトの、頭の周りのギザギザの数 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：ギザギザの数", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0"))
	float HairHighlightJaggedCount;

	/** 髪のハイライトのギザギザの形（1：三角、大きいほど毛先のような細くとがった形） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：ギザギザのとがり", ClampMin = "0.1", UIMin = "1.0", UIMax = "8.0"))
	float HairHighlightJaggedSharpness;

	/** 髪のハイライトの縁のぼかし（帯の幅に対する割合。0 ならくっきり） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：縁のぼかし", ClampMin = "0.0", ClampMax = "1.0"))
	float HairHighlightSoftness;

	/** カメラから見た頭の横の側へ行くほど、髪のハイライトを細くする割合（1 なら輪郭で消える） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：両端を細く", ClampMin = "0.0", ClampMax = "1.0"))
	float HairHighlightTaper;

	/** 計算で作る形が 2 のとき、切れ端が出る範囲（帯の周り）の幅。帯の幅に対する倍率 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|キャラクター", meta = (DisplayName = "髪のハイライト：切れ端の範囲", ClampMin = "1.0", UIMax = "8.0"))
	float HairHighlightArea;

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

	/** ガラスの影の濃さ。0 なら影を落とさず、1 なら不透明の物体と同じ濃さ。シャドウレイが通り抜けたガラスの物体ごとに、光を (1 - この値) 倍にする */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "ガラスの影の濃さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bCastShadows"))
	float GlassShadowOpacity;

	/** 色の付いた影：色ガラス（ガラスのメッシュの Custom Primitive Data [1][2][3]）の影と透けた光を、ガラスの色に染める割合。0 なら灰色の影 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "色ガラスの影の色", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bCastShadows"))
	float GlassShadowColor;

	/** 色の付いた光：ポイントライトとスポットライト（ネオンなど光る物の近くに置いたライト）の光を、ライトの色でくっきり段階で塗る。シャドウレイで光が届く範囲を決める */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "色の付いた光", ClampMin = "0", ClampMax = "1"))
	int32 LocalLights;

	/** 色の付いた光の明るさ（素材の色とライトの色に掛ける） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "色の付いた光：明るさ", ClampMin = "0.0", UIMax = "2.0"))
	float LocalLightStrength;

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

	/** 奥行きで太さが変わる線：接している部分や面の折れ目は細く、背景から大きく離れた輪郭は太くする（手描きの線のような強弱） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "奥行きで線の太さを変える", EditCondition = "bOutline"))
	bool bOutlineDepthWidth;

	/** 奥行きの差が小さい部分（接している部分・面の折れ目）の線の太さの倍率 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "細い線の倍率", ClampMin = "0.0", UIMin = "0.1", UIMax = "1.0", EditCondition = "bOutline && bOutlineDepthWidth"))
	float OutlineThinScale;

	/** 背景から大きく離れた輪郭の線の太さの倍率 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "太い線の倍率", ClampMin = "0.0", UIMin = "1.0", UIMax = "4.0", EditCondition = "bOutline && bOutlineDepthWidth"))
	float OutlineThickScale;

	/** この奥行きの差（cm）以上離れた輪郭で、線がいちばん太くなる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "太い線になる奥行きの差（cm）", ClampMin = "0.0", UIMin = "1.0", UIMax = "200.0", EditCondition = "bOutline && bOutlineDepthWidth"))
	float OutlineDepthRange;

	/** 線の色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線の色", HideAlphaChannel, EditCondition = "bOutline"))
	FLinearColor OutlineColor;

	// ---- トゥーン：反射 ----

	/** 金属・ガラスで反射・屈折を追いかける回数。0 なら普通の素材として塗る */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "反射・屈折の回数", ClampMin = "0", ClampMax = "16", UIMin = "0", UIMax = "8"))
	int32 ReflectionDepth;

	/** ガラスの縁の帯の強さ。輪郭に近い部分に色の帯を乗せ、背景が単色でもガラスに見えるようにする。0 なら帯なし */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "ガラスの縁の強さ", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float GlassEdgeStrength;

	/** ガラスの縁の帯の幅（0 ～ 1）。球なら、輪郭から半径のこの割合までが帯になる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "ガラスの縁の幅", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float GlassEdgeWidth;

	/** ガラスの縁の帯の色 */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|反射", meta = (DisplayName = "ガラスの縁の色", HideAlphaChannel))
	FLinearColor GlassEdgeColor;

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
