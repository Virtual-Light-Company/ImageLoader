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

REM Compile the class files if necessary
if exist vlc\content\image\ImageDecoder.class goto BUILD_JAR
  echo Compiling java classes
  javac vlc/content/*.java
  javac vlc/content/image/*.java

:BUILD_JAR
echo Constructing JAR file
jar cf ..\..\lib\imageloader-1.0.jar vlc\net\content\*.class vlc\net\content\image\*.class

echo Cleaning up
del /Q /S *.class

echo Done

ENDLOCAL
