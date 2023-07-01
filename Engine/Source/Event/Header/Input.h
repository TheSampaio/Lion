#pragma once

// Enumerates all keyboard keys
enum EKeyCode
{
	// Letters keys
	Key_A = 0x41,
	Key_B = 0x42,
	Key_C = 0x43,
	Key_D = 0x44,
	Key_E = 0x45,
	Key_F = 0x46,
	Key_G = 0x47,
	Key_H = 0x48,
	Key_I = 0x49,
	Key_J = 0x4A,
	Key_K = 0x4B,
	Key_L = 0x4C,
	Key_M = 0x4D,
	Key_N = 0x4E,
	Key_O = 0x4F,
	Key_P = 0x50,
	Key_Q = 0x51,
	Key_R = 0x52,
	Key_S = 0x53,
	Key_T = 0x54,
	Key_U = 0x55,
	Key_V = 0x56,
	Key_W = 0x57,
	Key_X = 0x58,
	Key_Y = 0x59,
	Key_Z = 0x5A,

	// Alpha numbers
	Key_0 = 0x30,
	Key_1 = 0x31,
	Key_2 = 0x32,
	Key_3 = 0x33,
	Key_4 = 0x34,
	Key_5 = 0x35,
	Key_6 = 0x36,
	Key_7 = 0x37,
	Key_8 = 0x38,
	Key_9 = 0x39,

	// Numpad keys
	Num_0 = 0x60,
	Num_1 = 0x61,
	Num_2 = 0x62,
	Num_3 = 0x63,
	Num_4 = 0x64,
	Num_5 = 0x65,
	Num_6 = 0x66,
	Num_7 = 0x67,
	Num_8 = 0x68,
	Num_9 = 0x69,

	Num_Add = 0x6B,
	Num_Subtract = 0x6D,
	Num_Multiply = 0x6A,
	Num_Divide = 0x6F,
	Num_Decimal = 0x6E,

	// Functional keys
	Key_F1 = 0x70,
	Key_F2 = 0x71,
	Key_F3 = 0x72,
	Key_F4 = 0x73,
	Key_F5 = 0x74,
	Key_F6 = 0x75,
	Key_F7 = 0x76,
	Key_F8 = 0x77,
	Key_F9 = 0x78,
	Key_F10 = 0x79,
	Key_F11 = 0x7A,
	Key_F12 = 0x7B,

	// Arrow keys
	Key_Up = 0x26,
	Key_Down = 0x28,
	Key_Right = 0x27,
	Key_Left = 0x25,

	// Other keys
	Key_Tab = 0x09,
	Key_End = 0x23,
	Key_Esc = 0x1B,
	Key_Menu = 0x12,
	Key_Home = 0x24,
	Key_Minus = 0xBD,
	Key_Space = 0x20,
	Key_Comma = 0xBC,
	Key_Pause = 0x13,
	Key_Return = 0x0D,
	Key_Period = 0xBE,
	Key_Insert = 0x2D,
	Key_Delete = 0x2D,
	Key_Backspace = 0x08,
	Key_Backslash = 0xE2,

	Key_Win = 0x5B,
	Key_Alt = 0x12,
	Key_Shift = 0x10,
	Key_Control = 0x11,

	Key_PageUp = 0x21,
	Key_PageDown = 0x22,

	Key_LockNum = 0x90,
	Key_LockCaps = 0x14,
	Key_LockScroll = 0x091
};

namespace owl
{
	class Input
	{
	public:
		// == GET methods ======

		// Verifies if the key is pressed every frame
		static bool OWL_API GetKeyPress(EKeyCode KeyCode) { return s_Keys[KeyCode]; }

		// Verifies if the key is released every frame
		static bool OWL_API GetKeyRelease(EKeyCode KeyCode) { return !s_Keys[KeyCode]; }

		// Verifies if the key was pressed in the last frame
		static bool OWL_API GetKeyTap(EKeyCode KeyCode) { return GetInstance().IGetKeyTap(KeyCode); }

		// === Friends ======

		friend class Window;

	protected:
		Input() {};

		// Deletes copy constructor and assigment operator
		Input(const Input&) = delete;
		Input operator=(const Input&) = delete;

		// Gets the class's static reference
		static Input& GetInstance() { static Input m_Instance; return m_Instance; }

	private:
		// Internal MAIN methods
		bool IGetKeyTap(EKeyCode KeyCode);

		// Static attributes
		static bool	s_Keys[256];
		static bool s_Ctrl[256];

		// Static methods
		static LRESULT CALLBACK Procedure(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
	};
}
