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
    // Main methods
    static void Log(ELogMode Mode, const char* Text, bool Pause = false, bool BreakLine = true) { return Instance().ILog(Mode, Text, Pause, BreakLine); }

private:
    Debug() {};

    // Instantiate a static class's reference
    inline static Debug& Instance() { static Debug Instance; return Instance; }

    // Delete copy constructor and assignment operator
    Debug(const Debug&) = delete;
    Debug operator=(const Debug&) = delete;

    // Main internal methods
    void ILog(ELogMode Mode, const char* Text, bool Pause, bool BreakLine);
};

#endif
