#pragma once

namespace Lion
{
    class Application;
    class Event;
    class EventCallback;
    class Graphics;

    class Window
    {
    public:
        static LION_API GLFWwindow* GetId() { return sInstance->mId; }
        static LION_API std::array<float, 4> GetBackgroundColor() { return sInstance->mBackgroundColor; }

        static LION_API void SetSize(uint width, uint height);
        static LION_API void SetBackgroundColor(float red, float green, float blue, float alpha);
        static LION_API void SetTitle(const std::string& title);

        static LION_API bool Close();

        friend Application;
        friend Graphics;

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
        std::array<float, 4> mBackgroundColor;

        Window();
        ~Window();

        static void SetEventCallback(const EventCallback& callback) { sInstance->mData.mEventCallback = callback; }

        static bool Create();
        static void PollEvents();
    };
}
