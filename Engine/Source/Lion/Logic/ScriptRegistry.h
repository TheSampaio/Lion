#pragma once

namespace Lion
{
	class Script;

	// Registry of native script classes, keyed by their type name.
	//
	// Scripts register themselves at start-up (see LION_REGISTER_SCRIPT), which lets scenes
	// reference them by name and lets the editor list the ones compiled into the running binary.
	class ScriptRegistry
	{
	public:
		using Factory = std::function<Scope<Script>()>;

		// Registers a script factory under 'name'. Re-registering the same name replaces it.
		static LION_API void Register(const std::string& name, Factory factory);

		// Instantiates a registered script, or returns null when the name is unknown/empty.
		static LION_API Scope<Script> Create(const std::string& name);

		static LION_API bool Contains(const std::string& name);

		// Registered script names, in registration order.
		static LION_API const std::vector<std::string>& GetNames();

		// Module scoping for hot reload; see ComponentRegistry for the details. A script's factory
		// points into the module that registered it, so it has to go before the module does.
		static LION_API void BeginModule();
		static LION_API void EndModule();
		static LION_API void UnloadModule();
	};
}

// Registers a Script subclass under its own type name, at static-initialization time.
#define LION_REGISTER_SCRIPT(Type)                                                                 \
	namespace                                                                                      \
	{                                                                                              \
		const bool kLionScriptRegistered_##Type = []                                               \
		{                                                                                          \
			::Lion::ScriptRegistry::Register(#Type,                                                \
				[]() -> ::Lion::Scope<::Lion::Script> { return ::Lion::MakeScope<Type>(); });       \
			return true;                                                                           \
		}();                                                                                       \
	}
