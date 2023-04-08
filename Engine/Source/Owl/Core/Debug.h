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

    // Delete copy constructor and assignment operator
    Debug(const Debug&) = delete;
    Debug operator=(const Debug&) = delete;

    void Log(ELogMode Mode, const char* Text, bool BreakLine = true, bool Pause = false);
};

#endif
