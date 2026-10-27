# 前髪越しの目で、目のテクスチャを色の段階（瞳・白目）ごとに切り抜いて Custom Depth に描くマテリアル（M_ToonEyeToneMask）を作る
# Toon Character コンポーネントが作るキャラクターのメッシュの複製で、目のスロットに割り当てる
# テクスチャの明るさ（線形の輝度）が LumMin ～ LumMax、鮮やかさ（(最大 - 最小) / 最大）が SatMin ～ SatMax の部分だけを残す Masked のマテリアル
# DepthBias（cm）だけ、カメラから遠ざける向きに頂点をずらす（同じ場所に重なる複製どうしで、線 → 瞳 → 白目の順に手前にする）
#
# 実行例（エディタのコンソール）:
#   py "D:/Pj_TrainingRayTrace/Plugins/ToonRayTracer/Scripts/create_eye_tone_mask_material.py"
import unreal

PATH = "/ToonRayTracer/Materials"
NAME = "M_ToonEyeToneMask"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

full_path = PATH + "/" + NAME
if eal.does_asset_exist(full_path):
    eal.delete_asset(full_path)

mat = asset_tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
# 不透明度のマスクが 0 未満の部分（範囲の外）を切り抜く
mat.set_editor_property("opacity_mask_clip_value", 0.0)
mat.set_editor_property("used_with_skeletal_mesh", True)
mat.set_editor_property("used_with_morph_targets", True)


def scalar(name, value, x, y):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def binary(cls, a, a_out, b, b_out, x, y):
    node = mel.create_material_expression(mat, cls, x, y)
    mel.connect_material_expressions(a, a_out, node, "A")
    mel.connect_material_expressions(b, b_out, node, "B")
    return node


texture = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1400, 0)
texture.set_editor_property("parameter_name", "Texture")
texture.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))

# 明るさ（線形の輝度）
weights = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1400, 300)
weights.set_editor_property("constant", unreal.LinearColor(0.2126, 0.7152, 0.0722, 1.0))
luminance = binary(unreal.MaterialExpressionDotProduct, texture, "RGB", weights, "", -1100, 200)

# 鮮やかさ：(最大 - 最小) / 最大
max_rg = binary(unreal.MaterialExpressionMax, texture, "R", texture, "G", -1100, -200)
max_rgb = binary(unreal.MaterialExpressionMax, max_rg, "", texture, "B", -900, -200)
min_rg = binary(unreal.MaterialExpressionMin, texture, "R", texture, "G", -1100, -350)
min_rgb = binary(unreal.MaterialExpressionMin, min_rg, "", texture, "B", -900, -350)
spread = binary(unreal.MaterialExpressionSubtract, max_rgb, "", min_rgb, "", -700, -300)
floor = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -900, -50)
floor.set_editor_property("r", 0.001)
safe_max = binary(unreal.MaterialExpressionMax, max_rgb, "", floor, "", -700, -150)
saturation = binary(unreal.MaterialExpressionDivide, spread, "", safe_max, "", -500, -250)

lum_min = scalar("LumMin", 0.0, -900, 400)
lum_max = scalar("LumMax", 1.0, -900, 500)
sat_min = scalar("SatMin", 0.0, -500, -450)
sat_max = scalar("SatMax", 1.0, -500, -550)

above_lum = binary(unreal.MaterialExpressionSubtract, luminance, "", lum_min, "", -600, 300)
below_lum = binary(unreal.MaterialExpressionSubtract, lum_max, "", luminance, "", -600, 450)
above_sat = binary(unreal.MaterialExpressionSubtract, saturation, "", sat_min, "", -300, -350)
below_sat = binary(unreal.MaterialExpressionSubtract, sat_max, "", saturation, "", -300, -500)
lum_inside = binary(unreal.MaterialExpressionMin, above_lum, "", below_lum, "", -300, 350)
sat_inside = binary(unreal.MaterialExpressionMin, above_sat, "", below_sat, "", -100, -400)
mask = binary(unreal.MaterialExpressionMin, lum_inside, "", sat_inside, "", 0, 0)
mel.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)

# カメラから遠ざける向き（カメラへ向かう向きの逆）に DepthBias だけずらす
# 頂点シェーダーでは CameraVector が使えないため、カメラの位置 - 頂点の位置 を正規化して、カメラへ向かう向きにする
camera_position = mel.create_material_expression(mat, unreal.MaterialExpressionCameraPositionWS, -900, 650)
world_position = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 750)
to_camera = binary(unreal.MaterialExpressionSubtract, camera_position, "", world_position, "", -700, 700)
camera_vector = mel.create_material_expression(mat, unreal.MaterialExpressionNormalize, -500, 700)
mel.connect_material_expressions(to_camera, "", camera_vector, "VectorInput")
bias = scalar("DepthBias", 0.0, -500, 850)
offset = binary(unreal.MaterialExpressionMultiply, camera_vector, "", bias, "", -250, 750)
negative = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 900)
negative.set_editor_property("r", -1.0)
away = binary(unreal.MaterialExpressionMultiply, offset, "", negative, "", 0, 800)
mel.connect_material_property(away, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

mel.recompile_material(mat)
ok = eal.save_asset(full_path, only_if_is_dirty=False)
unreal.log("TOON_EYE_TONE_MASK created: %s" % ok)
if not ok:
    raise RuntimeError("failed to save " + full_path)
