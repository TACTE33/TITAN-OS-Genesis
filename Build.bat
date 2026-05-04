@echo off
:: Direct link to your new SGDK path
set GDK=C:\sgdk211
set PATH=%GDK%\bin;%PATH%

echo ==============================================
echo       GENESIS BUILD: GEN-CMD TITAN OS
echo ==============================================

:: This calls the SGDK brain to compile your 'src/main.c'
make -f %GDK%\makefile.gen

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] 16-bit ROM Generated: out\rom.bin
) else (
    echo.
    echo [ERROR] Build failed. Review logic in main.c
    pause
)