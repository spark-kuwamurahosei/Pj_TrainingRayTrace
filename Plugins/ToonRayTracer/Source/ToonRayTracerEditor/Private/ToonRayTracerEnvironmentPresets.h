// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** 鳴潮風ルックの確認用：時間帯ごとの環境（Directional Light と Skylight の設定） */
enum class EToonEnvironmentPreset : uint8
{
	Morning,	// 朝
	Noon,		// 昼
	Evening,	// 夕方
	Night,		// 夜
};

namespace ToonRayTracerEnvironment
{
	/**
	 * エディタで開いているレベルの Directional Light と Skylight を、時間帯の設定に切り替える。
	 * 向き・色・強さ・空の太陽として使うかを変える（Ctrl+Z で元に戻せる）。
	 * 対象のライトが見つからなければ何もせず false を返す。
	 */
	bool ApplyPreset(EToonEnvironmentPreset Preset, FText& OutMessage);
}
