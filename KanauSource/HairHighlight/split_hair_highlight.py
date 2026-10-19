import numpy as np, cv2
bgra = cv2.imread('tx_hai.png', cv2.IMREAD_UNCHANGED).astype(np.float32)/255
rgb = bgra[...,2::-1].copy()
Y = (rgb[...,0]+rgb[...,1])*0.5 - rgb[...,2]
m = np.clip((Y-0.062)/0.05,0,1)
# ハイライトの周りの色：ハイライトを除いてぼかした色（重み付きのぼかし）
w = (1-m)**4
blur = lambda x: cv2.GaussianBlur(x,(0,0),10)
around = blur(rgb*w[...,None]) / np.maximum(blur(w),1e-4)[...,None]
# 細い毛の線の模様は残すため、ハイライトの部分だけ周りの色へ寄せる（明るさの差の分だけ）
base = rgb + (around - rgb) * m[...,None]
# 表で同じ見た目に戻るときの倍率（ハイライトの部分の 元 / ベース の平均）
sel = m > 0.6
print('gain rgb', (rgb[sel].mean(0)/np.maximum(base[sel].mean(0),1e-4)).round(3), 'count', sel.sum())
out = bgra.copy(); out[...,2::-1] = np.clip(base,0,1)
cv2.imwrite('tx_hai_base.png', (out*255+0.5).astype(np.uint8))
cv2.imwrite('tx_hai_hm.png', (m*255+0.5).astype(np.uint8))
v = np.concatenate([rgb, base, np.repeat(m[...,None],3,2)],1)
cv2.imwrite('split_view.png', cv2.resize((v[...,::-1]*255).astype(np.uint8),(1536,512),interpolation=cv2.INTER_AREA))
