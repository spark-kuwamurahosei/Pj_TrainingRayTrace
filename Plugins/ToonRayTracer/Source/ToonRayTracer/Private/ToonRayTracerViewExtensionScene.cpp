#include "ToonRayTracerViewExtension.h"
#include "ToonRayGenShader.h"
#include "ToonRayTracerCVars.h"
#include "ToonCharacterComponent.h"
#include "ToonCharacterProfile.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Texture2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"

// ToonRayTracerViewExtension のうち、ゲームスレッドでシーンから集める情報（Directional Light、キャラクターの顔と設定）

using namespace ToonRayTracerCVars;

// ゲームスレッドで、ビューファミリーのワールドにある Directional Light を探す
// 大気の太陽として使われているライトを優先し、なければ最も強いライトを使う
ToonRayTracerViewExtension::FToonDirectionalLight ToonRayTracerViewExtension::FindDirectionalLight(const FSceneViewFamily& ViewFamily)
{
	FToonDirectionalLight Result;

	const UWorld* World = ViewFamily.Scene ? ViewFamily.Scene->GetWorld() : nullptr;
	if (!World)
	{
		return Result;
	}

	const UDirectionalLightComponent* BestLight = nullptr;
	for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
	{
		const UDirectionalLightComponent* Light = *It;
		if (Light->GetWorld() != World || !Light->IsRegistered() || !Light->IsVisible() || !Light->bAffectsWorld || Light->Intensity <= 0.0f)
		{
			continue;
		}

		const bool bIsSun = Light->IsUsedAsAtmosphereSunLight() && Light->GetAtmosphereSunLightIndex() == 0;
		const bool bBestIsSun = BestLight && BestLight->IsUsedAsAtmosphereSunLight() && BestLight->GetAtmosphereSunLightIndex() == 0;
		if (!BestLight
			|| (bIsSun && !bBestIsSun)
			|| (bIsSun == bBestIsSun && Light->Intensity > BestLight->Intensity))
		{
			BestLight = Light;
		}
	}

	if (BestLight)
	{
		// GetDirection() は光が進む向きなので、光源へ向かう方向はその逆
		Result.ToLightDirection = FVector3f(-BestLight->GetDirection().GetSafeNormal());
		Result.Color = BestLight->GetLightColor();
		Result.Intensity = BestLight->Intensity;
		Result.bValid = true;
	}
	return Result;
}

