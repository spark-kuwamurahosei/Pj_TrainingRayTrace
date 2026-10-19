# 顔の影のしきい値マップ（SDF テクスチャ）のマスクを描くための下絵を作る
# r.ToonRayTracer.Debug.Geometry 7 / 8 で撮った「しきい値マップの座標」の画面と、普通の画面から、
# 画面の色をしきい値マップの座標に移して、顔を正面から見た下絵にする
#
# 使い方: venv の python で
#   python make_face_sdf_template.py <普通の画面.png> <Geometry 7 の画面.png> <Geometry 8 の画面.png> <出力フォルダ> [scale] [offsetY]
import os
import sys

import cv2
import numpy as np

SIZE = 1024


def srgb_to_linear(c):
    c = c.astype(np.float64) / 255.0
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def main():
    color_path, coarse_path, fine_path, out_dir = sys.argv[1:5]
    scale = float(sys.argv[5]) if len(sys.argv) > 5 else 1.6
    offset_y = float(sys.argv[6]) if len(sys.argv) > 6 else -0.4
    os.makedirs(out_dir, exist_ok=True)

    color = cv2.imread(color_path, cv2.IMREAD_COLOR)
    coarse = cv2.imread(coarse_path, cv2.IMREAD_COLOR)[:, :, ::-1]  # RGB
    fine = cv2.imread(fine_path, cv2.IMREAD_COLOR)[:, :, ::-1]

    inside = (coarse[:, :, 2] > 200) & (fine[:, :, 2] > 200)
    # 画面に出すまでの色の変換（トーンマップやガンマ）は分からないので、細かい値の画像から戻し方を求める
    # 細かい値（UV × 16 の小数部分）は 0 ～ 1 にほぼ均一に分布するので、符号化された値の累積分布がそのまま元の値になる
    vals = np.concatenate([fine[:, :, 0][inside], fine[:, :, 1][inside]])
    cdf = np.cumsum(np.bincount(vals, minlength=256).astype(np.float64))
    cdf /= cdf[-1]
    lut = (np.concatenate([[0.0], cdf[:-1]]) + cdf) / 2.0
    coarse_uv = lut[coarse[:, :, :2]]
    fine_uv = lut[fine[:, :, :2]]
    # 大まかな値（UV）と細かい値（UV × 16 の小数部分）を合わせる
    k = np.round(coarse_uv * 16.0 - fine_uv)
    uv = (k + fine_uv) / 16.0

    ys, xs = np.nonzero(inside)
    tx = np.clip((uv[ys, xs, 0] * SIZE).astype(int), 0, SIZE - 1)
    ty = np.clip((uv[ys, xs, 1] * SIZE).astype(int), 0, SIZE - 1)

    total = np.zeros((SIZE, SIZE, 3), np.float64)
    count = np.zeros((SIZE, SIZE), np.float64)
    np.add.at(total, (ty, tx), color[ys, xs].astype(np.float64))
    np.add.at(count, (ty, tx), 1.0)
    covered = count > 0
    template = np.zeros((SIZE, SIZE, 3), np.uint8)
    template[covered] = (total[covered] / count[covered][:, None]).astype(np.uint8)

    # 画面の画素の間で空いたマップの画素を埋める（映っている範囲の中の小さな穴だけ）
    region = cv2.morphologyEx(covered.astype(np.uint8) * 255, cv2.MORPH_CLOSE, np.ones((9, 9), np.uint8))
    holes = ((region > 0) & ~covered).astype(np.uint8) * 255
    template = cv2.inpaint(template, holes, 3, cv2.INPAINT_TELEA)
    template[region == 0] = (128, 128, 128)
    cv2.imwrite(os.path.join(out_dir, "face_region.png"), region)

    # 目安の線：顔の中央の縦線、目の高さの横線（マップの座標の決まりと同じ計算）
    guide = template.copy()
    center_x = SIZE // 2
    eye_y = int(round((0.5 - 0.5 * (0.0 - offset_y) / scale) * SIZE))
    cv2.line(guide, (center_x, 0), (center_x, SIZE - 1), (0, 200, 255), 1)
    cv2.line(guide, (0, eye_y), (SIZE - 1, eye_y), (0, 200, 255), 1)
    cv2.putText(guide, "light from this side ->", (SIZE - 330, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
    cv2.imwrite(os.path.join(out_dir, "face_template.png"), template)
    cv2.imwrite(os.path.join(out_dir, "face_template_guide.png"), guide)
    print("covered texels:", int(covered.sum()), "eye line y:", eye_y)


if __name__ == "__main__":
    main()
