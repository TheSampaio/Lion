#pragma once

// Enumerates all keyboard keys
enum OWL_API EKeyCode
{
	// Letters keys
	A = 'A',
	B = 'B',
	C = 'C',
	D = 'D',
	E = 'E',
	F = 'F',
	G = 'G',
	H = 'H',
	I = 'I',
	J = 'J',
	K = 'K',
	L = 'L',
	M = 'M',
	N = 'N',
	O = 'O',
	P = 'P',
	Q = 'Q',
	R = 'R',
	S = 'S',
	T = 'T',
	U = 'U',
	V = 'V',
	W = 'W',
	X = 'X',
	Y = 'Y',
	Z = 'Z',

	// Alpha numbers
	Key0 = '0',
	Key1 = '1',
	Key2 = '2',
	Key3 = '3',
	Key4 = '4',
	Key5 = '5',
	Key6 = '6',
	Key7 = '7',
	Key8 = '8',
	Key9 = '9',

	// Numpad keys
	Num0 = 0x60,
	Num1 = 0x61,
	Num2 = 0x62,
	Num3 = 0x63,
	Num4 = 0x64,
	Num5 = 0x65,
	Num6 = 0x66,
	Num7 = 0x67,
	Num8 = 0x68,
	Num9 = 0x69,

	Add = 0x6B,
	Subtract = 0x6D,
	Multiply = 0x6A,
	Divide = 0x6F,
	Decimal = 0x6E,

	// Functional keys
	F1 = 0x70,
	F2 = 0x71,
	F3 = 0x72,
	F4 = 0x73,
	F5 = 0x74,
	F6 = 0x75,
	F7 = 0x76,
	F8 = 0x77,
	F9 = 0x78,
	F10 = 0x79,
	F11 = 0x7A,
	F12 = 0x7B,

	// Arrow keys
	ArrowUp = 0x26,
	ArrowDown = 0x28,
	ArrowRight = 0x27,
	ArrowLeft = 0x25,

	// Other keys
	Tab = 0x09,
	End = 0x23,
	Esc = 0x1B,
	Menu = 0x12,
	Home = 0x24,
	Minus = 0xBD,
	Space = 0x20,
	Comma = 0xBC,
	Pause = 0x13,
	Return = 0x0D,
	Period = 0xBE,
	Insert = 0x2D,
	Delete = 0x2D,
	Backspace = 0x08,
	Backslash = 0xE2,

	Win = 0x5B,
	Alt = 0x12,
	Shift = 0x10,
	Control = 0x11,

	PageUp = 0x21,
	PageDown = 0x22,

	LockNum = 0x90,
	LockCaps = 0x14,
	LockScroll = 0x091
};

namespace owl
{
	class OWL_API Input
	{
	public:
		static bool GetKeyPress(EKeyCode KeyCode) { return s_Keys[KeyCode]; }

		static bool GetKeyRelease(EKeyCode KeyCode) { return !s_Keys[KeyCode]; }

		static bool GetKeyTap(EKeyCode KeyCode) { return GetInstance().IGetKeyTap(KeyCode); }

		// Friends
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
