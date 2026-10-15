#pragma once

#include "CoreMinimal.h"

// 解析的な物体のシーン（r.ToonRayTracer.TraceMode 0、r.ToonRayTracer.AnalyticScene で選ぶ）の型
// データ（本の各章のシーンの球・四角形・箱）と BVH の組み立ては ToonRayTracerAnalyticScene.cpp
namespace ToonRayTracerAnalytic
{
	// マテリアルの種類（シェーダー側の TOON_MATERIAL_* と一致させる）
	enum class EToonMaterialType : uint32
	{
		Default = 0,	// 未設定（灰色のランバート反射）
		Lambertian = 1,	// 拡散反射（本の lambertian）
		Metal = 2,		// 金属（本の metal）
		Dielectric = 3,	// ガラスなどの誘電体（本の dielectric）
		DiffuseLight = 4,	// 『The Next Week』第7章の光る素材（本の diffuse_light）
		Volume = 5,			// 『The Next Week』第9章の煙や霧（本の constant_medium と isotropic）
	};

	// テクスチャの種類（シェーダー側の TOON_TEXTURE_* と一致させる）
	enum class EToonTextureType : uint32
	{
		Solid = 0,				// 単色（本の solid_color）
		Checker = 1,			// 空間のチェッカー（本の checker_texture）
		SphereUVChecker = 2,	// 球のテクスチャ座標（UV）上のチェッカー
		Marble = 3,				// 『The Next Week』第5章の大理石模様（本の noise_texture）
		Image = 4,				// 『The Next Week』第4章の画像テクスチャ（本の image_texture）。画像がなければ UV のチェッカーで代用
	};

	struct FToonSphere
	{
		FVector Center;				// ワールド座標（cm）
		double Radius;				// 半径（cm）
		EToonMaterialType Material;
		FLinearColor Albedo;		// 反射率（本の albedo）
		float Fuzz;					// 金属の反射のぼけ具合（0 ～ 1）
		float RefractionIndex;		// 誘電体の屈折率（本の refraction_index）
		// 『The Next Week』第4章：テクスチャ
		EToonTextureType Texture = EToonTextureType::Solid;
		float TextureScale = 1.0f;	// チェッカー：1 マスの辺の長さ（cm）、UV のチェッカー：経度方向のマスの数、大理石：1m あたりの縞の細かさ
		FLinearColor Albedo2 = FLinearColor::Black;	// チェッカーのもう一方の色
		float TextureUnitLength = 100.0f;	// 大理石：本の 1 単位の長さ（cm）。第5章のシーンは 1m、第10章は本の単位をそのまま cm にしている
		float Density = 0.0f;		// 煙の濃さ（1cm あたりに散乱する確率、本の constant_medium の density）
		// 『The Next Week』第2章：シャッターが開いている間に中心が動く量（cm、本の center2 - center1）
		FVector Motion = FVector::ZeroVector;
	};

	// 『The Next Week』第6章：四角形（Q を角とし、U と V を 2 辺とする平行四辺形）
	struct FToonQuad
	{
		FVector Q;					// 角の位置（ワールド座標、cm）
		FVector U;					// 角から伸びる 1 辺（cm）
		FVector V;					// 角から伸びるもう 1 辺（cm）
		EToonMaterialType Material;
		FLinearColor Albedo;		// 反射率。光る素材では放つ光の色と強さ
	};

	// 『The Next Week』第8〜9章：箱（本の box を rotate_y で回転し、translate で移動したもの）
	// 本の箱は角を原点に置いて回転するが、ここでは中心・半分の大きさ・上下軸まわりの回転で表す
	struct FToonBox
	{
		FVector Center;				// 中心（ワールド座標、cm）
		FVector HalfExtent;			// 半分の大きさ（回転前の X, Y, Z 方向、cm）
		double YawDegrees;			// 上下軸（Z）まわりの回転（度）
		EToonMaterialType Material;
		FLinearColor Albedo;		// 反射率。煙では煙の色
		float Density = 0.0f;		// 煙の濃さ（1cm あたりに散乱する確率、本の constant_medium の density）
	};

	// 『The Next Week』第10章：地面に敷き詰めた箱の格子（本の final_scene の boxes1）
	struct FToonGroundGrid
	{
		bool bEnabled = false;
		FVector Origin = FVector::ZeroVector;	// 格子の角（ワールド座標、cm）
		double CellSize = 100.0;		// 1 マスの大きさ（cm）
		FIntPoint Count = FIntPoint(20, 20);	// マスの数
		float MinHeight = 1.0f;			// 箱の高さの範囲（cm）
		float MaxHeight = 101.0f;
		FLinearColor Albedo = FLinearColor::White;
	};

	// 『The Next Week』第3章：BVH（Bounding Volume Hierarchy、入れ子の箱）
	// 本の final_scene の boxes2：一辺 165 の箱の中にランダムに置いた半径 10 の白い球 1000 個を、
	// rotate_y(15 度) と translate(-100, 270, 395) で動かしたもの
	struct FToonSphereClusterBvh
	{
		// ノード 1 つを float4 2 つで表す（シェーダー側の BvhNodes と一致させる）
		// [0] = (箱の最小値, 内部ノードなら右の子の番号 / 葉なら最初の球の番号), [1] = (箱の最大値, 葉の球の数（内部ノードは 0）)
		TArray<FVector4f> Nodes;
		TArray<FVector4f> Spheres;	// xyz: 塊の原点からの中心, w: 半径
		FVector Origin = FVector::ZeroVector;	// 塊の原点（ワールド座標）
		FLinearColor Albedo = FLinearColor(0.73f, 0.73f, 0.73f);
	};

	// 『The Next Week』第3章：final_scene の、BVH で探す小さな球の塊（初めて使うときに一度だけ組み立てる）
	const FToonSphereClusterBvh& GetFinalSceneSphereCluster();

	// 解析的な物体のシーン（r.ToonRayTracer.AnalyticScene で選ぶ）
	struct FToonAnalyticScene
	{
		TConstArrayView<FToonSphere> Spheres;
		TConstArrayView<FToonQuad> Quads;
		TConstArrayView<FToonBox> Boxes;
		bool bBlackBackground;		// 光る物体だけで照らすシーンでは背景を黒にする
		FToonGroundGrid GroundGrid = {};
		bool bSphereCluster = false;	// 『The Next Week』第3章の BVH で探す小さな球の塊を置くか
	};

	FToonAnalyticScene GetAnalyticScene(int32 SceneIndex);
}
