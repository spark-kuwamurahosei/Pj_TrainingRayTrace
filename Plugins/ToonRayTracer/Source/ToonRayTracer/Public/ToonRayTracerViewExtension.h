// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "RendererInterface.h"
#include "RenderGraphResources.h"
#include "UObject/StrongObjectPtr.h"

class UTexture2D;
class FTextureResource;

/**
 * 
 */
class TOONRAYTRACER_API ToonRayTracerViewExtension : public FSceneViewExtensionBase
{
public:
	ToonRayTracerViewExtension(const FAutoRegister& AutoRegister);
	virtual ~ToonRayTracerViewExtension() override;

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}

	virtual void SubscribeToPostProcessingPass(
		EPostProcessingPass Pass,
		const FSceneView& InView,
		FPostProcessingPassDelegateArray& InOutPassCallbacks,
		bool bIsPassEnabled) override;

private:
	FScreenPassTexture RenderToonRayTracingPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);

	// トゥーンシェーディングに使う Directional Light の情報
	struct FToonDirectionalLight
	{
		// 光源へ向かう方向（ワールド空間、正規化済み）
		FVector3f ToLightDirection = FVector3f(0.0f, 0.0f, 1.0f);
		// ライトの色（線形）
		FLinearColor Color = FLinearColor::Black;
		// ライトの強さ（ルクス）
		float Intensity = 0.0f;
		bool bValid = false;
	};

	// ゲームスレッドで、ビューファミリーのワールドから Directional Light を探す
	static FToonDirectionalLight FindDirectionalLight(const FSceneViewFamily& ViewFamily);

	// レンダースレッドからのみアクセスする。BeginRenderViewFamily でレンダーコマンド経由で更新される
	FToonDirectionalLight DirectionalLight_RenderThread;
	// トゥーンの影色（文字列のコンソール変数をゲームスレッドで変換したもの）
	FLinearColor ToonShadowColor_RenderThread = FLinearColor(0.35f, 0.4f, 0.6f);
	// 肌の影色（文字列のコンソール変数をゲームスレッドで変換したもの）
	FLinearColor ToonSkinShadowColor_RenderThread = FLinearColor(0.75f, 0.5f, 0.5f);
	// トゥーンのアウトラインの色（文字列のコンソール変数をゲームスレッドで変換したもの）
	FLinearColor ToonOutlineColor_RenderThread = FLinearColor(0.02f, 0.02f, 0.04f);

	// 『The Next Week』第4章：球に貼る画像（r.ToonRayTracer.ImageTexture のパスから読み込む）
	// ゲームスレッドで読み込み、ガベージコレクションで消えないよう保持する
	TStrongObjectPtr<UTexture2D> ImageTexture;
	FString ImageTexturePath;			// 読み込みを試したパス（同じパスで何度も読み込まないため）
	// レンダースレッドからのみアクセスする
	FTextureResource* ImageTextureResource_RenderThread = nullptr;

	// カメラが静止している間、フレームをまたいで結果を平均するための状態（ビューごと）
	struct FAccumulationState
	{
		// これまでの平均値（線形色）を保持するテクスチャ
		TRefCountPtr<IPooledRenderTarget> Texture;
		// 動く物体への対応：前のフレームの深度（見えている物体が変わったかを調べる）
		TRefCountPtr<IPooledRenderTarget> DepthTexture;
		// 動く物体への対応：ピクセルの中心で何が見えていたかの要約（影や映り込みが変わったかを調べる）
		TRefCountPtr<IPooledRenderTarget> SignatureTexture;
		// 動く物体への対応：前のフレームに、シーン内で物体が動いていたかのフラグ
		TRefCountPtr<FRDGPooledBuffer> SceneMotionBuffer;
		// テクスチャに蓄積済みのフレーム数（上限で止まる）
		uint32 AccumulatedFrames = 0;
		// 乱数の種に使う、毎フレーム進むカウンタ（上限で止まらない）
		uint32 RandomSeed = 0;
		// 前フレームのカメラと設定（変化したらリセットする）
		FMatrix WorldToView = FMatrix::Identity;
		FMatrix ViewToClipNoAA = FMatrix::Identity;
		FIntPoint ViewRectSize = FIntPoint::ZeroValue;
		uint32 SettingsHash = 0;
		// 最後に使われたフレーム番号（閉じたビューポートの状態を破棄するため）
		uint32 LastUsedFrameNumber = 0;
	};

	// レンダースレッドからのみアクセスする。キーはビューの状態（FSceneViewStateInterface）のアドレス
	TMap<const void*, TSharedPtr<FAccumulationState>> AccumulationStates;

	// 汎用化 G4：物体ごとの代表色の表（GPUScene のインスタンス番号はシーンごとに振られるため、シーンごとに持つ）
	struct FObjectColorTableState
	{
		// 物体ごとの平均色を保持するバッファ
		TRefCountPtr<FRDGPooledBuffer> Buffer;
		// 作成したときの r.ToonRayTracer.ResetAccumulation の値（変わったら表を空にする）
		int32 ResetCounter = 0;
		// 最後に使われたフレーム番号（閉じたシーンの表を破棄するため）
		uint32 LastUsedFrameNumber = 0;
	};

	// レンダースレッドからのみアクセスする。キーはシーン（FSceneInterface）のアドレス
	TMap<const void*, TSharedPtr<FObjectColorTableState>> ObjectColorTables;
};
