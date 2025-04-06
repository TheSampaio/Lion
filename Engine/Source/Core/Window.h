#pragma once

namespace Lion
{
    class Application;
    class Event;
    class EventCallback;

    class Window
    {
    public:
        static LION_API GLFWwindow* GetId() { return sInstance->mId; }

        static LION_API void SetSize(uint width, uint height);
        static LION_API void SetTitle(const std::string& title);

        static LION_API bool Close();

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
