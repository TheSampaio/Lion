#ifndef OWL_APPLICATION_H
#define OWL_APPLICATION_H

#include "Debug.h"
#include "Renderer.h"
#include "Window.h"

class OWL_API Application
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
    inline Debug* GetDebug()       { return s_Debug.get(); }
    inline Window* GetWindow()     { return s_Window.get(); }
    inline Renderer* GetRenderer() { return s_Renderer.get(); }

    // Friends
    friend Window;
    friend Renderer;
    friend class Shader;

private:
    // Main methods
    bool Loop();

    // Static attributes
    static std::unique_ptr<Debug> s_Debug;
    static std::unique_ptr<Renderer> s_Renderer;
    static std::unique_ptr<Window> s_Window;
};

#endif
