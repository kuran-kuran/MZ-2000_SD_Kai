echo off
setlocal enabledelayedexpansion

set "exe_path=Release\MmzEncode.exe"
set "folder_path=%1\"
set "mztname=MMZIMAGE.mzt"

%exe_path% /80B %folder_path%0001.png %mztname%

for /l %%a in (1, 1, 9999) do (
  set count=%%a
  set "count_str=000!count!"
  set "filename1=!count_str:~-4!.png"
  set /a count += 1
  set "count_str=000!count!"
  set "filename2=!count_str:~-4!.png"
  if exist %folder_path%!filename1! (
    if exist %folder_path%!filename2! (
      %exe_path% /80B /add %folder_path%!filename1! %folder_path%!filename2! %mztname%
    ) else (
      goto :end
    )
  ) else (
    goto :end
  )
)

:end

endlocal

pause
