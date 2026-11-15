@echo off
rem Copies the built editor to ..\bin
copy /Y mcl\bin\Debug\mcl.exe ..\bin
copy /Y mgui\bin\Debug\mgui.exe ..\bin\m.exe
copy /Y bin\Debug\m.dll ..\bin
