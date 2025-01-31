#pragma once

//#include "../Events/Event.h"

namespace Lion
{
    class Application;
    class Event;
    class EventCallback;

    class Window
    {
    public:
        static void LION_API SetSize(uint width, uint height);
        static void LION_API SetTitle(const std::string& title);

        static bool LION_API Close();

        friend class Application;

    protected:
        static Window* sInstance;

        static void New();
        static void Delete();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

    private:
        using EventCallback = std::function<void(Event&)>;

        struct WindowData
        {
            std::string Title;
            uint Width, Height;
            EventCallback mEventCallback;
        };

        GLFWwindow* mId;
        WindowData mData;

        Window();
        ~Window();

        static void SetEventCallback(const EventCallback& callback) { sInstance->mData.mEventCallback = callback; }

        static bool Create();
        static void SwapBuffers();
        static void PollEvents();
    };
}
