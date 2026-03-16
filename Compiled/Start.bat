@echo off
setlocal

REM Get current date and time in format YYYY-MM-DD_HH-MM-SS
for /f %%A in ('powershell -NoProfile -Command "Get-Date -Format yyyy-MM-dd_HH-mm-ss"') do set LOGDATETIME=%%A

REM Set path to log folder and create it if it doesn't exist
set LOGDIR=logs
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

REM Change to the directory where Controller.exe is
cd /d "C:\Users\Okto\source\repos\CameraSystem\x64\Test\"

REM Use PowerShell to run the program and tee the output to both console and log file
powershell -NoProfile -Command ^
    "& { Start-Process -NoNewWindow -FilePath .\Controller.exe -RedirectStandardOutput (Join-Path '%LOGDIR%' 'controller_log_%LOGDATETIME%.txt') }"

echo recorder running ...
endlocal