// ゲームスレッドで、ビューファミリーのワールドにあるスケルタルメッシュから、顔の位置と向きを集める
// 頭と両目のボーンを持つメッシュを顔とみなす。ボーンの軸の向きはモデルによって違うため使わず、ボーンの位置だけから求める
// キャラクターごとの設定（パーツのスロット、幅や距離）も、ここで顔ごとに求めて一緒に渡す
// アクターに Toon Character コンポーネントがあり、プロファイルが選ばれていればその設定、なければ全体の設定を使う
// 全体の設定の幅や距離（cm）は、大きさの基準（ReferenceEyeDistance）に対する両目の間の距離の比で拡大縮小する
// ・右の向き：左目から右目へ（水平）
// ・前の向き：頭のボーンから両目の中点へ（水平で、右の向きに垂直）
void ToonRayTracerViewExtension::FindFaces(const FSceneViewFamily& ViewFamily, TArray<FToonFace>& OutFaces)
{
	const UWorld* World = ViewFamily.Scene ? ViewFamily.Scene->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const FName HeadBone(*CVarToonRayTracerToonFaceHeadBone.GetValueOnGameThread());
	const FName LeftEyeBone(*CVarToonRayTracerToonFaceLeftEyeBone.GetValueOnGameThread());
	const FName RightEyeBone(*CVarToonRayTracerToonFaceRightEyeBone.GetValueOnGameThread());
	const double WidthScale = FMath::Max(CVarToonRayTracerToonFaceWidthScale.GetValueOnGameThread(), 0.01f);
	const double ReferenceEyeDistance = FMath::Max(CVarToonRayTracerToonReferenceEyeDistance.GetValueOnGameThread(), 0.0f);

	// 全体の設定（パーツは 0 ～ 31 番のスロット番号、幅や距離は cm）
	const uint32 GlobalFaceMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonFaceSlotMask.GetValueOnGameThread(), 0));
	const uint32 GlobalEyeMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonEyeSlotMask.GetValueOnGameThread(), 0));
	const uint32 GlobalHairMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonHairSlotMask.GetValueOnGameThread(), 0));
	const uint32 GlobalSeeThroughMask = static_cast<uint32>(FMath::Max(CVarToonRayTracerToonSeeThroughSlotMask.GetValueOnGameThread(), 0));
	const int32 GlobalEyeTriangleStart = FMath::Max(CVarToonRayTracerToonEyeTriangleStart.GetValueOnGameThread(), 0);
	const int32 GlobalEyeTriangleEnd = FMath::Max(CVarToonRayTracerToonEyeTriangleEnd.GetValueOnGameThread(), 0);
	const float GlobalFaceShadowSoftness = FMath::Max(CVarToonRayTracerToonFaceShadowSoftness.GetValueOnGameThread(), 0.001f);
	const float GlobalHairShadowWidth = FMath::Max(CVarToonRayTracerToonHairShadowWidth.GetValueOnGameThread(), 0.0f);
	const float GlobalEyeShadowDistance = FMath::Max(CVarToonRayTracerToonEyeShadowDistance.GetValueOnGameThread(), 0.0f);
	const float GlobalSeeThroughDistance = FMath::Max(CVarToonRayTracerToonSeeThroughDistance.GetValueOnGameThread(), 0.0f);
	const float GlobalEyeHighlightSearchRadius = FMath::Max(CVarToonRayTracerToonEyeHighlightSearchRadius.GetValueOnGameThread(), 0.0f);
	const float GlobalHairShadowDistance = FMath::Max(CVarToonRayTracerToonHairShadowDistance.GetValueOnGameThread(), 0.0f);

	for (TObjectIterator<USkeletalMeshComponent> It; It && OutFaces.Num() < FToonRayGenShader::MaxFaces; ++It)
	{
		const USkeletalMeshComponent* Mesh = *It;
		// 眉毛の線の形を Custom Depth に描く複製は、キャラクターとして扱わない
		if (Mesh->GetWorld() != World || !Mesh->IsRegistered() || !Mesh->IsVisible() || UToonCharacterComponent::IsSeeThroughMaskMesh(Mesh))
		{
			continue;
		}

		const AActor* Owner = Mesh->GetOwner();
		const UToonCharacterComponent* Character = Owner ? Owner->FindComponentByClass<UToonCharacterComponent>() : nullptr;
		const UToonCharacterProfile* Profile = Character ? Character->Profile.Get() : nullptr;
		const FName MeshHeadBone = Profile && !Profile->HeadBone.IsNone() ? Profile->HeadBone : HeadBone;
		const FName MeshLeftEyeBone = Profile && !Profile->LeftEyeBone.IsNone() ? Profile->LeftEyeBone : LeftEyeBone;
		const FName MeshRightEyeBone = Profile && !Profile->RightEyeBone.IsNone() ? Profile->RightEyeBone : RightEyeBone;
		const double MeshWidthScale = Profile && Profile->FaceWidthScale > 0.0f ? static_cast<double>(Profile->FaceWidthScale) : WidthScale;
		if (Mesh->GetBoneIndex(MeshHeadBone) == INDEX_NONE || Mesh->GetBoneIndex(MeshLeftEyeBone) == INDEX_NONE || Mesh->GetBoneIndex(MeshRightEyeBone) == INDEX_NONE)
		{
			continue;
		}

		const FVector Head = Mesh->GetBoneLocation(MeshHeadBone);
		const FVector LeftEye = Mesh->GetBoneLocation(MeshLeftEyeBone);
		const FVector RightEye = Mesh->GetBoneLocation(MeshRightEyeBone);
		const double EyeDistance = FVector::Distance(LeftEye, RightEye);
		FVector Right = RightEye - LeftEye;
		Right.Z = 0.0;
		if (EyeDistance <= UE_KINDA_SMALL_NUMBER || !Right.Normalize())
		{
			continue;
		}

		const FVector Center = 0.5 * (LeftEye + RightEye);
		FVector Forward = Center - Head;
		Forward.Z = 0.0;
		Forward -= Right * FVector::DotProduct(Forward, Right);
		if (!Forward.Normalize())
		{
			continue;
		}

		FToonFace& Face = OutFaces.AddDefaulted_GetRef();
		Face.Center = Center;
		Face.Right = FVector3f(Right);
		Face.Forward = FVector3f(Forward);
		Face.HalfWidth = static_cast<float>(EyeDistance * MeshWidthScale);
		Face.Radius = static_cast<float>(EyeDistance * FaceRadiusScale);

		// パーツ：プロファイルのスロット名をセクションのビットに変換する。空なら全体の設定のスロット番号
		auto GetMask = [Mesh, Profile](const TArray<FName> UToonCharacterProfile::* Slots, uint32 GlobalMask)
		{
			return Profile && !(Profile->*Slots).IsEmpty() ? UToonCharacterProfile::SlotNamesToSectionMask(Mesh, Profile->*Slots) : uint64(GlobalMask);
		};
		// 眉毛の透けは、以前からのメッシュごとの上書き（Custom Primitive Data の [13]）も使う
		uint32 SeeThroughMask = GlobalSeeThroughMask;
		const TArray<float>& CustomData = Mesh->GetCustomPrimitiveData().Data;
		if (CustomData.IsValidIndex(13) && CustomData[13] > 0.0f)
		{
			SeeThroughMask = static_cast<uint32>(FMath::RoundToInt(CustomData[13]));
		}
		const uint64 FaceMask = GetMask(&UToonCharacterProfile::FaceSlots, GlobalFaceMask);
		const uint64 EyeMask = GetMask(&UToonCharacterProfile::EyeSlots, GlobalEyeMask);
		const uint64 HairMask = GetMask(&UToonCharacterProfile::HairSlots, GlobalHairMask);
		const uint64 SeeThroughSectionMask = GetMask(&UToonCharacterProfile::SeeThroughSlots, SeeThroughMask);
		Face.Slots = FUintVector4(static_cast<uint32>(FaceMask), static_cast<uint32>(FaceMask >> 32), static_cast<uint32>(EyeMask), static_cast<uint32>(EyeMask >> 32));
		Face.Slots2 = FUintVector4(static_cast<uint32>(HairMask), static_cast<uint32>(HairMask >> 32), static_cast<uint32>(SeeThroughSectionMask), static_cast<uint32>(SeeThroughSectionMask >> 32));
		// 上まぶたの影をかけないパーツは、プロファイルでだけ指定する（全体の設定はない）
		const uint64 EyeShadowExcludeMask = GetMask(&UToonCharacterProfile::EyeShadowExcludeSlots, 0u);
		// z：眉毛の線の形を Custom Depth に描いているか
		const bool bSeeThroughCustomDepth = Character && Character->HasSeeThroughMaskMesh();
		Face.Slots3 = FUintVector4(static_cast<uint32>(EyeShadowExcludeMask), static_cast<uint32>(EyeShadowExcludeMask >> 32), bSeeThroughCustomDepth ? 1u : 0u, 0u);

		// 幅や距離：プロファイルの値は両目の間の距離に対する倍率。0 なら全体の設定（cm）を、大きさの基準との比で拡大縮小する
		const double SizeScale = ReferenceEyeDistance > 0.0 ? EyeDistance / ReferenceEyeDistance : 1.0;
		auto GetWidth = [EyeDistance, SizeScale](float ProfileRatio, float GlobalWidth)
		{
			return static_cast<float>(ProfileRatio > 0.0f ? ProfileRatio * EyeDistance : GlobalWidth * SizeScale);
		};
		const bool bProfileEyeTriangles = Profile && !Profile->EyeSlots.IsEmpty();
		Face.Params = FVector4f(
			static_cast<float>(bProfileEyeTriangles ? Profile->EyeTriangleStart : GlobalEyeTriangleStart),
			static_cast<float>(bProfileEyeTriangles ? Profile->EyeTriangleEnd : GlobalEyeTriangleEnd),
			Profile && Profile->FaceShadowSoftness > 0.0f ? Profile->FaceShadowSoftness : GlobalFaceShadowSoftness,
			GetWidth(Profile ? Profile->HairShadowWidth : 0.0f, GlobalHairShadowWidth));
		Face.Params2 = FVector4f(
			GetWidth(Profile ? Profile->EyeShadowWidth : 0.0f, GlobalEyeShadowDistance),
			GetWidth(Profile ? Profile->SeeThroughDistance : 0.0f, GlobalSeeThroughDistance),
			GetWidth(Profile ? Profile->EyeHighlightSearchRadius : 0.0f, GlobalEyeHighlightSearchRadius),
			GetWidth(Profile ? Profile->HairShadowDistance : 0.0f, GlobalHairShadowDistance));

		// 見た目の上書きと、キャラクターの番号
		Face.CharacterId = Character ? Character->GetCharacterId() : 0;
		if (Profile)
		{
			Profile->GetLookOverrides(Face.LookOverrideMask, Face.Look);

			// 顔の影の作り方とプロファイルの SDF テクスチャ（テクスチャのリソースはレンダースレッドで読む）
			Face.FaceShadowMethod = static_cast<uint32>(Profile->FaceShadowMethod);
			if (Profile->FaceShadowMethod == EToonFaceShadowMethod::SDF && Profile->FaceShadowSDFTexture)
			{
				// シェーダーから直接読むので、すべてのミップを常にメモリに置く（ミップなしで取り込んでいれば 1 枚だけ）
				Profile->FaceShadowSDFTexture->SetForceMipLevelsToBeResident(ImageTextureResidentSeconds);
				Face.FaceSDFResource = Profile->FaceShadowSDFTexture->GetResource();
			}
			Face.FaceSDFScale = Profile->FaceShadowSDFScale;
			Face.FaceSDFOffsetY = Profile->FaceShadowSDFOffsetY;
			Face.bFaceSDFOffsetY = Profile->bOverrideFaceShadowSDFOffsetY;
		}
	}
}
