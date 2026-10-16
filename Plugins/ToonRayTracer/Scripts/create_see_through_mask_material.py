# 眉毛の透けで、前髪の上に見せる眉毛の線の形を Custom Depth に描くマテリアル（M_ToonSeeThroughMask）を作る
# Toon Character コンポーネントが作るキャラクターのメッシュの複製で、髪越しに見せるスロットに割り当てる
# テクスチャの明るさ（線形の輝度）が Threshold より暗い部分（眉毛の線）だけを残す Masked のマテリアル
#
# 実行例（エディタのコンソール）:
#   py "D:/Pj_TrainingRayTrace/Plugins/ToonRayTracer/Scripts/create_see_through_mask_material.py"
import unreal

PATH = "/ToonRayTracer/Materials"
NAME = "M_ToonSeeThroughMask"

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
# 不透明度のマスク（Threshold - 輝度）が 0 未満の部分（Threshold より明るい部分）を切り抜く
mat.set_editor_property("opacity_mask_clip_value", 0.0)
mat.set_editor_property("used_with_skeletal_mesh", True)
mat.set_editor_property("used_with_morph_targets", True)

texture = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
texture.set_editor_property("parameter_name", "Texture")
texture.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))

weights = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -800, 250)
weights.set_editor_property("constant", unreal.LinearColor(0.2126, 0.7152, 0.0722, 1.0))

luminance = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -500, 100)
mel.connect_material_expressions(texture, "RGB", luminance, "A")
mel.connect_material_expressions(weights, "", luminance, "B")

threshold = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, -100)
threshold.set_editor_property("parameter_name", "Threshold")
threshold.set_editor_property("default_value", 0.3)

mask = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -250, 0)
mel.connect_material_expressions(threshold, "", mask, "A")
mel.connect_material_expressions(luminance, "", mask, "B")
mel.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)

mel.recompile_material(mat)
ok = eal.save_asset(full_path, only_if_is_dirty=False)
unreal.log("TOON_SEE_THROUGH_MASK created: %s" % ok)
if not ok:
    raise RuntimeError("failed to save " + full_path)
