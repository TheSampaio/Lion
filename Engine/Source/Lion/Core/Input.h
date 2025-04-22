#pragma once

#include <Lion/Base/Platform.h>

namespace Lion
{
	class Application;

    // Enumerates all keyboard keys
    enum class KeyCode
    {
        // Letters keys
        A = GLFW_KEY_A,
        B = GLFW_KEY_B,
        C = GLFW_KEY_C,
        D = GLFW_KEY_D,
        E = GLFW_KEY_E,
        F = GLFW_KEY_F,
        G = GLFW_KEY_G,
        H = GLFW_KEY_H,
        I = GLFW_KEY_I,
        J = GLFW_KEY_J,
        K = GLFW_KEY_K,
        L = GLFW_KEY_L,
        M = GLFW_KEY_M,
        N = GLFW_KEY_N,
        O = GLFW_KEY_O,
        P = GLFW_KEY_P,
        Q = GLFW_KEY_Q,
        R = GLFW_KEY_R,
        S = GLFW_KEY_S,
        T = GLFW_KEY_T,
        U = GLFW_KEY_U,
        V = GLFW_KEY_V,
        W = GLFW_KEY_W,
        X = GLFW_KEY_X,
        Y = GLFW_KEY_Y,
        Z = GLFW_KEY_Z,

        // Alpha numbers
        Alpha0 = GLFW_KEY_0,
        Alpha1 = GLFW_KEY_1,
        Alpha2 = GLFW_KEY_2,
        Alpha3 = GLFW_KEY_3,
        Alpha4 = GLFW_KEY_4,
        Alpha5 = GLFW_KEY_5,
        Alpha6 = GLFW_KEY_6,
        Alpha7 = GLFW_KEY_7,
        Alpha8 = GLFW_KEY_8,
        Alpha9 = GLFW_KEY_9,

        // Numpad keys
        Num0 = GLFW_KEY_KP_0,
        Num1 = GLFW_KEY_KP_1,
        Num2 = GLFW_KEY_KP_2,
        Num3 = GLFW_KEY_KP_3,
        Num4 = GLFW_KEY_KP_4,
        Num5 = GLFW_KEY_KP_5,
        Num6 = GLFW_KEY_KP_6,
        Num7 = GLFW_KEY_KP_7,
        Num8 = GLFW_KEY_KP_8,
        Num9 = GLFW_KEY_KP_9,

        Add = GLFW_KEY_KP_ADD,
        Subtract = GLFW_KEY_KP_SUBTRACT,
        Multiply = GLFW_KEY_KP_MULTIPLY,
        Divide = GLFW_KEY_KP_DIVIDE,
        Decimal = GLFW_KEY_KP_DECIMAL,

        // Functional keys
        F1 = GLFW_KEY_F1,
        F2 = GLFW_KEY_F2,
        F3 = GLFW_KEY_F3,
        F4 = GLFW_KEY_F4,
        F5 = GLFW_KEY_F5,
        F6 = GLFW_KEY_F6,
        F7 = GLFW_KEY_F7,
        F8 = GLFW_KEY_F8,
        F9 = GLFW_KEY_F9,
        F10 = GLFW_KEY_F10,
        F11 = GLFW_KEY_F11,
        F12 = GLFW_KEY_F12,

        // Arrow keys
        Up = GLFW_KEY_UP,
        Down = GLFW_KEY_DOWN,
        Right = GLFW_KEY_RIGHT,
        Left = GLFW_KEY_LEFT,

        // Other keys
        Tab = GLFW_KEY_TAB,
        End = GLFW_KEY_END,
        Menu = GLFW_KEY_MENU,
        Home = GLFW_KEY_HOME,
        Minus = GLFW_KEY_MINUS,
        Space = GLFW_KEY_SPACE,
        Comma = GLFW_KEY_COMMA,
        Pause = GLFW_KEY_PAUSE,
        Escape = GLFW_KEY_ESCAPE,
        Return = GLFW_KEY_ENTER,
        Period = GLFW_KEY_PERIOD,
        Insert = GLFW_KEY_INSERT,
        Delete = GLFW_KEY_DELETE,
        Backspace = GLFW_KEY_BACKSPACE,
        Backslash = GLFW_KEY_BACKSLASH,

        Alt = GLFW_KEY_LEFT_ALT,
        Shift = GLFW_KEY_LEFT_SHIFT,
        Super = GLFW_KEY_LEFT_SUPER,
        Control = GLFW_KEY_LEFT_CONTROL,

        PageUp = GLFW_KEY_PAGE_UP,
        PageDown = GLFW_KEY_PAGE_DOWN,

        LockNum = GLFW_KEY_NUM_LOCK,
        LockCaps = GLFW_KEY_CAPS_LOCK,
        LockScroll = GLFW_KEY_SCROLL_LOCK
    };

	class Input
	{
	public:
        // Checks if the key is pressed every frames.
        static LION_API bool GetKeyPress(KeyCode keyCode);

        // Checks if the key is released every frames.
        static LION_API bool GetKeyRelease(KeyCode keyCode);

        // Checks if the key was pressed in the last frame.
        static LION_API bool GetKeyTap(KeyCode keyCode);

		friend Application;

	protected:
		static Input* sInstance;

		static void New();
		static void Delete();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

	private:
        Input() = default;

        static bool sControlKeys[256];
	};
}
