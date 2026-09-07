if exist "output" goto :START
mkdir output
:START
z80as -x -mGVRAMTOOL2000 -ooutput\GVRAMTOOL2000.mzt MAIN2000.ASM
rem z80as -x -mGVRAMTOOL2000-2 -ooutput\GVRAMTOOL2000-2.mzt MAIN2000-32x4.ASM
z80as -x -mGVRAMTOOL80B -ooutput\GVRAMTOOL80B.mzt MAIN80B.ASM
pause
goto :START
