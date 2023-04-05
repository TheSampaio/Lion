#ifndef OWL_DEBUG_H
#define OWL_DEBUG_H

// Enumerates log modes
enum ELogMode
{
    None = 0,
    Information,
    Warning,
    Error
};

class OWL_API Debug
{
public:
    Debug() {};

    void Log(ELogMode Mode, const char* Text, bool BreakLine = true, bool Pause = false);
};

#endif
