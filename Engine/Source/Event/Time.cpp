#include "Engine.h"
#include "Header/Time.h"

#include "../Core/Header/Window.h"
#include "../Logic/Header/Timer.h"

Lion::Time::Time()
    : Timer(nullptr)
{
    Timer = new Lion::Timer();
	m_DeltaTime = 0.0f;
}

Lion::Time::~Time()
{
    delete Timer;
}

void Lion::Time::DeltaTimeMonitor()
{
#ifdef LN_DEBUG
    static float TotalTime = 0.0f;
    static uint  FrameCount = 0;
#endif !LN_DEBUG
 
    m_DeltaTime = Timer->Reset();

#ifdef LN_DEBUG
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
#endif !LN_DEBUG
}
