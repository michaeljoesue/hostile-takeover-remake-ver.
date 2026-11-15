@echo off
rem Copies the built editor (Release) to ..\bin
copy /Y mcl\bin\Release\mcl.exe ..\bin
copy /Y mgui\bin\Release\mgui.exe ..\bin\m.exe
copy /Y bin\Release\m.dll ..\bin
