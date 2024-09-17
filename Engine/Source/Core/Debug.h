#pragma once

namespace Lion
{
	enum EDebugMode
	{
		Error = 0,
		Information,
		Trace,
		Warning,
	};

	class LION_API Debug
	{
	public:
		static void New();
		static Debug& Get();
		static void Delete();

		void Console(EDebugMode mode, const std::string& message);

	protected:
		static Debug* sInstance;

		Debug();

		Debug(const Debug&) = delete;
		Debug operator=(const Debug&) = delete;
	};
}

