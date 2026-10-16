@echo off
setlocal

rem Rebuild the plugin Content assets (T_EarthMap, M_ToonSeeThroughMask, L_ToonRT_*) with the engine version of the project.
rem Assets saved with UE5.8 cannot be loaded by UE5.5 - 5.7, so run this after copying the plugin into an older project.
rem (This file is ASCII only: cmd.exe misreads multibyte lines in batch files. See README for the Japanese description.)
rem
rem Usage:
rem   Double-click this file, or
rem   RebuildContent.bat [<EngineDir> [<Project.uproject>]] [/y]
rem   - Project : when omitted, the .uproject two folders above the plugin (<Project>\Plugins\ToonRayTracer)
rem   - Engine  : when omitted, the engine of EngineAssociation in the .uproject, found in the Epic Games Launcher install list
rem   - /y      : do not ask before deleting the old assets
rem Example:
rem   RebuildContent.bat "D:\UnrealEngine\UE_5.5" "D:\MyProject\MyProject.uproject"
rem
rem Notes:
rem   - The old assets in the Content folder of THIS plugin are deleted and recreated.
rem     Run it in the plugin copied into the older project, not in the UE5.8 repository.
rem   - Close the editor of the project first. The editor is started offscreen and closes itself when done.

for %%I in ("%~dp0..") do set "PLUGIN_DIR=%%~fI"
set "SCRIPT=%~dp0rebuild_content.py"
set "INTERACTIVE=0"
set "YES=0"
set "ENGINE_DIR="
set "PROJECT="

rem Arguments ("/y" can be anywhere)
for %%A in (%*) do (
  if /i "%%~A"=="/y" (set "YES=1") else if not defined ENGINE_DIR (set "ENGINE_DIR=%%~A") else if not defined PROJECT (set "PROJECT=%%~fA")
)
if "%~1"=="" set "INTERACTIVE=1"

rem Project: the .uproject two folders above the plugin
if not defined PROJECT (
  for %%P in ("%PLUGIN_DIR%\..\..\*.uproject") do if not defined PROJECT set "PROJECT=%%~fP"
)
if not defined PROJECT (
  echo .uproject not found above "%PLUGIN_DIR%". Pass it as the second argument.
  goto :fail
)
if not exist "%PROJECT%" (
  echo Project not found: "%PROJECT%"
  goto :fail
)
for %%P in ("%PROJECT%") do (
  set "PROJECT_DIR=%%~dpP"
  set "PROJECT_NAME=%%~nP"
)

rem Engine: EngineAssociation of the .uproject -> install location in the Epic Games Launcher list
if not defined ENGINE_DIR (
  for /f "usebackq delims=" %%E in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$a=(Get-Content -Raw -LiteralPath $env:PROJECT | ConvertFrom-Json).EngineAssociation; $l=Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'; if ($a -and (Test-Path $l)) { (Get-Content -Raw $l | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq ('UE_' + $a) } | Select-Object -First 1 -ExpandProperty InstallLocation }"`) do set "ENGINE_DIR=%%E"
)
if not defined ENGINE_DIR (
  echo The engine of the project was not found in the Epic Games Launcher install list.
  set /p "ENGINE_DIR=Enter the engine folder (e.g. D:\UnrealEngine\UE_5.5): "
)
if defined ENGINE_DIR set "ENGINE_DIR=%ENGINE_DIR:"=%"
set "EDITOR=%ENGINE_DIR%\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR%" (
  echo Editor not found: "%EDITOR%"
  goto :fail
)

set "LOG=%PROJECT_DIR%Saved\Logs\ToonRebuildContent.log"

rem The running editor keeps its log file open. If it cannot be opened for writing, the editor is still running
set "EDITOR_LOG=%PROJECT_DIR%Saved\Logs\%PROJECT_NAME%.log"
if exist "%EDITOR_LOG%" (
  2>nul (>>"%EDITOR_LOG%" call ) || (
    echo The editor of this project seems to be running. Close it and run again.
    goto :fail
  )
)

echo Plugin : "%PLUGIN_DIR%"
echo Engine : "%ENGINE_DIR%"
echo Project: "%PROJECT%"
if "%YES%"=="0" (
  choice /m "Delete the old assets and rebuild them"
  if errorlevel 2 goto :fail
)
if not exist "%PROJECT_DIR%Saved\Logs" mkdir "%PROJECT_DIR%Saved\Logs"

rem Delete the assets first. Unloadable assets with the same names would block creating the new ones
del /q "%PLUGIN_DIR%\Content\Textures\T_EarthMap.uasset" 2> nul
del /q "%PLUGIN_DIR%\Content\Materials\M_ToonSeeThroughMask.uasset" 2> nul
del /q "%PLUGIN_DIR%\Content\Maps\L_ToonRT_*.umap" 2> nul

echo Rebuilding (this takes a few minutes)...
"%EDITOR%" "%PROJECT%" -ExecCmds="py %SCRIPT:\=/%" -ToonRebuildQuit -RenderOffScreen -unattended -nosplash -nopause -stdout -FullStdOutLogOutput > "%LOG%" 2>&1

findstr /c:"TOON_REBUILD" "%LOG%"
findstr /c:"TOON_REBUILD finished: OK" "%LOG%" > nul
if errorlevel 1 (
  echo Failed. See the log: "%LOG%"
  goto :fail
)
echo Done.
if "%INTERACTIVE%"=="1" pause
exit /b 0

:fail
if "%INTERACTIVE%"=="1" pause
exit /b 1
