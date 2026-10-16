#include "ToonRayTracerAnalyticScene.h"
#include "Algo/Sort.h"
#include "Math/RandomStream.h"

namespace ToonRayTracerAnalytic
{
	namespace
	{
		// 『Ray Tracing in One Weekend』第11章のシーンをUEの単位系（cm, Z-up）に置き換えたもの
		// 本の座標（x: 右, y: 上, -z: 奥）を UE（+Y: 右, +Z: 上, +X: 奥）に対応させ、100倍して cm にする
		// 本では球の中心が y=0、地面の上面が y=-0.5 のため、全体を 50cm 持ち上げて地面の上面を z=0 にしている
		const FToonSphere GToonSpheres[] =
		{
			// 地面：『The Next Week』第4章のチェッカー（本の checker_texture(0.32, color(.2, .3, .1), color(.9, .9, .9))）
			{ FVector(0.0, 0.0, -10000.0), 10000.0, EToonMaterialType::Lambertian, FLinearColor(0.2f, 0.3f, 0.1f), 0.0f, 1.0f,
				EToonTextureType::Checker, 32.0f, FLinearColor(0.9f, 0.9f, 0.9f) },
			// 中央：『The Next Week』第5章の大理石模様のランバート（本の noise_texture(4)）
			// 本の第5章の球（半径 2m）の 1/4 の大きさなので、縞の細かさを 4 倍にして同じくらいの本数の縞が見えるようにしている
			// 第4章の球のテクスチャ座標の確認には EToonTextureType::SphereUVChecker（大きさ 16）を使う
			{ FVector(120.0, 0.0, 50.0), 50.0, EToonMaterialType::Lambertian, FLinearColor::White, 0.0f, 1.0f,
				EToonTextureType::Marble, 16.0f },
			// 左：中空のガラス球（外側はガラス、内側の一回り小さい球は「ガラスの中の空気」）
			{ FVector(100.0, -100.0, 50.0), 50.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.5f },
			{ FVector(100.0, -100.0, 50.0), 40.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.0f / 1.5f },
			// 右：ぼけの強い金色の金属
			{ FVector(100.0, 100.0, 50.0), 50.0, EToonMaterialType::Metal, FLinearColor(0.8f, 0.6f, 0.2f), 1.0f, 1.0f },
		};

		// 『The Next Week』第7章の simple_light のシーン
		// 本の座標（x: 右, y: 上, z: 奥）を UE（+Y: 右, +Z: 上, +X: 奥）に対応させ、本の 1 単位を 1m（100cm）にしている
		// 本のカメラは (26, 3, 6) から (0, 2, 0) を見ている。UE では (600, 2600, 300) から (0, 0, 200) を見る位置
		const FToonSphere GSimpleLightSpheres[] =
		{
			// 地面と球：本の noise_texture(4)
			{ FVector(0.0, 0.0, -100000.0), 100000.0, EToonMaterialType::Lambertian, FLinearColor::White, 0.0f, 1.0f, EToonTextureType::Marble, 4.0f },
			{ FVector(0.0, 0.0, 200.0), 200.0, EToonMaterialType::Lambertian, FLinearColor::White, 0.0f, 1.0f, EToonTextureType::Marble, 4.0f },
			// 上に浮かぶ球のライト
			{ FVector(0.0, 0.0, 700.0), 200.0, EToonMaterialType::DiffuseLight, FLinearColor(4.0f, 4.0f, 4.0f), 0.0f, 1.0f },
		};
		const FToonQuad GSimpleLightQuads[] =
		{
			// 四角形のライト（本の quad(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight)）
			{ FVector(-200.0, 300.0, 100.0), FVector(0.0, 200.0, 0.0), FVector(0.0, 0.0, 200.0), EToonMaterialType::DiffuseLight, FLinearColor(4.0f, 4.0f, 4.0f) },
		};

