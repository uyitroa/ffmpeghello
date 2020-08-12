from test1 import PyFrameWriter
import numpy as np
a = PyFrameWriter(b"ok.mp4", 1920, 1080, 60)
a.open_video()
#nparr = np.asarray(arr, np.uint8)
nparr = np.frombuffer(a, dtype=np.uint8)
print(nparr[:10])

nparr[:1000] = 255
print(nparr[:10])
for x in range(600):
	a.write_frame()
a.close_video()
