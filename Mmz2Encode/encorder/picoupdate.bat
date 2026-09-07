@echo off

REM ドロップされたMMZIMAGE.mztのパスを取得
set "MZT_FILE=%~1"

REM 引数がない場合は終了
if "%MZT_FILE%"=="" (
    echo MMZIMAGE.mzt をこのバッチファイルにドラッグ＆ドロップしてください。
    pause
    exit /b 1
)

REM バッチファイルと同じフォルダの picotool.exe を使う場合
set "SCRIPT_DIR=%~dp0\..\bin\"
set "PICOTOOL=%SCRIPT_DIR%picotool.exe"

"%PICOTOOL%" load -v -x "%MZT_FILE%" -t bin -o 0x10280000

pause
