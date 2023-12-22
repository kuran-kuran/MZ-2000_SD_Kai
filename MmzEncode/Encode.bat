echo off
setlocal enabledelayedexpansion

set "exe_path=Release\MmzEncode.exe"
set "folder_path=%1\"
set "mztname=EncodeImage.mzt"

%exe_path% %folder_path%0001.png %mztname%

for /l %%a in (1, 1, 9999) do (
  set count=%%a
  set "count_str=000!count!"
  set "filename1=!count_str:~-4!.png"
  set /a count += 1
  set "count_str=000!count!"
  set "filename2=!count_str:~-4!.png"
  if exist %folder_path%!filename1! (
    if exist %folder_path%!filename2! (
      %exe_path% /add %folder_path%!filename1! %folder_path%!filename2! %mztname%
    ) else (
      goto :end
    )
  ) else (
    goto :end
  )
)

goto :end

:test1:
set count=1
for %%f in (*) do (
  set "filename=%%~nf"
  set "ext=%%~xf"
  set "count_str=000!count!"
  set "filename1=!count_str:~-4!!ext!"
  echo "!filename1!"
  set /a count+=1
)

:end

endlocal

pause
