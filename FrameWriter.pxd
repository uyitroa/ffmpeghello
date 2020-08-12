cdef extern from "FrameWriter.cpp":
	pass

# Declare the class with cdef
cdef extern from "FrameWriter.h":
	cdef struct OutputStream:
		pass

	cdef cppclass FrameWriter:
		OutputStream video_st;

		FrameWriter();
		FrameWriter(const char *filename, int width, int height, int fps);
		void open_video();
		int write_frame();
		void close_video();
