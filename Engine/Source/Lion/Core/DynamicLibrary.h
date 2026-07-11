#pragma once

namespace Lion
{
	// Thin wrapper around a runtime-loaded shared library (LoadLibrary/dlopen).
	//
	// Loading a module runs its static initializers, which is how a game module's components and
	// scripts register themselves with the engine registries just by being loaded — the mechanism
	// behind user-defined components being visible to both the standalone game and the editor.
	class DynamicLibrary
	{
	public:
		LION_API DynamicLibrary() = default;
		LION_API ~DynamicLibrary();

		DynamicLibrary(const DynamicLibrary&) = delete;
		DynamicLibrary& operator=(const DynamicLibrary&) = delete;

		LION_API DynamicLibrary(DynamicLibrary&& other) noexcept;
		LION_API DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

		// Loads the library at 'path' (an unloaded instance is loaded; a loaded one is replaced).
		// Returns whether it loaded.
		LION_API bool Load(const std::string& path);

		// Unloads the library, if any. Any symbol obtained from it becomes invalid.
		LION_API void Unload();

		LION_API bool IsLoaded() const { return mHandle != nullptr; }

		// Resolves an exported symbol by name, or null when it is absent (or nothing is loaded).
		LION_API void* GetSymbol(const std::string& name) const;

		// Convenience typed accessor: GetFunction<Ret(*)(Args...)>("name").
		template<typename Function>
		Function GetFunction(const std::string& name) const
		{
			return reinterpret_cast<Function>(GetSymbol(name));
		}

	private:
		void* mHandle = nullptr;  // HMODULE on Windows, kept opaque so the header stays platform-free.
	};
}
