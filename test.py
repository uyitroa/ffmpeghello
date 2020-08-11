import pymyarray2
import numpy as np
arr = pymyarray2.PyMyArray(10)
print(arr)
#nparr = np.asarray(arr, np.uint8)
nparr = np.frombuffer(arr, dtype=np.uint8)
print(nparr[:10])

nparr[1] = 100
print(arr)
print(nparr[:10])
