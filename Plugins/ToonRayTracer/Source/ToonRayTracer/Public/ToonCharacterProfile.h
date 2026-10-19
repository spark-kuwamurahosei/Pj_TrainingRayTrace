#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ToonCharacterProfile.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UTexture2D;

// 顔の影の境目の作り方
UENUM(BlueprintType)
enum class EToonFaceShadowMethod : uint8
{
	// パネル（コンソール変数）の全体の設定に従う（「顔：影の境目を SDF テクスチャで作る」と「顔：SDF テクスチャ」）
	Global		UMETA(DisplayName = "全体の設定に従う"),
	// 頭の向きと光の向きから計算で作る
	Computed	UMETA(DisplayName = "計算で作る"),
	// このプロファイルの SDF テクスチャ（しきい値マップ）で作る
	SDF			UMETA(DisplayName = "SDF テクスチャで作る"),
};

// キャラクターのモデルごとのトゥーンの設定（プロファイル）
// モデルごとに 1 つ作り、キャラクターのアクターに付けた Toon Character コンポーネントで選ぶ
// 同じモデルを何体置いても、別のレベルでも、同じプロファイルを使い回せる
// ・パーツはマテリアルスロットの名前で指定する（モデルを読み込み直してスロットの順番が変わってもずれない）
// ・幅や距離は、両目の間の距離に対する倍率で指定する（キャラクターの大きさが違っても同じ値で合う）
// ・空や 0 の項目は、パネル（コンソール変数）の全体の設定を使う
UCLASS(BlueprintType, meta = (DisplayName = "Toon Character Profile"))
class TOONRAYTRACER_API UToonCharacterProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** このプロファイルを使うモデル。ボーンとスロットの名前の候補や、自動で入れる候補を探すのに使う（描画には使わない） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "モデル", meta = (DisplayName = "対象のモデル"))
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** 顔の向きを求める頭のボーン。空なら全体の設定（r.ToonRayTracer.Toon.FaceHeadBone） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ボーン", meta = (DisplayName = "頭のボーン", GetOptions = "GetBoneNameOptions"))
	FName HeadBone;

	/** 左目のボーン。顔の向きと、大きさの基準になる両目の間の距離を求める。空なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ボーン", meta = (DisplayName = "左目のボーン", GetOptions = "GetBoneNameOptions"))
	FName LeftEyeBone;

	/** 右目のボーン。空なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ボーン", meta = (DisplayName = "右目のボーン", GetOptions = "GetBoneNameOptions"))
	FName RightEyeBone;

	/** 顔の影の対象にするパーツ（マテリアルスロットの名前）。空なら全体の設定（スロット番号） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "顔のスロット", GetOptions = "GetSlotNameOptions"))
	TArray<FName> FaceSlots;

	/** 目の表現（陰影なし・上まぶたの影・瞳のハイライト）の対象にするパーツ。瞳の前に重なる透過のパーツも入れる。空なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "目のスロット", GetOptions = "GetSlotNameOptions"))
	TArray<FName> EyeSlots;

	/** 目のスロットの中で、目として扱う三角形の最初の番号。眉毛やまつ毛が目と同じスロットにあるときに使う */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "目の三角形の開始番号", ClampMin = "0"))
	int32 EyeTriangleStart = 0;

	/** 目のスロットの中で、目として扱う三角形の終わりの番号（この番号は含まない）。0 なら最後まで */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "目の三角形の終了番号", ClampMin = "0"))
	int32 EyeTriangleEnd = 0;

	/** 前髪の影を落とす髪のパーツ。空なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "髪のスロット", GetOptions = "GetSlotNameOptions"))
	TArray<FName> HairSlots;

	/** 眉毛の透けで、髪越しに見せるパーツ（眉毛など）。空なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "髪越しに見せるスロット", GetOptions = "GetSlotNameOptions"))
	TArray<FName> SeeThroughSlots;

	/** 目のスロットの中で、上まぶたの影をかけないパーツ（瞳・角膜など）。空なら目のスロット全体にかける */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "上まぶたの影をかけないスロット", GetOptions = "GetSlotNameOptions"))
	TArray<FName> EyeShadowExcludeSlots;

	/** 顔の半分の幅（両目の間の距離の何倍か）。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "顔の半分の幅", ClampMin = "0.0", UIMax = "3.0"))
	float FaceWidthScale = 0.0f;

	/** 顔の影の境目のぼかし幅（顔の半分の幅を 1 としたとき）。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "顔の境目のぼかし", ClampMin = "0.0", UIMax = "0.3"))
	float FaceShadowSoftness = 0.0f;

	/** 上まぶたの影の太さ。0 なら全体の設定（大きさの基準に合わせて自動で拡大縮小） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "上まぶたの影の太さ", ClampMin = "0.0", UIMax = "1.0"))
	float EyeShadowWidth = 0.0f;

	/** 瞳のハイライトで、目の範囲の端を探す距離。目の半径より大きくする。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "瞳のハイライトの探す距離", ClampMin = "0.0", UIMax = "2.0"))
	float EyeHighlightSearchRadius = 0.0f;

	/** 前髪の縁の下の影の幅。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "前髪の影の幅", ClampMin = "0.0", UIMax = "2.0"))
	float HairShadowWidth = 0.0f;

	/** 前髪の影（光の方向へ影を落とす方式）で、髪を探す距離。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "前髪の影の探す距離", ClampMin = "0.0", UIMax = "10.0"))
	float HairShadowDistance = 0.0f;

	/** 眉毛の透けで、髪の奥へ眉毛を探す距離。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "大きさ（両目の間の距離に対する倍率）", meta = (DisplayName = "眉毛の透けの探す距離", ClampMin = "0.0", UIMax = "10.0"))
	float SeeThroughDistance = 0.0f;

	/** 前髪の上に見せる眉毛を、眉毛のテクスチャに描かれた線の形にするときの明るさのしきい値（線形の輝度）
	 * テクスチャのこの明るさより暗い部分を眉毛の線とみなし、その形だけを Custom Depth に描いて、前髪の上に見せる
	 * 0 なら線の形を使わず、眉毛のメッシュの形のまま見せる（マテリアル M_ToonSeeThroughMask がないときも同じ） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "パーツ", meta = (DisplayName = "髪越しに見せる線のしきい値", ClampMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float SeeThroughMaskThreshold = 0.3f;

	// ---- 見た目（チェックを入れた項目だけ、全体の設定を上書きする） ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (InlineEditConditionToggle))
	bool bOverrideEdgeSoftness = false;

	/** 明るい面と影の境目のぼかし幅（N・L）。大きいほど柔らかい境目になる。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (DisplayName = "境目のぼかし", EditCondition = "bOverrideEdgeSoftness", ClampMin = "0.0", UIMax = "0.3"))
	float EdgeSoftness = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (InlineEditConditionToggle))
	bool bOverrideShadowColor = false;

	/** 影の面の色（素材の色に掛ける）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (DisplayName = "影色", EditCondition = "bOverrideShadowColor"))
	FLinearColor ShadowColor = FLinearColor(0.35f, 0.4f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (InlineEditConditionToggle))
	bool bOverrideSkinShadowColor = false;

	/** 肌の影の面の色。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (DisplayName = "肌の影色", EditCondition = "bOverrideSkinShadowColor"))
	FLinearColor SkinShadowColor = FLinearColor(0.62f, 0.42f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (InlineEditConditionToggle))
	bool bOverrideShadowSaturation = false;

	/** 影の色の明るさは保ったまま、彩度を上げる量。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|影", meta = (DisplayName = "影の色の鮮やかさ", EditCondition = "bOverrideShadowSaturation", ClampMin = "0.0", UIMax = "2.0"))
	float ShadowSaturation = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (InlineEditConditionToggle))
	bool bOverrideOutlineWidth = false;

	/** キャラクターの線の太さ。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (DisplayName = "線の太さ（ピクセル）", EditCondition = "bOverrideOutlineWidth", ClampMin = "0.0", UIMax = "5.0"))
	float OutlineWidth = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (InlineEditConditionToggle))
	bool bOverrideOutlineColorScale = false;

	/** 線の色を素材の色の何倍にするか（0 なら全体の線の色）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (DisplayName = "線の色の倍率", EditCondition = "bOverrideOutlineColorScale", ClampMin = "0.0", UIMax = "1.0"))
	float OutlineColorScale = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (InlineEditConditionToggle))
	bool bOverrideRimStrength = false;

	/** 光源の側の輪郭に入れる明るい縁の強さ。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (DisplayName = "リムライトの強さ", EditCondition = "bOverrideRimStrength", ClampMin = "0.0", UIMax = "5.0"))
	float RimStrength = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (InlineEditConditionToggle))
	bool bOverrideRimWidth = false;

	/** 光源の側の輪郭に入れる明るい縁の幅。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|輪郭", meta = (DisplayName = "リムライトの幅（ピクセル）", EditCondition = "bOverrideRimWidth", ClampMin = "0.0", UIMax = "10.0"))
	float RimWidth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (InlineEditConditionToggle))
	bool bOverrideFaceShadowCurve = false;

	/** 顔の影の境目を頬に沿って曲げる量。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "境目の曲がり", EditCondition = "bOverrideFaceShadowCurve", ClampMin = "0.0", UIMax = "1.0"))
	float FaceShadowCurve = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (InlineEditConditionToggle))
	bool bOverrideFaceShadowFrontBias = false;

	/** 横寄りの光で、鼻筋だけが細く明るくなるのを防ぐ補正。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "前からの光の補正", EditCondition = "bOverrideFaceShadowFrontBias", ClampMin = "0.0", ClampMax = "0.99"))
	float FaceShadowFrontBias = 0.4f;

	/** 顔の影の境目の作り方。「全体の設定に従う」なら、パネルの「顔：影の境目を SDF テクスチャで作る」と「顔：SDF テクスチャ」を使う */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "影の境目の作り方"))
	EToonFaceShadowMethod FaceShadowMethod = EToonFaceShadowMethod::Global;

	/** このキャラクターの顔の影のしきい値マップ。顔を正面から見た形で、光が画像の右から来るときに描いたマスクから sdf_shadow_threshold_map で作る（README の「顔の影の SDF 版」を参照） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "SDF テクスチャ", EditCondition = "FaceShadowMethod == EToonFaceShadowMethod::SDF", EditConditionHides))
	TObjectPtr<UTexture2D> FaceShadowSDFTexture;

	/** しきい値マップの半分の大きさ（顔の半分の幅を 1 としたとき）。0 なら全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "SDF テクスチャの大きさ", ClampMin = "0.0", UIMax = "3.0", EditCondition = "FaceShadowMethod == EToonFaceShadowMethod::SDF", EditConditionHides))
	float FaceShadowSDFScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (InlineEditConditionToggle))
	bool bOverrideFaceShadowSDFOffsetY = false;

	/** しきい値マップの中心の、目の高さからの上下の位置（顔の半分の幅を 1 としたとき。下が負）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|顔", meta = (DisplayName = "SDF テクスチャの上下の位置", UIMin = "-2.0", UIMax = "2.0", EditCondition = "bOverrideFaceShadowSDFOffsetY"))
	float FaceShadowSDFOffsetY = -0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (InlineEditConditionToggle))
	bool bOverrideEyeShadowBrightness = false;

	/** 上まぶたの影の部分を何倍の明るさにするか（1 なら影なし）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (DisplayName = "上まぶたの影の明るさ", EditCondition = "bOverrideEyeShadowBrightness", ClampMin = "0.0", ClampMax = "1.0"))
	float EyeShadowBrightness = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (InlineEditConditionToggle))
	bool bOverrideEyeHighlightStrength = false;

	/** 光に合わせた瞳のハイライトの強さ（0 ならなし）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (DisplayName = "瞳のハイライトの強さ", EditCondition = "bOverrideEyeHighlightStrength", ClampMin = "0.0", ClampMax = "1.0"))
	float EyeHighlightStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (InlineEditConditionToggle))
	bool bOverrideEyeHighlightThreshold = false;

	/** 1 に近いほどハイライトの点が小さい。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|目", meta = (DisplayName = "瞳のハイライトのしきい値", EditCondition = "bOverrideEyeHighlightThreshold", ClampMin = "0.0", ClampMax = "1.0"))
	float EyeHighlightThreshold = 0.99f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (InlineEditConditionToggle))
	bool bOverrideHairShadowStrength = false;

	/** 前髪の影の濃さ。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (DisplayName = "前髪の影の濃さ", EditCondition = "bOverrideHairShadowStrength", ClampMin = "0.0", ClampMax = "1.0"))
	float HairShadowStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (InlineEditConditionToggle))
	bool bOverrideHairShadowLightShift = false;

	/** 前髪の影を、影の幅に対して光の側へずらす割合。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (DisplayName = "前髪の影の光の側へのずらし", EditCondition = "bOverrideHairShadowLightShift", ClampMin = "0.0", UIMax = "2.0"))
	float HairShadowLightShift = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (InlineEditConditionToggle))
	bool bOverrideSeeThroughOpacity = false;

	/** 前髪の上に眉毛を見せる濃さ（0 なら透かさない）。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (DisplayName = "眉毛の透けの濃さ", EditCondition = "bOverrideSeeThroughOpacity", ClampMin = "0.0", ClampMax = "1.0"))
	float SeeThroughOpacity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (InlineEditConditionToggle))
	bool bOverrideSeeThroughBrightness = false;

	/** 眉毛の形を見せるときに、髪の色を何倍に暗くするか。チェックを入れたときだけ使い、外していれば全体の設定 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (DisplayName = "眉毛の透けの明るさ", EditCondition = "bOverrideSeeThroughBrightness", ClampMin = "0.0", ClampMax = "1.0"))
	float SeeThroughBrightness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (InlineEditConditionToggle))
	bool bOverrideSeeThroughColor = false;

	/** チェックを入れると、前髪の上の眉毛を、髪の色を暗くする代わりに眉毛そのものの色で塗る（光の色と、髪の明るさを掛ける）
	 * 色は、前髪から出て直接見えている部分の UE のマテリアルの色を自動で使う。まだ一度も画面に見えていないときだけ、ここで指定した色を使う */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "見た目|前髪・眉毛", meta = (DisplayName = "眉毛の透けの色", EditCondition = "bOverrideSeeThroughColor", HideAlphaChannel))
	FLinearColor SeeThroughColor = FLinearColor(0.3f, 0.08f, 0.09f);

	/** 対象のモデルのボーンとマテリアルスロットの名前から、ボーンとパーツの候補を自動で入れ、大きさの項目にも全体の設定から求めた値を入れる（入っている値は上書きする） */
	UFUNCTION(CallInEditor, Category = "モデル", meta = (DisplayName = "候補を自動で入れる"))
	void FillCandidates();

	/** 大きさの項目に、全体の設定から求めた値を入れる（入っている値は上書きする）
	 * 全体の設定の幅や距離（cm）を、大きさの基準（両目の間の距離 cm）で割った倍率になる。ここから調整を始める */
	UFUNCTION(CallInEditor, Category = "モデル", meta = (DisplayName = "大きさに全体の設定を入れる"))
	void FillSizesFromGlobalSettings();

	/** 見た目の項目に、今の全体の設定の値を入れる（チェックは変えない）。チェックを入れたときに、全体の設定から調整を始められる */
	UFUNCTION(CallInEditor, Category = "モデル", meta = (DisplayName = "見た目に全体の設定を入れる"))
	void FillLookFromGlobalSettings();

