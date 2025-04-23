#include "Engine.h"
#include "Clock.h"

#include <Lion/Core/Window.h>

namespace Lion
{
    Clock* Clock::sInstance = nullptr;
    float32 Clock::sFrameTime = 0.0f;

#ifndef LN_SHIPPING
    uint32 Clock::sFrameCount = 0;
    float32 Clock::sTotalTime = 0.0f;

#endif // LN_SHIPPING


    void Clock::New()
    {
        sInstance = new Clock();
    }

    void Clock::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    Clock::Clock()
    {
        mTimer = MakeScope<Timer>();
    }

    void Clock::UpdateFrameTime()
    {
        sFrameTime = sInstance->mTimer->Reset();

#ifndef LN_SHIPPING
        sTotalTime += sFrameTime;
        sFrameCount++;

        if (sTotalTime >= 1.0f)
        {
            std::stringstream text;
            text << std::fixed;
            text.precision(1);

            text << Window::GetTitle().c_str()
                << " | FPS: " << sFrameCount
                << " | MS: " << sFrameTime * 1000.0f
                << " | API: OpenGL";

            glfwSetWindowTitle(Window::GetId(), text.str().c_str());

            sFrameCount = 0;
            sTotalTime -= 1.0f;
        }

#endif // LN_SHIPPING
    }
}
