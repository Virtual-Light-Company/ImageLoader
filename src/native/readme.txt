Build Instructions:

Windows users:
Requires the Sun JDK, and Microsoft Visual C to compile.

1. Ensure that both the zlib and libpng libraries have
   been unpacked and placed in the directories,
   zlib and libpng respectively.

   e.g.
     cd ajn\chapter10\src
     mkdir zlib
     pkunzip -d archive\zlib.zip zlib
     pkunzip -d archive\lpng102.zip .
     move lpng102 libpng

2. Edit the build script (build.bat under Windows, build.sh
   under unix) and ensure that the given paths are correct
   for your machine.

3. Run the build script.

    +
    |--------libjpeg
    |--------libpng
    |--------libtiff
    |          |
               +----libtiff
                       |
                       +-----
    |--------zlib
                
