#pragma once

// Enumerates the possible debug's modes
enum EDebugMode
{
	Specification = 0,
	Information,
	Warning,
	Error
};

// Enumerates the possible question's modes
enum EQuestionMode
{
	Affirmative = 0,
	Negative,
};

namespace owl
{
	class Debug
	{
	public:
		// === MAIN methods ======

		// Displays a text in a message box
		static void OWL_API Message(EDebugMode Mode, const char* Text) { GetInstance().IMessage(Mode, Text); }

		// Displays a text in the application's console
		static void OWL_API Console(EDebugMode Mode, const char* Text, bool bBreakLine = true) { GetInstance().IConsole(Mode, Text, bBreakLine); }

		static bool OWL_API Question(EQuestionMode Mode, const char* Text) { return GetInstance().IQuestion(Mode, Text); }

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
		void IConsole(EDebugMode Mode, const char* Text, bool bBreakLine);
		void IMessage(EDebugMode Mode, std::string_view Text);
		bool IQuestion(EQuestionMode Mode, std::string_view Text);
	};
}