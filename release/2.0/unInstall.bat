@echo off

pip uninstall -r requirements.txt

if errorlevel 1 (
    echo.
    echo Requirements uninstallation failed. please run uninstall.bat again.
) else (
    echo.
    echo Requirements uninstalled successfully.
)
pause