#ifndef OWL_APPLICATION_H
#define OWL_APPLICATION_H

#include "Debug.h"
#include "Renderer.h"
#include "Window.h"

class OWL_API Application
{
public:
    Application();
    ~Application();

    // Delete copy constructor and assignment operator
    Application(const Application&) = delete;
    Application operator=(const Application&) = delete;

    // Main methods
    bool Run();

    // Friends
    friend Window;
    friend Renderer;
    friend class Shader;
    friend class Mesh;

protected:
    virtual void Start() = 0;
    virtual void Update(float DeltaTime) = 0;
    virtual void Draw() = 0;
    virtual void Finalize() = 0;

    // Get methods
    inline Debug* GetDebug()       { return s_Debug.get(); }
    inline Window* GetWindow()     { return s_Window.get(); }
    inline Renderer* GetRenderer() { return s_Renderer.get(); }

private:
    // Main methods
    bool Loop();
    void Init();

    // Static attributes
    static std::unique_ptr<Debug> s_Debug;
    static std::unique_ptr<Renderer> s_Renderer;
    static std::unique_ptr<Window> s_Window;
};

#endif
