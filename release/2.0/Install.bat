@echo off

pip install -r requirements.txt

if errorlevel 1 (
    echo.
    echo Requirements installation failed. please remove venv folder and run install.bat again.
) else (
    echo.
    echo Requirements installed successfully.
)
pause