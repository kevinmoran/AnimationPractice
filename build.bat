@echo off
@setlocal

set start=%time%

@REM -------------------------------------------------------------------------------------------------------------------

set EXE_FILENAME=main.exe

set COMMON_COMPILER_FLAGS=/nologo /EHa- /GR- /fp:fast /Oi /W4 /FC /Fm /Fe%EXE_FILENAME%

IF "%~1"=="-release" (
    set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% /DNDEBUG /O2
    set BUILD_DIR=build-release\
    echo BUILDING RELEASE
) ELSE (
    set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% /DDEBUG /DDEBUG_BUILD /Od /MDd /Zi
    set BUILD_DIR=build\
)

set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref
set SYSTEM_LIBS=user32.lib gdi32.lib winmm.lib d3d11.lib d3dcompiler.lib

@REM Uncomment one of these to choose between normal or Single Translation Unit build 
@REM set SRC_FILES=../main.cpp ../Collision.cpp ../Player.cpp ../Camera.cpp ../ObjLoading.cpp ../D3D11Helpers.cpp
set SRC_FILES=../jumbo.cpp

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

cl %COMPILER_FLAGS% %SRC_FILES% /link %LINKER_FLAGS% %SYSTEM_LIBS%

popd

@REM -------------------------------------------------------------------------------------------------------------------

set end=%time%
set options="tokens=1-4 delims=:.,"
for /f %options% %%a in ("%start%") do set start_h=%%a&set /a start_m=100%%b %% 100&set /a start_s=100%%c %% 100&set /a start_ms=100%%d %% 100
for /f %options% %%a in ("%end%") do set end_h=%%a&set /a end_m=100%%b %% 100&set /a end_s=100%%c %% 100&set /a end_ms=100%%d %% 100

set /a hours=%end_h%-%start_h%
set /a mins=%end_m%-%start_m%
set /a secs=%end_s%-%start_s%
set /a ms=%end_ms%-%start_ms%
if %ms% lss 0 set /a secs = %secs% - 1 & set /a ms = 100%ms%
if %secs% lss 0 set /a mins = %mins% - 1 & set /a secs = 60%secs%
if %mins% lss 0 set /a hours = %hours% - 1 & set /a mins = 60%mins%
if %hours% lss 0 set /a hours = 24%hours%
if 1%ms% lss 100 set ms=0%ms%

:: Mission accomplished
set /a totalsecs = %hours%*3600 + %mins%*60 + %secs%
echo DONE: %hours%:%mins%:%secs%.%ms% (%totalsecs%.%ms%s total)