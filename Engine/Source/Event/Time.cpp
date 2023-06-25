#include "Engine.h"
#include "Header/Time.h"

#include "../Core/Header/Window.h"
#include "../Logic/Header/Timer.h"

owl::Time::Time()
    : Timer(nullptr)
{
    Timer = new owl::Timer();
	m_DeltaTime = 0.0f;
}

owl::Time::~Time()
{
    delete Timer;
}

void owl::Time::DeltaTimeMonitor()
{
#ifdef WL_DEBUG
    static float TotalTime = 0.0f;
    static uint  FrameCount = 0;
#endif
 
    m_DeltaTime = Timer->Reset();

#ifdef WL_DEBUG
    TotalTime += m_DeltaTime;
    FrameCount++;

    if (TotalTime >= 1.0f)
    {
        std::stringstream Text;
        Text << std::fixed;
        Text.precision(1);

        Text << Window::GetTitle().c_str() << " | FPS: " << FrameCount << " | MS: " << m_DeltaTime * 1000 << " | D3D11";

        std::string Title = Text.str();
        SetWindowText(Window::GetInstance().m_hWindow, std::wstring(Title.begin(), Title.end()).data());

        FrameCount = 0;
        TotalTime -= 1.0f;
    }
#endif
}
