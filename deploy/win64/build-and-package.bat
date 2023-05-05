@rem  Run this from within the top-level dir: deploy\clean-build-and-package
@echo on

set STARTPWD=%CD%
set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\x64;%PATH%

rem @set /p VERSION=<build_win32\version.h
rem @set VERSION=%VERSION:#define SV_VERSION "=%
rem set VERSION=%VERSION:"=%

set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %vcvarsall% (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

if not exist "C:\Program Files (x86)\SMLNJ\bin" (
@   echo Could not find SML/NJ, required for Repoint
@   exit /b 2
)

set VERSION=%1
shift
if "%VERSION%" == "" (
@echo "Usage: win.bat <version>"
exit /b 1
)

@echo Building version %VERSION%

call %vcvarsall% amd64
if errorlevel 1 exit /b %errorlevel%

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

del /q /s build

meson build --buildtype release -Dtests=disabled "-Db_vscrt=mt"
if errorlevel 1 exit /b %errorlevel%

ninja -C build
if errorlevel 1 exit /b %errorlevel%

cd build
set NAME=Christopher Cannam
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a expressive-means.dll
if errorlevel 1 exit /b %errorlevel%

cd ..
set DIR=ExpressiveMeans-win64-%VERSION%.zip
del /q /s %DIR%
mkdir %DIR%
copy build\expressive-means.dll %DIR%
copy expressive-means.cat %DIR%
copy COPYING %DIR%\COPYING.txt
copy README.md %DIR%\README.txt

set PATH=%ORIGINALPATH%
cd %STARTPWD%
@echo Done, now test and zip the directory %DIR%

