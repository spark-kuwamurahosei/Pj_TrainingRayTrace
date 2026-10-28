#include "ToonCharacterProfile.h"
#include "ToonCharacterComponent.h"
#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "ReferenceSkeleton.h"
#include "HAL/IConsoleManager.h"

namespace
{
	float GetConsoleFloat(const TCHAR* Name)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		return Variable ? Variable->GetFloat() : 0.0f;
	}

	// 色のコンソール変数（"R,G,B"）を読む。読めなければ Default
	FLinearColor GetConsoleColor(const TCHAR* Name, const FLinearColor& Default)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		TArray<FString> Parts;
		if (!Variable || Variable->GetString().ParseIntoArray(Parts, TEXT(","), true) != 3)
		{
			return Default;
		}
		return FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
	}

	bool ContainsAny(const FString& Name, std::initializer_list<const TCHAR*> Keywords)
	{
		for (const TCHAR* Keyword : Keywords)
		{
			if (Name.Contains(Keyword, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	// 名前の最後が左右を表すか（eye_L、EyeL、目.L、目_L、Eye_Left など）
	bool EndsWithSide(const FString& Name, bool bLeft)
	{
		const FString Lower = Name.ToLower();
		const TCHAR* Letter = bLeft ? TEXT("l") : TEXT("r");
		const TCHAR* Word = bLeft ? TEXT("left") : TEXT("right");
		return Lower.EndsWith(Word) || Lower.EndsWith(FString(TEXT("_")) + Letter) || Lower.EndsWith(FString(TEXT(".")) + Letter)
			|| (Name.Len() >= 2 && Name.EndsWith(bLeft ? TEXT("L") : TEXT("R"), ESearchCase::CaseSensitive));
	}
}

TArray<FString> UToonCharacterProfile::GetBoneNameOptions() const
{
	TArray<FString> Options;
	if (SkeletalMesh)
	{
		const FReferenceSkeleton& Skeleton = SkeletalMesh->GetRefSkeleton();
		for (int32 Index = 0; Index < Skeleton.GetNum(); ++Index)
		{
			Options.Add(Skeleton.GetBoneName(Index).ToString());
		}
	}
	return Options;
}

TArray<FString> UToonCharacterProfile::GetSlotNameOptions() const
{
	TArray<FString> Options;
	if (SkeletalMesh)
	{
		for (const FSkeletalMaterial& Material : SkeletalMesh->GetMaterials())
		{
			Options.Add(Material.MaterialSlotName.ToString());
		}
	}
	return Options;
}

void UToonCharacterProfile::FillCandidates()
{
	if (!SkeletalMesh)
	{
		return;
	}
	Modify();

	// ボーン：頭は「頭」か head で終わる名前、目は eye か「目」を含み、左右を表す名前で終わるもの
	// （まぶた・眉毛・まつ毛・ハイライトなど、目の周りの補助ボーンは除く）
	const FReferenceSkeleton& Skeleton = SkeletalMesh->GetRefSkeleton();
	HeadBone = NAME_None;
	LeftEyeBone = NAME_None;
	RightEyeBone = NAME_None;
	for (int32 Index = 0; Index < Skeleton.GetNum(); ++Index)
	{
		const FName BoneName = Skeleton.GetBoneName(Index);
		const FString Name = BoneName.ToString();
		if (HeadBone.IsNone() && (Name == TEXT("頭") || Name.EndsWith(TEXT("head"), ESearchCase::IgnoreCase)))
		{
			HeadBone = BoneName;
		}
		const bool bEye = ContainsAny(Name, { TEXT("eye"), TEXT("目") })
			&& !ContainsAny(Name, { TEXT("lid"), TEXT("brow"), TEXT("lash"), TEXT("hi"), TEXT("まぶた"), TEXT("眉"), TEXT("両目"), TEXT("dummy"), TEXT("shadow") });
		if (bEye && LeftEyeBone.IsNone() && EndsWithSide(Name, true))
		{
			LeftEyeBone = BoneName;
		}
		if (bEye && RightEyeBone.IsNone() && EndsWithSide(Name, false))
		{
			RightEyeBone = BoneName;
		}
	}

	// パーツ：マテリアルスロットの名前に含まれる言葉で分ける
	FaceSlots.Reset();
	SkinSlots.Reset();
	EyeSlots.Reset();
	HairSlots.Reset();
	SeeThroughSlots.Reset();
	for (const FSkeletalMaterial& Material : SkeletalMesh->GetMaterials())
	{
		const FName SlotName = Material.MaterialSlotName;
		const FString Name = SlotName.ToString();
		if (ContainsAny(Name, { TEXT("brow"), TEXT("mayu"), TEXT("眉") }))
		{
			SeeThroughSlots.Add(SlotName);
		}
		else if (ContainsAny(Name, { TEXT("eye"), TEXT("hitomi"), TEXT("iris"), TEXT("pupil"), TEXT("瞳"), TEXT("目") })
			&& !ContainsAny(Name, { TEXT("lash"), TEXT("まつ") }))
		{
			EyeSlots.Add(SlotName);
		}
		else if (ContainsAny(Name, { TEXT("hair"), TEXT("bang"), TEXT("kami"), TEXT("髪") })
			&& !ContainsAny(Name, { TEXT("accessory"), TEXT("acc"), TEXT("pin"), TEXT("飾") }))
		{
			HairSlots.Add(SlotName);
		}
		else if (ContainsAny(Name, { TEXT("face"), TEXT("kao"), TEXT("顔") }))
		{
			FaceSlots.Add(SlotName);
		}
		else if (ContainsAny(Name, { TEXT("skin"), TEXT("hada"), TEXT("肌") }) || Name.EndsWith(TEXT("_sk"), ESearchCase::IgnoreCase))
		{
			SkinSlots.Add(SlotName);
		}
	}
	// 顔の肌も肌として扱う
	SkinSlots.Append(FaceSlots);

	FillSizesFromGlobalSettings();
	FillLookFromGlobalSettings();
}

void UToonCharacterProfile::FillSizesFromGlobalSettings()
{
	Modify();

	// 割合の項目は、そのまま全体の設定を入れる
	FaceWidthScale = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.FaceWidthScale"));
	FaceShadowSoftness = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowSoftness"));

	// cm の項目は、大きさの基準（全体の設定を合わせたキャラクターの両目の間の距離）で割って、倍率にする
	// 基準が 0（拡大縮小しない）のときは、対象のモデルの両目の間の距離で割る（このモデルで全体の設定と同じ cm になる）
	double EyeDistance = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.ReferenceEyeDistance"));
	if (EyeDistance <= 0.0 && SkeletalMesh && !LeftEyeBone.IsNone() && !RightEyeBone.IsNone())
	{
		const FReferenceSkeleton& Skeleton = SkeletalMesh->GetRefSkeleton();
		const int32 Left = Skeleton.FindBoneIndex(LeftEyeBone);
		const int32 Right = Skeleton.FindBoneIndex(RightEyeBone);
		if (Left != INDEX_NONE && Right != INDEX_NONE)
		{
			auto GetComponentSpaceLocation = [&Skeleton](int32 BoneIndex)
			{
				FTransform Transform = Skeleton.GetRefBonePose()[BoneIndex];
				for (int32 Parent = Skeleton.GetParentIndex(BoneIndex); Parent != INDEX_NONE; Parent = Skeleton.GetParentIndex(Parent))
				{
					Transform = Transform * Skeleton.GetRefBonePose()[Parent];
				}
				return Transform.GetLocation();
			};
			EyeDistance = FVector::Distance(GetComponentSpaceLocation(Left), GetComponentSpaceLocation(Right));
		}
	}
	if (EyeDistance <= 0.0)
	{
		return;
	}
	auto ToRatio = [EyeDistance](const TCHAR* Name)
	{
		return static_cast<float>(GetConsoleFloat(Name) / EyeDistance);
	};
	EyeShadowWidth = ToRatio(TEXT("r.ToonRayTracer.Toon.EyeShadowDistance"));
	EyeHighlightSearchRadius = ToRatio(TEXT("r.ToonRayTracer.Toon.EyeHighlightSearchRadius"));
	HairShadowWidth = ToRatio(TEXT("r.ToonRayTracer.Toon.HairShadowWidth"));
	HairShadowDistance = ToRatio(TEXT("r.ToonRayTracer.Toon.HairShadowDistance"));
	SeeThroughDistance = ToRatio(TEXT("r.ToonRayTracer.Toon.SeeThroughDistance"));
}

void UToonCharacterProfile::FillLookFromGlobalSettings()
{
	Modify();
	ShadowColor = GetConsoleColor(TEXT("r.ToonRayTracer.Toon.ShadowColor"), ShadowColor);
	SkinShadowColor = GetConsoleColor(TEXT("r.ToonRayTracer.Toon.SkinShadowColor"), SkinShadowColor);
	ShadowSaturation = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.ShadowSaturation"));
	EdgeSoftness = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.EdgeSoftness"));
	OutlineWidth = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.OutlineWidthSkinned"));
	OutlineColorScale = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.CharacterOutlineColorScale"));
	RimStrength = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimStrength"));
	RimWidth = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.CharacterRimWidth"));
	FaceShadowCurve = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowCurve"));
	FaceShadowFrontBias = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.FaceShadowFrontBias"));
	EyeShadowBrightness = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.EyeShadowBrightness"));
	EyeHighlightStrength = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightStrength"));
	EyeHighlightThreshold = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.EyeHighlightThreshold"));
	HairShadowStrength = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.HairShadowStrength"));
	HairHighlightStrength = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightStrength"));
	HairHighlightWidth = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightWidth"));
	HairHighlightShift = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.HairHighlightShift"));
	HairShadowLightShift = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.HairShadowLightShift"));
	SeeThroughOpacity = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughOpacity"));
	SeeThroughBrightness = GetConsoleFloat(TEXT("r.ToonRayTracer.Toon.SeeThroughBrightness"));
	SeeThroughColor = GetConsoleColor(TEXT("r.ToonRayTracer.Toon.SeeThroughColor"), SeeThroughColor);
}

void UToonCharacterProfile::GetLookOverrides(uint32& OutMask, FVector4f (&OutLook)[NumLookVectors]) const
{
	OutMask = 0;
	for (FVector4f& Vector : OutLook)
	{
		Vector = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
	}
	auto SetFloat = [&OutMask, &OutLook](bool bOverride, int32 Vector, int32 Component, float Value)
	{
		if (bOverride)
		{
			OutLook[Vector][Component] = Value;
			OutMask |= 1u << (Vector * 4 + Component);
		}
	};
	auto SetColor = [&SetFloat](bool bOverride, int32 Vector, const FLinearColor& Color)
	{
		SetFloat(bOverride, Vector, 0, Color.R);
		SetFloat(bOverride, Vector, 1, Color.G);
		SetFloat(bOverride, Vector, 2, Color.B);
	};
	SetColor(bOverrideShadowColor, 0, ShadowColor);
	SetFloat(bOverrideShadowSaturation, 0, 3, ShadowSaturation);
	SetColor(bOverrideSkinShadowColor, 1, SkinShadowColor);
	SetFloat(bOverrideOutlineWidth, 1, 3, OutlineWidth);
	SetFloat(bOverrideOutlineColorScale, 2, 0, OutlineColorScale);
	SetFloat(bOverrideRimStrength, 2, 1, RimStrength);
	SetFloat(bOverrideRimWidth, 2, 2, RimWidth);
	SetFloat(bOverrideFaceShadowCurve, 2, 3, FaceShadowCurve);
	SetFloat(bOverrideFaceShadowFrontBias, 3, 0, FaceShadowFrontBias);
	SetFloat(bOverrideEyeShadowBrightness, 3, 1, EyeShadowBrightness);
	SetFloat(bOverrideEyeHighlightStrength, 3, 2, EyeHighlightStrength);
	SetFloat(bOverrideEyeHighlightThreshold, 3, 3, EyeHighlightThreshold);
	SetFloat(bOverrideHairShadowStrength, 4, 0, HairShadowStrength);
	SetFloat(bOverrideHairShadowLightShift, 4, 1, HairShadowLightShift);
	SetFloat(bOverrideSeeThroughOpacity, 4, 2, SeeThroughOpacity);
	SetFloat(bOverrideSeeThroughBrightness, 4, 3, SeeThroughBrightness);
	// 色を指定したら、髪を暗くする代わりにその色で塗る
	SetColor(bOverrideSeeThroughColor, 5, SeeThroughColor);
	SetFloat(bOverrideSeeThroughColor, 5, 3, 1.0f);
	SetFloat(bOverrideEdgeSoftness, 6, 0, EdgeSoftness);
	SetFloat(bOverrideHairHighlightStrength, 6, 1, HairHighlightStrength);
	SetFloat(bOverrideHairHighlightWidth, 6, 2, HairHighlightWidth);
	SetFloat(bOverrideHairHighlightShift, 6, 3, HairHighlightShift);
}

uint64 UToonCharacterProfile::SlotNamesToSectionMask(const USkeletalMeshComponent* Mesh, const TArray<FName>& SlotNames)
{
	const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	const FSkeletalMeshRenderData* RenderData = Asset ? Asset->GetResourceForRendering() : nullptr;
	if (SlotNames.IsEmpty() || !RenderData || RenderData->LODRenderData.IsEmpty())
	{
		return 0;
	}

	// LOD 0 のセクションの順番が、レイトレーシングのジオメトリ番号になる
	const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
	const FSkeletalMeshLODInfo* LODInfo = Asset->GetLODInfo(0);
	const TArray<FSkelMeshRenderSection>& Sections = RenderData->LODRenderData[0].RenderSections;
	uint64 Mask = 0;
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num() && SectionIndex < 64; ++SectionIndex)
	{
		int32 MaterialIndex = Sections[SectionIndex].MaterialIndex;
		if (LODInfo && LODInfo->LODMaterialMap.IsValidIndex(SectionIndex) && LODInfo->LODMaterialMap[SectionIndex] != INDEX_NONE)
		{
			MaterialIndex = LODInfo->LODMaterialMap[SectionIndex];
		}
		if (Materials.IsValidIndex(MaterialIndex) && SlotNames.Contains(Materials[MaterialIndex].MaterialSlotName))
		{
			Mask |= uint64(1) << SectionIndex;
		}
	}
	return Mask;
}

#if WITH_EDITOR
void UToonCharacterProfile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	for (TObjectIterator<UToonCharacterComponent> It; It; ++It)
	{
		if (It->Profile == this)
		{
			It->RefreshSeeThroughMaskMesh();
		}
	}
}
#endif
