@echo off

REM A quick and dirty batch file to build the test application

if exist ImageViewer.class goto CHECKDLL
  echo Compiling class
  javac -classpath %CLASSPATH%;.. ImageViewer.java

:CHECKDLL
if exist image_decode.dll goto END
  echo Getting image_decode.dll

  if exist ..\src\image_decode.dll goto COPY
    cd ..\src
    call buildlib
    cd ..\example

  :COPY
  copy ..\src\image_decode.dll .

:END

echo Done.
