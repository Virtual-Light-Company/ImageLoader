Image loader.

Image types supported:
----------------------
GIF
Jpeg
PNG
Portable pixmap
Portable graymap
Targa
Tiff - LZW compressed images are not decoded correctly on Win32/VC++5.0
MS & OS/2 bitmaps


Directory Map.
--------------
+ lib     - Native library files
+ jars    - Java JAR files
+ src     - Java and C source code.
+ example - Sample application to test image loaders.
+ docs    - Design documentation

Note For Win32 Users
---------------------

There is a really major series of bugs with Visual C++ v5.0. zlib fails to
work correctly due to something odd that VC++ is doing with byte sizes or
ordering or something like that. Apart from that, by simply swapping from
v5 to v6 you get a 50% speed increase!. So, if you are compiling the code
from scratch, we heartily recommend that either you use VC++ 6.0 or use the
Cygnus Win32 tools.
