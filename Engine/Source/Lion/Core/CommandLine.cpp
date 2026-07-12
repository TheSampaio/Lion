#include "Engine.h"
#include "CommandLine.h"

namespace Lion
{
	namespace
	{
		std::vector<std::string>& Arguments()
		{
			static std::vector<std::string> arguments;
			return arguments;
		}
	}

	void CommandLine::Set(int32 count, const char8* const* values)
	{
		Arguments().assign(values, values + count);
	}

	int32 CommandLine::GetCount()
	{
		return static_cast<int32>(Arguments().size());
	}

	const std::string& CommandLine::Get(int32 index)
	{
		static const std::string empty;

		if (index < 0 || index >= GetCount())
			return empty;

		return Arguments()[index];
	}
}
