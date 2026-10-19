# Kanau の顔の影のマスク（アニメ調の顔影）を計算で作る（光の角度ごとに 9 枚）
# 顔を正面から見た形で、光は画像の右から来る。白が明るい面。01 が光が真後ろ（180 度）、09 が正面（0 度）
# 座標は顔の半分の幅を 1 とした、画像の右を正にした左右の位置 s と、目の高さを 0 にした上下の位置 h（上が正）
# 下絵から読み取った位置：目の中心 s = ±0.6、h = 0／鼻の先 h ≈ -0.36／口 h ≈ -0.77／顎の先 h ≈ -1.26
import math
import os
import sys

import cv2
import numpy as np

SIZE = 1024
out_dir = sys.argv[1]
scale = float(sys.argv[2]) if len(sys.argv) > 2 else 1.6
offset_y = float(sys.argv[3]) if len(sys.argv) > 3 else -0.4
count = 9
os.makedirs(out_dir, exist_ok=True)

v, u = np.mgrid[0:SIZE, 0:SIZE].astype(np.float64)
u = (u + 0.5) / SIZE
v = (v + 0.5) / SIZE
s = (u - 0.5) * 2.0 * scale
h = -(v - 0.5) * 2.0 * scale + offset_y

NOSE_TIP = (0.0, -0.36)
previous = None
masks = []
for i in range(count):
    theta = math.pi * (1.0 - i / (count - 1))
    side = math.sin(theta)
    # 境目の基本の位置：正面では顔の外、真横で鼻筋（s = 0）、真後ろでは光の側の顔の端
    base = -math.cos(theta) * 1.05
    # 頬：目の下（h ≈ -0.45）で影の側へふくらむ（明るい面が頬に沿って回り込む）
    cheek = 0.28 * side * np.exp(-((h + 0.45) / 0.45) ** 2)
    # 額：目より上では光の側へ少し戻る（額の丸み）
    forehead = 0.12 * side * np.clip(h - 0.25, 0.0, 1.5)
    # 顎：口より下では顔の中央へ寄っていく（顎の先に向かって細くなる）
    chin = np.clip(-(h + 0.85), 0.0, 1.0) * 0.5
    boundary = base - cheek + forehead
    boundary = np.where(h < -0.85, boundary * (1.0 - chin), boundary)
    lit = s > boundary

    # 鼻の影：斜め前から真横の光（およそ 20 ～ 90 度）で、鼻の先の左下（影の側）に小さな三角の影を落とす
    deg = math.degrees(theta)
    # 真横より後ろの光では、鼻の影のあたりは影の面に含まれるので、真横（90 度）までは入れ、白い範囲が広がる一方になるようにする
    if 20.0 < deg <= 95.0:
        strength = min(1.0, (deg - 20.0) / 45.0)
        width = 0.22 * strength
        height = 0.16 * strength + 0.04
        ns, nh = NOSE_TIP
        # 鼻の先から左下へ伸びる三角（上辺は鼻の先の高さ、左へ width、下へ height）
        in_tri = (s <= ns) & (s >= ns - width) & (h <= nh) & (h >= nh - height * (1.0 - (ns - s) / max(width, 1e-6)))
        lit &= ~in_tri

    if i == count - 1:
        lit[:] = True
    if previous is not None:
        lit |= previous  # 白い範囲は 01 から 09 へ広がる一方にする
    previous = lit
    masks.append(lit)
    cv2.imwrite(os.path.join(out_dir, "%02d.png" % (i + 1)), lit.astype(np.uint8) * 255)

# 確認用：下絵に 9 枚の境目を重ねた一覧
template = cv2.imread(os.path.join(os.path.dirname(out_dir.rstrip("/\\")), "face_template.png"))
if template is not None:
    tiles = []
    for i, lit in enumerate(masks):
        t = template.copy()
        shade = t.copy()
        shade[~lit] = (shade[~lit] * 0.45).astype(np.uint8)
        edge = cv2.morphologyEx(lit.astype(np.uint8), cv2.MORPH_GRADIENT, np.ones((3, 3), np.uint8)) > 0
        shade[edge] = (0, 0, 255)
        deg = 180.0 * (1.0 - i / (count - 1))
        cv2.putText(shade, "%02d  %.1f deg" % (i + 1, deg), (20, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.6, (255, 255, 255), 4)
        tiles.append(cv2.resize(shade, (341, 341)))
    rows = [np.hstack(tiles[r * 3:r * 3 + 3]) for r in range(3)]
    cv2.imwrite(os.path.join(os.path.dirname(out_dir.rstrip("/\\")), "masks_preview.png"), np.vstack(rows))
print("ok")
