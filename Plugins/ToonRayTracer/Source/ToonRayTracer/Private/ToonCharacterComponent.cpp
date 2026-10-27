#include "ToonCharacterComponent.h"
#include "ToonCharacterProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Templates/Function.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "RHI.h"
#include "ToonRayTracerCompat.h"

const FName UToonCharacterComponent::SeeThroughMaskTag(TEXT("ToonSeeThroughMask"));

namespace
{
	// 眉毛の線の形を描くマテリアル（プラグインの Content。Scripts/create_see_through_mask_material.py で作る）
	// テクスチャの明るさが Threshold より暗い部分（眉毛の線）だけを残す Masked のマテリアル
	const TCHAR* SeeThroughMaskMaterialPath = TEXT("/ToonRayTracer/Materials/M_ToonSeeThroughMask.M_ToonSeeThroughMask");
	// 前髪越しの目で、目のテクスチャを色の段階ごとに切り抜くマテリアル（Scripts/create_eye_tone_mask_material.py で作る）
	// テクスチャの明るさと鮮やかさが範囲の中の部分だけを残し、DepthBias だけカメラから遠ざける
	const TCHAR* EyeToneMaskMaterialPath = TEXT("/ToonRayTracer/Materials/M_ToonEyeToneMask.M_ToonEyeToneMask");
	// 瞳：鮮やかさ（(最大 - 最小) / 最大）がこの値以上で、明るさがこの値の範囲の部分
	// 瞳の暗い部分（上側の影）も瞳に含める。眉毛やまつ毛の線と色が近いため、線（ステンシル 1）と重なる部分は線を手前に残す
	constexpr float EyeToneIrisMinSaturation = 0.25f;
	constexpr float EyeToneIrisMinLuminance = 0.03f;
	constexpr float EyeToneIrisMaxLuminance = 0.92f;
	// 白目：明るさがこの値以上で、鮮やかさがこの値以下の部分
	constexpr float EyeToneWhiteMinLuminance = 0.55f;
	constexpr float EyeToneWhiteMaxSaturation = 0.12f;
	// 重なる場所で 線 → 瞳 → 白目 の順に手前に残すための、カメラから遠ざけるずらし（cm）
	constexpr float EyeToneIrisDepthBias = 0.1f;
	constexpr float EyeToneWhiteDepthBias = 0.2f;

	// 元のマテリアルから、そのパーツの色が描かれたテクスチャを探す
	// 色を表す名前のテクスチャのパラメーターを優先し、なければ使われているテクスチャのうち、ノーマルマップ以外の sRGB のもの
	// （最初のテクスチャがノーマルマップだと、青みの強い色として判定され、パーツ全体が瞳になってしまうため）
	UTexture* FindColorTexture(const UMaterialInterface* Original)
	{
		if (!Original)
		{
			return nullptr;
		}
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Original->GetAllTextureParameterInfo(ParameterInfos, ParameterIds);
		static const TCHAR* ColorNames[] = { TEXT("Diffuse"), TEXT("BaseColor"), TEXT("Albedo"), TEXT("Color") };
		for (const TCHAR* ColorName : ColorNames)
		{
			for (const FMaterialParameterInfo& Info : ParameterInfos)
			{
				UTexture* Texture = nullptr;
				if (Info.Name.ToString().Contains(ColorName) && Original->GetTextureParameterValue(Info, Texture) && Texture)
				{
					return Texture;
				}
			}
		}

		TArray<UTexture*> Textures;
#if TOON_RAY_TRACER_USED_TEXTURES_OPTIONAL
		Original->GetUsedTextures(Textures);
#else
		Original->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);
#endif
		for (UTexture* Texture : Textures)
		{
			if (Texture && Texture->SRGB && Texture->CompressionSettings != TC_Normalmap)
			{
				return Texture;
			}
		}
		return Textures.IsEmpty() ? nullptr : Textures[0];
	}
}

UToonCharacterComponent::UToonCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UToonCharacterComponent::IsSeeThroughMaskMesh(const UActorComponent* Component)
{
	return Component && Component->ComponentHasTag(SeeThroughMaskTag);
}

