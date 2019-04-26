@echo off
:next
set filename=%1
if '%filename%'=='' goto on_exit
ffmpeg.exe -i %filename% %filename:~0,-3%.mp4
del %filename%
shift
goto next
:on_exit