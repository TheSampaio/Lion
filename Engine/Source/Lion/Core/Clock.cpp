#include "Engine.h"
#include "Clock.h"

#include <Lion/Core/Build.h>
#include <Lion/Core/Window.h>
#include <Lion/Render/RendererAPI.h>

namespace Lion
{
    static const char8* GraphicsApiName()
    {
        switch (RendererAPI::GetAPI())
        {
            case GraphicsAPI::OpenGL: return "OpenGL";
            case GraphicsAPI::Vulkan: return "Vulkan";
            default:                  return "None";
        }
    }

    namespace
    {
        // The build's default; an application (the editor) may override it.
        bool& ShowFrameStats()
        {
#ifdef LN_SHIPPING
            static bool show = false;
#else
            static bool show = true;
#endif

            return show;
        }
    }

    Clock* Clock::sInstance = nullptr;
    float32 Clock::sFrameTime = 0.0f;
    uint32 Clock::sFrameCount = 0;
    float32 Clock::sTotalTime = 0.0f;

    void Clock::SetShowFrameStats(bool show)
    {
        ShowFrameStats() = show;
    }

    bool Clock::GetShowFrameStats()
    {
        return ShowFrameStats();
    }

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

        if (!ShowFrameStats())
            return;

        sTotalTime += sFrameTime;
        sFrameCount++;

        if (sTotalTime >= 1.0f)
        {
            std::stringstream text;
            text << std::fixed;
            text.precision(1);

            // No labels: a reader does not need to be told that "OpenGL" is the graphics API, and the
            // units carry the rest. The frame time stays alongside the framerate because it is the one
            // that is linear, and so the one you optimise against.
            text << Window::GetTitle().c_str()
                << " | " << sFrameCount << " fps"
                << " | " << sFrameTime * 1000.0f << " ms"
                << " | " << BuildConfiguration()
                << " | " << GraphicsApiName();

            Window::SetDisplayTitle(text.str());

            sFrameCount = 0;
            sTotalTime -= 1.0f;
        }
    }
}
