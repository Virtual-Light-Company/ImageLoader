These are the standard windows libraries for when you are building the DLL.
They are named using the unix convention so that compilation works as needed.
However, for runtime, you will need to rename them to what they are expecting
that they are named.

libpng.dll -> libpng13.dll
libjpeg.dll -> jpeg62.dll
libtiff.dll -> libtiff3.dll
libz.dll -> zlib1.dll

These will then need to be placed somewhere in your path that Java can find
them.

Justin
