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

	/** カメラから見えている面に GBuffer の滑らかな法線を使う */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "滑らかな法線を使う"))
	bool bUseGBufferNormal;

	/** マテリアルを Custom Primitive Data で指定していないメッシュに、UE のマテリアルの色（GBuffer のベースカラー）を使う */
	UPROPERTY(EditAnywhere, config, Category = "表示", meta = (DisplayName = "マテリアルの色を使う"))
	bool bUseGBufferBaseColor;

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

	/** 線の太さ（ピクセル） */
	UPROPERTY(EditAnywhere, config, Category = "トゥーン|アウトライン", meta = (DisplayName = "線の太さ（ピクセル）", ClampMin = "0.0", UIMin = "0.5", UIMax = "5.0", EditCondition = "bOutline"))
	float OutlineWidth;

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
