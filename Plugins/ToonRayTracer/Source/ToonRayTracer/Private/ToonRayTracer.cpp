// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToonRayTracer.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "ToonRayTracerViewExtension.h"
#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FToonRayTracerModule"

void FToonRayTracerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// シェーダーディレクトリの仮想パス登録
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ToonRayTracer"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/ToonRayTracer"), PluginShaderDir);

	// SVEの登録はエンジン本体の初期化が完全に終わるまで待機する
	FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FToonRayTracerModule::OnPostEngineInit);
}

void FToonRayTracerModule::OnPostEngineInit()
{
	// エンジンの準備が完了してからSVEをインスタンス化して登録する
	ToonViewExtension = FSceneViewExtensions::NewExtension<ToonRayTracerViewExtension>();
}

void FToonRayTracerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	// プラグイン終了時にSVEの登録を解除
	ToonViewExtension.Reset();
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FToonRayTracerModule, ToonRayTracer)