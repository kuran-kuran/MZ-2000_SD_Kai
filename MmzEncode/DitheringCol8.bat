setlocal enabledelayedexpansion

rem ffmpeg -i "bad_apple/image-000000001.png" -vf pad=w=iw+52:h=ih:x=26:y=0:color=white -s 640x200 "bad_apple640x200/0001.png"
for /l %%a in (1, 1, 9999) do (
  set count=%%a
  set "count_str=000!count!"
  set "infilename=!count_str:~-4!.png"
  set "outfilename=!count_str:~-4!.png"
  ..\DitheringCol8\Release\DitheringCol8 /blue "bad_apple640x200/!infilename!" "bad_apple640x200_1/!outfilename!"
)

endlocal

pause
