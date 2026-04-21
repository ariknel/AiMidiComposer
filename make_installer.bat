@echo off
setlocal EnableDelayedExpansion
title AI MIDI Composer - Build Installer
cd /d "%~dp0"

echo.
echo  ============================================================
echo   AI MIDI Composer - Installer Builder
echo  ============================================================
echo.
echo  Working dir: %CD%
echo.

if not exist "build\AIMidiComposerVST_artefacts\Release\VST3\AI MIDI Composer.vst3" (
    echo  [ERROR] VST3 not found. Run build.bat first.
    pause & exit /b 1
)
echo  [OK] VST3 found.

if not exist "sidecar\dist\sidecar\sidecar.cmd" (
    echo  [ERROR] sidecar.cmd not found. Run build_sidecar.bat first.
    pause & exit /b 1
)
echo  [OK] Sidecar found.

if not exist "sidecar\.venv\Scripts\python.exe" (
    echo  [ERROR] venv not found. Run build_sidecar.bat first.
    pause & exit /b 1
)
echo  [OK] Venv found.

echo.
echo  [..] Splitting venv into two zip archives...
echo       (This takes 3-5 minutes - please wait)
echo.

if not exist "sidecar\dist" mkdir "sidecar\dist"
if exist "sidecar\dist\venv_a.zip" del /q "sidecar\dist\venv_a.zip"
if exist "sidecar\dist\venv_b.zip" del /q "sidecar\dist\venv_b.zip"

set "SPLIT_PY=%TEMP%\split_%RANDOM%.py"
python -c "open(r'%SPLIT_PY%','w').write(open('installer\split_venv.py').read())"
python "!SPLIT_PY!"
if errorlevel 1 (
    echo  [ERROR] Venv split failed.
    del /q "!SPLIT_PY!" 2>nul
    pause & exit /b 1
)
del /q "!SPLIT_PY!"

if not exist "sidecar\dist\venv_a.zip" (
    echo  [ERROR] venv_a.zip was not created.
    pause & exit /b 1
)
if not exist "sidecar\dist\venv_b.zip" (
    echo  [ERROR] venv_b.zip was not created.
    pause & exit /b 1
)
for %%F in ("sidecar\dist\venv_a.zip") do echo  [OK] venv_a.zip  %%~zF bytes
for %%F in ("sidecar\dist\venv_b.zip") do echo  [OK] venv_b.zip  %%~zF bytes

:: Create empty placeholder zips for parts c/d if they don't exist
:: (Inno Setup 5 requires all Source: files exist at compile time)
if not exist "sidecar\dist\venv_c.zip" (
    python -c "import zipfile; zipfile.ZipFile('sidecar/dist/venv_c.zip','w').close()"
    echo  [..] Created empty venv_c.zip placeholder
)
if not exist "sidecar\dist\venv_d.zip" (
    python -c "import zipfile; zipfile.ZipFile('sidecar/dist/venv_d.zip','w').close()"
    echo  [..] Created empty venv_d.zip placeholder
)

set "ISCC="
for %%P in (
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "C:\Program Files\Inno Setup 6\ISCC.exe"
    "C:\Program Files (x86)\Inno Setup 5\ISCC.exe"
    "C:\Program Files\Inno Setup 5\ISCC.exe"
) do (
    if "!ISCC!"=="" if exist %%P set "ISCC=%%~P"
)

if "!ISCC!"=="" (
    echo  [ERROR] Inno Setup not found. Download: https://jrsoftware.org/isdl.php
    pause & exit /b 1
)
echo  [OK] Inno Setup: !ISCC!

if not exist dist mkdir dist
if exist "dist\AIMidiComposer-Installer.exe" del /q "dist\AIMidiComposer-Installer.exe"

echo.
echo  [..] Running Inno Setup...
echo.
"!ISCC!" installer.iss
if errorlevel 1 (
    echo  [ERROR] Inno Setup failed.
    pause & exit /b 1
)

if not exist "dist\AIMidiComposer-Installer.exe" (
    echo  [ERROR] Installer exe missing after build.
    pause & exit /b 1
)

echo.
echo  ============================================================
echo   SUCCESS
echo  ============================================================
for %%F in ("dist\AIMidiComposer-Installer.exe") do echo   dist\AIMidiComposer-Installer.exe  (%%~zF bytes)
echo.
pause
