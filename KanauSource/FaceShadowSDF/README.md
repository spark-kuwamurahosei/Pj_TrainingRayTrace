# Kanau の顔の影のしきい値マップ（SDF テクスチャ）

`/Game/Kanau/FaceShadow/T_FaceShadowSDF_Kanau` の元データ。Kanau のモデルを参照するため、`feature/kanau-model` ブランチにだけ置く。

| ファイル | 内容 |
|---|---|
| `face_template.png` / `face_template_guide.png` | マスクを描くための下絵（顔を正面から見た形。ToonRayTracer の `FaceShadowSDFScale` 1.6、`FaceShadowSDFOffsetY` -0.4 の座標） |
| `masks/01.png` ～ `09.png` | 光の角度ごとのマスク（白が明るい面、光は画像の右から。01 が光が真後ろ、09 が正面） |
| `make_anime_face_masks.py` | マスクを計算で作るスクリプト（境目は真横の光で鼻筋、頬に沿ってふくらむ。斜め前から真横の光で鼻の影の三角を入れる） |
| `masks_preview.png` | 9 枚のマスクの境目を下絵に重ねた一覧 |
| `T_FaceShadowSDF_Kanau.png` | マスクから作ったしきい値マップ（16 ビットのグレースケール） |

## 作り直し方

1. マスクを直す（`make_anime_face_masks.py` の数値を変えて作り直すか、GIMP などで描き直す。白い範囲は 01 から 09 へ広がる一方にする）
2. [sdf_shadow_threshold_map](https://github.com/akasaki1211/sdf_shadow_threshold_map)（MIT）でしきい値マップを作る：
   `python run.py -i <masks のフォルダ> -o <出力先> -n T_FaceShadowSDF_Kanau -b 16`
3. エディタで取り込む：
   `py "<プロジェクト>/Plugins/ToonRayTracer/Scripts/FaceShadowSDF/import_face_shadow_sdf.py" "<出力先>/T_FaceShadowSDF_Kanau.png" "/Game/Kanau/FaceShadow"`
