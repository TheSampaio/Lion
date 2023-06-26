#pragma once

// Enumerates the possible debug's modes
enum EDebugMode
{
	Specification = 0,
	Information,
	Warning,
	Error
};

namespace owl
{
	class OWL_API Debug
	{
	public:
		// === MAIN methods ======

		// Displays a text in a message box
		static void Message(EDebugMode Mode, const char* Text) { GetInstance().IMessage(Mode, Text); }

		// Displays a text in the application's console
		static void Console(EDebugMode Mode, const char* Text, bool bBreakLine = true) { GetInstance().IConsole(Mode, Text, bBreakLine); }

	protected:
		Debug();

		// Deletes copy constructor and assigment operator
		Debug(const Debug&) = delete;
		Debug operator=(const Debug&) = delete;

		// Gets the class's static reference
		static Debug& GetInstance() { static Debug s_Instance; return s_Instance; }

	private:
		// Attributes
		HANDLE m_hConsole;

		// Internal MAIN methods
		void IMessage(EDebugMode Mode, std::string_view Text);
		void IConsole(EDebugMode Mode, const char* Text, bool bBreakLine);
	};
}