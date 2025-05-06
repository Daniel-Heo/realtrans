@echo off

echo AMD ROCm 지원 버전 설치를 시작합니다...

pip install -r requirements_amd.txt

if errorlevel 1 (
    echo.
    echo Requirements installation failed. please remove venv folder and run install.bat again.
) else (
    echo.
    echo Requirements installed successfully.
    echo AMD ROCm 지원 버전이 성공적으로 설치되었습니다.
)
pause