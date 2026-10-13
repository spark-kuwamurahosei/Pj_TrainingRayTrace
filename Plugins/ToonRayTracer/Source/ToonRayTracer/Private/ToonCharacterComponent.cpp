#include "ToonCharacterComponent.h"
#include "ToonCharacterProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

UToonCharacterComponent::UToonCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UToonCharacterComponent::UpdateMeasuredEyeDistance()
{
	MeasuredEyeDistance = 0.0f;
	const AActor* Owner = GetOwner();
	const USkeletalMeshComponent* Mesh = Owner ? Owner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
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

void UToonCharacterComponent::OnRegister()
{
	Super::OnRegister();
	UpdateMeasuredEyeDistance();
	WriteCharacterId();
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
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UToonCharacterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateMeasuredEyeDistance();
}
#endif
