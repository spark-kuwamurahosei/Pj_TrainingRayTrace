#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "ScreenPass.h"

class FSceneView;

// Original モード：レイトレーシングの後に掛ける仕上げの処理（ToonPostEffects.usf）に渡す値
struct FToonPostEffectsInputs
{
	// レイトレーシングの結果を書き込んだ出力（背景を含む画面全体）。その場で書き換える
	FScreenPassTexture Output;
	// RayGen の蓄積テクスチャ（線形の色）と、ぼかしの元（光の側のリムの色と、キャラクターの割合）
	FRDGTextureRef AccumulationTexture = nullptr;
	FRDGTextureRef GlowTexture = nullptr;
	// Directional Light の色（Diffusion Filter で掛ける）
	FVector3f LightColor = FVector3f(1.0f);
	// 出力の色の形式（0：表示の値、1：トーンマップの逆をかけた値、2：線形の色）と、トーンマップで掛かる露出
	uint32 OutputEncoding = 0;
	float EyeAdaptationExposure = 0.0f;
	// トーンマップの逆をかけるとき、表示の値のいちばん明るいチャンネルの上限（RayGen と同じ値）
	float MaxDisplayBrightness = 1.0f;
	// 画面の上端・下端で掛けるグラデーションの色
	FLinearColor GradationTopColor = FLinearColor::White;
	FLinearColor GradationBottomColor = FLinearColor::White;
};

// Original モードが有効なら、キャラクターをぼかして重ね（Diffusion Filter・Soft Focus・Bloom）、画面全体へ上から下へのグラデーションを掛けるパスを追加する
// 強さはコンソール変数（r.ToonRayTracer.Toon.Original*）から読む。どれも 0 なら何もしない
void AddToonPostEffectsPasses(FRDGBuilder& GraphBuilder, const FSceneView& View, const FToonPostEffectsInputs& Inputs);
