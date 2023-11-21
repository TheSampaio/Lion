:: Title
ECHO OFF
TITLE Build
CLS

:: Main
CD ../
CALL premake5 vs2022
ECHO .
PAUSE
