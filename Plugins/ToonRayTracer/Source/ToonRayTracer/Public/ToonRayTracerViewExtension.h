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
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	// マテリアルの発光：ベースパスの直後のシーンの色（ライティング前なので、発光だけが入っている）を写し取る
	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}

	virtual void SubscribeToPostProcessingPass(
		EPostProcessingPass Pass,
		const FSceneView& InView,
		FPostProcessingPassDelegateArray& InOutPassCallbacks,
		bool bIsPassEnabled) override;

private:
	// トーンマップの後（出力解像度）に描く
	FScreenPassTexture RenderToonRayTracingPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);

	// 鳴潮風ルック フェーズ3：TSR の前（描画解像度、トーンマップ前の HDR）に描く
	FScreenPassTexture RenderToonRayTracingPassBeforeTSR(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);

	FScreenPassTexture RenderToonRayTracing(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs,
		bool bBeforeTSR);

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

	// 顔の影：キャラクターの顔の位置と向き（頭と両目のボーンから求める）
	struct FToonFace
	{
		// 両目の中点（ワールド空間）
		FVector Center = FVector::ZeroVector;
		// 顔の右と前の向き（水平、正規化済み）
		FVector3f Right = FVector3f(1.0f, 0.0f, 0.0f);
		FVector3f Forward = FVector3f(0.0f, 1.0f, 0.0f);
		// 顔の半分の幅と、この顔として扱う範囲の半径（cm）
		float HalfWidth = 0.0f;
		float Radius = 0.0f;
		// キャラクターごとの設定（プロファイルか全体の設定から、このキャラクター用に求めた値）
		// Slots：顔のスロット（0 ～ 31 番、32 ～ 63 番のビット）、目のスロット（同じ）
		// Slots2：髪のスロット、髪越しに見せるスロット
		FUintVector4 Slots = FUintVector4(0, 0, 0, 0);
		FUintVector4 Slots2 = FUintVector4(0, 0, 0, 0);
		// Params：目の三角形の開始番号、終了番号、顔の境目のぼかし、前髪の影の幅（cm）
		// Params2：上まぶたの影の太さ（cm）、眉毛の透けの探す距離（cm）、瞳のハイライトの探す距離（cm）、前髪の影の探す距離（cm）
		FVector4f Params = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		FVector4f Params2 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		// キャラクターの番号（Toon Character コンポーネントがなければ 0）
		uint32 CharacterId = 0;
		// 見た目の上書き（UToonCharacterProfile::GetLookOverrides を参照）。全体の設定との組み合わせはレンダースレッドで決める
		uint32 LookOverrideMask = 0;
		FVector4f Look[5] = { FVector4f(0.0f, 0.0f, 0.0f, 0.0f), FVector4f(0.0f, 0.0f, 0.0f, 0.0f), FVector4f(0.0f, 0.0f, 0.0f, 0.0f), FVector4f(0.0f, 0.0f, 0.0f, 0.0f), FVector4f(0.0f, 0.0f, 0.0f, 0.0f) };
	};

	// ゲームスレッドで、ビューファミリーのワールドにあるスケルタルメッシュから、顔の位置と向きを集める
	static void FindFaces(const FSceneViewFamily& ViewFamily, TArray<FToonFace>& OutFaces);

	// レンダースレッドからのみアクセスする。BeginRenderViewFamily でレンダーコマンド経由で更新される
	FToonDirectionalLight DirectionalLight_RenderThread;
	// 顔の影：キャラクターの顔の位置と向き
	TArray<FToonFace> Faces_RenderThread;
	// マテリアルの発光：ビューごとに、ベースパスの直後に写し取ったシーンの色（同じフレームのトゥーンのパスで使う）
	TMap<const FSceneView*, FRDGTextureRef> EmissiveTextures_RenderThread;
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
