@echo off

set VSTOOLS=%VS120COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS110COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS100COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS90COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS80COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS71COMNTOOLS%
if "%VSTOOLS%"=="" (
echo ERROR: Visual studio is not found.
set ERRORLEVEL=-1
goto :end
)

echo VSTOOLS = %VSTOOLS%
call "%VSTOOLS%vsvars32.bat"
cl /O2 /LD /I../../../../public/include %1

:end
