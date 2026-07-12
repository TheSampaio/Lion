#include "Engine.h"
#include "GameModule.h"

#include <Lion/Core/DynamicLibrary.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/ScriptRegistry.h>

namespace Lion
{
	bool LoadGameModule(DynamicLibrary& module, const std::string& path)
	{
		ComponentRegistry::BeginModule();
		ScriptRegistry::BeginModule();

		const bool loaded = module.Load(path);

		ComponentRegistry::EndModule();
		ScriptRegistry::EndModule();

		return loaded;
	}

	void UnloadGameModule(DynamicLibrary& module)
	{
		ComponentRegistry::UnloadModule();
		ScriptRegistry::UnloadModule();
		module.Unload();
	}
}
