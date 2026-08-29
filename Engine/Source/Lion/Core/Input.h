#pragma once

namespace Lion
{
	class Application;

	enum class InputDevice
	{
		Keyboard,
		MouseButton,
		GamepadButton,
		GamepadAxis,
	};

	enum class GamepadButton
	{
		A = GLFW_GAMEPAD_BUTTON_A,
		B = GLFW_GAMEPAD_BUTTON_B,
		X = GLFW_GAMEPAD_BUTTON_X,
		Y = GLFW_GAMEPAD_BUTTON_Y,
		LeftBumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
		RightBumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
		Back = GLFW_GAMEPAD_BUTTON_BACK,
		Start = GLFW_GAMEPAD_BUTTON_START,
		Guide = GLFW_GAMEPAD_BUTTON_GUIDE,
		LeftThumb = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
		RightThumb = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
		DpadUp = GLFW_GAMEPAD_BUTTON_DPAD_UP,
		DpadRight = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
		DpadDown = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
		DpadLeft = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
	};

	enum class GamepadAxis
	{
		LeftX = GLFW_GAMEPAD_AXIS_LEFT_X,
		LeftY = GLFW_GAMEPAD_AXIS_LEFT_Y,
		RightX = GLFW_GAMEPAD_AXIS_RIGHT_X,
		RightY = GLFW_GAMEPAD_AXIS_RIGHT_Y,
		LeftTrigger = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
		RightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
	};

	struct InputBinding
	{
		InputDevice device = InputDevice::Keyboard;
		int32 code = 0;
		float32 scale = 1.0f;
		int32 gamepad = -1;   // -1 listens to every connected gamepad; 0..15 selects one.
	};

	struct InputAction
	{
		std::string name;
		float32 deadzone = 0.2f;
		std::vector<InputBinding> bindings;
	};

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
		static constexpr const char8* kDefaultActionMapFile = "Config/Input.lninput";

        // Checks if the key is pressed every frames.
        static LION_API bool GetKeyPress(KeyCode keyCode);

        // Checks if the key is released every frames.
        static LION_API bool GetKeyRelease(KeyCode keyCode);

        // Checks if the key was pressed in the last frame.
        static LION_API bool GetKeyTap(KeyCode keyCode);

		static LION_API bool GetMouseButtonPress(int32 button);
		static LION_API bool IsGamepadConnected(int32 gamepad = 0);
		static LION_API std::string GetGamepadName(int32 gamepad = 0);
		static LION_API bool GetGamepadButtonPress(GamepadButton button, int32 gamepad = 0);
		static LION_API float32 GetGamepadAxis(GamepadAxis axis, int32 gamepad = 0);

		// Named project actions combine keyboard, mouse and normalized GLFW gamepad bindings. A game asks
		// for the action and stays independent of whether the player uses an Xbox, PlayStation or generic
		// pad whose mapping GLFW understands.
		static LION_API float32 GetActionStrength(const std::string& action);
		static LION_API bool GetActionPress(const std::string& action);
		static LION_API bool GetActionTap(const std::string& action);

		static LION_API const std::vector<InputAction>& GetActionMap();
		static LION_API void SetActionMap(const std::vector<InputAction>& actions);
		static LION_API bool LoadActionMap(const std::string& path);
		static LION_API bool SaveActionMap(const std::string& path, const std::vector<InputAction>& actions);

		friend Application;

	protected:
		static Input* sInstance;

		static void New();
		static void Delete();
		static void Update();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

	private:
        Input() = default;

		static bool sControlKeys[GLFW_KEY_LAST + 1];
		static std::vector<InputAction> sActions;
		static std::unordered_map<std::string, float32> sActionStrengths;
		static std::unordered_map<std::string, float32> sPreviousActionStrengths;

		static float32 EvaluateAction(const InputAction& action);
	};
}
