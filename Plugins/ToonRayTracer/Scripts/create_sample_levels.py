# ToonRayTracer の検証用・サンプルのレベルを、プラグインの Content（/ToonRayTracer/Maps）に作る
# モデルは含めず、UE エンジンの基本形状（/Engine/BasicShapes）だけで組み立てる
#
#   L_ToonRT_Materials  素材の検証。メッシュごとの設定（Custom Primitive Data）の見本を並べる
#   L_ToonRT_Character  キャラクターの検証。キャラクターを置く場所と、正面・斜め・横・見下ろしのカメラを置いた舞台
#   L_ToonRT_Analytic   解析的な物体のシーン（r.ToonRayTracer.TraceMode 0）の検証。シーンごとに本のカメラに相当するカメラを置く
#
# 実行例（エディタのコンソール。今開いているレベルは先に保存しておく。既にあるレベルは中身を消して作り直す）:
#   py "D:/Pj_TrainingRayTrace/Plugins/ToonRayTracer/Scripts/create_sample_levels.py"
# 作り終えたら、実行前に開いていたレベルに戻る
import unreal

MAP_PATH = "/ToonRayTracer/Maps"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane")
# 基本形状の白いマテリアル（割り当てないと、既定のグリッドのマテリアルになることがある）
WHITE = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")


def new_level(name):
    path = MAP_PATH + "/" + name
    if eal.does_asset_exist(path):
        # 既にあるレベルは削除せずに開き、中のアクターを全部消してから組み直す
        # （削除したレベルがメモリに残っていると、同じ名前で新しく作れないため）
        if not les.load_level(path):
            raise RuntimeError("load_level failed: " + path)
        actors = [actor for actor in eas.get_all_level_actors() if not isinstance(actor, (unreal.WorldSettings, unreal.Brush))]
        eas.destroy_actors(actors)
    elif not les.new_level(path):
        raise RuntimeError("new_level failed: " + path)
    return path


def look_at(location, target):
    return unreal.MathLibrary.find_look_at_rotation(location, target)


def mesh(label, static_mesh, location, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, 0.0), data=None, folder="Meshes"):
    actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component = actor.static_mesh_component
    component.set_static_mesh(static_mesh)
    component.set_material(0, WHITE)
    # Custom Primitive Data（README の「メッシュごとの設定」）。初期値に書き込むと保存される
    for index, value in enumerate(data or []):
        component.set_default_custom_primitive_data_float(index, float(value))
    return actor


def camera(label, location, rotation, fov=60.0):
    actor = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*location), rotation)
    actor.set_actor_label(label)
    actor.set_folder_path("Cameras")
    # set_editor_property だけでは保存したレベルに画角が残らず 90 度に戻るため、関数で設定する
    component = actor.get_component_by_class(unreal.CameraComponent)
    component.modify()
    component.set_field_of_view(fov)
    return actor


def environment(sun_rotation=(0.0, -40.0, 40.0)):
    # 太陽（トゥーンの光源）と空。カメラ（-X 側）から見て左手前から照らし、影を右奥の壁に落とす
    # unreal.Rotator は (Roll, Pitch, Yaw) の順。Lumen で描く背景と、Skylight の色（影を空の色に染める）にも使う
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 500.0), unreal.Rotator(*sun_rotation))
    sun.set_actor_label("DirectionalLight")
    sun.set_folder_path("Lighting")
    sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun_component.set_editor_property("atmosphere_sun_light", True)

    sky = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))
    sky.set_actor_label("SkyAtmosphere")
    sky.set_folder_path("Lighting")

    sky_light = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0.0, 0.0, 600.0))
    sky_light.set_actor_label("SkyLight")
    sky_light.set_folder_path("Lighting")
    sky_light_component = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_light_component.set_editor_property("real_time_capture", True)

    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, -100.0))
    fog.set_actor_label("ExponentialHeightFog")
    fog.set_folder_path("Lighting")


def floor(size=4000.0):
    # Plane は 100cm 四方
    mesh("Floor", PLANE, (0.0, 0.0, 0.0), (size / 100.0, size / 100.0, 1.0), folder="Stage")


