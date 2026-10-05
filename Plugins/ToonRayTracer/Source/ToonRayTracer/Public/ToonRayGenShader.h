// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "RayTracingPayloadType.h"
#include "ShaderParameterStruct.h"

class FToonRayGenShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonRayGenShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonRayGenShader, FGlobalShader);

	// シェーダーに渡せる球の最大数
	static constexpr int32 MaxSpheres = 8;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		// シーンの TLAS（Translated World 空間で構築されている）
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		// 0: 解析的な球、1: シーンの TLAS に TraceRay
		SHADER_PARAMETER(uint32, TraceMode)
		// クリップ空間 → Translated World 空間（カメラ位置が原点）への変換行列
		SHADER_PARAMETER(FMatrix44f, ClipToTranslatedWorld)
		// 出力テクスチャ内の描画範囲（ViewRect）
		SHADER_PARAMETER(FIntPoint, ViewRectMin)
		SHADER_PARAMETER(FIntPoint, ViewRectSize)
		// 球のリスト（xyz: Translated World 空間の中心、w: 半径）
		SHADER_PARAMETER_ARRAY(FVector4f, Spheres, [MaxSpheres])
		SHADER_PARAMETER(uint32, NumSpheres)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TOON_MAX_SPHERES"), MaxSpheres);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};

// パイプライン作成に必須のヒットシェーダー（フェーズ1では未使用）
class FToonClosestHitShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonClosestHitShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonClosestHitShader, FGlobalShader);

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};

// SBT作成に必須のMissシェーダー
class FToonMissShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FToonMissShader);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FToonMissShader, FGlobalShader);

	using FParameters = FEmptyShaderParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::Default;
	}
};
