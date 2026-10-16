# 『The Next Week』第4章の地球の画像（SourceArt/earthmap.jpg）を、テクスチャ T_EarthMap としてプラグインの Content に取り込む
# r.ToonRayTracer.ImageTexture の既定のパス（/ToonRayTracer/Textures/T_EarthMap）で、解析的なシーン 5 の地球の球に貼られる
# 画像は本のリポジトリ（CC0）の earthmap.jpg
#
# 実行例（エディタのコンソール）:
#   py "D:/Pj_TrainingRayTrace/Plugins/ToonRayTracer/Scripts/import_earth_map.py"
import os
import unreal

PATH = "/ToonRayTracer/Textures"
NAME = "T_EarthMap"
SOURCE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "earthmap.jpg"))

if not os.path.isfile(SOURCE):
    raise RuntimeError("source image not found: " + SOURCE)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", PATH)
task.set_editor_property("destination_name", NAME)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

full_path = PATH + "/" + NAME
ok = unreal.EditorAssetLibrary.does_asset_exist(full_path) and unreal.EditorAssetLibrary.save_asset(full_path, only_if_is_dirty=False)
unreal.log("TOON_EARTH_MAP imported: %s" % ok)
if not ok:
    raise RuntimeError("failed to import or save " + full_path)
