#ifndef OWL_APPLICATION_H
#define OWL_APPLICATION_H

#include "Debug.h"
#include "Input.h"
#include "Renderer.h"
#include "Window.h"

#include "../../Events/Include/Timer.h"

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
    friend Window;
    friend class Mesh;

protected:
    virtual void Start() = 0;
    virtual void Update(float DeltaTime) = 0;
    virtual void Draw() = 0;
    virtual void Finalize() = 0;

    // TODO: virtual void OnPause() = 0;

    // Get methods
    static inline Input* GetInput()       { return s_Input.get(); }
    static inline Renderer* GetRenderer() { return s_Renderer.get(); }
    static inline Timer* GetTimer()       { return s_Timer.get(); }
    static inline Window* GetWindow()     { return s_Window.get(); }

private:
    // Main methods
    bool Loop();
    void Init();

    float FrameTimeMonitor();

    // Static attributes
    static std::unique_ptr<Input> s_Input;
    static std::unique_ptr<Renderer> s_Renderer;
    static std::unique_ptr<Timer> s_Timer;
    static std::unique_ptr<Window> s_Window;

    static float s_FrameTime;

    // Static methods
    // TODO: void Pause();
    // TODO: void Resume();
};

#endif
