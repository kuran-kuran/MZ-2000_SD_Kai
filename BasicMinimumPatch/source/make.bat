if exist "output" goto :START
mkdir output
:START
z80as -x -ooutput\MZ-1Z001MINSDPATCH.bin MZ-1Z001MINSDPATCH.ASM
z80as -x -ooutput\MZ-1Z002MINSDPATCH.bin MZ-1Z002MINSDPATCH.ASM
z80as -x -ooutput\SB-5520MINSDPATCH.bin SB-5520MINSDPATCH.ASM
pause
goto :START
