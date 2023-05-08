#ifndef OWL_INPUT_H
#define OWL_INPUT_H

// Enumerates all keyboard keys
enum OWL_API EKeyCode
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
	Key0 = GLFW_KEY_0,
    Key1 = GLFW_KEY_1,
    Key2 = GLFW_KEY_2,
    Key3 = GLFW_KEY_3,
    Key4 = GLFW_KEY_4,
    Key5 = GLFW_KEY_5,
    Key6 = GLFW_KEY_6,
    Key7 = GLFW_KEY_7,
    Key8 = GLFW_KEY_8,
    Key9 = GLFW_KEY_9,

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

	Enter = GLFW_KEY_KP_ENTER,
	Decimal = GLFW_KEY_KP_DECIMAL,

	// Functional keys
	F1  = GLFW_KEY_F1,
	F2  = GLFW_KEY_F2,
	F3  = GLFW_KEY_F3,
	F4  = GLFW_KEY_F4,
	F5  = GLFW_KEY_F5,
	F6  = GLFW_KEY_F6,
	F7  = GLFW_KEY_F7,
	F8  = GLFW_KEY_F8,
	F9  = GLFW_KEY_F9,
	F10 = GLFW_KEY_F10,
	F11 = GLFW_KEY_F11,
	F12 = GLFW_KEY_F12,
	
	// Arrow keys
	ArrowUp = GLFW_KEY_UP,
	ArrowDown = GLFW_KEY_DOWN,
	ArrowRight = GLFW_KEY_RIGHT,
	ArrowLeft = GLFW_KEY_LEFT,

	// Other keys
	Tab = GLFW_KEY_TAB,
	End = GLFW_KEY_END,
	Menu = GLFW_KEY_MENU,
	Home = GLFW_KEY_HOME,
	Minus = GLFW_KEY_MINUS,
	Space = GLFW_KEY_SPACE,
	Equal = GLFW_KEY_EQUAL,
	Comma = GLFW_KEY_COMMA,
	Pause = GLFW_KEY_PAUSE,
	Escape = GLFW_KEY_ESCAPE,
	Return = GLFW_KEY_ENTER,
	Period = GLFW_KEY_PERIOD,
	Insert = GLFW_KEY_INSERT,
	Delete = GLFW_KEY_DELETE,
	Semicolon = GLFW_KEY_SEMICOLON,
	Backspace = GLFW_KEY_BACKSPACE,
	Apostrophe = GLFW_KEY_APOSTROPHE,
	GraveAccent = GLFW_KEY_GRAVE_ACCENT,
	PrintScreen = GLFW_KEY_PRINT_SCREEN,

	LockNum = GLFW_KEY_NUM_LOCK,
	LockCaps = GLFW_KEY_CAPS_LOCK,
	LockScroll = GLFW_KEY_SCROLL_LOCK,

	Slash = GLFW_KEY_SLASH,
	Backslash = GLFW_KEY_BACKSLASH,

	BracketRight = GLFW_KEY_RIGHT_BRACKET,
	BracketLeft = GLFW_KEY_LEFT_BRACKET,

	PageUp = GLFW_KEY_PAGE_UP,
	PageDown = GLFW_KEY_PAGE_DOWN,

	SuperRight = GLFW_KEY_RIGHT_SUPER,
	SuperLeft = GLFW_KEY_LEFT_SUPER,

	AltRight = GLFW_KEY_RIGHT_ALT,
	AltLeft = GLFW_KEY_LEFT_ALT,

	ShiftRight = GLFW_KEY_RIGHT_SHIFT,
	ShiftLeft = GLFW_KEY_LEFT_SHIFT,

	ControlRight = GLFW_KEY_RIGHT_CONTROL,
	ControlLeft = GLFW_KEY_LEFT_CONTROL,
};

class OWL_API Input
{
public:
	Input() {};

	// Delete copy constructor and assignment operator
	Input(const Input&) = delete;
	Input operator=(const Input&) = delete;

	// Main methods
	bool GetKeyPressed(EKeyCode KeyCode);
	bool GetKeyReleased(EKeyCode KeyCode);
	bool GetKeyTapped(EKeyCode KeyCode);

	// Friends
	friend class Application;

private:
	// Main methods
	void ProcessEvents();
	void ProcessCallbacks();

	// Callbacks methods
	static void FramebufferCallback(GLFWwindow* Id, int Width, int Height);

	// Static Attributes
	static int s_KeyCode;
	static bool s_bPressed;
};

#endif // !OWL_INPUT_H
