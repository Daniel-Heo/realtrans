@echo off

pip uninstall -r requirements_amd.txt

if errorlevel 1 (
    echo.
    echo Requirements_amd uninstallation failed. please run uninstall_amd.bat again.
) else (
    echo.
    echo Requirements_amd uninstalled successfully.
)
pause