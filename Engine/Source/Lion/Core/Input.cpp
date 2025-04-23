#include "Engine.h"
#include "Input.h"

#include <Lion/Core/Window.h>

namespace Lion
{
    Input* Input::sInstance = nullptr;
    bool   Input::sControlKeys[256] = {false};

    void Input::New()
    {
        sInstance = new Input();
    }

    void Input::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    bool Input::GetKeyPress(KeyCode keyCode)
    {
        return static_cast<bool>(glfwGetKey(Window::GetId(), static_cast<int32>(keyCode)) == GLFW_PRESS);
    }

    bool Input::GetKeyRelease(KeyCode keyCode)
    {
        return static_cast<bool>(glfwGetKey(Window::GetId(), static_cast<int32>(keyCode)) == GLFW_RELEASE);
    }

    bool Input::GetKeyTap(KeyCode keyCode)
    {
        const int32 keyCodeId = static_cast<int32>(keyCode);

        if (GetKeyPress(keyCode))
            sControlKeys[keyCodeId] = true;

        if (GetKeyRelease(keyCode) && sControlKeys[keyCodeId])
        {
            sControlKeys[keyCodeId] = false;
            return true;
        }

        return false;
    }
}