USkeletalMeshComponent* UToonCharacterComponent::FindCharacterMesh(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	TArray<USkeletalMeshComponent*> Meshes;
	Actor->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (!IsSeeThroughMaskMesh(Mesh))
		{
			return Mesh;
		}
	}
	return nullptr;
}

void UToonCharacterComponent::UpdateMeasuredEyeDistance()
{
	MeasuredEyeDistance = 0.0f;
	const USkeletalMeshComponent* Mesh = FindCharacterMesh(GetOwner());
	if (!Mesh)
	{
		return;
	}

	// プロファイルのボーン名、なければ全体の設定のボーン名を使う
	auto GetBone = [](FName ProfileBone, const TCHAR* ConsoleVariable)
	{
		if (!ProfileBone.IsNone())
		{
			return ProfileBone;
		}
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(ConsoleVariable);
		return Variable ? FName(*Variable->GetString()) : FName();
	};
	const FName LeftEye = GetBone(Profile ? Profile->LeftEyeBone : FName(), TEXT("r.ToonRayTracer.Toon.FaceLeftEyeBone"));
	const FName RightEye = GetBone(Profile ? Profile->RightEyeBone : FName(), TEXT("r.ToonRayTracer.Toon.FaceRightEyeBone"));
	if (Mesh->GetBoneIndex(LeftEye) != INDEX_NONE && Mesh->GetBoneIndex(RightEye) != INDEX_NONE)
	{
		MeasuredEyeDistance = static_cast<float>(FVector::Distance(Mesh->GetBoneLocation(LeftEye), Mesh->GetBoneLocation(RightEye)));
	}
}

void UToonCharacterComponent::WriteCharacterId()
{
	// float で正確に表せる範囲（1 ～ 2^24）で、1 から順に割り当てる
	static uint32 NextCharacterId = 1;
	if (CharacterId == 0)
	{
		CharacterId = NextCharacterId;
		NextCharacterId = NextCharacterId >= (1u << 24) ? 1 : NextCharacterId + 1;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	TArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		// 実行中の値はメッシュが登録し直されると初期値で上書きされるため、初期値に書き込む
		Mesh->SetDefaultCustomPrimitiveDataFloat(CustomDataCharacterId, static_cast<float>(CharacterId));
	}
}

// 眉毛の透け：前髪の奥の眉毛を、テクスチャに描かれた線の形で見せるため、眉毛の線だけを Custom Depth に描く
// レイトレーシングはマテリアルを評価しないため、隠れた眉毛のテクスチャの色（どこが線か）が分からない。そこで、
// キャラクターのメッシュの複製を作り、髪越しに見せるスロットだけを、テクスチャの暗い部分を残すマテリアルで Custom Depth に描く
// 複製は本体のボーンに追従し（Leader Pose）、通常の画面・レイトレーシング・影には映らない
// 前髪越しの目：同じように、目のスロットを色の段階（線・瞳・白目）ごとに切り抜いた複製を、Custom Depth のステンシルの値を変えて描く
// シェーダーは前髪に当たったピクセルのステンシルで、奥にある目の段階を知り、その段階の色を半透明で重ねる
USkeletalMeshComponent* UToonCharacterComponent::CreateMaskMeshCopy(USkeletalMeshComponent* Mesh, UMaterialInterface* MaskMaterial, const TSet<FName>& Slots,
	int32 StencilValue, TFunctionRef<void(UMaterialInstanceDynamic*)> SetParameters)
{
	AActor* Owner = GetOwner();
	USkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
	const FSkeletalMeshRenderData* RenderData = Asset->GetResourceForRendering();

	USkeletalMeshComponent* MaskMesh = NewObject<USkeletalMeshComponent>(Owner,
		MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("ToonSeeThroughMask")),
		RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
	MaskMesh->ComponentTags.Add(SeeThroughMaskTag);
	MaskMesh->SetSkeletalMeshAsset(Asset);
	MaskMesh->SetupAttachment(Mesh);
	MaskMesh->SetLeaderPoseComponent(Mesh);
	// Custom Depth にだけ描く
	MaskMesh->bRenderInMainPass = false;
	MaskMesh->bRenderInDepthPass = false;
	MaskMesh->bRenderCustomDepth = true;
	MaskMesh->CustomDepthStencilValue = StencilValue;
	MaskMesh->bVisibleInRayTracing = false;
	MaskMesh->bVisibleInReflectionCaptures = false;
	MaskMesh->bVisibleInRealTimeSkyCaptures = false;
	MaskMesh->bAffectDistanceFieldLighting = false;
	MaskMesh->SetCastShadow(false);
	MaskMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaskMesh->SetGenerateOverlapEvents(false);
	MaskMesh->SetCanEverAffectNavigation(false);
	MaskMesh->bSelectable = false;

	// 対象のスロットに、元のマテリアルのテクスチャで形を切り抜くマテリアルを割り当てる
	const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
	{
		if (!Slots.Contains(Materials[MaterialIndex].MaterialSlotName))
		{
			continue;
		}
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(MaskMaterial, MaskMesh);
		if (UTexture* ColorTexture = FindColorTexture(Mesh->GetMaterial(MaterialIndex)))
		{
			Instance->SetTextureParameterValue(TEXT("Texture"), ColorTexture);
		}
		SetParameters(Instance);
		MaskMesh->SetMaterial(MaterialIndex, Instance);
	}

	MaskMesh->RegisterComponent();

	// 対象のスロット以外は描かない（Custom Depth に描く量を減らす）
	for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
		{
			MaskMesh->ShowMaterialSection(MaterialIndex, INDEX_NONE, Slots.Contains(Materials[MaterialIndex].MaterialSlotName), LODIndex);
		}
	}
	return MaskMesh;
}

