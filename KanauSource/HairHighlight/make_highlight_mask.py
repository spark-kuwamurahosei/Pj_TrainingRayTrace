import numpy as np, cv2
def lin(c): return np.where(c<=0.04045, c/12.92, ((c+0.055)/1.055)**2.4)
o = cv2.imread('tx_hai.png', cv2.IMREAD_UNCHANGED).astype(np.float32)[...,2::-1]/255
b = cv2.imread('tx_hai_base.png', cv2.IMREAD_UNCHANGED).astype(np.float32)[...,2::-1]/255
d = np.maximum(lin(o)-lin(b),0)
L = d @ np.array([0.2126,0.7152,0.0722],np.float32)
print('L pct', np.percentile(L,[90,99,99.9]).round(4), 'max', L.max().round(4))
D = np.percentile(L[L>0.005], 98)
sel = L>0.01
col = d[sel].sum(0)/L[sel].sum()
print('D', round(float(D),4), 'color per unit lum', col.round(3), 'color*D', (col*D).round(4))
m = np.clip(L/D,0,1)
cv2.imwrite('tx_hai_hm2.png', (m*255+0.5).astype(np.uint8))
print('stored linear')
