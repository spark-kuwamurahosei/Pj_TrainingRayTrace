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

	/** 1 ピクセルあたりのサンプル数（アンチエイリアス） */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "サンプル数", ClampMin = "1", ClampMax = "64", UIMin = "1", UIMax = "16"))
	int32 SamplesPerPixel;

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

	// ---- トゥーン：影 ----

	/** シャドウレイによるくっきりした影を付ける */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影を付ける"))
	bool bCastShadows;

	/** シャドウレイの始点を面から浮かせる距離（cm）。明暗の境目に斑点が出たら上げる */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|影", meta = (DisplayName = "影のバイアス（cm）", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0", EditCondition = "bCastShadows"))
	float ShadowBias;

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