void UToonCharacterComponent::CreateSeeThroughMaskMesh()
{
	DestroySeeThroughMaskMesh();

	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || !Profile || Profile->SeeThroughMaskThreshold <= 0.0f)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = FindCharacterMesh(Owner);
	USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	if (!Asset || !Asset->GetResourceForRendering())
	{
		return;
	}

	// プロファイルのスロット。空なら、全体の設定のスロット番号（シェーダーと同じ。スロット番号 n なら 2^n のビット）から名前にする
	const TArray<FSkeletalMaterial>& AssetMaterials = Asset->GetMaterials();
	auto GetSlots = [&AssetMaterials](const TArray<FName>& ProfileSlots, const TCHAR* GlobalMaskVariable)
	{
		TSet<FName> Slots(ProfileSlots);
		if (Slots.IsEmpty())
		{
			const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(GlobalMaskVariable);
			const uint32 Mask = Variable ? static_cast<uint32>(Variable->GetInt()) : 0u;
			for (int32 MaterialIndex = 0; MaterialIndex < FMath::Min(AssetMaterials.Num(), 32); ++MaterialIndex)
			{
				if ((Mask & (1u << MaterialIndex)) != 0)
				{
					Slots.Add(AssetMaterials[MaterialIndex].MaterialSlotName);
				}
			}
		}
		return Slots;
	};
	const TSet<FName> SeeThroughSlots = GetSlots(Profile->SeeThroughSlots, TEXT("r.ToonRayTracer.Toon.SeeThroughSlotMask"));
	const TSet<FName> EyeSlots = GetSlots(Profile->EyeSlots, TEXT("r.ToonRayTracer.Toon.EyeSlotMask"));
	if (SeeThroughSlots.IsEmpty() && EyeSlots.IsEmpty())
	{
		return;
	}
	UMaterialInterface* MaskMaterial = LoadObject<UMaterialInterface>(nullptr, SeeThroughMaskMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!MaskMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToonRayTracer] %s was not found. Run Plugins/ToonRayTracer/Scripts/create_see_through_mask_material.py in the editor to create it"), SeeThroughMaskMaterialPath);
		return;
	}

	// 線：髪越しに見せるスロット（眉毛など）と目のスロットの、テクスチャの暗い部分（ステンシル 1）
	TSet<FName> LineSlots(SeeThroughSlots);
	LineSlots.Append(EyeSlots);
	const float Threshold = Profile->SeeThroughMaskThreshold;
	SeeThroughMaskMesh = CreateMaskMeshCopy(Mesh, MaskMaterial, LineSlots, EyeToneStencilLine,
		[Threshold](UMaterialInstanceDynamic* Instance) { Instance->SetScalarParameterValue(TEXT("Threshold"), Threshold); });

	// 前髪越しの目：目のスロットを、瞳（鮮やかな部分）と白目（明るく色みの少ない部分）に分けて描く
	// 線より奥にずらし（DepthBias）、重なる場所では 線 → 瞳 → 白目 の順に手前の段階が残るようにする
	if (EyeSlots.IsEmpty())
	{
		return;
	}
	UMaterialInterface* ToneMaterial = LoadObject<UMaterialInterface>(nullptr, EyeToneMaskMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!ToneMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToonRayTracer] %s was not found. Run Plugins/ToonRayTracer/Scripts/create_eye_tone_mask_material.py in the editor to create it"), EyeToneMaskMaterialPath);
		return;
	}
	EyeToneMeshes.Add(CreateMaskMeshCopy(Mesh, ToneMaterial, EyeSlots, EyeToneStencilIris, [](UMaterialInstanceDynamic* Instance)
	{
		Instance->SetScalarParameterValue(TEXT("LumMin"), EyeToneIrisMinLuminance);
		Instance->SetScalarParameterValue(TEXT("LumMax"), EyeToneIrisMaxLuminance);
		Instance->SetScalarParameterValue(TEXT("SatMin"), EyeToneIrisMinSaturation);
		Instance->SetScalarParameterValue(TEXT("SatMax"), 1.0f);
		Instance->SetScalarParameterValue(TEXT("DepthBias"), EyeToneIrisDepthBias);
	}));
	EyeToneMeshes.Add(CreateMaskMeshCopy(Mesh, ToneMaterial, EyeSlots, EyeToneStencilWhite, [](UMaterialInstanceDynamic* Instance)
	{
		Instance->SetScalarParameterValue(TEXT("LumMin"), EyeToneWhiteMinLuminance);
		Instance->SetScalarParameterValue(TEXT("LumMax"), 1.0f);
		Instance->SetScalarParameterValue(TEXT("SatMin"), 0.0f);
		Instance->SetScalarParameterValue(TEXT("SatMax"), EyeToneWhiteMaxSaturation);
		Instance->SetScalarParameterValue(TEXT("DepthBias"), EyeToneWhiteDepthBias);
	}));

	// ステンシルを描くには、Custom Depth をステンシル付き（r.CustomDepth 3）にする必要がある。実行中だけ切り替える（保存しない）
	if (IConsoleVariable* CustomDepth = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth")))
	{
		if (CustomDepth->GetInt() < 3)
		{
			CustomDepth->Set(3, ECVF_SetByCode);
			UE_LOG(LogTemp, Log, TEXT("[ToonRayTracer] r.CustomDepth was set to 3 (enabled with stencil) for the eyes seen through the bangs"));
		}
	}
}

