#include "Engine.h"
#include "Build.h"

namespace Lion
{
	const char8* BuildConfiguration()
	{
#if defined(LN_DEBUG)
		return "Debug";
#elif defined(LN_RELEASE)
		return "Release";
#else
		return "Shipping";
#endif
	}
}
