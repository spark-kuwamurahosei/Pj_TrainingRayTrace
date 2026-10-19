# Kanau の髪のハイライト（テクスチャ）

元の髪のテクスチャ（tx_hai_0000_01_cl_h1）に描き込まれていたハイライトを、ベースカラーとハイライトの形に分けたもの。

| ファイル | 内容 |
|---|---|
| tx_hai_base.png | ハイライトを抜いたベースカラー（ハイライトの部分を周りの髪の色で埋めた。sRGB） |
| tx_hai_hm.png | ハイライトの形（元のテクスチャとの明るさの差。線形、グレースケール） |
| split_hair_highlight.py | 分ける手順 1：黄色みでハイライトを見つけ、ベースカラーを作る |
| make_highlight_mask.py | 分ける手順 2：元のテクスチャとベースカラーの差から、ハイライトの形と色（HairHighlightTextureColor の初期値）を求める |

作り直すとき：元のテクスチャを tx_hai.png として書き出し、同じフォルダで split_hair_highlight.py、make_highlight_mask.py の順に実行する（numpy と opencv-python が必要）。
make_highlight_mask.py の出力 tx_hai_hm2.png を tx_hai_hm.png にする。

UE では、tx_hai_0000_01_cl_h1_base（sRGB）と tx_hai_0000_01_cl_h1_hm（sRGB なし、Grayscale）を、
プラグインの M_ToonHairHighlightMask のインスタンス mi_hai_0000_01_cl_h1_toon に指定し、髪のスロットに割り当てている。
プロファイル TCP_chr_0000_01 の「髪のハイライトをテクスチャから描く」をオンにしている。
