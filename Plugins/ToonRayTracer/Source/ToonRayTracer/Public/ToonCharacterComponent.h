#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ToonCharacterComponent.generated.h"

class UToonCharacterProfile;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// キャラクターごとのトゥーンの設定
// キャラクターのアクターに付けて、そのモデルのプロファイル（Toon Character Profile）を選ぶ
// 付けていないキャラクターは、パネル（コンソール変数）の全体の設定を使う
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent, DisplayName = "Toon Character"))
class TOONRAYTRACER_API UToonCharacterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UToonCharacterComponent();

	/** このキャラクターのモデルのプロファイル。空なら全体の設定を使う */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "トゥーン", meta = (DisplayName = "プロファイル"))
	TObjectPtr<UToonCharacterProfile> Profile;

	/** 確認用：このキャラクターの両目の間の距離（cm）。パネルの「大きさの基準」に入れると、全体の設定の幅がこのキャラクターに合う */
	UPROPERTY(VisibleAnywhere, Transient, Category = "トゥーン", meta = (DisplayName = "両目の間の距離（cm）"))
	float MeasuredEyeDistance = 0.0f;

	// このキャラクターの番号（実行中に割り当てる）。同じアクターのスケルタルメッシュの Custom Primitive Data の [14] に書き込み、
	// シェーダーが体や服の点からも、このキャラクターの設定を見つけられるようにする
	static constexpr int32 CustomDataCharacterId = 14;
	uint32 GetCharacterId() const { return CharacterId; }

	// 眉毛の透け：眉毛の線の形を Custom Depth に描くための、キャラクターのメッシュの複製に付ける目印
	// 複製は通常の画面・レイトレーシング・影には映らないため、キャラクターとして扱わない
	static const FName SeeThroughMaskTag;
	static bool IsSeeThroughMaskMesh(const UActorComponent* Component);
	// アクターのキャラクターのメッシュ（眉毛の線の形を描く複製を除いた、最初のスケルタルメッシュ）
	static USkeletalMeshComponent* FindCharacterMesh(const AActor* Actor);

	// 前髪越しの目：Custom Depth のステンシルの値（線・瞳・白目）。シェーダーの TOON_EYE_TONE_* と同じ
	static constexpr int32 EyeToneStencilLine = 1;
	static constexpr int32 EyeToneStencilIris = 2;
	static constexpr int32 EyeToneStencilWhite = 3;

	// 眉毛の線の形を Custom Depth に描いているか（シェーダーは前髪の奥の眉毛を、この形で見せる）
	bool HasSeeThroughMaskMesh() const { return SeeThroughMaskMesh != nullptr; }
	// プロファイルの内容（髪越しに見せるスロットや線のしきい値）が変わったときに、眉毛の線の形を描く複製を作り直す
	void RefreshSeeThroughMaskMesh() { if (IsRegistered()) { CreateSeeThroughMaskMesh(); } }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateMeasuredEyeDistance();
	void WriteCharacterId();
	void CreateSeeThroughMaskMesh();
	void DestroySeeThroughMaskMesh();
	// キャラクターのメッシュの複製を作り、Slots のスロットだけを MaskMaterial で切り抜いて、Custom Depth にステンシル StencilValue で描く
	USkeletalMeshComponent* CreateMaskMeshCopy(USkeletalMeshComponent* Mesh, UMaterialInterface* MaskMaterial, const TSet<FName>& Slots,
		int32 StencilValue, TFunctionRef<void(UMaterialInstanceDynamic*)> SetParameters);

	uint32 CharacterId = 0;

	// 眉毛の線の形だけを Custom Depth に描く、キャラクターのメッシュの複製（保存しない）
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> SeeThroughMaskMesh;

	// 前髪越しの目：目のスロットを瞳・白目に切り抜いて Custom Depth に描く複製（保存しない）
	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> EyeToneMeshes;
};
