// Fill out your copyright notice in the Description page of Project Settings.

#include "ToonRayGenShader.h"

IMPLEMENT_GLOBAL_SHADER(FToonRayGenShader, "/Plugin/ToonRayTracer/Private/ToonRayGen.usf", "ToonRayGenMain", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FToonClosestHitShader, "/Plugin/ToonRayTracer/Private/ToonRayGen.usf", "closesthit=ToonClosestHitMain", SF_RayHitGroup);
IMPLEMENT_GLOBAL_SHADER(FToonMissShader, "/Plugin/ToonRayTracer/Private/ToonRayGen.usf", "ToonMissMain", SF_RayMiss);
IMPLEMENT_GLOBAL_SHADER(FToonObjectColorResolveCS, "/Plugin/ToonRayTracer/Private/ToonObjectColorResolve.usf", "ToonObjectColorResolveCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FToonPostEffectsPrepareCS, "/Plugin/ToonRayTracer/Private/ToonPostEffects.usf", "ToonPostEffectsPrepareCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FToonPostEffectsBlurCS, "/Plugin/ToonRayTracer/Private/ToonPostEffects.usf", "ToonPostEffectsBlurCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FToonPostEffectsCS, "/Plugin/ToonRayTracer/Private/ToonPostEffects.usf", "ToonPostEffectsCS", SF_Compute);
