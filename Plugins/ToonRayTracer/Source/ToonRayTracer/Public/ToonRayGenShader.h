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

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
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