#if WITH_EDITOR
	// 髪越しに見せるスロットや線のしきい値を変えたら、このプロファイルを使うキャラクターの眉毛の線の形を作り直す
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	TArray<FString> GetBoneNameOptions() const;

	UFUNCTION()
	TArray<FString> GetSlotNameOptions() const;

	// 見た目の上書き：シェーダーへ渡す 5 つの float4 の値と、上書きする成分のビット（成分の番号 = 4 × ベクトルの番号 + 成分）
	//   [0] 影色 RGB、影の色の鮮やかさ
	//   [1] 肌の影色 RGB、線の太さ
	//   [2] 線の色の倍率、リムライトの強さ、リムライトの幅、顔の境目の曲がり
	//   [3] 顔の前からの光の補正、上まぶたの影の明るさ、瞳のハイライトの強さ、しきい値
	//   [4] 前髪の影の濃さ、前髪の影の光の側へのずらし、眉毛の透けの濃さ、明るさ
	//   [5] 眉毛の透けで塗る色 RGB、色で塗る割合
	//   [6] 明るい面と影の境目のぼかし、（空き）、（空き）、（空き）
	static constexpr int32 NumLookVectors = 7;
	void GetLookOverrides(uint32& OutMask, FVector4f (&OutLook)[NumLookVectors]) const;

	// メッシュのマテリアルスロットの名前を、レイトレーシングのジオメトリ番号（LOD 0 のセクション番号）のビットに変換する（0 ～ 63 番）
	static uint64 SlotNamesToSectionMask(const USkeletalMeshComponent* Mesh, const TArray<FName>& SlotNames);
};
