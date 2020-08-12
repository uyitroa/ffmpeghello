# distutils: language = c++

from cpython cimport Py_buffer


# Declare the class with cdef
cdef extern from "FrameWriter.h":
	cdef cppclass FrameWriter:
		FrameWriter();
		FrameWriter(const char *filename, int width, int height, int fps);
		void open_video();
		void close_video();
		int write_frame();
		unsigned char *getbuffer();
		int getbuffersize();


cdef class PyFrameWriter:
	cdef FrameWriter c_writer  # Hold a C++ instance which we're wrapping

	def __cinit__(self, const char *filename, int width, int height, int fps):
		self.c_writer = FrameWriter(filename, width, height, fps)
		# self.view_count = 0

	def open_video(self):
		return self.c_writer.open_video()

	def close_video(self):
		return self.c_writer.close_video()

	def write_frame(self):
		return self.c_writer.write_frame()

	def __getbuffer__(self, Py_buffer *buffer, int flags):
		cdef Py_ssize_t itemsize = sizeof(unsigned char)

		# self.shape[0] = self.v.size() / self.ncols
		# self.shape[1] = self.ncols
		#
		# # Stride 1 is the distance, in bytes, between two items in a row;
		# # this is the distance between two adjacent items in the vector.
		# # Stride 0 is the distance between the first elements of adjacent rows.
		# self.strides[1] = <Py_ssize_t>(  <char *>&(self.v[1])
		# 							   - <char *>&(self.v[0]))
		# self.strides[0] = self.ncols * self.strides[1]

		buffer.buf = self.c_writer.getbuffer()
		buffer.format = 'b'                     # uint8_t
		buffer.internal = NULL                  # see References
		buffer.itemsize = itemsize
		buffer.len = <Py_ssize_t>self.c_writer.getbuffersize()   # product(shape) * itemsize
		buffer.ndim = 1
		buffer.obj = self
		buffer.readonly = 0
		buffer.shape = &buffer.len
		buffer.strides = &itemsize
		buffer.suboffsets = NULL                # for pointer arrays only

		# self.view_count += 1

	def __releasebuffer__(self, Py_buffer *buffer):
		# self.view_count -= 1
		pass
#
# cdef class AVFrameBuffer:
# 	cdef char *buffer
# 	def __cinit__(self, pyframewriter):
# 		self.buffer = pyframewriter.getbuffer()
# 		self.size = pyframewriter.getbuffersize()
# 		self.view_count = 0
#
# 	def __getbuffer__(self, Py_buffer *buffer, int flags):
# 		cdef Py_ssize_t itemsize = sizeof(self.buffer[0])
#
# 		# self.shape[0] = self.v.size() / self.ncols
# 		# self.shape[1] = self.ncols
# 		#
# 		# # Stride 1 is the distance, in bytes, between two items in a row;
# 		# # this is the distance between two adjacent items in the vector.
# 		# # Stride 0 is the distance between the first elements of adjacent rows.
# 		# self.strides[1] = <Py_ssize_t>(  <char *>&(self.v[1])
# 		# 							   - <char *>&(self.v[0]))
# 		# self.strides[0] = self.ncols * self.strides[1]
#
# 		buffer.buf = &(self.buffer[0])
# 		buffer.format = 'b'                     # uint8_t
# 		buffer.internal = NULL                  # see References
# 		buffer.itemsize = itemsize
# 		buffer.len = self.size   # product(shape) * itemsize
# 		buffer.ndim = 1
# 		buffer.obj = self
# 		buffer.readonly = 0
# 		buffer.shape = 1
# 		buffer.strides = itemsize
# 		buffer.suboffsets = NULL                # for pointer arrays only
#
# 		self.view_count += 1
#
# 	def __releasebuffer__(self, Py_buffer *buffer):
# 		self.view_count -= 1

def getwriter():
	print("HELLO")
	cdef FrameWriter zzz
	zzz = FrameWriter("test.mp4", 1920, 1080, 60)
