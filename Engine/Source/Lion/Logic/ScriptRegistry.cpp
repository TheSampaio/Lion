#include "Engine.h"
#include "ScriptRegistry.h"

#include <Lion/Logic/Script.h>

namespace Lion
{
	namespace
	{
		// Function-local statics: scripts register during static initialization, so the storage must
		// be constructed on first use rather than depending on translation-unit ordering.
		std::unordered_map<std::string, ScriptRegistry::Factory>& Factories()
		{
			static std::unordered_map<std::string, ScriptRegistry::Factory> factories;
			return factories;
		}

		std::vector<std::string>& RegisteredNames()
		{
			static std::vector<std::string> names;
			return names;
		}

		// Names registered while a module was loading; dropped again when it is unloaded.
		bool& InModule()
		{
			static bool inModule = false;
			return inModule;
		}

		std::vector<std::string>& ModuleNames()
		{
			static std::vector<std::string> names;
			return names;
		}
	}

	void ScriptRegistry::Register(const std::string& name, Factory factory)
	{
		if (name.empty() || !factory)
			return;

		const bool isNew = Factories().find(name) == Factories().end();
		Factories()[name] = std::move(factory);

		if (isNew)
		{
			RegisteredNames().push_back(name);

			if (InModule())
				ModuleNames().push_back(name);
		}
	}

	Scope<Script> ScriptRegistry::Create(const std::string& name)
	{
		const auto it = Factories().find(name);
		return (it != Factories().end()) ? it->second() : nullptr;
	}

	bool ScriptRegistry::Contains(const std::string& name)
	{
		return Factories().find(name) != Factories().end();
	}

	const std::vector<std::string>& ScriptRegistry::GetNames()
	{
		return RegisteredNames();
	}

	void ScriptRegistry::BeginModule()
	{
		InModule() = true;
	}

	void ScriptRegistry::EndModule()
	{
		InModule() = false;
	}

	void ScriptRegistry::UnloadModule()
	{
		for (const std::string& name : ModuleNames())
		{
			Factories().erase(name);

			auto& names = RegisteredNames();
			names.erase(std::remove(names.begin(), names.end(), name), names.end());
		}

		ModuleNames().clear();
	}
}
