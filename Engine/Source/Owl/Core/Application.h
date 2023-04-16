#ifndef OWL_APPLICATION_H
#define OWL_APPLICATION_H

#include "Debug.h"
#include "Input.h"
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
    friend Input;
    friend Renderer;
    friend class Mesh;

protected:
    virtual void Start() = 0;
    virtual void Update(float DeltaTime) = 0;
    virtual void Draw() = 0;
    virtual void Finalize() = 0;

    // Get methods
    static inline Renderer* GetRenderer() { return s_Renderer.get(); }
    static inline Window* GetWindow()     { return s_Window.get(); }
    static inline Input* GetInput()       { return s_Input.get(); }

private:
    // Main methods
    bool Loop();
    void Init();

    // Static attributes
    static std::unique_ptr<Input> s_Input;
    static std::unique_ptr<Renderer> s_Renderer;
    static std::unique_ptr<Window> s_Window;
};

#endif
