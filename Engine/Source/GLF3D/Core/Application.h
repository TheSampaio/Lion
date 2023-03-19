#ifndef GLF3D_APPLICATION_H
#define GLF3D_APPLICATION_H

#include "Debug.h"
#include "Window.h"

class GLF3D_API Application
{
public:
    Application();

    // Main methods
    bool Run();

    virtual void Start() {};
    virtual void Update(float DeltaTime) {};
    virtual void DrawCall() {};
    virtual void Finalize() {};

    // Get methods
    inline Debug* GetDebug()   { return s_Debug.get(); }
    inline Window* GetWindow() { return s_Window.get(); }

    // Friends
    friend Window;

private:
    // Main methods
    bool Loop();

    // Static attributes
    static std::unique_ptr<Debug> s_Debug;
    static std::unique_ptr<Window> s_Window;
};

#endif