void UToonCharacterComponent::DestroySeeThroughMaskMesh()
{
	if (SeeThroughMaskMesh)
	{
		SeeThroughMaskMesh->DestroyComponent();
		SeeThroughMaskMesh = nullptr;
	}
	for (USkeletalMeshComponent* ToneMesh : EyeToneMeshes)
	{
		if (ToneMesh)
		{
			ToneMesh->DestroyComponent();
		}
	}
	EyeToneMeshes.Reset();
}

void UToonCharacterComponent::OnRegister()
{
	Super::OnRegister();
	UpdateMeasuredEyeDistance();
	WriteCharacterId();
	CreateSeeThroughMaskMesh();
}

void UToonCharacterComponent::OnUnregister()
{
	DestroySeeThroughMaskMesh();
	Super::OnUnregister();
}

void UToonCharacterComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	const AActor* Owner = GetOwner();
	if (Owner && !bDestroyingHierarchy)
	{
		TArray<USkeletalMeshComponent*> Meshes;
		Owner->GetComponents<USkeletalMeshComponent>(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			Mesh->SetDefaultCustomPrimitiveDataFloat(CustomDataCharacterId, 0.0f);
		}
	}
	DestroySeeThroughMaskMesh();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UToonCharacterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateMeasuredEyeDistance();
	CreateSeeThroughMaskMesh();
}
#endif
