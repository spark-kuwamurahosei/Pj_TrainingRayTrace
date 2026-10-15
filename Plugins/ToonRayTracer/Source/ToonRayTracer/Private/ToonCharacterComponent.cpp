#include "ToonCharacterComponent.h"
#include "ToonCharacterProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"

const FName UToonCharacterComponent::SeeThroughMaskTag(TEXT("ToonSeeThroughMask"));

namespace
{
	// 眉毛の線の形を描くマテリアル（プラグインの Content。Scripts/create_see_through_mask_material.py で作る）
	// テクスチャの明るさが Threshold より暗い部分（眉毛の線）だけを残す Masked のマテリアル
	const TCHAR* SeeThroughMaskMaterialPath = TEXT("/ToonRayTracer/Materials/M_ToonSeeThroughMask.M_ToonSeeThroughMask");
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
void UToonCharacterComponent::CreateSeeThroughMaskMesh()
{
	DestroySeeThroughMaskMesh();

	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || !Profile || Profile->SeeThroughSlots.IsEmpty() || Profile->SeeThroughMaskThreshold <= 0.0f)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = FindCharacterMesh(Owner);
	USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	const FSkeletalMeshRenderData* RenderData = Asset ? Asset->GetResourceForRendering() : nullptr;
	if (!RenderData)
	{
		return;
	}
	UMaterialInterface* MaskMaterial = LoadObject<UMaterialInterface>(nullptr, SeeThroughMaskMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!MaskMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToonRayTracer] %s was not found. Run Plugins/ToonRayTracer/Scripts/create_see_through_mask_material.py in the editor to create it"), SeeThroughMaskMaterialPath);
		return;
	}

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
	MaskMesh->bVisibleInRayTracing = false;
	MaskMesh->bVisibleInReflectionCaptures = false;
	MaskMesh->bVisibleInRealTimeSkyCaptures = false;
	MaskMesh->bAffectDistanceFieldLighting = false;
	MaskMesh->SetCastShadow(false);
	MaskMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaskMesh->SetGenerateOverlapEvents(false);
	MaskMesh->SetCanEverAffectNavigation(false);
	MaskMesh->bSelectable = false;

	// 髪越しに見せるスロットに、元のマテリアルのテクスチャで線の形を切り抜くマテリアルを割り当てる
	const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
	{
		if (!Profile->SeeThroughSlots.Contains(Materials[MaterialIndex].MaterialSlotName))
		{
			continue;
		}
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(MaskMaterial, MaskMesh);
		const UMaterialInterface* Original = Mesh->GetMaterial(MaterialIndex);
		TArray<UTexture*> Textures;
		if (Original)
		{
			Original->GetUsedTextures(Textures);
		}
		// 元のマテリアルの最初のテクスチャを、眉毛の線が描かれたテクスチャとみなす
		if (!Textures.IsEmpty() && Textures[0])
		{
			Instance->SetTextureParameterValue(TEXT("Texture"), Textures[0]);
		}
		Instance->SetScalarParameterValue(TEXT("Threshold"), Profile->SeeThroughMaskThreshold);
		MaskMesh->SetMaterial(MaterialIndex, Instance);
	}

	MaskMesh->RegisterComponent();

	// 髪越しに見せるスロット以外は描かない（Custom Depth に描く量を眉毛だけにする）
	for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
		{
			const bool bShow = Profile->SeeThroughSlots.Contains(Materials[MaterialIndex].MaterialSlotName);
			MaskMesh->ShowMaterialSection(MaterialIndex, INDEX_NONE, bShow, LODIndex);
		}
	}

	SeeThroughMaskMesh = MaskMesh;
}

void UToonCharacterComponent::DestroySeeThroughMaskMesh()
{
	if (SeeThroughMaskMesh)
	{
		SeeThroughMaskMesh->DestroyComponent();
		SeeThroughMaskMesh = nullptr;
	}
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
