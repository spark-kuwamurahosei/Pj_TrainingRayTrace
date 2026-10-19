# 顔の影のしきい値マップ（sdf_shadow_threshold_map で作った 16 ビットのグレースケール PNG）を、テクスチャとして取り込む
# しきい値マップは色ではなく値なので、sRGB を切り、16 ビットのグレースケールのまま、ミップなしで使う
#
# 実行例（エディタのコンソール）:
#   py "D:/Pj_TrainingRayTrace/Plugins/ToonRayTracer/Scripts/FaceShadowSDF/import_face_shadow_sdf.py" "D:/path/T_FaceShadowSDF_Chara.png" "/Game/Chara/FaceShadow"
import os
import sys
import unreal

source = sys.argv[1]
destination = sys.argv[2] if len(sys.argv) > 2 else "/Game/FaceShadow"
name = os.path.splitext(os.path.basename(source))[0]

task = unreal.AssetImportTask()
task.set_editor_property("filename", source)
task.set_editor_property("destination_path", destination)
task.set_editor_property("destination_name", name)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

path = destination + "/" + name
texture = unreal.load_asset(path)
texture.set_editor_property("srgb", False)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
ok = unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log("TOON_FACE_SDF imported: %s %s" % (path, ok))
