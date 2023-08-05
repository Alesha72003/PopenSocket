from distutils.core import setup, Extension
setup(name="PopenSocket", version="1.0", ext_modules=[Extension("PopenSocket", ["PopenSocket.c", "winAPITools.c"])])