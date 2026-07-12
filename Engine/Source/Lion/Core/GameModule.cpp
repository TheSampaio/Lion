#include "Engine.h"
#include "GameModule.h"

#include <Lion/Core/DynamicLibrary.h>
#include <Lion/Logic/ComponentRegistry.h>

namespace Lion
{
	bool LoadGameModule(DynamicLibrary& module, const std::string& path)
	{
		ComponentRegistry::BeginModule();

		const bool loaded = module.Load(path);

		ComponentRegistry::EndModule();

		return loaded;
	}

	void UnloadGameModule(DynamicLibrary& module)
	{
		ComponentRegistry::UnloadModule();
		module.Unload();
	}
}
