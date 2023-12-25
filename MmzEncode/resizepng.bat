setlocal enabledelayedexpansion

rem ffmpeg -i "bad_apple/image-000000001.png" -vf pad=w=iw+52:h=ih:x=26:y=0:color=black -s 640x200 "bad_apple640x200/0001.png"
for /l %%a in (1, 1, 9999) do (
  set count=%%a
  set "count_str=000!count!"
  set "infilename=image-00000!count_str:~-4!.png"
  set "outfilename=!count_str:~-4!.png"
  ffmpeg -i "bad_apple/!infilename!" -vf pad=w=iw+52:h=ih:x=26:y=0:color=black -s 640x200 "bad_apple640x200/!outfilename!"
)

endlocal

pause
