# プラグインの Content のアセットを、今のエンジンの版でまとめて作り直す（RebuildContent.bat から呼ぶ）
#   T_EarthMap            import_earth_map.py
#   M_ToonSeeThroughMask  create_see_through_mask_material.py
#   M_ToonEyeToneMask     create_eye_tone_mask_material.py
#   L_ToonRT_*            create_sample_levels.py
# UE5.8 で保存したアセットは、それより前の版では読み込めないため、古い版のプロジェクトに入れたときに使う
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPTS = ["import_earth_map.py", "create_see_through_mask_material.py", "create_eye_tone_mask_material.py", "create_sample_levels.py"]

failed = []
for name in SCRIPTS:
    unreal.log("TOON_REBUILD start: " + name)
    try:
        path = os.path.join(HERE, name)
        with open(path, encoding="utf-8") as f:
            exec(compile(f.read(), path, "exec"), {"__name__": "__main__", "__file__": path})
        unreal.log("TOON_REBUILD done: " + name)
    except Exception as e:
        unreal.log_error("TOON_REBUILD failed: %s: %s" % (name, e))
        failed.append(name)

unreal.log("TOON_REBUILD finished: " + ("OK" if not failed else "FAILED " + ", ".join(failed)))

# RebuildContent.bat から起動したとき（-ToonRebuildQuit）は、作り終えたらエディタを閉じる
if "-toonrebuildquit" in unreal.SystemLibrary.get_command_line().lower():
    unreal.SystemLibrary.quit_editor()
