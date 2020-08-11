import pymyarray2
import numpy as np
arr = pymyarray2.PyMyArray(10)
print(arr)
nparr = np.asarray(arr, np.uint8)
print(nparr[:10])
