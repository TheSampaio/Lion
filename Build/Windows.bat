:: Title
TITLE Windows Builder
ECHO OFF

:: Premake5
CD ..
CLS
CALL premake5.exe vs2022

:: Pause Console
ECHO .
PAUSE
