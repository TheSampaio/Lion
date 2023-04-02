#ifndef GLF3D_DEBUG_H
#define GLF3D_DEBUG_H

// Enumerates log modes
enum ELogMode
{
    None = 0,
    Information,
    Warning,
    Error
};

class GLF3D_API Debug
{
public:
    Debug() {};

    void Log(ELogMode Mode, const char* Text, bool BreakLine = true, bool Pause = false);
};

#endif
