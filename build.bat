@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
call "C:\Qt\5.7\mingw53_32\bin\qtenv2.bat"
::call "C:\Qt5.9.2\5.9\msvc2015\bin\qtenv2.bat"
::call "C:\Qt\Qt5.9.1\5.9.1\msvc2015_64\bin\qtenv2.bat"

::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
::call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
::call "C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" x86

cd /D %~dp0
call compile_install.bat

pause
exit /b
