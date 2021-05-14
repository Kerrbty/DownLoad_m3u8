@echo off
:next
set filename=%1
if '%filename%'=='' goto on_exit
if "%filename:~-1%^"==""^" goto on_quotation
ffmpeg.exe -i %filename% %filename:~0,-3%.mp4
goto continue
:on_quotation
ffmpeg.exe -i %filename% %filename:~0,-4%.mp4"
:continue
del %filename%
shift
goto next
:on_exit