		// 『The Next Week』第7章のコーネルボックス
		// 本の単位をそのまま cm にし、座標は上と同じく（本の x, y, z）→（UE の Y, Z, X）に置き換えている
		// 本のカメラは (278, 278, -800) から +z 方向を見ている。UE では (-800, 278, 278) から +X 方向を見る位置
		const FToonQuad GCornellBoxQuads[] =
		{
			// 緑の壁（右）、赤い壁（左）
			{ FVector(0.0, 555.0, 0.0), FVector(0.0, 0.0, 555.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.12f, 0.45f, 0.15f) },
			{ FVector(0.0, 0.0, 0.0), FVector(0.0, 0.0, 555.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.65f, 0.05f, 0.05f) },
			// 天井のライト
			{ FVector(332.0, 343.0, 554.0), FVector(0.0, -130.0, 0.0), FVector(-105.0, 0.0, 0.0), EToonMaterialType::DiffuseLight, FLinearColor(15.0f, 15.0f, 15.0f) },
			// 白い床、天井、奥の壁
			{ FVector(0.0, 0.0, 0.0), FVector(0.0, 555.0, 0.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
			{ FVector(555.0, 555.0, 555.0), FVector(0.0, -555.0, 0.0), FVector(-555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
			{ FVector(555.0, 0.0, 0.0), FVector(0.0, 555.0, 0.0), FVector(0.0, 0.0, 555.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
		};

		// 本の box(a, b) を rotate_y(Angle) で回転し、translate(Offset) で移動した箱を、中心・半分の大きさ・回転に置き換える
		// 座標はコーネルボックスと同じく（本の x, y, z）→（UE の Y, Z, X）に置き換える。本の rotate_y は y 軸まわりなので UE の Z 軸まわりの回転になる
		FToonBox MakeBookBox(const FVector& BookSize, double AngleDegrees, const FVector& BookOffset, EToonMaterialType Material, const FLinearColor& Albedo, float Density = 0.0f)
		{
			// 本の rotate_y は角（原点）まわりに回すので、箱の中心も角まわりに回してから移動する
			const double Angle = FMath::DegreesToRadians(AngleDegrees);
			const FVector BookCenter = BookSize * 0.5;
			const FVector RotatedCenter(
				FMath::Cos(Angle) * BookCenter.X + FMath::Sin(Angle) * BookCenter.Z,
				BookCenter.Y,
				-FMath::Sin(Angle) * BookCenter.X + FMath::Cos(Angle) * BookCenter.Z);
			const FVector BookWorldCenter = RotatedCenter + BookOffset;

			FToonBox Box;
			Box.Center = FVector(BookWorldCenter.Z, BookWorldCenter.X, BookWorldCenter.Y);
			Box.HalfExtent = FVector(BookSize.Z, BookSize.X, BookSize.Y) * 0.5;
			Box.YawDegrees = AngleDegrees;
			Box.Material = Material;
			Box.Albedo = Albedo;
			Box.Density = Density;
			return Box;
		}

		// 『The Next Week』第8章のコーネルボックス：白い箱を 2 つ置く（本の box(0,0,0)-(165,330,165) を 15 度、box(0,0,0)-(165,165,165) を -18 度）
		const FToonBox GCornellBoxBoxes[] =
		{
			MakeBookBox(FVector(165.0, 330.0, 165.0), 15.0, FVector(265.0, 0.0, 295.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f)),
			MakeBookBox(FVector(165.0, 165.0, 165.0), -18.0, FVector(130.0, 0.0, 65.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f)),
		};

		// 『The Rest of Your Life』第12章の最終シーン：コーネルボックスに、アルミの背の高い箱とガラスの球を置く
		// 本の box(0,0,0)-(165,330,165) を 15 度回して (265, 0, 295) へ移動（アルミ：金属、色 (0.8, 0.85, 0.88)、ぼけなし）
		const FToonBox GRestOfYourLifeBoxes[] =
		{
			MakeBookBox(FVector(165.0, 330.0, 165.0), 15.0, FVector(265.0, 0.0, 295.0), EToonMaterialType::Metal, FLinearColor(0.8f, 0.85f, 0.88f)),
		};

		// 本の sphere(point3(190, 90, 190), 90, glass)。座標は（本の x, y, z）→（UE の Y, Z, X）に置き換える
		const FToonSphere GRestOfYourLifeSpheres[] =
		{
			{ FVector(190.0, 190.0, 90.0), 90.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.5f },
		};

		// 『The Next Week』第9章の cornell_smoke：天井のライトを大きく暗くし、2 つの箱を黒い煙と白い煙にする（濃さ 0.01）
		const FToonQuad GCornellSmokeQuads[] =
		{
			{ FVector(0.0, 555.0, 0.0), FVector(0.0, 0.0, 555.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.12f, 0.45f, 0.15f) },
			{ FVector(0.0, 0.0, 0.0), FVector(0.0, 0.0, 555.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.65f, 0.05f, 0.05f) },
			// 本の quad(point3(113,554,127), vec3(330,0,0), vec3(0,0,305), light(7,7,7))
			{ FVector(127.0, 113.0, 554.0), FVector(0.0, 330.0, 0.0), FVector(305.0, 0.0, 0.0), EToonMaterialType::DiffuseLight, FLinearColor(7.0f, 7.0f, 7.0f) },
			{ FVector(0.0, 0.0, 0.0), FVector(0.0, 555.0, 0.0), FVector(555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
			{ FVector(555.0, 555.0, 555.0), FVector(0.0, -555.0, 0.0), FVector(-555.0, 0.0, 0.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
			{ FVector(555.0, 0.0, 0.0), FVector(0.0, 555.0, 0.0), FVector(0.0, 0.0, 555.0), EToonMaterialType::Lambertian, FLinearColor(0.73f, 0.73f, 0.73f) },
		};
		const FToonBox GCornellSmokeBoxes[] =
		{
			MakeBookBox(FVector(165.0, 330.0, 165.0), 15.0, FVector(265.0, 0.0, 295.0), EToonMaterialType::Volume, FLinearColor(0.0f, 0.0f, 0.0f), 0.01f),
			MakeBookBox(FVector(165.0, 165.0, 165.0), -18.0, FVector(130.0, 0.0, 65.0), EToonMaterialType::Volume, FLinearColor(1.0f, 1.0f, 1.0f), 0.01f),
		};

		// 『The Next Week』第10章の final_scene
		// 本の単位をそのまま cm にし、座標は（本の x, y, z）→（UE の Y, Z, X）に置き換えている
		// 本のカメラは (478, 278, -600) から (278, 278, 0) を見ている。UE では (-600, 478, 278) から Yaw -18.4 度の方向を見る位置
		// 1000 個の小さな球の塊は、第3章の BVH（下の BuildBvhNode で組み立てる入れ子の箱）で別に持つ
		const FToonSphere GFinalSceneSpheres[] =
		{
			// 移動ブラーで動く、オレンジのランバートの球（本の center2 = center1 + vec3(30, 0, 0)。本の x は UE の Y）
			{ FVector(200.0, 400.0, 400.0), 50.0, EToonMaterialType::Lambertian, FLinearColor(0.7f, 0.3f, 0.1f), 0.0f, 1.0f,
				EToonTextureType::Solid, 1.0f, FLinearColor::Black, 100.0f, 0.0f, FVector(0.0, 30.0, 0.0) },
			// ガラスの球
			{ FVector(45.0, 260.0, 150.0), 50.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.5f },
			// ぼけの強い金属の球
			{ FVector(145.0, 0.0, 150.0), 50.0, EToonMaterialType::Metal, FLinearColor(0.8f, 0.8f, 0.9f), 1.0f, 1.0f },
			// 中が青く濁ったガラスの球（ガラスの球と、同じ大きさの青い煙の球を重ねる）
			{ FVector(145.0, 360.0, 150.0), 70.0, EToonMaterialType::Dielectric, FLinearColor::White, 0.0f, 1.5f },
			{ FVector(145.0, 360.0, 150.0), 70.0, EToonMaterialType::Volume, FLinearColor(0.2f, 0.4f, 0.9f), 0.0f, 1.0f,
				EToonTextureType::Solid, 1.0f, FLinearColor::Black, 100.0f, 0.2f },
			// シーン全体を包む薄い霧
			{ FVector(0.0, 0.0, 0.0), 5000.0, EToonMaterialType::Volume, FLinearColor::White, 0.0f, 1.0f,
				EToonTextureType::Solid, 1.0f, FLinearColor::Black, 100.0f, 0.0001f },
			// 地球の球（第4章の画像テクスチャ。r.ToonRayTracer.ImageTexture のテクスチャが見つからなければ UV のチェッカーで代用）
			{ FVector(400.0, 400.0, 200.0), 100.0, EToonMaterialType::Lambertian, FLinearColor(0.1f, 0.3f, 0.7f), 0.0f, 1.0f,
				EToonTextureType::Image, 16.0f, FLinearColor(0.2f, 0.6f, 0.2f) },
			// 大理石の球（本の noise_texture(0.2)。本の 1 単位 = 1cm）
			{ FVector(300.0, 220.0, 280.0), 80.0, EToonMaterialType::Lambertian, FLinearColor::White, 0.0f, 1.0f,
				EToonTextureType::Marble, 0.2f, FLinearColor::Black, 1.0f },
		};
		const FToonQuad GFinalSceneQuads[] =
		{
			// 本の quad(point3(123,554,147), vec3(300,0,0), vec3(0,0,265), light(7,7,7))
			{ FVector(147.0, 123.0, 554.0), FVector(0.0, 300.0, 0.0), FVector(265.0, 0.0, 0.0), EToonMaterialType::DiffuseLight, FLinearColor(7.0f, 7.0f, 7.0f) },
		};
		FToonGroundGrid MakeFinalSceneGroundGrid()
		{
			// 本は x, z を -1000 から 100 ずつ 20 個、高さ 1 ～ 101 の緑の箱を並べる
			FToonGroundGrid Grid;
			Grid.bEnabled = true;
			Grid.Origin = FVector(-1000.0, -1000.0, 0.0);
			Grid.CellSize = 100.0;
			Grid.Count = FIntPoint(20, 20);
			Grid.MinHeight = 1.0f;
			Grid.MaxHeight = 101.0f;
			Grid.Albedo = FLinearColor(0.48f, 0.83f, 0.53f);
			return Grid;
		}

		// uint の値をそのまま float のビットとして入れる（シェーダー側で asuint で取り出す）
		float UintAsFloat(uint32 Value)
		{
			float Result;
			FMemory::Memcpy(&Result, &Value, sizeof(Result));
			return Result;
		}

		// 本の bvh_node のコンストラクタ：箱の一番長い軸で球を並べ替え、半分に分けることを繰り返す
		// 葉には 1 ～ 2 個の球を入れる（本と同じ）。ノードは深さ優先の順に並べ、左の子をすぐ次に置く
		int32 BuildBvhNode(FToonSphereClusterBvh& Bvh, int32 Start, int32 End)
		{
			FBox3f Bounds(ForceInit);
			for (int32 Index = Start; Index < End; ++Index)
			{
				const FVector4f& Sphere = Bvh.Spheres[Index];
				const FVector3f Center(Sphere.X, Sphere.Y, Sphere.Z);
				Bounds += FBox3f(Center - FVector3f(Sphere.W), Center + FVector3f(Sphere.W));
			}

			const int32 NodeIndex = Bvh.Nodes.Num() / 2;
			Bvh.Nodes.AddDefaulted(2);

			const int32 Count = End - Start;
			if (Count <= 2)
			{
				Bvh.Nodes[NodeIndex * 2] = FVector4f(Bounds.Min, UintAsFloat(static_cast<uint32>(Start)));
				Bvh.Nodes[NodeIndex * 2 + 1] = FVector4f(Bounds.Max, UintAsFloat(static_cast<uint32>(Count)));
				return NodeIndex;
			}

			const FVector3f Extent = Bounds.GetSize();
			const int32 Axis = Extent.X >= Extent.Y && Extent.X >= Extent.Z ? 0 : (Extent.Y >= Extent.Z ? 1 : 2);
			TArrayView<FVector4f> Range(Bvh.Spheres.GetData() + Start, Count);
			Algo::Sort(Range, [Axis](const FVector4f& A, const FVector4f& B) { return A[Axis] < B[Axis]; });

			const int32 Mid = Start + Count / 2;
			BuildBvhNode(Bvh, Start, Mid);	// 左の子（NodeIndex + 1 になる）
			const int32 RightIndex = BuildBvhNode(Bvh, Mid, End);

			Bvh.Nodes[NodeIndex * 2] = FVector4f(Bounds.Min, UintAsFloat(static_cast<uint32>(RightIndex)));
			Bvh.Nodes[NodeIndex * 2 + 1] = FVector4f(Bounds.Max, UintAsFloat(0u));
			return NodeIndex;
		}
	}

	const FToonSphereClusterBvh& GetFinalSceneSphereCluster()
	{
		static const FToonSphereClusterBvh Bvh = []()
		{
			FToonSphereClusterBvh Result;
			// 本の translate(-100, 270, 395) を（本の x, y, z）→（UE の Y, Z, X）に置き換えた位置を塊の原点にする
			Result.Origin = FVector(395.0, -100.0, 270.0);

			// 毎回同じ配置になるよう、乱数の種を固定する
			FRandomStream Random(2026);
			const double Angle = FMath::DegreesToRadians(15.0);
			for (int32 Index = 0; Index < 1000; ++Index)
			{
				// 本の point3::random(0, 165) を rotate_y(15) で回す（回転の中心は箱の角）
				const FVector Book(Random.FRandRange(0.0f, 165.0f), Random.FRandRange(0.0f, 165.0f), Random.FRandRange(0.0f, 165.0f));
				const FVector Rotated(
					FMath::Cos(Angle) * Book.X + FMath::Sin(Angle) * Book.Z,
					Book.Y,
					-FMath::Sin(Angle) * Book.X + FMath::Cos(Angle) * Book.Z);
				Result.Spheres.Add(FVector4f(FVector3f(Rotated.Z, Rotated.X, Rotated.Y), 10.0f));
			}

			BuildBvhNode(Result, 0, Result.Spheres.Num());
			return Result;
		}();
		return Bvh;
	}

	FToonAnalyticScene GetAnalyticScene(int32 SceneIndex)
	{
		switch (SceneIndex)
		{
		case 1:
			return { MakeArrayView(GSimpleLightSpheres), MakeArrayView(GSimpleLightQuads), TConstArrayView<FToonBox>(), true };
		case 2:
			return { TConstArrayView<FToonSphere>(), MakeArrayView(GCornellBoxQuads), TConstArrayView<FToonBox>(), true };
		case 3:
			return { TConstArrayView<FToonSphere>(), MakeArrayView(GCornellBoxQuads), MakeArrayView(GCornellBoxBoxes), true };
		case 4:
			return { TConstArrayView<FToonSphere>(), MakeArrayView(GCornellSmokeQuads), MakeArrayView(GCornellSmokeBoxes), true };
		case 5:
			return { MakeArrayView(GFinalSceneSpheres), MakeArrayView(GFinalSceneQuads), TConstArrayView<FToonBox>(), true, MakeFinalSceneGroundGrid(), true };
		case 6:
			return { MakeArrayView(GRestOfYourLifeSpheres), MakeArrayView(GCornellBoxQuads), MakeArrayView(GRestOfYourLifeBoxes), true };
		default:
			return { MakeArrayView(GToonSpheres), TConstArrayView<FToonQuad>(), TConstArrayView<FToonBox>(), false };
		}
	}
}
