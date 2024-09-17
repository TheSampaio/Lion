ECHO OFF
TITLE Build
CLS

:: Main
CALL premake5 vs2022

IF ERRORLEVEL 1 (
    ECHO Failed to generate projects. Retrying...
    ECHO .
    CD ../
    CALL premake5 vs2022
)
ECHO .
PAUSE
