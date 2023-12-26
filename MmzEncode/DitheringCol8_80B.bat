setlocal enabledelayedexpansion

for /l %%a in (1, 1, 9999) do (
  set count=%%a
  set "count_str=000!count!"
  set "infilename=!count_str:~-4!.png"
  set "outfilename=!count_str:~-4!.png"
  ..\DitheringCol8\Release\DitheringCol8 /blue "bad_apple320x200/!infilename!" "bad_apple320x200_1/!outfilename!"
)

endlocal

pause
