@echo off

REM  ************************************************************************
REM  *                The Virtual Light Company Copyright (c) 1999
REM  *                               Java Source
REM  *
REM  * This code is licensed under the GNU Library GPL. Please read
REM  * license.txt for the full details. A copy of the LGPL may be found at
REM  *
REM  * http://www.gnu.org/copyleft/lgpl.html
REM  *
REM  * Project:    Image Content Handlers
REM  *
REM  * Version History
REM  * Date        TR/IWOR  Version  Programmer
REM  * ----------  -------  -------  ----------------------------------------
REM  *
REM  ************************************************************************

SETLOCAL

REM ############################# Edit this #################################
REM Set this to where the jdk lives
set jdk_home_=c:\progra~1\jdk1.2

REM Set this to the directory where M$ devstudio stuff is
set dev_=c:\progra~1\devstudio
REM #########################################################################

REM Set the path to the compilers & necessary environment variables

set PATH=%PATH%;%dev_%\vc\bin;%dev_%\sharedide\bin;%jdk_home_%\bin
set LIB=%dev_%\vc\lib
set INCLUDE=%dev_%\vc\include;%jdk_home_%\include;%jdk_home_%\include\win32
set CLASSPATH=%CLASSPATH%;..\java

REM Ensure that the jdkhome path is correctly setup
if exist %jdk_home_%\bin\java.exe goto JDKHOME_OKAY
  echo jdk_home_ variable not set correctly, edit this script and run again
  goto EXIT

:JDKHOME_OKAY
REM Compile the class files if necessary
if exist ..\java\vlc\content\image\ImageDecoder.class goto BUILD_LIBS
  echo Compiling java classes
  javac ../java/vlc/content/*.java ../java/vlc/content/image/*.java

:BUILD_LIBS
REM Now build the libraries

REM Build the zlib library for win32 systems
if exist zlib goto ZLIB_OKAY
  echo zlib library not properly unpacked into zlib
  goto EXIT

:ZLIB_OKAY
  echo Building zlib
  cd zlib
  nmake -nologo /f msdos\Makefile.w32
  copy zlib.lib ..\lib
  cd ..

REM Build the libpng library for win32 systems
if exist libpng GOTO PNG_OKAY
  echo PNG library not properly unpacked into libpng
  goto EXIT

:PNG_OKAY
  echo Building libpng
  cd libpng
  nmake -nologo /f scripts\makefile.w32
  copy libpng.lib ..\lib
  cd ..

REM Build the libjpeg library for win32 systems
if exist libjpeg GOTO JPG_OKAY
  echo Jpeg library not properly unpacked into libjpeg
  goto EXIT

:JPG_OKAY
  echo Building libjpeg
  cd libjpeg
  nmake -nologo /f ..\archive\jpeg\jpeg.mak
  copy libjpeg.lib ..\lib
  cd ..

REM Build the libtiff library for win32 systems
if exist libtiff GOTO TIFF_OKAY
  echo Tiff library not properly unpacked into libtiff
  goto EXIT

:TIFF_OKAY
  REM copy required files
  if not exist libtiff\libtiff\fax3sm.c copy archive\tiff\fax3sm.c libtiff\libtiff
  if not exist libtiff\libtiff\version.h copy archive\tiff\version.h libtiff\libtiff
  if not exist libtiff\libtiff\tif_custom.c copy archive\tiff\tif_custom.c libtiff\libtiff
  if not exist libtiff\libtiff\tiff.mak copy archive\tiff\tiff.mak libtiff\libtiff

  echo Building libtiff
  cd libtiff\libtiff
  nmake -nologo /f tiff.mak
  copy libtiff.lib ..\..\lib
  cd ..\..

REM create the shared library
echo Building shared library
set LIB=%LIB%;.\lib
nmake /f makefile.w32

REM Clean up

del *.obj
del vlc_content_image_ImageDecoder.h
del image_decode.exp
del image_decode.lib

echo Build complete: 'image_decode.dll' created

:EXIT

ENDLOCAL
