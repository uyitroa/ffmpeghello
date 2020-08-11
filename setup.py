from distutils.core import setup, Extension
setup(ext_modules=[Extension("pymyarray2", ["pymyarray2.cpp"],                   include_dirs = ['include'],
                    libraries = ['avformat'],
                    library_dirs = ['lib'])])
