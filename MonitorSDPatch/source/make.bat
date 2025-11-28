if exist "output" goto :START
mkdir output
:START
z80as -ooutput\MonitorSDPatch.bin MonitorSDPatch.asm
z80as -x -mMONITORSDPATCH -ooutput\MonitorSDPatch.mzt MonitorSDPatch.asm
pause
goto :START