# ---- 素材の検証 ----
def create_materials_level():
    path = new_level("L_ToonRT_Materials")
    environment()
    floor()
    # 影を受ける壁
    mesh("Wall_Back", CUBE, (600.0, 0.0, 200.0), (0.2, 16.0, 4.0), folder="Stage")
    mesh("Wall_Left", CUBE, (0.0, -800.0, 200.0), (12.0, 0.2, 4.0), folder="Stage")

    # Custom Primitive Data：[0] 種類（1 ランバート / 2 金属 / 3 ガラス）、[1-3] 色、[4] ぼけ、[5] 屈折率、
    # [6-8] 影色、[9] ハイライト、[10] リムライト、[11] 線の太さ（負なら線なし）
    y = -600.0
    samples = [
        ("01_GBufferColor_NoData", CUBE, (0.8, 0.8, 0.8), []),
        ("02_Lambert_White", CUBE, (0.8, 0.8, 0.8), [1, 0.8, 0.8, 0.8]),
        ("03_Lambert_Red_Highlight_Rim", SPHERE, (1.0, 1.0, 1.0), [1, 0.8, 0.15, 0.15, 0, 0, 0, 0, 0, 1, 1]),
        ("04_Metal_Silver", SPHERE, (1.0, 1.0, 1.0), [2, 0.8, 0.8, 0.8, 0, 0, 0, 0, 0, 1, 0]),
        ("05_Metal_Gold", SPHERE, (1.0, 1.0, 1.0), [2, 0.9, 0.7, 0.3, 0, 0, 0, 0, 0, 1, 0]),
        ("06_Glass", SPHERE, (1.0, 1.0, 1.0), [3, 1, 1, 1, 0, 1.5]),
        ("07_Lambert_CustomShadowColor", CYLINDER, (1.0, 1.0, 1.0), [1, 0.9, 0.85, 0.6, 0, 0, 0.3, 0.35, 0.7]),
        ("08_Lambert_NoOutline", CUBE, (0.8, 0.8, 0.8), [1, 0.3, 0.6, 0.9, 0, 0, 0, 0, 0, 0, 0, -1]),
    ]
    for label, static_mesh, scale, data in samples:
        mesh(label, static_mesh, (200.0, y, 50.0 * scale[2]), scale, data=data, folder="Samples")
        y += 170.0

    camera("Cam_Front", (-700.0, 0.0, 250.0), look_at(unreal.Vector(-700.0, 0.0, 250.0), unreal.Vector(200.0, 0.0, 50.0)))
    camera("Cam_Side", (-200.0, -1200.0, 350.0), look_at(unreal.Vector(-200.0, -1200.0, 350.0), unreal.Vector(200.0, 0.0, 50.0)))
    les.save_current_level()
    return path


# ---- キャラクターの検証 ----
def create_character_level():
    path = new_level("L_ToonRT_Character")
    environment()
    floor()
    mesh("Wall_Back", CUBE, (500.0, 0.0, 250.0), (0.2, 12.0, 5.0), folder="Stage")
    mesh("Pillar", CUBE, (300.0, -350.0, 250.0), (1.0, 1.0, 5.0), folder="Stage")
    # 映り込みの確認用に、緑の金属の球を置く
    mesh("Prop_MetalSphere", SPHERE, (150.0, 350.0, 60.0), (1.2, 1.2, 1.2), data=[2, 0.2, 0.8, 0.3, 0, 0, 0, 0, 0, 1, 0], folder="Stage")

    # キャラクターを置く場所（原点）。キャラクターのアクターに Toon Character コンポーネントを付け、プロファイルを選ぶ
    spot = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(0.0, 0.0, 0.0))
    spot.set_actor_label("CharacterSpot_PlaceCharacterHere")
    spot.set_folder_path("Stage")

    # 身長 160cm 前後のキャラクターの顔（高さ 150cm 付近）を狙う。大きさが違うモデルは、カメラの位置を合わせて使う
    face = unreal.Vector(0.0, 0.0, 150.0)
    for label, location in [
        ("Cam_Front", (-250.0, 0.0, 150.0)),
        ("Cam_ThreeQuarter", (-180.0, -180.0, 155.0)),
        ("Cam_Side", (0.0, -260.0, 150.0)),
        ("Cam_TopDown", (-120.0, 0.0, 280.0)),
        ("Cam_FullBody", (-450.0, 0.0, 110.0)),
    ]:
        target = unreal.Vector(0.0, 0.0, 95.0) if label == "Cam_FullBody" else face
        camera(label, location, look_at(unreal.Vector(*location), target), 45.0)
    les.save_current_level()
    return path


# ---- 解析的な物体のシーン ----
def create_analytic_level():
    path = new_level("L_ToonRT_Analytic")
    environment()
    # TraceMode 0 でもシーンの TLAS が必要なため、見えない位置に小さなメッシュを置いておく
    mesh("TLAS_Placeholder", CUBE, (0.0, 0.0, -100000.0), (0.1, 0.1, 0.1), folder="Stage")

    # README の「解析的な物体のシーン」の、本のカメラに相当する位置。画角は本の縦の画角を 16:9 の横の画角に直した値
    camera("Cam_Scene0_OneWeekend", (-50.0, 0.0, 50.0), unreal.Rotator(0.0, 0.0, 0.0), 34.0)
    camera("Cam_Scene1_SimpleLight", (600.0, 2600.0, 300.0), look_at(unreal.Vector(600.0, 2600.0, 300.0), unreal.Vector(0.0, 0.0, 200.0)), 34.0)
    camera("Cam_Scene2to4_6_CornellBox", (-800.0, 278.0, 278.0), unreal.Rotator(0.0, 0.0, 0.0), 66.0)
    camera("Cam_Scene5_FinalScene", (-600.0, 478.0, 278.0), unreal.Rotator(0.0, 0.0, -18.4), 66.0)
    les.save_current_level()
    return path


original = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split(".")[0]
created = [create_materials_level(), create_character_level(), create_analytic_level()]
if original and not original.startswith("/Temp/"):
    les.load_level(original)
unreal.log("TOON_SAMPLE_LEVELS created: " + ", ".join(created))
