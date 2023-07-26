from distutils.core import setup, Extension
setup(name="AsyncPopenSocket", version="1.0", ext_modules=[Extension("AsyncPopenSocket", ["asyncPopenSocket.c", "winAPITools.c"])])